/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stddef.h>

static int is_valid_reg(uint8_t reg) { return reg < 16U ? 1 : 0; }

static int is_arith_only_fastpath_candidate(const graphion_insn *program,
                                            size_t program_len,
                                            bool *halt_terminated) {
  size_t i;
  bool has_halt = false;
  for (i = 0U; i < program_len; ++i) {
    const graphion_insn in = program[i];
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        has_halt = true;
        break;
      case GVM_OP_MOV_IMM:
        if (!is_valid_reg(in.a)) {
          return 0;
        }
        break;
      case GVM_OP_ADD:
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return 0;
        }
        break;
      default:
        return 0;
    }
  }
  if (halt_terminated != NULL) {
    *halt_terminated = has_halt;
  }
  return 1;
}

static void run_arith_fastpath_c_halt_terminated(graphion_vm *vm) {
  const graphion_insn *p = vm->program + vm->pc;
  const graphion_insn *const end = vm->program + vm->program_len;
  int64_t *const regs = vm->regs;

  for (;;) {
    const graphion_insn in = *p++;
    if (in.op == GVM_OP_ADD && p < end) {
      const graphion_insn next = *p;
      if (next.op == GVM_OP_ADD && next.a == in.a && in.b != in.a && next.b != in.a) {
        regs[in.a] += regs[in.b] + regs[next.b];
        p++;
        continue;
      }
    }
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return;
      case GVM_OP_MOV_IMM:
        regs[in.a] = (int64_t)in.imm;
        break;
      case GVM_OP_ADD:
        regs[in.a] += regs[in.b];
        break;
      default:
        vm->pc = (size_t)(p - vm->program);
        return;
    }
  }
}

#if !(defined(GRAPHION_ENABLE_ASM) && !defined(_MSC_VER))
static void run_arith_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p = vm->program + vm->pc;
  const graphion_insn *const end = vm->program + vm->program_len;
  int64_t *const regs = vm->regs;

  while (p < end) {
    const graphion_insn in = *p++;
    if (in.op == GVM_OP_ADD && p < end) {
      const graphion_insn next = *p;
      if (next.op == GVM_OP_ADD && next.a == in.a && in.b != in.a && next.b != in.a) {
        regs[in.a] += regs[in.b] + regs[next.b];
        p++;
        continue;
      }
    }
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return;
      case GVM_OP_MOV_IMM:
        regs[in.a] = (int64_t)in.imm;
        break;
      case GVM_OP_ADD:
        regs[in.a] += regs[in.b];
        break;
      default:
        vm->pc = (size_t)(p - vm->program);
        return;
    }
  }
  vm->pc = vm->program_len;
}
#endif

#if defined(GRAPHION_ENABLE_ASM) && !defined(_MSC_VER)
extern size_t graphion_vm_run_hotpath_arith_asm(int64_t *regs,
                                                const graphion_insn *program,
                                                size_t program_len,
                                                int *halted);
#endif

static int64_t count_visited_levels(const int32_t *levels, size_t count) {
  size_t i;
  int64_t total = 0;
  for (i = 0; i < count; ++i) {
    if (levels[i] >= 0) {
      total++;
    }
  }
  return total;
}

void graphion_vm_init(graphion_vm *vm) {
  size_t i;
  if (vm == NULL) {
    return;
  }
  for (i = 0; i < 16U; ++i) {
    vm->regs[i] = 0;
  }
  vm->program = NULL;
  vm->program_len = 0U;
  vm->pc = 0U;
  vm->halted = false;
  vm->arith_only_fastpath = false;
  vm->arith_only_halt_terminated = false;
  vm->csr_graph = NULL;
  vm->bfs_levels = NULL;
  vm->bfs_queue = NULL;
  vm->bfs_capacity = 0U;
  vm->hypergraph = NULL;
}

int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len) {
  if (vm == NULL || program == NULL || program_len == 0U) {
    return -1;
  }
  vm->program = program;
  vm->program_len = program_len;
  vm->pc = 0U;
  vm->halted = false;
  vm->arith_only_fastpath =
      is_arith_only_fastpath_candidate(program, program_len, &vm->arith_only_halt_terminated) != 0;
  return 0;
}

void graphion_vm_bind_csr(graphion_vm *vm,
                          const graphion_csr_graph *graph,
                          int32_t *bfs_levels,
                          uint32_t *bfs_queue,
                          size_t bfs_capacity) {
  if (vm == NULL) {
    return;
  }
  vm->csr_graph = graph;
  vm->bfs_levels = bfs_levels;
  vm->bfs_queue = bfs_queue;
  vm->bfs_capacity = bfs_capacity;
}

