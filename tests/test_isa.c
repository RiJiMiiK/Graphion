/* SPDX-License-Identifier: MIT */

#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include "parser/bytecode.h"
#include "vm/vm.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  const char *name;
  const uint8_t *bytes;
  size_t byte_len;
  int expected_rc;
  size_t out_capacity;
  size_t expected_count;
  graphion_insn expected_program[8];
} isa_decode_fixture;

typedef struct {
  const char *name;
  const graphion_insn *program;
  size_t program_len;
  int expected_load_rc;
  int expected_run_rc;
  int expect_halted;
  size_t expected_pc;
  int64_t expected_regs[16];
  int bind_csr;
  int bind_hypergraph;
  int bind_frontier;
  int expect_frontier_state;
  size_t expected_frontier_input_len;
  uint32_t expected_frontier_input[8];
  size_t expected_frontier_output_len;
  uint32_t expected_frontier_output[8];
} isa_execute_fixture;

static int regs_match(const int64_t *lhs, const int64_t *rhs, size_t count) {
  size_t i;
  for (i = 0U; i < count; ++i) {
    if (lhs[i] != rhs[i]) {
      return 0;
    }
  }
  return 1;
}

static int insns_match(const graphion_insn *lhs, const graphion_insn *rhs, size_t count) {
  size_t i;
  for (i = 0U; i < count; ++i) {
    if (lhs[i].op != rhs[i].op || lhs[i].a != rhs[i].a || lhs[i].b != rhs[i].b ||
        lhs[i].imm != rhs[i].imm) {
      return 0;
    }
  }
  return 1;
}

static int frontier_match(const uint32_t *lhs, const uint32_t *rhs, size_t count) {
  size_t i;
  for (i = 0U; i < count; ++i) {
    if (lhs[i] != rhs[i]) {
      return 0;
    }
  }
  return 1;
}

int test_isa_decode_golden_fixtures(void) {
  static const uint8_t valid_program[] = {
      GVM_OP_MOV_IMM, 0U, 0U, 7U,  0U,  0U,  0U,
      GVM_OP_MOV_IMM, 1U, 0U, 251U, 255U, 255U, 255U,
      GVM_OP_ADD,     0U, 1U, 0U,  0U,  0U,  0U,
      GVM_OP_HALT,    0U, 0U, 0U,  0U,  0U,  0U,
  };
  static const uint8_t truncated_program[] = {
      GVM_OP_HALT, 0U, 0U, 0U, 0U, 0U,
  };
  static const uint8_t frontier_program[] = {
      GVM_OP_FRONTIER_CLEAR, 0U, 0U, 0U, 0U, 0U, 0U,
      GVM_OP_FRONTIER_PUSH,  1U, 2U, 0U, 0U, 0U, 0U,
      GVM_OP_HALT,           0U, 0U, 0U, 0U, 0U, 0U,
  };
  static const isa_decode_fixture fixtures[] = {
      {
          "decode_valid_program",
          valid_program,
          sizeof(valid_program),
          GBC_OK,
          8U,
          4U,
          {
              {GVM_OP_MOV_IMM, 0U, 0U, 7},
              {GVM_OP_MOV_IMM, 1U, 0U, -5},
              {GVM_OP_ADD, 0U, 1U, 0},
              {GVM_OP_HALT, 0U, 0U, 0},
          },
      },
      {
          "decode_truncated_program",
          truncated_program,
          sizeof(truncated_program),
          GBC_ERR_TRUNCATED,
          2U,
          0U,
          {{0U, 0U, 0U, 0}},
      },
      {
          "decode_capacity_error",
          valid_program,
          sizeof(valid_program),
          GBC_ERR_CAPACITY,
          2U,
          0U,
          {{0U, 0U, 0U, 0}},
      },
      {
          "decode_frontier_push_program",
          frontier_program,
          sizeof(frontier_program),
          GBC_OK,
          8U,
          3U,
          {
              {GVM_OP_FRONTIER_CLEAR, 0U, 0U, 0},
              {GVM_OP_FRONTIER_PUSH, 1U, 2U, 0},
              {GVM_OP_HALT, 0U, 0U, 0},
          },
      },
  };
  size_t i;

  for (i = 0U; i < sizeof(fixtures) / sizeof(fixtures[0]); ++i) {
    graphion_insn decoded[8];
    size_t count = 0U;
    int rc;

    memset(decoded, 0, sizeof(decoded));
    rc = graphion_decode_bytecode(fixtures[i].bytes,
                                  fixtures[i].byte_len,
                                  decoded,
                                  fixtures[i].out_capacity,
                                  &count);
    if (rc != fixtures[i].expected_rc) {
      return (int)(10 + i);
    }
    if (rc == GBC_OK) {
      if (count != fixtures[i].expected_count) {
        return (int)(20 + i);
      }
      if (!insns_match(decoded, fixtures[i].expected_program, count)) {
        return (int)(30 + i);
      }
    }
  }

  return 0;
}

