/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_vm_helpers.h"

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
  if (TEST_REG_I(vm, 1) != 4) {
    return 4;
  }
  return 0;
}

int test_vm_bfs_level_count_opcode(void) {
  graphion_vm vm;
  graphion_csr_graph graph;
  const uint32_t offsets[] = {0, 2, 3, 5, 6};
  const uint32_t neighbors[] = {1, 2, 3, 0, 3, 1};
  int32_t levels[4];
  uint32_t queue[4];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 0},
      {GVM_OP_BFS_LEVEL_COUNT, 0, 1, 0},
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
  if (TEST_REG_I(vm, 1) != 3) {
    return 4;
  }
  return 0;
}

int test_vm_bfs_order_opcode(void) {
  graphion_vm vm;
  graphion_csr_graph graph;
  const uint32_t offsets[] = {0, 2, 3, 5, 6};
  const uint32_t neighbors[] = {1, 2, 3, 0, 3, 1};
  int32_t levels[4];
  uint32_t queue[4];
  uint32_t frontier[4];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 0},
      {GVM_OP_BFS_ORDER, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  rc = graphion_csr_graph_init(&graph, 4U, 6U, offsets, neighbors);
  if (rc != 0) {
    return 1;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, levels, queue, 4U);
  graphion_vm_bind_frontier(&vm, frontier, 0U, frontier, 4U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 3;
  }
  if (TEST_REG_I(vm, 1) != 4 || vm.frontier_output_len != 4U) {
    return 4;
  }
  if (vm.frontier_output[0] != 0U || vm.frontier_output[1] != 1U || vm.frontier_output[2] != 2U ||
      vm.frontier_output[3] != 3U) {
    return 5;
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
      {GVM_OP_INCIDENT_SUM, 0, 4, 0},
      {GVM_OP_HYPEREDGE_NODE_SUM, 2, 5, 0},
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
  if (TEST_REG_I(vm, 1) != 2 || TEST_REG_I(vm, 3) != 3 || TEST_REG_I(vm, 4) != 1 || TEST_REG_I(vm, 5) != 3) {
    return 4;
  }
  return 0;
}
