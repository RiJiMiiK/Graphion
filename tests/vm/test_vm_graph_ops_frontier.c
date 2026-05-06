/* SPDX-License-Identifier: MIT */

#include "test_vm_helpers.h"

int test_vm_frontier_primitives(void) {
  graphion_vm vm;
  uint32_t frontier_a[8] = {1U, 4U, 7U, 10U, 0U, 0U, 0U, 0U};
  uint32_t frontier_b[8] = {0U};
  const graphion_insn program[] = {
      {GVM_OP_FRONTIER_CLEAR, 0U, 0U, 0},
      {GVM_OP_FRONTIER_FILTER_LT_IMM, 1U, 0U, 7},
      {GVM_OP_FRONTIER_SWAP, 2U, 0U, 0},
      {GVM_OP_FRONTIER_MAP_ADD_IMM, 3U, 0U, 1},
      {GVM_OP_FRONTIER_SWAP, 4U, 0U, 0},
      {GVM_OP_FRONTIER_REDUCE_SUM, 5U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  graphion_vm_bind_frontier(&vm, frontier_a, 4U, frontier_b, 8U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  if (!vm.frontier_fastpath || vm.weighted_sum_fastpath || vm.arith_only_fastpath) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 3;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 4;
  }
  if (TEST_REG_I(vm, 0) != 0 || TEST_REG_I(vm, 1) != 2 || TEST_REG_I(vm, 2) != 2 || TEST_REG_I(vm, 3) != 2 ||
      TEST_REG_I(vm, 4) != 2 || TEST_REG_I(vm, 5) != 7) {
    return 5;
  }
  if (vm.frontier_input_len != 2U || vm.frontier_output_len != 0U) {
    return 6;
  }
  if (vm.frontier_input[0] != 2U || vm.frontier_input[1] != 5U) {
    return 7;
  }
  return 0;
}

int test_vm_frontier_errors(void) {
  graphion_vm vm;
  uint32_t frontier_a[2] = {0U, 0U};
  uint32_t frontier_b[2] = {0U, 0U};
  const graphion_insn overflow_program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 1},
      {GVM_OP_FRONTIER_PUSH, 0U, 1U, 0},
      {GVM_OP_MOV_IMM, 0U, 0U, 2},
      {GVM_OP_FRONTIER_PUSH, 0U, 1U, 0},
      {GVM_OP_MOV_IMM, 0U, 0U, 3},
      {GVM_OP_FRONTIER_PUSH, 0U, 1U, 0},
  };
  const graphion_insn invalid_map_program[] = {
      {GVM_OP_FRONTIER_MAP_ADD_IMM, 0U, 0U, -1},
  };
  int rc;

  graphion_vm_init(&vm);
  graphion_vm_bind_frontier(&vm, frontier_a, 0U, frontier_b, 2U);
  rc = graphion_vm_load(&vm, overflow_program, sizeof(overflow_program) / sizeof(overflow_program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_FRONTIER_OVERFLOW) {
    return 2;
  }
  if (vm.frontier_output_len != 2U || vm.frontier_output[0] != 1U || vm.frontier_output[1] != 2U) {
    return 3;
  }

  frontier_a[0] = 0U;
  graphion_vm_init(&vm);
  graphion_vm_bind_frontier(&vm, frontier_a, 1U, frontier_b, 2U);
  rc = graphion_vm_load(&vm, invalid_map_program, sizeof(invalid_map_program) / sizeof(invalid_map_program[0]));
  if (rc != 0) {
    return 4;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_FRONTIER_OVERFLOW) {
    return 5;
  }
  if (vm.frontier_output_len != 0U) {
    return 6;
  }
  return 0;
}

int test_vm_neighbor_iteration_primitives(void) {
  graphion_vm vm;
  graphion_csr_graph graph;
  const uint32_t offsets[] = {0U, 2U, 3U, 5U, 6U};
  const uint32_t neighbors[] = {1U, 2U, 3U, 0U, 3U, 1U};
  uint32_t frontier_a[8] = {0U, 2U, 0U, 0U, 0U, 0U, 0U, 0U};
  uint32_t frontier_b[8] = {0U};
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 2},
      {GVM_OP_NEIGHBORS_OF, 0U, 0U, 0},
      {GVM_OP_FRONTIER_SWAP, 1U, 0U, 0},
      {GVM_OP_NEIGHBORS_EXPAND, 2U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  int rc;

  rc = graphion_csr_graph_init(&graph, 4U, 6U, offsets, neighbors);
  if (rc != 0) {
    return 1;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, NULL, NULL, 0U);
  graphion_vm_bind_frontier(&vm, frontier_a, 2U, frontier_b, 8U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 3;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 4;
  }
  if (vm.frontier_output_len != 3U || vm.frontier_output[0] != 1U || vm.frontier_output[1] != 2U ||
      vm.frontier_output[2] != 1U) {
    return 5;
  }
  if (TEST_REG_I(vm, 1) != 2 || TEST_REG_I(vm, 2) != 3) {
    return 6;
  }
  return 0;
}

int test_vm_neighbor_iteration_errors(void) {
  graphion_vm vm;
  graphion_csr_graph graph;
  const uint32_t offsets[] = {0U, 2U, 3U, 5U, 6U};
  const uint32_t neighbors[] = {1U, 2U, 3U, 0U, 3U, 1U};
  uint32_t frontier_a[2] = {0U, 2U};
  uint32_t frontier_b[2] = {0U};
  const graphion_insn overflow_program[] = {
      {GVM_OP_NEIGHBORS_EXPAND, 0U, 0U, 0},
  };
  const graphion_insn invalid_node_program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 99},
      {GVM_OP_NEIGHBORS_OF, 0U, 0U, 0},
  };
  const graphion_insn invalid_reg_program[] = {
      {GVM_OP_NEIGHBORS_OF, 16U, 0U, 0},
  };
  int rc;

  rc = graphion_csr_graph_init(&graph, 4U, 6U, offsets, neighbors);
  if (rc != 0) {
    return 1;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, NULL, NULL, 0U);
  graphion_vm_bind_frontier(&vm, frontier_a, 2U, frontier_b, 2U);
  rc = graphion_vm_load(&vm, overflow_program, sizeof(overflow_program) / sizeof(overflow_program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_FRONTIER_OVERFLOW) {
    return 3;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, NULL, NULL, 0U);
  graphion_vm_bind_frontier(&vm, frontier_a, 2U, frontier_b, 2U);
  rc = graphion_vm_load(&vm, invalid_node_program, sizeof(invalid_node_program) / sizeof(invalid_node_program[0]));
  if (rc != 0) {
    return 4;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_NODE_ID) {
    return 5;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, NULL, NULL, 0U);
  graphion_vm_bind_frontier(&vm, frontier_a, 2U, frontier_b, 2U);
  rc = graphion_vm_load(&vm, invalid_reg_program, sizeof(invalid_reg_program) / sizeof(invalid_reg_program[0]));
  if (rc != 0) {
    return 6;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_REG) {
    return 7;
  }
  return 0;
}

