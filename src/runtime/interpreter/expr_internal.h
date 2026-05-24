/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_EXPR_INTERNAL_H
#define GRAPHION_RUNTIME_INTERPRETER_EXPR_INTERNAL_H

#include "runtime/interpreter/expr.h"

int try_parse_direct_builtin(const char **cursor,
                             graphion_runtime_program *program,
                             parsed_expr_result *result_out,
                             uint8_t base_reg,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic);

int try_parse_special_builtin(const char **cursor,
                              graphion_runtime_program *program,
                              parsed_expr_result *result_out,
                              uint8_t base_reg,
                              unsigned int line,
                              graphion_runtime_diagnostic *diagnostic);

int try_parse_opcode_builtin(const char **cursor,
                             graphion_runtime_program *program,
                             parsed_expr_result *result_out,
                             uint8_t base_reg,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic);

static inline int expr_cursor_starts_missing_argument(const char *cursor) {
  skip_spaces(&cursor);
  return *cursor == ')' || *cursor == ',';
}

static inline int fail_expected_builtin_argument(graphion_runtime_diagnostic *diagnostic,
                                                 unsigned int line,
                                                 const char *name,
                                                 const char *argument) {
  char message[128];

  if (argument == NULL) {
    snprintf(message, sizeof(message), "expected %s argument", name);
  } else {
    snprintf(message, sizeof(message), "expected %s %s argument", name, argument);
  }
  return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
}

#endif
