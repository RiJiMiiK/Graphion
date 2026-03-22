/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"
#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include <limits.h>
#include <string.h>

#define TEST_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

static void test_set_reg_i(graphion_vm *vm, uint8_t reg, int64_t value) {
  vm->regs[reg].kind = GVM_VALUE_INT;
  vm->regs[reg].as.int_value = value;
}

static void test_set_value_float(graphion_vm_value *value, double number) {
  value->kind = GVM_VALUE_FLOAT;
  value->as.float_value = number;
}

static void test_set_value_string(graphion_vm_value *value, const char *text) {
  value->kind = GVM_VALUE_STRING;
  value->as.string_value = text;
}

static int run_vm_program(graphion_vm *vm, const graphion_insn *program, size_t len) {
  int rc;
  graphion_vm_init(vm);
  rc = graphion_vm_load(vm, program, len);
  if (rc != 0) {
    return rc;
  }
  return graphion_vm_run(vm);
}

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
  if (TEST_REG_I(vm, 0) != 42) {
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
  if (rc != GVM_ERR_INVALID_MOV_IMM_REG) {
    return 2;
  }
  return 0;
}

int test_vm_typed_register_defaults(void) {
  graphion_vm vm;
  size_t i;

  graphion_vm_init(&vm);
  for (i = 0U; i < 16U; ++i) {
    if (vm.regs[i].kind != GVM_VALUE_INT || vm.regs[i].as.int_value != 0) {
      return 1;
    }
  }
  return 0;
}

int test_vm_value_movement_and_globals(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_GLOBAL, 1, 0, 0},
      {GVM_OP_MOV, 2, 1, 0},
      {GVM_OP_LOAD_CONST, 3, 0, 1},
      {GVM_OP_STORE_GLOBAL, 3, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 3.5);
  test_set_value_string(&const_pool[1], "graphion");
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 3.5) {
    return 4;
  }
  if (vm.regs[1].kind != GVM_VALUE_FLOAT || vm.regs[1].as.float_value != 3.5) {
    return 5;
  }
  if (vm.regs[2].kind != GVM_VALUE_FLOAT || vm.regs[2].as.float_value != 3.5) {
    return 6;
  }
  if (globals[1].kind != GVM_VALUE_STRING || strcmp(globals[1].as.string_value, "graphion") != 0) {
    return 7;
  }
  return 0;
}

int test_vm_typed_value_errors(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[1];
  graphion_vm_value globals[1];
  const graphion_insn bad_const_program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 1},
  };
  const graphion_insn bad_global_program[] = {
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.25);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  rc = graphion_vm_load(&vm, bad_const_program, sizeof(bad_const_program) / sizeof(bad_const_program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_CONST_INDEX) {
    return 2;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, bad_global_program, sizeof(bad_global_program) / sizeof(bad_global_program[0]));
  if (rc != 0) {
    return 3;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_GLOBAL_INDEX) {
    return 4;
  }

  graphion_vm_init(&vm);
  test_set_reg_i(&vm, 0U, 7);
  vm.regs[1].kind = GVM_VALUE_FLOAT;
  vm.regs[1].as.float_value = 2.0;
  rc = graphion_vm_load(&vm, (const graphion_insn[]){{GVM_OP_ADD, 0, 1, 0}}, 1U);
  if (rc != 0) {
    return 5;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return 6;
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

int test_vm_superinstruction_add_pair_semantics(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 1},  {GVM_OP_MOV_IMM, 1, 0, 2}, {GVM_OP_MOV_IMM, 2, 0, 3},
      {GVM_OP_ADD, 0, 1, 0},      {GVM_OP_ADD, 0, 2, 0},     {GVM_OP_MOV_IMM, 3, 0, 5},
      {GVM_OP_ADD, 3, 3, 0},      {GVM_OP_ADD, 3, 1, 0},     {GVM_OP_HALT, 0, 0, 0},
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
  if (TEST_REG_I(vm, 0) != 6) {
    return 4;
  }
  if (TEST_REG_I(vm, 3) != 12) {
    return 5;
  }
  return 0;
}

int test_vm_superinstruction_movimm_add_semantics(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},   {GVM_OP_ADD, 1, 0, 0},      {GVM_OP_MOV_IMM, 2, 0, 3},
      {GVM_OP_ADD, 2, 2, 0},       {GVM_OP_MOV_IMM, 3, 0, -2}, {GVM_OP_ADD, 1, 3, 0},
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
  if (TEST_REG_I(vm, 0) != 7) {
    return 4;
  }
  if (TEST_REG_I(vm, 1) != 5) {
    return 5;
  }
  if (TEST_REG_I(vm, 2) != 6) {
    return 6;
  }
  if (TEST_REG_I(vm, 3) != -2) {
    return 7;
  }
  return 0;
}

