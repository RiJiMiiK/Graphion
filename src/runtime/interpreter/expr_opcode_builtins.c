/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/expr_internal.h"

#include <string.h>

typedef struct {
  const char *name;
  graphion_opcode opcode;
} expr_named_builtin_entry;

static void set_result_reg(parsed_expr_result *result, uint8_t reg_index) {
  result->kind = EXPR_RESULT_REG;
  result->reg_index = reg_index;
  result->const_index = 0U;
  result->global_index = 0U;
}

static int parse_named_unary_opcode_builtin(const char **cursor,
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
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  *cursor = after_name + 1;
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

static int parse_named_binary_opcode_builtin(const char **cursor,
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
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  *cursor = after_name + 1;
  rc = parse_expression(cursor, program, &lhs, target_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ',') {
    char message[112];

    snprintf(message, sizeof(message), "expected ',' between %s arguments", name);
    return fail(diagnostic, line, 1U, message, GINT_ERR_PARSE);
  }
  (*cursor)++;
  rc = parse_expression(cursor, program, &rhs, scratch_reg, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(cursor);
  if (**cursor != ')') {
    char message[112];

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

int try_parse_opcode_builtin(const char **cursor,
                             graphion_runtime_program *program,
                             parsed_expr_result *result_out,
                             uint8_t base_reg,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  static const expr_named_builtin_entry unary_builtins[] = {
      {"floor", GVM_OP_FLOOR},       {"ceil", GVM_OP_CEIL},
      {"round", GVM_OP_ROUND},       {"trunc", GVM_OP_TRUNC},
      {"fract", GVM_OP_FRACT},       {"sign", GVM_OP_SIGN},
      {"cbrt", GVM_OP_CBRT},         {"sin", GVM_OP_SIN},
      {"csc", GVM_OP_CSC},           {"sec", GVM_OP_SEC},
      {"cot", GVM_OP_COT},           {"acsc", GVM_OP_ACSC},
      {"asec", GVM_OP_ASEC},         {"acot", GVM_OP_ACOT},
      {"sech", GVM_OP_SECH},         {"csch", GVM_OP_CSCH},
      {"coth", GVM_OP_COTH},         {"sinh", GVM_OP_SINH},
      {"asinh", GVM_OP_ASINH},       {"acosh", GVM_OP_ACOSH},
      {"cosh", GVM_OP_COSH},         {"tanh", GVM_OP_TANH},
      {"atanh", GVM_OP_ATANH},       {"cos", GVM_OP_COS},
      {"tan", GVM_OP_TAN},           {"asin", GVM_OP_ASIN},
      {"acos", GVM_OP_ACOS},         {"atan", GVM_OP_ATAN},
      {"len", GVM_OP_LEN},           {"degrees", GVM_OP_DEGREES},
      {"radians", GVM_OP_RADIANS},   {"isnan", GVM_OP_ISNAN},
      {"isinf", GVM_OP_ISINF},       {"isfinite", GVM_OP_ISFINITE},
      {"node_count", GVM_OP_GRAPH_NODE_COUNT},
      {"edge_count", GVM_OP_GRAPH_EDGE_COUNT},
      {"is_directed", GVM_OP_GRAPH_IS_DIRECTED},
      {"is_weighted", GVM_OP_GRAPH_IS_WEIGHTED},
      {"orientation", GVM_OP_GRAPH_ORIENTATION},
      {"node_ids", GVM_OP_GRAPH_NODE_IDS},
      {"nodes", GVM_OP_GRAPH_NODES},
      {"edges", GVM_OP_GRAPH_EDGES},
  };
  static const expr_named_builtin_entry binary_builtins[] = {
      {"atan2", GVM_OP_ATAN2},
      {"hypot", GVM_OP_HYPOT},
      {"copysign", GVM_OP_COPYSIGN},
      {"log", GVM_OP_LOG},
      {"contains", GVM_OP_SET_CONTAINS},
      {"node_attrs", GVM_OP_GRAPH_NODE_ATTRS},
      {"hyperedge_vertices", GVM_OP_HYPERGRAPH_HYPEREDGE_VERTICES},
      {"hyperedge_attrs", GVM_OP_HYPERGRAPH_HYPEREDGE_ATTRS},
      {"has_node", GVM_OP_GRAPH_HAS_NODE},
      {"neighbors", GVM_OP_GRAPH_NEIGHBORS},
      {"indegree", GVM_OP_GRAPH_INDEGREE},
      {"outdegree", GVM_OP_GRAPH_OUTDEGREE},
  };
  size_t i;

  for (i = 0U; i < sizeof(unary_builtins) / sizeof(unary_builtins[0]); ++i) {
    const expr_named_builtin_entry *entry = &unary_builtins[i];
    size_t name_len = strlen(entry->name);

    if (strncmp(*cursor, entry->name, name_len) == 0 && !is_ident_char((*cursor)[name_len])) {
      return parse_named_unary_opcode_builtin(cursor,
                                              entry->name,
                                              entry->opcode,
                                              program,
                                              result_out,
                                              base_reg,
                                              line,
                                              diagnostic);
    }
  }

  for (i = 0U; i < sizeof(binary_builtins) / sizeof(binary_builtins[0]); ++i) {
    const expr_named_builtin_entry *entry = &binary_builtins[i];
    size_t name_len = strlen(entry->name);

    if (strncmp(*cursor, entry->name, name_len) == 0 && !is_ident_char((*cursor)[name_len])) {
      return parse_named_binary_opcode_builtin(cursor,
                                               entry->name,
                                               entry->opcode,
                                               program,
                                               result_out,
                                               base_reg,
                                               line,
                                               diagnostic);
    }
  }

  return 0;
}
