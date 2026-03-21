/* SPDX-License-Identifier: MIT */

#include "parser/ast.h"

#include <stddef.h>

static int valid_ast_opcode(uint8_t op) {
  switch (op) {
    case GIR_OP_NOP:
    case GIR_OP_HALT:
    case GIR_OP_MOV_IMM:
    case GIR_OP_ADD:
    case GIR_OP_FRONTIER_CLEAR:
    case GIR_OP_FRONTIER_PUSH:
    case GIR_OP_FRONTIER_FILTER_LT_IMM:
    case GIR_OP_FRONTIER_MAP_ADD_IMM:
    case GIR_OP_FRONTIER_REDUCE_SUM:
    case GIR_OP_FRONTIER_SWAP:
    case GIR_OP_NEIGHBORS_OF:
    case GIR_OP_NEIGHBORS_EXPAND:
    case GIR_OP_INCIDENT_OF:
    case GIR_OP_HYPEREDGE_NODES_OF:
    case GIR_OP_NEIGHBOR_WEIGHT_SUM:
    case GIR_OP_NEIGHBOR_ATTR_SUM:
    case GIR_OP_BFS_LEVELS:
    case GIR_OP_INCIDENT_COUNT:
    case GIR_OP_HYPEREDGE_SIZE:
    case GIR_OP_INCIDENT_SUM:
    case GIR_OP_HYPEREDGE_NODE_SUM:
      return 1;
    default:
      return 0;
  }
}

static int lower_stmt(const graphion_ast_stmt *stmt, graphion_ir_insn *out_ir) {
  if (!valid_ast_opcode(stmt->op)) {
    return GAST_ERR_INVALID_OPCODE;
  }

  out_ir->op = stmt->op;
  out_ir->a = 0U;
  out_ir->b = 0U;
  out_ir->imm = 0;

  switch (stmt->op) {
    case GIR_OP_NOP:
    case GIR_OP_HALT:
      if (stmt->lhs.kind != GAST_OPERAND_NONE || stmt->rhs.kind != GAST_OPERAND_NONE) {
        return GAST_ERR_INVALID_OPERAND;
      }
      return GAST_OK;

    case GIR_OP_MOV_IMM:
    case GIR_OP_FRONTIER_CLEAR:
    case GIR_OP_FRONTIER_FILTER_LT_IMM:
    case GIR_OP_FRONTIER_MAP_ADD_IMM:
    case GIR_OP_FRONTIER_REDUCE_SUM:
    case GIR_OP_FRONTIER_SWAP:
    case GIR_OP_NEIGHBORS_OF:
    case GIR_OP_NEIGHBORS_EXPAND:
    case GIR_OP_INCIDENT_OF:
    case GIR_OP_HYPEREDGE_NODES_OF:
      if (stmt->lhs.kind != GAST_OPERAND_REGISTER || stmt->rhs.kind != GAST_OPERAND_IMMEDIATE) {
        return GAST_ERR_INVALID_OPERAND;
      }
      out_ir->a = stmt->lhs.reg;
      out_ir->imm = stmt->rhs.imm;
      return GAST_OK;

    case GIR_OP_ADD:
    case GIR_OP_FRONTIER_PUSH:
    case GIR_OP_NEIGHBOR_WEIGHT_SUM:
    case GIR_OP_NEIGHBOR_ATTR_SUM:
    case GIR_OP_BFS_LEVELS:
    case GIR_OP_INCIDENT_COUNT:
    case GIR_OP_HYPEREDGE_SIZE:
    case GIR_OP_INCIDENT_SUM:
    case GIR_OP_HYPEREDGE_NODE_SUM:
      if (stmt->lhs.kind != GAST_OPERAND_REGISTER || stmt->rhs.kind != GAST_OPERAND_REGISTER) {
        return GAST_ERR_INVALID_OPERAND;
      }
      out_ir->a = stmt->lhs.reg;
      out_ir->b = stmt->rhs.reg;
      return GAST_OK;
  }

  return GAST_ERR_INVALID_OPCODE;
}

int graphion_ast_lower_to_ir(const graphion_ast_stmt *ast_program,
                             size_t ast_count,
                             graphion_ir_insn *out_ir,
                             size_t out_capacity,
                             size_t *out_count) {
  size_t i;
  int rc;

  if (ast_program == NULL || out_ir == NULL || out_count == NULL) {
    return GAST_ERR_INVALID_ARG;
  }
  if (ast_count > out_capacity) {
    return GAST_ERR_CAPACITY;
  }

  for (i = 0U; i < ast_count; ++i) {
    rc = lower_stmt(&ast_program[i], &out_ir[i]);
    if (rc != GAST_OK) {
      return rc;
    }
  }

  *out_count = ast_count;
  return GAST_OK;
}
