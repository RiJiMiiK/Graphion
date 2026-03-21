/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_PARSER_AST_H
#define GRAPHION_PARSER_AST_H

#include <stddef.h>
#include <stdint.h>

#include "compiler/ir.h"

typedef struct {
  size_t line;
  size_t column;
} graphion_ast_position;

typedef enum {
  GAST_OPERAND_NONE = 0,
  GAST_OPERAND_REGISTER = 1,
  GAST_OPERAND_IMMEDIATE = 2
} graphion_ast_operand_kind;

typedef struct {
  uint8_t kind;
  uint8_t reg;
  int32_t imm;
} graphion_ast_operand;

typedef struct {
  uint8_t op;
  graphion_ast_operand lhs;
  graphion_ast_operand rhs;
  graphion_ast_position start;
  graphion_ast_position end;
} graphion_ast_stmt;

typedef enum {
  GAST_OK = 0,
  GAST_ERR_INVALID_ARG = -1,
  GAST_ERR_CAPACITY = -2,
  GAST_ERR_INVALID_OPERAND = -3,
  GAST_ERR_INVALID_OPCODE = -4
} graphion_ast_result;

int graphion_ast_lower_to_ir(const graphion_ast_stmt *ast_program,
                             size_t ast_count,
                             graphion_ir_insn *out_ir,
                             size_t out_capacity,
                             size_t *out_count);

#endif