int test_isa_execute_golden_fixtures(void) {
  static const graphion_insn program_addition[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 7},
      {GVM_OP_MOV_IMM, 1U, 0U, 35},
      {GVM_OP_ADD, 0U, 1U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_invalid_reg[] = {
      {GVM_OP_MOV_IMM, 17U, 0U, 1},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_unknown_opcode[] = {
      {255U, 0U, 0U, 0},
  };
  static const graphion_insn program_bfs[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 0},
      {GVM_OP_BFS_LEVELS, 0U, 1U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_hypergraph_sum[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 1},
      {GVM_OP_INCIDENT_SUM, 0U, 1U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_frontier[] = {
      {GVM_OP_FRONTIER_CLEAR, 0U, 0U, 0},
      {GVM_OP_FRONTIER_FILTER_LT_IMM, 1U, 0U, 7},
      {GVM_OP_FRONTIER_SWAP, 2U, 0U, 0},
      {GVM_OP_FRONTIER_MAP_ADD_IMM, 3U, 0U, 1},
      {GVM_OP_FRONTIER_SWAP, 4U, 0U, 0},
      {GVM_OP_FRONTIER_REDUCE_SUM, 5U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_neighbors[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 2},
      {GVM_OP_NEIGHBORS_OF, 0U, 0U, 0},
      {GVM_OP_FRONTIER_SWAP, 1U, 0U, 0},
      {GVM_OP_NEIGHBORS_EXPAND, 2U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_neighbors_of[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 2},
      {GVM_OP_NEIGHBORS_OF, 0U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_incident_of[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 1},
      {GVM_OP_INCIDENT_OF, 0U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_hyperedge_nodes_of[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 1},
      {GVM_OP_HYPEREDGE_NODES_OF, 0U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const graphion_insn program_hyperedge_traversal[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 1},
      {GVM_OP_INCIDENT_OF, 0U, 0U, 0},
      {GVM_OP_FRONTIER_SWAP, 1U, 0U, 0},
      {GVM_OP_MOV_IMM, 2U, 0U, 1},
      {GVM_OP_HYPEREDGE_NODES_OF, 2U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  static const isa_execute_fixture fixtures[] = {
      {
          "exec_addition_halt",
          program_addition,
          sizeof(program_addition) / sizeof(program_addition[0]),
          0,
          0,
          1,
          4U,
          {42, 35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          0,
          0,
          0,
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_invalid_register_mov_imm",
          program_invalid_reg,
          sizeof(program_invalid_reg) / sizeof(program_invalid_reg[0]),
          0,
          GVM_ERR_INVALID_MOV_IMM_REG,
          0,
          1U,
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          0,
          0,
          0,
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_unknown_opcode",
          program_unknown_opcode,
          sizeof(program_unknown_opcode) / sizeof(program_unknown_opcode[0]),
          0,
          GVM_ERR_UNKNOWN_OPCODE,
          0,
          1U,
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          0,
          0,
          0,
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_bfs_levels",
          program_bfs,
          sizeof(program_bfs) / sizeof(program_bfs[0]),
          0,
          0,
          1,
          3U,
          {0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          1,
          0,
          0,
          0,
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_hypergraph_incident_sum",
          program_hypergraph_sum,
          sizeof(program_hypergraph_sum) / sizeof(program_hypergraph_sum[0]),
          0,
          0,
          1,
          3U,
          {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          1,
          0,
          0,
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_frontier_pipeline",
          program_frontier,
          sizeof(program_frontier) / sizeof(program_frontier[0]),
          0,
          0,
          1,
          7U,
          {0, 2, 2, 2, 2, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          0,
          1,
          1,
          2U,
          {2U, 5U, 0U, 0U, 0U, 0U, 0U, 0U},
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_neighbors_of_frontier",
          program_neighbors_of,
          sizeof(program_neighbors_of) / sizeof(program_neighbors_of[0]),
          0,
          0,
          1,
          3U,
          {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          1,
          0,
          1,
          1,
          4U,
          {1U, 4U, 7U, 10U, 0U, 0U, 0U, 0U},
          2U,
          {0U, 3U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_neighbor_iteration",
          program_neighbors,
          sizeof(program_neighbors) / sizeof(program_neighbors[0]),
          0,
          0,
          1,
          5U,
          {2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          1,
          0,
          1,
          1,
          2U,
          {0U, 3U, 0U, 0U, 0U, 0U, 0U, 0U},
          3U,
          {1U, 2U, 1U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_incident_of_frontier",
          program_incident_of,
          sizeof(program_incident_of) / sizeof(program_incident_of[0]),
          0,
          0,
          1,
          3U,
          {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          1,
          1,
          1,
          4U,
          {1U, 4U, 7U, 10U, 0U, 0U, 0U, 0U},
          2U,
          {0U, 1U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_hyperedge_nodes_of_frontier",
          program_hyperedge_nodes_of,
          sizeof(program_hyperedge_nodes_of) / sizeof(program_hyperedge_nodes_of[0]),
          0,
          0,
          1,
          3U,
          {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          1,
          1,
          1,
          4U,
          {1U, 4U, 7U, 10U, 0U, 0U, 0U, 0U},
          2U,
          {1U, 3U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "exec_hyperedge_traversal",
          program_hyperedge_traversal,
          sizeof(program_hyperedge_traversal) / sizeof(program_hyperedge_traversal[0]),
          0,
          0,
          1,
          6U,
          {1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          0,
          1,
          1,
          1,
          2U,
          {0U, 1U, 0U, 0U, 0U, 0U, 0U, 0U},
          2U,
          {1U, 3U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
  };
  const uint32_t csr_offsets[] = {0U, 2U, 3U, 5U, 6U};
  const uint32_t csr_neighbors[] = {1U, 2U, 3U, 0U, 3U, 1U};
  const uint32_t hg_node_offsets[] = {0U, 1U, 3U, 5U, 7U};
  const uint32_t hg_node_hyperedges[] = {0U, 0U, 1U, 0U, 2U, 1U, 2U};
  const uint32_t hg_hyperedge_offsets[] = {0U, 3U, 5U, 7U};
  const uint32_t hg_hyperedge_nodes[] = {0U, 1U, 2U, 1U, 3U, 2U, 3U};
  graphion_csr_graph csr_graph;
  graphion_hypergraph hypergraph;
  int32_t levels[4];
  uint32_t queue[4];
  uint32_t frontier_a[8] = {1U, 4U, 7U, 10U, 0U, 0U, 0U, 0U};
  uint32_t frontier_b[8] = {0U};
  size_t i;
  int rc;

  rc = graphion_csr_graph_init(&csr_graph, 4U, 6U, csr_offsets, csr_neighbors);
  if (rc != 0) {
    return 100;
  }
  rc = graphion_hypergraph_init(&hypergraph,
                                4U,
                                3U,
                                7U,
                                hg_node_offsets,
                                hg_node_hyperedges,
                                hg_hyperedge_offsets,
                                hg_hyperedge_nodes);
  if (rc != 0) {
    return 101;
  }

  for (i = 0U; i < sizeof(fixtures) / sizeof(fixtures[0]); ++i) {
    graphion_vm vm;
    graphion_vm_init(&vm);
    if (fixtures[i].bind_csr) {
      memset(levels, 0, sizeof(levels));
      memset(queue, 0, sizeof(queue));
      graphion_vm_bind_csr(&vm, &csr_graph, levels, queue, 4U);
    }
    if (fixtures[i].bind_hypergraph) {
      graphion_vm_bind_hypergraph(&vm, &hypergraph);
    }
    if (fixtures[i].bind_frontier) {
      memset(frontier_b, 0, sizeof(frontier_b));
      frontier_a[0] = 1U;
      frontier_a[1] = 4U;
      frontier_a[2] = 7U;
      frontier_a[3] = 10U;
      frontier_a[4] = 0U;
      frontier_a[5] = 0U;
      frontier_a[6] = 0U;
      frontier_a[7] = 0U;
      graphion_vm_bind_frontier(&vm, frontier_a, 4U, frontier_b, 8U);
    }

    rc = graphion_vm_load(&vm, fixtures[i].program, fixtures[i].program_len);
    if (rc != fixtures[i].expected_load_rc) {
      return (int)(110 + i);
    }
    if (rc == 0) {
      rc = graphion_vm_run(&vm);
      if (rc != fixtures[i].expected_run_rc) {
        return (int)(120 + i);
      }
      if ((vm.halted ? 1 : 0) != fixtures[i].expect_halted) {
        return (int)(130 + i);
      }
      if (vm.pc != fixtures[i].expected_pc) {
        return (int)(140 + i);
      }
      if (!regs_match(vm.regs, fixtures[i].expected_regs, 16U)) {
        return (int)(150 + i);
      }
      if (fixtures[i].expect_frontier_state) {
        if (vm.frontier_input_len != fixtures[i].expected_frontier_input_len) {
          return (int)(160 + i);
        }
        if (!frontier_match(vm.frontier_input, fixtures[i].expected_frontier_input, vm.frontier_input_len)) {
          return (int)(170 + i);
        }
        if (vm.frontier_output_len != fixtures[i].expected_frontier_output_len) {
          return (int)(180 + i);
        }
        if (!frontier_match(vm.frontier_output, fixtures[i].expected_frontier_output, vm.frontier_output_len)) {
          return (int)(190 + i);
        }
      }
    }
  }

  return 0;
}