int test_vm_weighted_graph_opcodes(void) {
  graphion_vm vm;
  graphion_csr_graph graph;
  const uint32_t offsets[] = {0U, 2U, 3U, 5U, 6U};
  const uint32_t neighbors[] = {1U, 2U, 3U, 0U, 3U, 1U};
  const int64_t weights[] = {5, 8, 13, 21, 34, 55};
  const uint32_t edge_attrs[] = {10U, 11U, 12U, 13U, 14U, 15U};
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 0},
      {GVM_OP_NEIGHBOR_WEIGHT_SUM, 0U, 1U, 0},
      {GVM_OP_MOV_IMM, 2U, 0U, 2},
      {GVM_OP_NEIGHBOR_ATTR_SUM, 2U, 3U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  int rc;

  rc = graphion_csr_graph_init_with_edge_data(&graph, 4U, 6U, offsets, neighbors, weights, edge_attrs);
  if (rc != 0) {
    return 1;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, NULL, NULL, 0U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  if (!vm.weighted_sum_fastpath || vm.frontier_fastpath || vm.arith_only_fastpath) {
    return 3;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 4;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 5;
  }
  if (TEST_REG_I(vm, 1) != 13 || TEST_REG_I(vm, 3) != 27) {
    return 6;
  }
  return 0;
}

int test_vm_weighted_graph_opcode_errors(void) {
  graphion_vm vm;
  graphion_csr_graph weighted_graph;
  graphion_csr_graph topology_graph;
  const uint32_t offsets[] = {0U, 2U, 3U, 5U, 6U};
  const uint32_t neighbors[] = {1U, 2U, 3U, 0U, 3U, 1U};
  const int64_t weights[] = {5, 8, 13, 21, 34, 55};
  const graphion_insn weight_program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 0},
      {GVM_OP_NEIGHBOR_WEIGHT_SUM, 0U, 1U, 0},
  };
  const graphion_insn attr_program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 0},
      {GVM_OP_NEIGHBOR_ATTR_SUM, 0U, 1U, 0},
  };
  const graphion_insn invalid_node_program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 99},
      {GVM_OP_NEIGHBOR_WEIGHT_SUM, 0U, 1U, 0},
  };
  int rc;

  rc = graphion_csr_graph_init(&topology_graph, 4U, 6U, offsets, neighbors);
  if (rc != 0) {
    return 1;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &topology_graph, NULL, NULL, 0U);
  rc = graphion_vm_load(&vm, weight_program, sizeof(weight_program) / sizeof(weight_program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_CSR_WEIGHTS_UNBOUND) {
    return 3;
  }

  rc = graphion_csr_graph_init_with_edge_data(&weighted_graph, 4U, 6U, offsets, neighbors, weights, NULL);
  if (rc != 0) {
    return 4;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &weighted_graph, NULL, NULL, 0U);
  rc = graphion_vm_load(&vm, attr_program, sizeof(attr_program) / sizeof(attr_program[0]));
  if (rc != 0) {
    return 5;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_CSR_EDGE_ATTRS_UNBOUND) {
    return 6;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &weighted_graph, NULL, NULL, 0U);
  rc = graphion_vm_load(&vm, invalid_node_program, sizeof(invalid_node_program) / sizeof(invalid_node_program[0]));
  if (rc != 0) {
    return 7;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_NODE_ID) {
    return 8;
  }
  return 0;
}