void graphion_vm_bind_hypergraph(graphion_vm *vm, const graphion_hypergraph *graph) {
  if (vm == NULL) {
    return;
  }
  vm->hypergraph = graph;
}

int graphion_vm_run(graphion_vm *vm) {
  if (vm == NULL || vm->program == NULL) {
    return -1;
  }

  if (vm->arith_only_fastpath) {
#if defined(GRAPHION_ENABLE_ASM) && !defined(_MSC_VER)
    if (vm->arith_only_halt_terminated) {
      run_arith_fastpath_c_halt_terminated(vm);
    } else {
      int halted = 0;
      vm->pc = graphion_vm_run_hotpath_arith_asm(vm->regs, vm->program, vm->program_len, &halted);
      vm->halted = halted != 0;
    }
#else
    if (vm->arith_only_halt_terminated) {
      run_arith_fastpath_c_halt_terminated(vm);
    } else {
      run_arith_fastpath_c(vm);
    }
#endif
    return 0;
  }

  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc++];
    int64_t *regs = vm->regs;
    const uint8_t a = in.a;
    const uint8_t b = in.b;

    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        break;
      case GVM_OP_MOV_IMM:
        if (!is_valid_reg(a)) {
          return -2;
        }
        regs[a] = (int64_t)in.imm;
        break;
      case GVM_OP_ADD:
        if (!is_valid_reg(a) || !is_valid_reg(b)) {
          return -3;
        }
        regs[a] += regs[b];
        break;
      case GVM_OP_BFS_LEVELS: {
        uint32_t source;
        int rc;
        if (!is_valid_reg(a) || !is_valid_reg(b)) {
          return -3;
        }
        if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
          return -5;
        }
        if (regs[a] < 0) {
          return -6;
        }
        source = (uint32_t)regs[a];
        if ((size_t)source >= vm->csr_graph->node_count) {
          return -6;
        }
        rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
        if (rc != 0) {
          return -7;
        }
        regs[b] = count_visited_levels(vm->bfs_levels, vm->csr_graph->node_count);
        break;
      }
      case GVM_OP_INCIDENT_COUNT: {
        uint32_t node;
        if (!is_valid_reg(a) || !is_valid_reg(b)) {
          return -3;
        }
        if (vm->hypergraph == NULL) {
          return -8;
        }
        if (regs[a] < 0) {
          return -9;
        }
        node = (uint32_t)regs[a];
        if ((size_t)node >= vm->hypergraph->node_count) {
          return -9;
        }
        regs[b] = (int64_t)(vm->hypergraph->node_offsets[node + 1U] - vm->hypergraph->node_offsets[node]);
        break;
      }
      case GVM_OP_HYPEREDGE_SIZE: {
        uint32_t hyperedge;
        if (!is_valid_reg(a) || !is_valid_reg(b)) {
          return -3;
        }
        if (vm->hypergraph == NULL) {
          return -8;
        }
        if (regs[a] < 0) {
          return -10;
        }
        hyperedge = (uint32_t)regs[a];
        if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
          return -10;
        }
        regs[b] = (int64_t)(vm->hypergraph->hyperedge_offsets[hyperedge + 1U] -
                            vm->hypergraph->hyperedge_offsets[hyperedge]);
        break;
      }
      case GVM_OP_INCIDENT_SUM: {
        uint32_t node;
        if (!is_valid_reg(a) || !is_valid_reg(b)) {
          return -3;
        }
        if (vm->hypergraph == NULL) {
          return -8;
        }
        if (regs[a] < 0) {
          return -9;
        }
        node = (uint32_t)regs[a];
        if ((size_t)node >= vm->hypergraph->node_count) {
          return -9;
        }
        regs[b] = (int64_t)graphion_hypergraph_incident_sum(vm->hypergraph, node);
        break;
      }
      case GVM_OP_HYPEREDGE_NODE_SUM: {
        uint32_t hyperedge;
        if (!is_valid_reg(a) || !is_valid_reg(b)) {
          return -3;
        }
        if (vm->hypergraph == NULL) {
          return -8;
        }
        if (regs[a] < 0) {
          return -10;
        }
        hyperedge = (uint32_t)regs[a];
        if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
          return -10;
        }
        regs[b] = (int64_t)graphion_hypergraph_hyperedge_node_sum(vm->hypergraph, hyperedge);
        break;
      }
      default:
        return -4;
    }
  }

  return 0;
}