int test_vm_deterministic_mode_toggle(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 35},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  graphion_vm_set_deterministic(&vm, true);
  if (!vm.deterministic_mode) {
    return 1;
  }
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  if (!vm.arith_only_fastpath || !vm.arith_only_halt_terminated) {
    return 3;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 4;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 5;
  }
  if (TEST_REG_I(vm, 0) != 42 || TEST_REG_I(vm, 1) != 35) {
    return 6;
  }

  graphion_vm_set_deterministic(&vm, false);
  if (vm.deterministic_mode) {
    return 7;
  }
  return 0;
}

int test_vm_deterministic_mode_unknown_opcode(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {99, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  graphion_vm_set_deterministic(&vm, true);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_UNKNOWN_OPCODE) {
    return 2;
  }
  if (vm.halted) {
    return 3;
  }
  if (vm.pc != 2U) {
    return 4;
  }
  if (TEST_REG_I(vm, 0) != 7) {
    return 5;
  }
  return 0;
}

int test_vm_deterministic_mode_graph_semantics(void) {
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
  graphion_vm_set_deterministic(&vm, true);
  graphion_vm_bind_csr(&vm, &graph, levels, queue, 4U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 2;
  }
  if (vm.arith_only_fastpath || vm.arith_only_halt_terminated) {
    return 3;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 4;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 5;
  }
  if (TEST_REG_I(vm, 1) != 4) {
    return 6;
  }
  return 0;
}

int test_vm_add_wraparound_semantics(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 1, 0, 1},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  test_set_reg_i(&vm, 0U, INT64_MAX);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (TEST_REG_I(vm, 0) != INT64_MIN) {
    return 3;
  }
  if (!vm.halted) {
    return 4;
  }
  return 0;
}

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
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 3;
  }
  if (TEST_REG_I(vm, 0) != 0 || TEST_REG_I(vm, 1) != 2 || TEST_REG_I(vm, 2) != 2 || TEST_REG_I(vm, 3) != 2 ||
      TEST_REG_I(vm, 4) != 2 || TEST_REG_I(vm, 5) != 7) {
    return 4;
  }
  if (vm.frontier_input_len != 2U || vm.frontier_output_len != 0U) {
    return 5;
  }
  if (vm.frontier_input[0] != 2U || vm.frontier_input[1] != 5U) {
    return 6;
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
  if (rc != GVM_ERR_INVALID_FRONTIER_VALUE) {
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
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 3;
  }
  if (!vm.halted || vm.pc != (sizeof(program) / sizeof(program[0]))) {
    return 4;
  }
  if (TEST_REG_I(vm, 1) != 13 || TEST_REG_I(vm, 3) != 27) {
    return 5;
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
  return 0;
}

int test_vm_snapshot_format(void) {
  graphion_vm vm;
  char snapshot[512];
  char tiny[16];
  size_t snapshot_len;
  size_t tiny_len;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 35},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  const char *required_lines[] = {
      "GRAPHION_VM_SNAPSHOT_V1\n",
      "pc=4\n",
      "program_bound=1\n",
      "program_len=4\n",
      "halted=1\n",
      "deterministic_mode=1\n",
      "arith_only_fastpath=1\n",
      "arith_only_halt_terminated=1\n",
      "weighted_sum_fastpath=0\n",
      "const_bound=0\n",
      "const_count=0\n",
      "globals_bound=0\n",
      "global_count=0\n",
      "csr_bound=0\n",
      "hypergraph_bound=0\n",
      "frontier_bound=0\n",
      "frontier_input_len=0\n",
      "frontier_output_len=0\n",
      "frontier_capacity=0\n",
      "regs=[42,35,0,0,0,0,0,0,0,0,0,0,0,0,0,0]\n",
  };
  int rc;
  size_t i;

  graphion_vm_init(&vm);
  graphion_vm_set_deterministic(&vm, true);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }

  snapshot_len = graphion_vm_write_snapshot(&vm, snapshot, sizeof(snapshot));
  if (snapshot_len == 0U || snapshot_len >= sizeof(snapshot)) {
    return 3;
  }
  for (i = 0U; i < sizeof(required_lines) / sizeof(required_lines[0]); ++i) {
    if (strstr(snapshot, required_lines[i]) == NULL) {
      return 4;
    }
  }

  tiny_len = graphion_vm_write_snapshot(&vm, tiny, sizeof(tiny));
  if (tiny_len != snapshot_len) {
    return 5;
  }
  if (tiny[sizeof(tiny) - 1U] != '\0') {
    return 6;
  }
  if (strncmp(tiny, snapshot, sizeof(tiny) - 1U) != 0) {
    return 7;
  }
  return 0;
}

