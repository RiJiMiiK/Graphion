/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/expr_internal.h"
#include "runtime/interpreter/builtin_catalog.h"

typedef struct {
  const char *name;
  uint8_t opcode;
} direct_builtin_spec;

#define GRAPHION_DIRECT_SPEC_ENTRY(name, opcode) {name, opcode},

static const direct_builtin_spec unary_direct_builtins[] = {
    GRAPHION_UNARY_DIRECT_BUILTINS(GRAPHION_DIRECT_SPEC_ENTRY)
};

static const direct_builtin_spec binary_direct_builtins[] = {
    GRAPHION_BINARY_DIRECT_BUILTINS(GRAPHION_DIRECT_SPEC_ENTRY)
};

#undef GRAPHION_DIRECT_SPEC_ENTRY

static int matches_direct_builtin_name(const char *cursor, const char *name) {
  size_t len = strlen(name);

  return strncmp(cursor, name, len) == 0 && !is_ident_char(cursor[len]);
}

static int fail_expected_open_paren_for_builtin(graphion_runtime_diagnostic *diagnostic,
                                                unsigned int line,
                                                const char *name) {
  static char message[128];

  snprintf(message, sizeof(message), "expected '(' after %s", name);
  return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
}

static int fail_expected_unary_builtin_close(graphion_runtime_diagnostic *diagnostic,
                                             unsigned int line,
                                             const char *name) {
  static char message[128];

  snprintf(message, sizeof(message), "expected ')' after %s argument", name);
  return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
}

static int fail_expected_binary_builtin_separator(graphion_runtime_diagnostic *diagnostic,
                                                  unsigned int line,
                                                  const char *name) {
  static char message[128];

  snprintf(message, sizeof(message), "expected ',' between %s arguments", name);
  return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
}

static int fail_expected_binary_builtin_close(graphion_runtime_diagnostic *diagnostic,
                                              unsigned int line,
                                              const char *name) {
  static char message[128];

  snprintf(message, sizeof(message), "expected ')' after %s arguments", name);
  return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
}

static void set_result_reg(parsed_expr_result *result, uint8_t reg_index) {
  result->kind = EXPR_RESULT_REG;
  result->reg_index = reg_index;
  result->const_index = 0U;
  result->global_index = 0U;
}

static int try_parse_unary_direct_builtin(const direct_builtin_spec *specs,
                                          size_t count,
                                          const char **cursor,
                                          graphion_runtime_program *program,
                                          parsed_expr_result *result_out,
                                          uint8_t base_reg,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  for (i = 0U; i < count; ++i) {
    parsed_expr_result value;
    const uint8_t target_reg = base_reg;
    const char *after_name;
    int rc;

    if (!matches_direct_builtin_name(*cursor, specs[i].name)) {
      continue;
    }
    after_name = *cursor + strlen(specs[i].name);
    skip_spaces(&after_name);
    if (*after_name != '(') {
      return fail_expected_open_paren_for_builtin(diagnostic, line, specs[i].name);
    }
    *cursor = after_name + 1;
    rc = parse_expression(cursor, program, &value, base_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail_expected_unary_builtin_close(diagnostic, line, specs[i].name);
    }
    (*cursor)++;
    rc = ensure_expr_in_reg(program, &value, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, specs[i].opcode, target_reg, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    set_result_reg(result_out, target_reg);
    return 1;
  }

  return 0;
}

static int try_parse_binary_direct_builtin(const direct_builtin_spec *specs,
                                           size_t count,
                                           const char **cursor,
                                           graphion_runtime_program *program,
                                           parsed_expr_result *result_out,
                                           uint8_t base_reg,
                                           unsigned int line,
                                           graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  for (i = 0U; i < count; ++i) {
    parsed_expr_result lhs;
    parsed_expr_result rhs;
    const uint8_t target_reg = base_reg;
    const uint8_t scratch_reg = (uint8_t)(base_reg + 1U);
    const char *after_name;
    int rc;

    if (!matches_direct_builtin_name(*cursor, specs[i].name)) {
      continue;
    }
    after_name = *cursor + strlen(specs[i].name);
    skip_spaces(&after_name);
    if (*after_name != '(') {
      return fail_expected_open_paren_for_builtin(diagnostic, line, specs[i].name);
    }
    *cursor = after_name + 1;
    rc = parse_expression(cursor, program, &lhs, target_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ',') {
      return fail_expected_binary_builtin_separator(diagnostic, line, specs[i].name);
    }
    (*cursor)++;
    rc = parse_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(cursor);
    if (**cursor != ')') {
      return fail_expected_binary_builtin_close(diagnostic, line, specs[i].name);
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
    rc = program_emit(program, specs[i].opcode, target_reg, scratch_reg, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    set_result_reg(result_out, target_reg);
    return 1;
  }

  return 0;
}

int try_parse_direct_builtin(const char **cursor,
                             graphion_runtime_program *program,
                             parsed_expr_result *result_out,
                             uint8_t base_reg,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  int rc;

  rc = try_parse_unary_direct_builtin(unary_direct_builtins,
                                      sizeof(unary_direct_builtins) / sizeof(unary_direct_builtins[0]),
                                      cursor,
                                      program,
                                      result_out,
                                      base_reg,
                                      line,
                                      diagnostic);
  if (rc != 0) {
    return rc;
  }
  return try_parse_binary_direct_builtin(binary_direct_builtins,
                                         sizeof(binary_direct_builtins) / sizeof(binary_direct_builtins[0]),
                                         cursor,
                                         program,
                                         result_out,
                                         base_reg,
                                         line,
                                         diagnostic);
}