int test_vm_hyperedge_traversal_primitives(void) {
  graphion_vm vm;
  graphion_hypergraph graph;
  const uint32_t node_offsets[] = {0U, 1U, 3U, 5U, 7U};
  const uint32_t node_hyperedges[] = {0U, 0U, 1U, 0U, 2U, 1U, 2U};
  const uint32_t hyperedge_offsets[] = {0U, 3U, 5U, 7U};
  const uint32_t hyperedge_nodes[] = {0U, 1U, 2U, 1U, 3U, 2U, 3U};
  uint32_t frontier_a[8] = {0U};
  uint32_t frontier_b[8] = {0U};
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 1},
      {GVM_OP_INCIDENT_OF, 0U, 0U, 0},
      {GVM_OP_FRONTIER_SWAP, 1U, 0U, 0},
      {GVM_OP_MOV_IMM, 2U, 0U, 1},
      {GVM_OP_HYPEREDGE_NODES_OF, 2U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  int rc;

  rc = graphion_hypergraph_init(&graph,
                                4U,
                                3U,
                                7U,
                                node_offsets,
                                node_hyperedges,
                                hyperedge_offsets,
                                hyperedge_nodes);
  if (rc != 0) {
    return 1;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_hypergraph(&vm, &graph);
  graphion_vm_bind_frontier(&vm, frontier_a, 0U, frontier_b, 8U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 3;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 4;
  }
  if (vm.frontier_input_len != 2U || vm.frontier_input[0] != 0U || vm.frontier_input[1] != 1U) {
    return 5;
  }
  if (vm.frontier_output_len != 2U || vm.frontier_output[0] != 1U || vm.frontier_output[1] != 3U) {
    return 6;
  }
  if (TEST_REG_I(vm, 1) != 2 || TEST_REG_I(vm, 2) != 1) {
    return 7;
  }
  return 0;
}

int test_vm_hyperedge_traversal_errors(void) {
  graphion_vm vm;
  graphion_hypergraph graph;
  const uint32_t node_offsets[] = {0U, 1U, 3U, 5U, 7U};
  const uint32_t node_hyperedges[] = {0U, 0U, 1U, 0U, 2U, 1U, 2U};
  const uint32_t hyperedge_offsets[] = {0U, 3U, 5U, 7U};
  const uint32_t hyperedge_nodes[] = {0U, 1U, 2U, 1U, 3U, 2U, 3U};
  uint32_t frontier_a[2] = {0U};
  uint32_t frontier_b[2] = {0U};
  const graphion_insn overflow_program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 1},
      {GVM_OP_INCIDENT_OF, 0U, 0U, 0},
  };
  const graphion_insn invalid_hyperedge_program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 99},
      {GVM_OP_HYPEREDGE_NODES_OF, 0U, 0U, 0},
  };
  const graphion_insn invalid_reg_program[] = {
      {GVM_OP_HYPEREDGE_NODES_OF, 16U, 0U, 0},
  };
  int rc;

  rc = graphion_hypergraph_init(&graph,
                                4U,
                                3U,
                                7U,
                                node_offsets,
                                node_hyperedges,
                                hyperedge_offsets,
                                hyperedge_nodes);
  if (rc != 0) {
    return 1;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_hypergraph(&vm, &graph);
  graphion_vm_bind_frontier(&vm, frontier_a, 0U, frontier_b, 1U);
  rc = graphion_vm_load(&vm, overflow_program, sizeof(overflow_program) / sizeof(overflow_program[0]));
  if (rc != 0) {
    return 2;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_FRONTIER_OVERFLOW) {
    return 3;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_hypergraph(&vm, &graph);
  graphion_vm_bind_frontier(&vm, frontier_a, 0U, frontier_b, 2U);
  rc = graphion_vm_load(&vm, invalid_hyperedge_program, sizeof(invalid_hyperedge_program) / sizeof(invalid_hyperedge_program[0]));
  if (rc != 0) {
    return 4;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_HYPEREDGE_ID) {
    return 5;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_hypergraph(&vm, &graph);
  graphion_vm_bind_frontier(&vm, frontier_a, 0U, frontier_b, 2U);
  rc = graphion_vm_load(&vm, invalid_reg_program, sizeof(invalid_reg_program) / sizeof(invalid_reg_program[0]));
  if (rc != 0) {
    return 6;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_REG) {
    return 7;
  }
  return 0;
}