int test_vm_fastpath_shape_cache_load_flags(void) {
  graphion_vm vm1;
  graphion_vm vm2;
  graphion_vm vm3;
  graphion_vm vm4;
  const graphion_insn program_fast[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_ADD, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  const graphion_insn program_generic[] = {
      {GVM_OP_MOV_IMM, 0, 0, 0},
      {GVM_OP_BFS_LEVELS, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };

  graphion_vm_init(&vm1);
  graphion_vm_init(&vm2);
  graphion_vm_init(&vm3);
  graphion_vm_init(&vm4);

  if (graphion_vm_load(&vm1, program_fast, sizeof(program_fast) / sizeof(program_fast[0])) != 0) {
    return 1;
  }
  if (!vm1.arith_only_fastpath || !vm1.arith_only_halt_terminated) {
    return 2;
  }

  if (graphion_vm_load(&vm2, program_fast, sizeof(program_fast) / sizeof(program_fast[0])) != 0) {
    return 3;
  }
  if (!vm2.arith_only_fastpath || !vm2.arith_only_halt_terminated) {
    return 4;
  }

  if (graphion_vm_load(&vm3, program_generic, sizeof(program_generic) / sizeof(program_generic[0])) != 0) {
    return 5;
  }
  if (vm3.arith_only_fastpath || vm3.arith_only_halt_terminated) {
    return 6;
  }

  if (graphion_vm_load(&vm4, program_generic, sizeof(program_generic) / sizeof(program_generic[0])) != 0) {
    return 7;
  }
  if (vm4.arith_only_fastpath || vm4.arith_only_halt_terminated) {
    return 8;
  }

  return 0;
}

int test_vm_fastpath_shape_cache_same_pointer_content_change(void) {
  graphion_vm vm_fast;
  graphion_vm vm_generic;
  graphion_insn program[3] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_ADD, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };

  graphion_vm_init(&vm_fast);
  if (graphion_vm_load(&vm_fast, program, sizeof(program) / sizeof(program[0])) != 0) {
    return 1;
  }
  if (!vm_fast.arith_only_fastpath || !vm_fast.arith_only_halt_terminated) {
    return 2;
  }

  program[0].imm = 0;
  program[1].op = GVM_OP_BFS_LEVELS;
  program[1].a = 0;
  program[1].b = 1;
  program[1].imm = 0;

  graphion_vm_init(&vm_generic);
  if (graphion_vm_load(&vm_generic, program, sizeof(program) / sizeof(program[0])) != 0) {
    return 3;
  }
  if (vm_generic.arith_only_fastpath || vm_generic.arith_only_halt_terminated) {
    return 4;
  }

  return 0;
}

int test_vm_dispatch_variant_edge_semantics(void) {
  graphion_vm vm;
  const graphion_insn halt_before_invalid[] = {
      {GVM_OP_MOV_IMM, 0, 0, 5},
      {GVM_OP_HALT, 0, 0, 0},
      {255, 0, 0, 0},
  };
  const graphion_insn invalid_opcode[] = {
      {GVM_OP_MOV_IMM, 0, 0, 5},
      {255, 0, 0, 0},
  };
  const graphion_insn nop_halt[] = {
      {GVM_OP_NOP, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  rc = run_vm_program(&vm, halt_before_invalid, sizeof(halt_before_invalid) / sizeof(halt_before_invalid[0]));
  if (rc != 0) {
    return 1;
  }
  if (!vm.halted || TEST_REG_I(vm, 0) != 5 || vm.pc != 2U) {
    return 2;
  }

  rc = run_vm_program(&vm, invalid_opcode, sizeof(invalid_opcode) / sizeof(invalid_opcode[0]));
  if (rc != -4) {
    return 3;
  }
  if (vm.halted || vm.pc != 2U || TEST_REG_I(vm, 0) != 5) {
    return 4;
  }

  rc = run_vm_program(&vm, nop_halt, sizeof(nop_halt) / sizeof(nop_halt[0]));
  if (rc != 0) {
    return 5;
  }
  if (!vm.halted || vm.pc != 2U) {
    return 6;
  }

  return 0;
}
