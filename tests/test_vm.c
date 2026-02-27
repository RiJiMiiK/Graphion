/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"
#include "graph/csr_graph.h"
#include "graph/hypergraph.h"

int test_vm_addition_program(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 35},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (!vm.halted) {
    return 3;
  }
  if (vm.regs[0] != 42) {
    return 4;
  }
  return 0;
}

int test_vm_invalid_register_fails(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 17, 0, 7},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != -2) {
    return 2;
  }
  return 0;
}

int test_vm_bfs_levels_opcode(void) {
  graphion_vm vm;
  graphion_csr_graph graph;
  const uint32_t offsets[] = {0, 2, 3, 5, 6};
  const uint32_t neighbors[] = {1, 2, 3, 0, 3, 1};
  int32_t levels[4];
  uint32_t queue[4];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 0},
      {GVM_OP_BFS_LEVELS, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  rc = graphion_csr_graph_init(&graph, 4U, 6U, offsets, neighbors);
  if (rc != 0) {
    return 1;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, levels, queue, 4U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 3;
  }
  if (vm.regs[1] != 4) {
    return 4;
  }
  return 0;
}

int test_vm_hypergraph_opcodes(void) {
  graphion_vm vm;
  graphion_hypergraph graph;
  const uint32_t node_offsets[] = {0, 1, 3, 5, 7};
  const uint32_t node_hyperedges[] = {0, 0, 1, 0, 2, 1, 2};
  const uint32_t hyperedge_offsets[] = {0, 3, 5, 7};
  const uint32_t hyperedge_nodes[] = {0, 1, 2, 1, 3, 2, 3};
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 1},
      {GVM_OP_INCIDENT_COUNT, 0, 1, 0},
      {GVM_OP_MOV_IMM, 2, 0, 0},
      {GVM_OP_HYPEREDGE_SIZE, 2, 3, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  rc = graphion_hypergraph_init(&graph, 4U, 3U, 7U, node_offsets, node_hyperedges, hyperedge_offsets,
                                hyperedge_nodes);
  if (rc != 0) {
    return 1;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_hypergraph(&vm, &graph);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 3;
  }
  if (vm.regs[1] != 2 || vm.regs[3] != 3) {
    return 4;
  }
  return 0;
}
