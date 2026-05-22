/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/expr_internal.h"

#include <string.h>

static void set_result_reg(parsed_expr_result *result, uint8_t reg_index) {
  result->kind = EXPR_RESULT_REG;
  result->reg_index = reg_index;
  result->const_index = 0U;
  result->global_index = 0U;
}

static int parse_named_unary_builtin(const char **cursor,
                                     const char *name,
                                     graphion_opcode opcode,
                                     graphion_runtime_program *program,
                                     parsed_expr_result *result_out,
                                     uint8_t base_reg,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result value;
  const uint8_t target_reg = base_reg;
  const char *after_name = *cursor + strlen(name);
  int rc;

  skip_spaces(&after_name);
  if (*after_name != '(') {
    char message[96];

    snprintf(message, sizeof(message), "expected '(' after %s", name);
    *cursor = after_name;
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  *cursor = after_name + 1;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail_expected_builtin_argument(diagnostic, line, name, NULL);
  }
  rc = parse_expression(cursor, program, &value, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ')') {
    char message[112];

    snprintf(message, sizeof(message), "expected ')' after %s argument", name);
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  (*cursor)++;
  rc = ensure_expr_in_reg(program, &value, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, opcode, target_reg, 0U, 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  set_result_reg(result_out, target_reg);
  return 1;
}

static int parse_named_binary_builtin(const char **cursor,
                                      const char *name,
                                      graphion_opcode opcode,
                                      graphion_runtime_program *program,
                                      parsed_expr_result *result_out,
                                      uint8_t base_reg,
                                      unsigned int line,
                                      graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  parsed_expr_result rhs;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  const char *after_name = *cursor + strlen(name);
  int rc;

  skip_spaces(&after_name);
  if (*after_name != '(') {
    char message[96];

    snprintf(message, sizeof(message), "expected '(' after %s", name);
    *cursor = after_name;
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  *cursor = after_name + 1;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail_expected_builtin_argument(diagnostic, line, name, "first");
  }
  rc = parse_expression(cursor, program, &lhs, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ',') {
    char message[120];

    snprintf(message, sizeof(message), "expected ',' between %s arguments", name);
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  (*cursor)++;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail_expected_builtin_argument(diagnostic, line, name, "second");
  }
  rc = parse_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ')') {
    char message[120];

    snprintf(message, sizeof(message), "expected ')' after %s arguments", name);
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  (*cursor)++;
  rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = ensure_expr_in_reg(program, &rhs, scratch_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, opcode, target_reg, scratch_reg, 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  set_result_reg(result_out, target_reg);
  return 1;
}

static int parse_log_constant_base_builtin(const char **cursor,
                                           const char *name,
                                           int64_t base_value,
                                           graphion_runtime_program *program,
                                           parsed_expr_result *result_out,
                                           uint8_t base_reg,
                                           unsigned int line,
                                           graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result value;
  const uint8_t target_reg = base_reg;
  const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
  const char *after_name = *cursor + strlen(name);
  int rc;

  skip_spaces(&after_name);
  if (*after_name != '(') {
    char message[96];

    snprintf(message, sizeof(message), "expected '(' after %s", name);
    *cursor = after_name;
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  *cursor = after_name + 1;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail_expected_builtin_argument(diagnostic, line, name, NULL);
  }
  rc = parse_expression(cursor, program, &value, base_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ')') {
    char message[112];

    snprintf(message, sizeof(message), "expected ')' after %s argument", name);
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  (*cursor)++;
  rc = ensure_expr_in_reg(program, &value, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit_load_int(program, scratch_reg, base_value, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, GVM_OP_LOG, target_reg, scratch_reg, 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  set_result_reg(result_out, target_reg);
  return 1;
}

static int parse_clamp_builtin(const char **cursor,
                               graphion_runtime_program *program,
                               parsed_expr_result *result_out,
                               uint8_t base_reg,
                               unsigned int line,
                               graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result value;
  parsed_expr_result lo;
  parsed_expr_result hi;
  const uint8_t target_reg = base_reg;
  const uint8_t lo_reg = (uint8_t)(base_reg + 1U);
  const uint8_t hi_reg = (uint8_t)(base_reg + 2U);
  const char *after_name = *cursor + 5;
  int rc;

  skip_spaces(&after_name);
  if (*after_name != '(') {
    *cursor = after_name;
    return fail(diagnostic, line, 1U, "expected '(' after clamp", GINT_ERR_PARSE);
  }
  *cursor = after_name + 1;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail(diagnostic, line, 1U, "expected clamp value", GINT_ERR_PARSE);
  }
  rc = parse_expression(cursor, program, &value, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ',') {
    return fail(diagnostic, line, 1U, "expected ',' after clamp value", GINT_ERR_PARSE);
  }
  (*cursor)++;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail(diagnostic, line, 1U, "expected clamp lower bound", GINT_ERR_PARSE);
  }
  rc = parse_expression(cursor, program, &lo, lo_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ',') {
    return fail(diagnostic, line, 1U, "expected ',' after clamp lower bound", GINT_ERR_PARSE);
  }
  (*cursor)++;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail(diagnostic, line, 1U, "expected clamp upper bound", GINT_ERR_PARSE);
  }
  rc = parse_expression(cursor, program, &hi, hi_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ')') {
    return fail(diagnostic, line, 1U, "expected ')' after clamp arguments", GINT_ERR_PARSE);
  }
  (*cursor)++;
  rc = ensure_expr_in_reg(program, &value, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = ensure_expr_in_reg(program, &lo, lo_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = ensure_expr_in_reg(program, &hi, hi_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, GVM_OP_CLAMP, target_reg, lo_reg, hi_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  set_result_reg(result_out, target_reg);
  return 1;
}

static int parse_fma_builtin(const char **cursor,
                             graphion_runtime_program *program,
                             parsed_expr_result *result_out,
                             uint8_t base_reg,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  parsed_expr_result lhs;
  parsed_expr_result mul_rhs;
  parsed_expr_result add_rhs;
  const uint8_t target_reg = base_reg;
  const uint8_t mul_rhs_reg = (uint8_t)(base_reg + 1U);
  const uint8_t add_rhs_reg = (uint8_t)(base_reg + 2U);
  const char *after_name = *cursor + 3;
  int rc;

  skip_spaces(&after_name);
  if (*after_name != '(') {
    *cursor = after_name;
    return fail(diagnostic, line, 1U, "expected '(' after fma", GINT_ERR_PARSE);
  }
  *cursor = after_name + 1;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail_expected_builtin_argument(diagnostic, line, "fma", "first");
  }
  rc = parse_expression(cursor, program, &lhs, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ',') {
    return fail(diagnostic, line, 1U, "expected ',' after fma first argument", GINT_ERR_PARSE);
  }
  (*cursor)++;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail_expected_builtin_argument(diagnostic, line, "fma", "second");
  }
  rc = parse_expression(cursor, program, &mul_rhs, mul_rhs_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ',') {
    return fail(diagnostic, line, 1U, "expected ',' after fma second argument", GINT_ERR_PARSE);
  }
  (*cursor)++;
  if (expr_cursor_starts_missing_argument(*cursor)) {
    return fail_expected_builtin_argument(diagnostic, line, "fma", "third");
  }
  rc = parse_expression(cursor, program, &add_rhs, add_rhs_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ')') {
    return fail(diagnostic, line, 1U, "expected ')' after fma arguments", GINT_ERR_PARSE);
  }
  (*cursor)++;
  rc = ensure_expr_in_reg(program, &lhs, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = ensure_expr_in_reg(program, &mul_rhs, mul_rhs_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = ensure_expr_in_reg(program, &add_rhs, add_rhs_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, GVM_OP_FMA, target_reg, mul_rhs_reg, add_rhs_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  set_result_reg(result_out, target_reg);
  return 1;
}

int try_parse_special_builtin(const char **cursor,
                              graphion_runtime_program *program,
                              parsed_expr_result *result_out,
                              uint8_t base_reg,
                              unsigned int line,
                              graphion_runtime_diagnostic *diagnostic) {
  if (strncmp(*cursor, "clamp", 5U) == 0 && !is_ident_char((*cursor)[5])) {
    return parse_clamp_builtin(cursor, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "fma", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    return parse_fma_builtin(cursor, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "fdim", 4U) == 0 && !is_ident_char((*cursor)[4])) {
    return parse_named_binary_builtin(cursor, "fdim", GVM_OP_FDIM, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "remainder", 9U) == 0 && !is_ident_char((*cursor)[9])) {
    return parse_named_binary_builtin(
        cursor, "remainder", GVM_OP_REMAINDER, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "rint", 4U) == 0 && !is_ident_char((*cursor)[4])) {
    return parse_named_unary_builtin(cursor, "rint", GVM_OP_RINT, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "sqrt", 4U) == 0 && !is_ident_char((*cursor)[4])) {
    return parse_named_unary_builtin(cursor, "sqrt", GVM_OP_SQRT, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "expm1", 5U) == 0 && !is_ident_char((*cursor)[5])) {
    return parse_named_unary_builtin(
        cursor, "expm1", GVM_OP_EXPM1, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "exp2", 4U) == 0 && !is_ident_char((*cursor)[4])) {
    return parse_named_unary_builtin(cursor, "exp2", GVM_OP_EXP2, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "log1p", 5U) == 0 && !is_ident_char((*cursor)[5])) {
    return parse_named_unary_builtin(
        cursor, "log1p", GVM_OP_LOG1P, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "erf", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    return parse_named_unary_builtin(cursor, "erf", GVM_OP_ERF, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "erfc", 4U) == 0 && !is_ident_char((*cursor)[4])) {
    return parse_named_unary_builtin(cursor, "erfc", GVM_OP_ERFC, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "gamma", 5U) == 0 && !is_ident_char((*cursor)[5])) {
    return parse_named_unary_builtin(cursor, "gamma", GVM_OP_GAMMA, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "lgamma", 6U) == 0 && !is_ident_char((*cursor)[6])) {
    return parse_named_unary_builtin(
        cursor, "lgamma", GVM_OP_LGAMMA, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "exp", 3U) == 0 && !is_ident_char((*cursor)[3])) {
    return parse_named_unary_builtin(cursor, "exp", GVM_OP_EXP, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "ln", 2U) == 0 && !is_ident_char((*cursor)[2])) {
    return parse_named_unary_builtin(cursor, "ln", GVM_OP_LN, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "log10", 5U) == 0 && !is_ident_char((*cursor)[5])) {
    return parse_log_constant_base_builtin(cursor, "log10", 10, program, result_out, base_reg, line, diagnostic);
  }
  if (strncmp(*cursor, "log2", 4U) == 0 && !is_ident_char((*cursor)[4])) {
    return parse_log_constant_base_builtin(cursor, "log2", 2, program, result_out, base_reg, line, diagnostic);
  }
  return 0;
}
