/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_EXPR_H
#define GRAPHION_RUNTIME_INTERPRETER_EXPR_H

#include "runtime/interpreter/program.h"

typedef enum {
  OPERAND_LITERAL = 1,
  OPERAND_GLOBAL = 2
} parsed_operand_kind;

typedef struct {
  parsed_operand_kind kind;
  size_t const_index;
  size_t global_index;
} parsed_operand;

typedef enum {
  EXPR_RESULT_LITERAL = 1,
  EXPR_RESULT_GLOBAL = 2,
  EXPR_RESULT_REG = 3
} parsed_expr_kind;

typedef struct {
  parsed_expr_kind kind;
  size_t const_index;
  size_t global_index;
  uint8_t reg_index;
} parsed_expr_result;

int parse_identifier_token(const char **cursor,
                           char *buffer,
                           size_t buffer_size,
                           unsigned int line,
                           graphion_runtime_diagnostic *diagnostic);
int parse_string_literal(graphion_runtime_program *program,
                         const char **cursor,
                         graphion_vm_value *value_out,
                         unsigned int line,
                         graphion_runtime_diagnostic *diagnostic);
int parse_scalar_literal(graphion_runtime_program *program,
                         const char **cursor,
                         graphion_vm_value *value_out,
                         unsigned int line,
                         graphion_runtime_diagnostic *diagnostic);
int parse_operand(const char **cursor,
                  graphion_runtime_program *program,
                  parsed_operand *operand_out,
                  unsigned int line,
                  graphion_runtime_diagnostic *diagnostic);
int emit_load_operand(graphion_runtime_program *program,
                      const parsed_operand *operand,
                      uint8_t reg,
                      unsigned int line,
                      graphion_runtime_diagnostic *diagnostic);
int ensure_expr_in_reg(graphion_runtime_program *program,
                       parsed_expr_result *expr,
                       uint8_t reg,
                       unsigned int line,
                       graphion_runtime_diagnostic *diagnostic);
int copy_trimmed_segment(const char *start,
                         const char *end,
                         char *buffer,
                         size_t buffer_size,
                         unsigned int line,
                         graphion_runtime_diagnostic *diagnostic);
int scan_ternary_segments(const char *cursor,
                          const char **true_end_out,
                          const char **condition_start_out,
                          const char **condition_end_out,
                          const char **false_start_out,
                          const char **expr_end_out);
int parse_expression(const char **cursor,
                     graphion_runtime_program *program,
                     parsed_expr_result *result_out,
                     uint8_t base_reg,
                     unsigned int line,
                     graphion_runtime_diagnostic *diagnostic);

#endif
