/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stddef.h>

static int is_valid_reg(uint8_t reg) { return reg < 16U ? 1 : 0; }

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

  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc];
    vm->pc++;

    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        break;
      case GVM_OP_MOV_IMM:
        if (!is_valid_reg(in.a)) {
          return -2;
        }
        vm->regs[in.a] = (int64_t)in.imm;
        break;
      case GVM_OP_ADD:
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return -3;
        }
        vm->regs[in.a] += vm->regs[in.b];
        break;
      case GVM_OP_BFS_LEVELS: {
        uint32_t source;
        int rc;
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return -3;
        }
        if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
          return -5;
        }
        if (vm->regs[in.a] < 0) {
          return -6;
        }
        source = (uint32_t)vm->regs[in.a];
        if ((size_t)source >= vm->csr_graph->node_count) {
          return -6;
        }
        rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
        if (rc != 0) {
          return -7;
        }
        vm->regs[in.b] = count_visited_levels(vm->bfs_levels, vm->csr_graph->node_count);
        break;
      }
      case GVM_OP_INCIDENT_COUNT: {
        uint32_t node;
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return -3;
        }
        if (vm->hypergraph == NULL) {
          return -8;
        }
        if (vm->regs[in.a] < 0) {
          return -9;
        }
        node = (uint32_t)vm->regs[in.a];
        if ((size_t)node >= vm->hypergraph->node_count) {
          return -9;
        }
        vm->regs[in.b] = (int64_t)graphion_hypergraph_incident_count(vm->hypergraph, node);
        break;
      }
      case GVM_OP_HYPEREDGE_SIZE: {
        uint32_t hyperedge;
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return -3;
        }
        if (vm->hypergraph == NULL) {
          return -8;
        }
        if (vm->regs[in.a] < 0) {
          return -10;
        }
        hyperedge = (uint32_t)vm->regs[in.a];
        if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
          return -10;
        }
        vm->regs[in.b] = (int64_t)graphion_hypergraph_hyperedge_size(vm->hypergraph, hyperedge);
        break;
      }
      default:
        return -4;
    }
  }

  return 0;
}
