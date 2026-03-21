/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_PARSER_FRONTEND_H
#define GRAPHION_PARSER_FRONTEND_H

#include <stddef.h>

#include "compiler/ir.h"
#include "parser/ast.h"

typedef enum {
  GFE_OK = 0,
  GFE_ERR_INVALID_ARG = -1,
  GFE_ERR_CAPACITY = -2,
  GFE_ERR_PARSE = -3
} graphion_frontend_result;

typedef struct {
  size_t line;
  size_t column;
} graphion_frontend_position;

typedef enum {
  GFE_DIAG_NONE = 0,
  GFE_DIAG_INVALID_ARGUMENT = 1,
  GFE_DIAG_SOURCE_TOO_LARGE = 2,
  GFE_DIAG_INVALID_TOKEN = 3,
  GFE_DIAG_EXPECTED_MNEMONIC = 4,
  GFE_DIAG_UNKNOWN_MNEMONIC = 5,
  GFE_DIAG_EXPECTED_REGISTER = 6,
  GFE_DIAG_EXPECTED_COMMA = 7,
  GFE_DIAG_EXPECTED_IMMEDIATE = 8,
  GFE_DIAG_EXPECTED_LINE_END = 9
} graphion_frontend_diagnostic_code;

typedef struct {
  int code;
  graphion_frontend_position start;
  graphion_frontend_position end;
  const char *message;
} graphion_frontend_diagnostic;

int graphion_parse_source_to_ir(const char *source,
                                graphion_ir_insn *out_ir,
                                size_t out_capacity,
                                size_t *out_count);

int graphion_parse_source_to_ast(const char *source,
                                 graphion_ast_stmt *out_ast,
                                 size_t out_capacity,
                                 size_t *out_count);

int graphion_parse_source_to_ir_with_position(const char *source,
                                              graphion_ir_insn *out_ir,
                                              size_t out_capacity,
                                              size_t *out_count,
                                              graphion_frontend_position *error_pos);

int graphion_parse_source_to_ir_with_diagnostic(const char *source,
                                                graphion_ir_insn *out_ir,
                                                size_t out_capacity,
                                                size_t *out_count,
                                                graphion_frontend_diagnostic *diagnostic);

int graphion_parse_source_to_ast_with_diagnostic(const char *source,
                                                 graphion_ast_stmt *out_ast,
                                                 size_t out_capacity,
                                                 size_t *out_count,
                                                 graphion_frontend_diagnostic *diagnostic);

#endif
