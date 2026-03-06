/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stddef.h>

typedef struct {
  const graphion_insn *program;
  size_t program_len;
  bool arith_only_fastpath;
  bool arith_only_halt_terminated;
} graphion_vm_shape_cache_entry;

enum { GRAPHION_VM_SHAPE_CACHE_SIZE = 64 };

static graphion_vm_shape_cache_entry g_shape_cache[GRAPHION_VM_SHAPE_CACHE_SIZE];

static size_t shape_cache_slot(const graphion_insn *program, size_t program_len) {
  uintptr_t p = (uintptr_t)program;
  return (size_t)((p ^ (p >> 7U) ^ (uintptr_t)(program_len * 1315423911U)) &
                  (GRAPHION_VM_SHAPE_CACHE_SIZE - 1U));
}

static int shape_cache_lookup(const graphion_insn *program,
                              size_t program_len,
                              bool *arith_only_fastpath,
                              bool *arith_only_halt_terminated) {
  const size_t slot = shape_cache_slot(program, program_len);
  const graphion_vm_shape_cache_entry e = g_shape_cache[slot];
  if (e.program != program || e.program_len != program_len) {
    return 0;
  }
  *arith_only_fastpath = e.arith_only_fastpath;
  *arith_only_halt_terminated = e.arith_only_halt_terminated;
  return 1;
}

static void shape_cache_store(const graphion_insn *program,
                              size_t program_len,
                              bool arith_only_fastpath,
                              bool arith_only_halt_terminated) {
  const size_t slot = shape_cache_slot(program, program_len);
  g_shape_cache[slot].program = program;
  g_shape_cache[slot].program_len = program_len;
  g_shape_cache[slot].arith_only_fastpath = arith_only_fastpath;
  g_shape_cache[slot].arith_only_halt_terminated = arith_only_halt_terminated;
}

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
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return;
      case GVM_OP_MOV_IMM:
        regs[in.a] = (int64_t)in.imm;
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.b == in.a) {
            regs[next.a] += regs[in.a];
            p++;
          }
        }
        break;
      case GVM_OP_ADD:
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.a == in.a && in.b != in.a && next.b != in.a) {
            regs[in.a] += regs[in.b] + regs[next.b];
            p++;
            break;
          }
        }
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
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return;
      case GVM_OP_MOV_IMM:
        regs[in.a] = (int64_t)in.imm;
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.b == in.a) {
            regs[next.a] += regs[in.a];
            p++;
          }
        }
        break;
      case GVM_OP_ADD:
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.a == in.a && in.b != in.a && next.b != in.a) {
            regs[in.a] += regs[in.b] + regs[next.b];
            p++;
            break;
          }
        }
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
  bool halt_terminated = false;
  bool arith_only_fastpath = false;
  if (vm == NULL || program == NULL || program_len == 0U) {
    return -1;
  }
  vm->program = program;
  vm->program_len = program_len;
  vm->pc = 0U;
  vm->halted = false;

  if (!shape_cache_lookup(program, program_len, &arith_only_fastpath, &halt_terminated)) {
    arith_only_fastpath = is_arith_only_fastpath_candidate(program, program_len, &halt_terminated) != 0;
    shape_cache_store(program, program_len, arith_only_fastpath, halt_terminated);
  }
  vm->arith_only_fastpath = arith_only_fastpath;
  vm->arith_only_halt_terminated = halt_terminated;
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

static int op_nop(graphion_vm *vm, const graphion_insn *in) {
  (void)vm;
  (void)in;
  return 0;
}

static int op_halt(graphion_vm *vm, const graphion_insn *in) {
  (void)in;
  vm->halted = true;
  return 0;
}

static int op_mov_imm(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return -2;
  }
  vm->regs[in->a] = (int64_t)in->imm;
  return 0;
}

static int op_add(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return -3;
  }
  vm->regs[in->a] += vm->regs[in->b];
  return 0;
}

static int op_bfs_levels(graphion_vm *vm, const graphion_insn *in) {
  uint32_t source;
  int rc;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return -3;
  }
  if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
    return -5;
  }
  if (vm->regs[in->a] < 0) {
    return -6;
  }
  source = (uint32_t)vm->regs[in->a];
  if ((size_t)source >= vm->csr_graph->node_count) {
    return -6;
  }
  rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
  if (rc != 0) {
    return -7;
  }
  vm->regs[in->b] = count_visited_levels(vm->bfs_levels, vm->csr_graph->node_count);
  return 0;
}

static int op_incident_count(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return -3;
  }
  if (vm->hypergraph == NULL) {
    return -8;
  }
  if (vm->regs[in->a] < 0) {
    return -9;
  }
  node = (uint32_t)vm->regs[in->a];
  if ((size_t)node >= vm->hypergraph->node_count) {
    return -9;
  }
  vm->regs[in->b] = (int64_t)(vm->hypergraph->node_offsets[node + 1U] - vm->hypergraph->node_offsets[node]);
  return 0;
}

static int op_hyperedge_size(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return -3;
  }
  if (vm->hypergraph == NULL) {
    return -8;
  }
  if (vm->regs[in->a] < 0) {
    return -10;
  }
  hyperedge = (uint32_t)vm->regs[in->a];
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return -10;
  }
  vm->regs[in->b] =
      (int64_t)(vm->hypergraph->hyperedge_offsets[hyperedge + 1U] - vm->hypergraph->hyperedge_offsets[hyperedge]);
  return 0;
}

