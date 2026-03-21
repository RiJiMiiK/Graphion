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
    }
  }

  return 0;
}