static int op_incident_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return -3;
  }
  if (vm->hypergraph == NULL) {
    return -8;
  }
  if (vm->regs[in->a] < 0) {
    return -9;
  }
  node = (uint32_t)vm->regs[in->a];
  if ((size_t)node >= vm->hypergraph->node_count) {
    return -9;
  }
  vm->regs[in->b] = (int64_t)graphion_hypergraph_incident_sum(vm->hypergraph, node);
  return 0;
}

static int op_hyperedge_node_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return -3;
  }
  if (vm->hypergraph == NULL) {
    return -8;
  }
  if (vm->regs[in->a] < 0) {
    return -10;
  }
  hyperedge = (uint32_t)vm->regs[in->a];
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return -10;
  }
  vm->regs[in->b] = (int64_t)graphion_hypergraph_hyperedge_node_sum(vm->hypergraph, hyperedge);
  return 0;
}

#if defined(GRAPHION_VM_DISPATCH_SWITCH)
static int run_dispatch_switch(graphion_vm *vm) {
  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc++];
    int rc;
    switch (in.op) {
      case GVM_OP_NOP:
        rc = op_nop(vm, &in);
        break;
      case GVM_OP_HALT:
        rc = op_halt(vm, &in);
        break;
      case GVM_OP_MOV_IMM:
        rc = op_mov_imm(vm, &in);
        break;
      case GVM_OP_ADD:
        rc = op_add(vm, &in);
        break;
      case GVM_OP_BFS_LEVELS:
        rc = op_bfs_levels(vm, &in);
        break;
      case GVM_OP_INCIDENT_COUNT:
        rc = op_incident_count(vm, &in);
        break;
      case GVM_OP_HYPEREDGE_SIZE:
        rc = op_hyperedge_size(vm, &in);
        break;
      case GVM_OP_INCIDENT_SUM:
        rc = op_incident_sum(vm, &in);
        break;
      case GVM_OP_HYPEREDGE_NODE_SUM:
        rc = op_hyperedge_node_sum(vm, &in);
        break;
      default:
        return -4;
    }
    if (rc != 0) {
      return rc;
    }
  }
  return 0;
}
#endif

#if defined(GRAPHION_VM_DISPATCH_JUMPTABLE)
static int run_dispatch_jumptable(graphion_vm *vm) {
  typedef int (*handler_fn)(graphion_vm *, const graphion_insn *);
  static const handler_fn table[256] = {
      [GVM_OP_NOP] = op_nop,
      [GVM_OP_HALT] = op_halt,
      [GVM_OP_MOV_IMM] = op_mov_imm,
      [GVM_OP_ADD] = op_add,
      [GVM_OP_BFS_LEVELS] = op_bfs_levels,
      [GVM_OP_INCIDENT_COUNT] = op_incident_count,
      [GVM_OP_HYPEREDGE_SIZE] = op_hyperedge_size,
      [GVM_OP_INCIDENT_SUM] = op_incident_sum,
      [GVM_OP_HYPEREDGE_NODE_SUM] = op_hyperedge_node_sum,
  };

  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc++];
    const handler_fn fn = table[in.op];
    int rc;
    if (fn == NULL) {
      return -4;
    }
    rc = fn(vm, &in);
    if (rc != 0) {
      return rc;
    }
  }
  return 0;
}
#endif

#if defined(GRAPHION_VM_DISPATCH_COMPUTED_GOTO) && !defined(_MSC_VER)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
static int run_dispatch_computed_goto(graphion_vm *vm) {
  static void *dispatch[256] = {
      [GVM_OP_NOP] = &&L_nop,
      [GVM_OP_HALT] = &&L_halt,
      [GVM_OP_MOV_IMM] = &&L_mov_imm,
      [GVM_OP_ADD] = &&L_add,
      [GVM_OP_BFS_LEVELS] = &&L_bfs_levels,
      [GVM_OP_INCIDENT_COUNT] = &&L_incident_count,
      [GVM_OP_HYPEREDGE_SIZE] = &&L_hyperedge_size,
      [GVM_OP_INCIDENT_SUM] = &&L_incident_sum,
      [GVM_OP_HYPEREDGE_NODE_SUM] = &&L_hyperedge_node_sum,
  };

  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc++];
    int rc;
    if (dispatch[in.op] == NULL) {
      return -4;
    }
    goto *dispatch[in.op];
L_nop:
    rc = op_nop(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_halt:
    rc = op_halt(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_mov_imm:
    rc = op_mov_imm(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_add:
    rc = op_add(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bfs_levels:
    rc = op_bfs_levels(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_incident_count:
    rc = op_incident_count(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_hyperedge_size:
    rc = op_hyperedge_size(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_incident_sum:
    rc = op_incident_sum(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_hyperedge_node_sum:
    rc = op_hyperedge_node_sum(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  }
  return 0;
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

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

#if defined(GRAPHION_VM_DISPATCH_COMPUTED_GOTO) && !defined(_MSC_VER)
  return run_dispatch_computed_goto(vm);
#elif defined(GRAPHION_VM_DISPATCH_JUMPTABLE)
  return run_dispatch_jumptable(vm);
#else
  return run_dispatch_switch(vm);
#endif
}
