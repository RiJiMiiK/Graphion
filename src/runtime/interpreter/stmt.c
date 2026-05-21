/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/stmt.h"

static int fail_unknown_variable(graphion_runtime_diagnostic *diagnostic,
                                 unsigned int line,
                                 const char *name) {
  char message[GRAPHION_RUNTIME_DIAGNOSTIC_MESSAGE_MAX];

  snprintf(message, sizeof(message), "unknown variable '%s'", name);
  return fail(diagnostic, line, 1U, message, GINT_ERR_UNKNOWN_VARIABLE);
}

static int fail_unknown_graph_variable(graphion_runtime_diagnostic *diagnostic,
                                       unsigned int line,
                                       const char *name) {
  char message[GRAPHION_RUNTIME_DIAGNOSTIC_MESSAGE_MAX];

  snprintf(message, sizeof(message), "unknown graph variable '%s'", name);
  return fail(diagnostic, line, 1U, message, GINT_ERR_UNKNOWN_VARIABLE);
}

static int fail_unknown_hypergraph_variable(
    graphion_runtime_diagnostic *diagnostic,
    unsigned int line,
    const char *name) {
  char message[GRAPHION_RUNTIME_DIAGNOSTIC_MESSAGE_MAX];

  snprintf(message, sizeof(message), "unknown hypergraph variable '%s'", name);
  return fail(diagnostic, line, 1U, message, GINT_ERR_UNKNOWN_VARIABLE);
}

static unsigned int source_column(const char *line_text, const char *cursor) {
  if (line_text == NULL || cursor == NULL || cursor < line_text) {
    return 1U;
  }
  return (unsigned int)(cursor - line_text) + 1U;
}

static const char *assignment_operator_text(char assign_op,
                                            int power_assign,
                                            int floor_div_assign,
                                            int bit_shl_assign,
                                            int bit_shr_assign) {
  if (power_assign) {
    return "**=";
  }
  if (floor_div_assign) {
    return "//=";
  }
  if (bit_shl_assign) {
    return "<<=";
  }
  if (bit_shr_assign) {
    return ">>=";
  }
  switch (assign_op) {
    case '+':
      return "+=";
    case '-':
      return "-=";
    case '*':
      return "*=";
    case '/':
      return "/=";
    case '%':
      return "%=";
    case '&':
      return "&=";
    case '|':
      return "|=";
    case '^':
      return "^=";
    default:
      return "=";
  }
}

static int remap_missing_assignment_rhs_error(
    int rc,
    graphion_runtime_diagnostic *diagnostic,
    unsigned int line,
    unsigned int column,
    const char *op,
    int remap_prefix_token) {
  char message[GRAPHION_RUNTIME_DIAGNOSTIC_MESSAGE_MAX];

  if (rc == GINT_ERR_PARSE && diagnostic != NULL &&
      diagnostic->message != NULL &&
      (strcmp(diagnostic->message, "expected scalar literal") == 0 ||
       (remap_prefix_token &&
        strncmp(diagnostic->message, "expected expression before ", 27U) == 0))) {
    snprintf(message, sizeof(message), "expected expression after '%s'", op);
    return fail(diagnostic, line, column, message, GINT_ERR_PARSE);
  }
  return rc;
}

int parse_assignment(const char *line_text,
                            graphion_runtime_program *program,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  parsed_expr_result expr;
  parsed_expr_result key_expr;
  size_t target_index = 0U;
  int indexed_target = 0;
  char assign_op = '=';
  int power_assign = 0;
  int floor_div_assign = 0;
  int bit_and_assign = 0;
  int bit_or_assign = 0;
  int bit_xor_assign = 0;
  int bit_shl_assign = 0;
  int bit_shr_assign = 0;
  unsigned int assign_op_column;
  int rc;

  rc = parse_identifier_token(&cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (*cursor == '[') {
    int existing;
    indexed_target = 1;
    existing = program_find_global_index(program, target);
    if (existing < 0) {
      return fail_unknown_variable(diagnostic, line, target);
    }
    target_index = (size_t)existing;
    cursor++;
    rc = parse_expression(&cursor, program, &key_expr, 1U, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &key_expr, 1U, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    skip_spaces(&cursor);
    if (*cursor != ']') {
      return fail(diagnostic, line, source_column(line_text, cursor), "expected ']' after assignment target index", GINT_ERR_PARSE);
    }
    cursor++;
    skip_spaces(&cursor);
  }
  assign_op_column = source_column(line_text, cursor);
  if (cursor[0] == '*' && cursor[1] == '*' && cursor[2] == '=') {
    assign_op = '*';
    power_assign = 1;
    cursor += 3;
  } else if (cursor[0] == '/' && cursor[1] == '/' && cursor[2] == '=') {
    assign_op = '/';
    floor_div_assign = 1;
    cursor += 3;
  } else if (cursor[0] == '<' && cursor[1] == '<' && cursor[2] == '=') {
    assign_op = '<';
    bit_shl_assign = 1;
    cursor += 3;
  } else if (cursor[0] == '>' && cursor[1] == '>' && cursor[2] == '=') {
    assign_op = '>';
    bit_shr_assign = 1;
    cursor += 3;
  } else if (cursor[0] == '&' && cursor[1] == '=') {
    assign_op = '&';
    bit_and_assign = 1;
    cursor += 2;
  } else if (cursor[0] == '|' && cursor[1] == '=') {
    assign_op = '|';
    bit_or_assign = 1;
    cursor += 2;
  } else if (cursor[0] == '^' && cursor[1] == '=') {
    assign_op = '^';
    bit_xor_assign = 1;
    cursor += 2;
  } else if ((*cursor == '+' || *cursor == '-' || *cursor == '*' || *cursor == '/' || *cursor == '%') &&
      cursor[1] == '=') {
    assign_op = *cursor;
    cursor += 2;
  } else if (*cursor == '=') {
    cursor++;
  } else {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected '='", GINT_ERR_PARSE);
  }
  if (indexed_target && assign_op != '=') {
    return fail(diagnostic, line, assign_op_column, "compound indexed assignment is not supported", GINT_ERR_PARSE);
  }
  if (!indexed_target && assign_op == '=') {
    rc = program_find_or_add_global(program, target, line, diagnostic, &target_index);
    if (rc != GINT_OK) {
      return rc;
    }
  } else if (!indexed_target) {
    int existing = program_find_global_index(program, target);
    if (existing < 0) {
      return fail_unknown_variable(diagnostic, line, target);
    }
    target_index = (size_t)existing;
  }
  const char *expr_start = cursor;
  rc = parse_expression(&cursor, program, &expr, indexed_target ? 2U : 0U, line, diagnostic);
  if (rc != GINT_OK) {
    const char *trimmed_expr_start = expr_start;
    skip_spaces(&trimmed_expr_start);
    if (assign_op == '=' && *trimmed_expr_start != '\0') {
      point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
      point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
      point_ternary_diagnostic_from_segment(diagnostic, line_text, trimmed_expr_start, 1U);
      point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
      return rc;
    }
    return remap_missing_assignment_rhs_error(
        rc,
        diagnostic,
        line,
        assign_op_column,
        assignment_operator_text(assign_op,
                                 power_assign,
                                 floor_div_assign,
                                 bit_shl_assign,
                                 bit_shr_assign),
        assign_op != '=');
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, source_column(line_text, cursor), "unexpected trailing tokens after assignment", GINT_ERR_PARSE);
  }
  if (assign_op != '=') {
    rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = ensure_expr_in_reg(program, &expr, 1U, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program,
                      power_assign ? GVM_OP_POW :
                      bit_shl_assign ? GVM_OP_BIT_SHL :
                      bit_shr_assign ? GVM_OP_BIT_SHR :
                      bit_and_assign ? GVM_OP_BIT_AND :
                      bit_or_assign ? GVM_OP_BIT_OR :
                      bit_xor_assign ? GVM_OP_BIT_XOR :
                      floor_div_assign ? GVM_OP_FLOOR_DIV :
                      assign_op == '+' ? GVM_OP_ADD :
                      assign_op == '-' ? GVM_OP_SUB :
                      assign_op == '*' ? GVM_OP_MUL :
                      assign_op == '/' ? GVM_OP_DIV : GVM_OP_MOD,
                      0U,
                      1U,
                      0,
                      line,
                      diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
  }
  if (indexed_target) {
    rc = ensure_expr_in_reg(program, &expr, 2U, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, GVM_OP_DICT_SET_KEY, 0U, 1U, 2, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
  }
  if (expr.kind == EXPR_RESULT_LITERAL) {
    return program_emit(
        program, GVM_OP_STORE_CONST_GLOBAL, 0U, (uint8_t)target_index, (int32_t)expr.const_index, line, diagnostic);
  }
  if (expr.kind == EXPR_RESULT_GLOBAL) {
    return program_emit(program, GVM_OP_COPY_GLOBAL, 0U, (uint8_t)target_index, (int32_t)expr.global_index, line, diagnostic);
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, expr.reg_index, 0U, (int32_t)target_index, line, diagnostic);
}

int parse_print(const char *line_text,
                       const graphion_runtime_scope *scope,
                       graphion_runtime_program *program,
                       unsigned int line,
                       graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  parsed_expr_result expr;
  int rc;
  skip_spaces(&cursor);
  if (strncmp(cursor, "print", 5U) != 0 || is_ident_char(cursor[5])) {
    return fail(diagnostic, line, 1U, "expected 'print'", GINT_ERR_PARSE);
  }
  cursor += 5;
  skip_spaces(&cursor);
  if (*cursor != '(') {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected '(' after print", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor == ')' || *cursor == '\0') {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected print argument", GINT_ERR_PARSE);
  }
  {
    const char *scan = cursor;
    int depth = 0;
    int in_string = 0;
    int has_concat = 0;
    int has_stringish = 0;
    const char *segment_start = cursor;
    while (*scan != '\0') {
      if (in_string) {
        if (*scan == '"') {
          in_string = 0;
        }
        scan++;
        continue;
      }
      if (*scan == '"') {
        in_string = 1;
        scan++;
        continue;
      }
      if (*scan == '(') {
        depth++;
        scan++;
        continue;
      }
      if (*scan == ')') {
        if (depth == 0) {
          break;
        }
        depth--;
        scan++;
        continue;
      }
      if (depth == 0 && *scan == '+') {
        const char *trim_start = segment_start;
        const char *trim_end = scan;
        while (trim_start < trim_end && (*trim_start == ' ' || *trim_start == '\t' || *trim_start == '\r')) {
          trim_start++;
        }
        while (trim_end > trim_start && (trim_end[-1] == ' ' || trim_end[-1] == '\t' || trim_end[-1] == '\r')) {
          trim_end--;
        }
        if (trim_start < trim_end) {
          if (*trim_start == '"') {
            has_stringish = 1;
          } else {
            size_t len = (size_t)(trim_end - trim_start);
            if (scope != NULL && len < GRAPHION_RUNTIME_NAME_MAX) {
              char name[GRAPHION_RUNTIME_NAME_MAX];
              memcpy(name, trim_start, len);
              name[len] = '\0';
              if (scope_find_index(scope, name) >= 0) {
                const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
                if (value != NULL && value->kind == GVM_VALUE_STRING) {
                  has_stringish = 1;
                }
              }
            }
          }
        }
        has_concat = 1;
        segment_start = scan + 1;
      }
      scan++;
    }
    if (has_concat) {
      const char *trim_start = segment_start;
      const char *trim_end = scan;
      while (trim_start < trim_end && (*trim_start == ' ' || *trim_start == '\t' || *trim_start == '\r')) {
        trim_start++;
      }
      while (trim_end > trim_start && (trim_end[-1] == ' ' || trim_end[-1] == '\t' || trim_end[-1] == '\r')) {
        trim_end--;
      }
      if (trim_start < trim_end) {
        if (*trim_start == '"') {
          has_stringish = 1;
        } else {
          size_t len = (size_t)(trim_end - trim_start);
          if (scope != NULL && len < GRAPHION_RUNTIME_NAME_MAX) {
            char name[GRAPHION_RUNTIME_NAME_MAX];
            memcpy(name, trim_start, len);
            name[len] = '\0';
            if (scope_find_index(scope, name) >= 0) {
              const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
              if (value != NULL && value->kind == GVM_VALUE_STRING) {
                has_stringish = 1;
              }
            }
          }
        }
      }
    }
    if (has_concat && has_stringish) {
      const char *part_cursor = cursor;
      const char *part_start = cursor;
      for (;;) {
        int depth2 = 0;
        int in_string2 = 0;
        while (*part_cursor != '\0') {
          if (in_string2) {
            if (*part_cursor == '"') {
              in_string2 = 0;
            }
            part_cursor++;
            continue;
          }
          if (*part_cursor == '"') {
            in_string2 = 1;
            part_cursor++;
            continue;
          }
          if (*part_cursor == '(') {
            depth2++;
            part_cursor++;
            continue;
          }
          if (*part_cursor == ')') {
            if (depth2 == 0) {
              break;
            }
            depth2--;
            part_cursor++;
            continue;
          }
          if (depth2 == 0 && *part_cursor == '+') {
            break;
          }
          part_cursor++;
        }
        {
          char segment[512];
          size_t len = (size_t)(part_cursor - part_start);
          parsed_expr_result part_expr;
          const char *segment_cursor = segment;
          if (len >= sizeof(segment)) {
            return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
          }
          memcpy(segment, part_start, len);
          segment[len] = '\0';
          rc = parse_expression(&segment_cursor, program, &part_expr, 0U, line, diagnostic);
          if (rc != GINT_OK) {
            point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
            point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, part_start + (segment_cursor - segment), 1U);
            point_ternary_diagnostic_from_segment(diagnostic, line_text, part_start, 1U);
            point_delimiter_diagnostic_at_cursor(diagnostic, line_text, part_start + (segment_cursor - segment), 1U);
            return rc;
          }
          skip_spaces(&segment_cursor);
          if (*segment_cursor != '\0') {
            return fail(diagnostic, line, 1U, "unexpected trailing tokens after print", GINT_ERR_PARSE);
          }
          if (part_expr.kind == EXPR_RESULT_LITERAL) {
            rc = program_emit(program, GVM_OP_PRINT_CONST_PART, 0U, 0U, (int32_t)part_expr.const_index, line, diagnostic);
          } else if (part_expr.kind == EXPR_RESULT_GLOBAL) {
            rc = program_emit(program, GVM_OP_PRINT_GLOBAL_PART, 0U, 0U, (int32_t)part_expr.global_index, line, diagnostic);
          } else {
            rc = program_emit(program, GVM_OP_PRINT_REG_PART, part_expr.reg_index, 0U, 0, line, diagnostic);
          }
          if (rc != GINT_OK) {
            return rc;
          }
        }
        if (*part_cursor == ')') {
          cursor = part_cursor;
          break;
        }
        part_cursor++;
        part_start = part_cursor;
      }
      if (*cursor != ')') {
        return fail(diagnostic, line, source_column(line_text, cursor), "expected ')' after print argument", GINT_ERR_PARSE);
      }
      cursor++;
      skip_spaces(&cursor);
      if (*cursor != '\0') {
        return fail(diagnostic, line, 1U, "unexpected trailing tokens after print", GINT_ERR_PARSE);
      }
      return program_emit(program, GVM_OP_PRINT_NEWLINE, 0U, 0U, 0, line, diagnostic);
    }
  }
  {
    const char *expr_start = cursor;
    rc = parse_expression(&cursor, program, &expr, 0U, line, diagnostic);
    if (rc != GINT_OK) {
      point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
      point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
      point_ternary_diagnostic_from_segment(diagnostic, line_text, expr_start, 1U);
      point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
      return rc;
    }
  }
  skip_spaces(&cursor);
  if (*cursor != ')') {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected ')' after print argument", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after print", GINT_ERR_PARSE);
  }
  if (expr.kind == EXPR_RESULT_LITERAL) {
    return program_emit(program, GVM_OP_PRINT_CONST, 0U, 0U, (int32_t)expr.const_index, line, diagnostic);
  }
  if (expr.kind == EXPR_RESULT_GLOBAL) {
    return program_emit(program, GVM_OP_PRINT_GLOBAL, 0U, 0U, (int32_t)expr.global_index, line, diagnostic);
  }
  return program_emit(program, GVM_OP_PRINT_REG, expr.reg_index, 0U, 0, line, diagnostic);
}

int parse_graph_declaration_with_node_count(const char *line_text,
                                            size_t node_count,
                                            graphion_runtime_program *program,
                                            unsigned int line,
                                            graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  size_t target_index = 0U;
  int block_declaration = 0;
  int rc;

  skip_spaces(&cursor);
  if (strncmp(cursor, "graph", 5U) != 0 || is_ident_char(cursor[5])) {
    return fail(diagnostic, line, 1U, "expected 'graph'", GINT_ERR_PARSE);
  }
  cursor += 5;
  if (*cursor != ' ' && *cursor != '\t') {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected graph name", GINT_ERR_PARSE);
  }
  rc = parse_identifier_token(&cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (*cursor == ':') {
    block_declaration = 1;
  } else if (*cursor != ';') {
    return fail(diagnostic,
                line,
                source_column(line_text, cursor),
                "expected ';' or ':' after graph declaration",
                GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic,
                line,
                source_column(line_text, cursor),
                "unexpected trailing tokens after graph declaration",
                GINT_ERR_PARSE);
  }
  rc = program_find_or_add_global(program, target, line, diagnostic, &target_index);
  if (rc != GINT_OK) {
    return rc;
  }
  if (!block_declaration && node_count != 0U) {
    return fail(diagnostic, line, 1U, "graph node block requires ':'", GINT_ERR_PARSE);
  }
  if (node_count > (size_t)INT32_MAX) {
    return fail(diagnostic, line, 1U, "too many graph nodes", GINT_ERR_CAPACITY);
  }
  rc = program_emit(program, GVM_OP_GRAPH_NEW, 0U, 0U, (int32_t)node_count, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
}

static int parse_graph_declaration(const char *line_text,
                                   graphion_runtime_program *program,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic) {
  return parse_graph_declaration_with_node_count(line_text, 0U, program, line, diagnostic);
}

static int parse_hypergraph_declaration(const char *line_text,
                                        graphion_runtime_program *program,
                                        unsigned int line,
                                        graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  size_t target_index = 0U;
  int rc;

  skip_spaces(&cursor);
  if (strncmp(cursor, "hypergraph", 10U) != 0 || is_ident_char(cursor[10])) {
    return fail(diagnostic, line, 1U, "expected 'hypergraph'", GINT_ERR_PARSE);
  }
  cursor += 10;
  if (*cursor != ' ' && *cursor != '\t') {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected hypergraph name", GINT_ERR_PARSE);
  }
  rc = parse_identifier_token(&cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (*cursor != ';') {
    return fail(diagnostic,
                line,
                source_column(line_text, cursor),
                "expected ';' after hypergraph declaration",
                GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic,
                line,
                source_column(line_text, cursor),
                "unexpected trailing tokens after hypergraph declaration",
                GINT_ERR_PARSE);
  }
  rc = program_find_or_add_global(program, target, line, diagnostic, &target_index);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, GVM_OP_HYPERGRAPH_NEW, 0U, 0U, 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
}

static int parse_graph_mutation_statement(const char *line_text,
                                          graphion_runtime_program *program,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  static const struct {
    const char *name;
    size_t len;
    graphion_opcode opcode;
    int endpoint_count;
  } specs[] = {
      {"add_node", 8U, GVM_OP_GRAPH_ADD_NODE, 1},
      {"add_edge", 8U, GVM_OP_GRAPH_ADD_EDGE, 2},
      {"remove_node", 11U, GVM_OP_GRAPH_REMOVE_NODE, 1},
      {"remove_edge", 11U, GVM_OP_GRAPH_REMOVE_EDGE, 2},
  };
  const char *name = NULL;
  const char *cursor = line_text;
  char graph_name[GRAPHION_RUNTIME_NAME_MAX];
  parsed_expr_result first;
  parsed_expr_result second;
  graphion_opcode opcode = GVM_OP_NOP;
  int graph_index;
  int endpoint_count = 0;
  size_t i;
  int rc;

  for (i = 0U; i < sizeof(specs) / sizeof(specs[0]); ++i) {
    if (strncmp(line_text, specs[i].name, specs[i].len) == 0 && !is_ident_char(line_text[specs[i].len])) {
      name = specs[i].name;
      cursor = line_text + specs[i].len;
      opcode = specs[i].opcode;
      endpoint_count = specs[i].endpoint_count;
      break;
    }
  }
  if (name == NULL) {
    return fail(diagnostic, line, 1U, "expected graph mutation statement", GINT_ERR_PARSE);
  }
  skip_spaces(&cursor);
  if (*cursor != '(') {
    char message[96];
    snprintf(message, sizeof(message), "expected '(' after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  rc = parse_identifier_token(&cursor, graph_name, sizeof(graph_name), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  graph_index = program_find_global_index(program, graph_name);
  if (graph_index < 0) {
    return fail_unknown_graph_variable(diagnostic, line, graph_name);
  }
  skip_spaces(&cursor);
  if (*cursor != ',') {
    char message[128];
    snprintf(message, sizeof(message), "expected ',' after %s graph", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  {
    const char *expr_start = cursor;
  rc = parse_expression(&cursor, program, &first, 1U, line, diagnostic);
  if (rc != GINT_OK) {
    point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
    point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    point_ternary_diagnostic_from_segment(diagnostic, line_text, expr_start, 1U);
    point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    return rc;
  }
  }
  rc = ensure_expr_in_reg(program, &first, 1U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (endpoint_count == 1) {
    if (*cursor != ')') {
      char message[128];
      snprintf(message, sizeof(message), "expected ')' after %s arguments", name);
      return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
    }
    cursor++;
    skip_spaces(&cursor);
    if (*cursor != '\0') {
      char message[128];
      snprintf(message, sizeof(message), "unexpected trailing tokens after %s", name);
      return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
    }
    rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = program_emit(program, opcode, 0U, 1U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
  }
  if (*cursor != ',') {
    char message[128];
    snprintf(message, sizeof(message), "expected ',' between %s endpoints", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  {
    const char *expr_start = cursor;
  rc = parse_expression(&cursor, program, &second, 2U, line, diagnostic);
  if (rc != GINT_OK) {
    point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
    point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    point_ternary_diagnostic_from_segment(diagnostic, line_text, expr_start, 1U);
    point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    return rc;
  }
  }
  rc = ensure_expr_in_reg(program, &second, 2U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ')') {
    char message[128];
    snprintf(message, sizeof(message), "expected ')' after %s arguments", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    char message[128];
    snprintf(message, sizeof(message), "unexpected trailing tokens after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, opcode, 0U, 1U, 2, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
}

static int parse_hypergraph_mutation_statement(const char *line_text,
                                               graphion_runtime_program *program,
                                               unsigned int line,
                                               graphion_runtime_diagnostic *diagnostic) {
  static const struct {
    const char *name;
    size_t len;
    graphion_opcode opcode;
  } specs[] = {
      {"add_vertex", 10U, GVM_OP_HYPERGRAPH_ADD_VERTEX},
      {"add_hyperedge", 13U, GVM_OP_HYPERGRAPH_ADD_HYPEREDGE},
      {"remove_vertex", 13U, GVM_OP_HYPERGRAPH_REMOVE_VERTEX},
      {"remove_hyperedge", 16U, GVM_OP_HYPERGRAPH_REMOVE_HYPEREDGE},
  };
  const char *name = NULL;
  const char *cursor = line_text;
  char graph_name[GRAPHION_RUNTIME_NAME_MAX];
  parsed_expr_result arg;
  graphion_opcode opcode = GVM_OP_NOP;
  int graph_index;
  size_t i;
  int rc;

  for (i = 0U; i < sizeof(specs) / sizeof(specs[0]); ++i) {
    if (strncmp(line_text, specs[i].name, specs[i].len) == 0 && !is_ident_char(line_text[specs[i].len])) {
      name = specs[i].name;
      cursor = line_text + specs[i].len;
      opcode = specs[i].opcode;
      break;
    }
  }
  if (name == NULL) {
    return fail(diagnostic, line, 1U, "expected hypergraph mutation statement", GINT_ERR_PARSE);
  }
  skip_spaces(&cursor);
  if (*cursor != '(') {
    char message[96];
    snprintf(message, sizeof(message), "expected '(' after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  rc = parse_identifier_token(&cursor, graph_name, sizeof(graph_name), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  graph_index = program_find_global_index(program, graph_name);
  if (graph_index < 0) {
    return fail_unknown_hypergraph_variable(diagnostic, line, graph_name);
  }
  skip_spaces(&cursor);
  if (*cursor != ',') {
    char message[128];
    snprintf(message, sizeof(message), "expected ',' after %s hypergraph", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  {
    const char *expr_start = cursor;
  rc = parse_expression(&cursor, program, &arg, 1U, line, diagnostic);
  if (rc != GINT_OK) {
    point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
    point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    point_ternary_diagnostic_from_segment(diagnostic, line_text, expr_start, 1U);
    point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    return rc;
  }
  }
  rc = ensure_expr_in_reg(program, &arg, 1U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ')') {
    char message[128];
    snprintf(message, sizeof(message), "expected ')' after %s arguments", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    char message[128];
    snprintf(message, sizeof(message), "unexpected trailing tokens after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, opcode, 0U, 1U, 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
}

static int parse_graph_attr_mutation_statement(const char *line_text,
                                               graphion_runtime_program *program,
                                               unsigned int line,
                                               graphion_runtime_diagnostic *diagnostic) {
  static const struct {
    const char *name;
    size_t len;
    graphion_opcode opcode;
    int arg_count;
  } specs[] = {
      {"set_node_attrs", 14U, GVM_OP_GRAPH_SET_NODE_ATTRS, 2},
      {"set_edge_attrs", 14U, GVM_OP_GRAPH_SET_EDGE_ATTRS, 3},
      {"set_edge_weight", 15U, GVM_OP_GRAPH_SET_EDGE_WEIGHT, 3},
  };
  parsed_expr_result args[4];
  const char *name = NULL;
  const char *cursor = line_text;
  char graph_name[GRAPHION_RUNTIME_NAME_MAX];
  graphion_opcode opcode = GVM_OP_NOP;
  int graph_index;
  int arg_count = 0;
  size_t i;
  int rc;

  for (i = 0U; i < sizeof(specs) / sizeof(specs[0]); ++i) {
    if (strncmp(line_text, specs[i].name, specs[i].len) == 0 && !is_ident_char(line_text[specs[i].len])) {
      name = specs[i].name;
      cursor = line_text + specs[i].len;
      opcode = specs[i].opcode;
      arg_count = specs[i].arg_count;
      break;
    }
  }
  if (name == NULL) {
    return fail(diagnostic, line, 1U, "expected graph attribute mutation statement", GINT_ERR_PARSE);
  }
  skip_spaces(&cursor);
  if (*cursor != '(') {
    char message[96];
    snprintf(message, sizeof(message), "expected '(' after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  rc = parse_identifier_token(&cursor, graph_name, sizeof(graph_name), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  graph_index = program_find_global_index(program, graph_name);
  if (graph_index < 0) {
    return fail_unknown_graph_variable(diagnostic, line, graph_name);
  }
  for (i = 0U; i < (size_t)arg_count; ++i) {
    skip_spaces(&cursor);
    if (*cursor != ',') {
      char message[128];
      snprintf(message, sizeof(message), "expected ',' between %s arguments", name);
      return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
    }
    cursor++;
    {
      const char *expr_start = cursor;
    rc = parse_expression(&cursor, program, &args[i], (uint8_t)(i + 1U), line, diagnostic);
    if (rc != GINT_OK) {
      point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
      point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
      point_ternary_diagnostic_from_segment(diagnostic, line_text, expr_start, 1U);
      point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
      return rc;
    }
    }
    rc = ensure_expr_in_reg(program, &args[i], (uint8_t)(i + 1U), line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  skip_spaces(&cursor);
  if (*cursor != ')') {
    char message[128];
    snprintf(message, sizeof(message), "expected ')' after %s arguments", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    char message[128];
    snprintf(message, sizeof(message), "unexpected trailing tokens after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, opcode, 0U, 1U, arg_count >= 2 ? 2 : 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
}

static int parse_hypergraph_attr_mutation_statement(const char *line_text,
                                                    graphion_runtime_program *program,
                                                    unsigned int line,
                                                    graphion_runtime_diagnostic *diagnostic) {
  static const struct {
    const char *name;
    size_t len;
    graphion_opcode opcode;
  } specs[] = {
      {"set_vertex_attrs", 16U, GVM_OP_HYPERGRAPH_SET_VERTEX_ATTRS},
      {"set_hyperedge_attrs", 19U, GVM_OP_HYPERGRAPH_SET_HYPEREDGE_ATTRS},
  };
  parsed_expr_result target;
  parsed_expr_result attrs;
  const char *name = NULL;
  const char *cursor = line_text;
  char graph_name[GRAPHION_RUNTIME_NAME_MAX];
  graphion_opcode opcode = GVM_OP_NOP;
  int graph_index;
  size_t i;
  int rc;

  for (i = 0U; i < sizeof(specs) / sizeof(specs[0]); ++i) {
    if (strncmp(line_text, specs[i].name, specs[i].len) == 0 && !is_ident_char(line_text[specs[i].len])) {
      name = specs[i].name;
      cursor = line_text + specs[i].len;
      opcode = specs[i].opcode;
      break;
    }
  }
  if (name == NULL) {
    return fail(diagnostic, line, 1U, "expected hypergraph attribute mutation statement", GINT_ERR_PARSE);
  }
  skip_spaces(&cursor);
  if (*cursor != '(') {
    char message[96];
    snprintf(message, sizeof(message), "expected '(' after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  rc = parse_identifier_token(&cursor, graph_name, sizeof(graph_name), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  graph_index = program_find_global_index(program, graph_name);
  if (graph_index < 0) {
    return fail_unknown_hypergraph_variable(diagnostic, line, graph_name);
  }
  skip_spaces(&cursor);
  if (*cursor != ',') {
    char message[128];
    snprintf(message, sizeof(message), "expected ',' between %s arguments", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  {
    const char *expr_start = cursor;
  rc = parse_expression(&cursor, program, &target, 1U, line, diagnostic);
  if (rc != GINT_OK) {
    point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
    point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    point_ternary_diagnostic_from_segment(diagnostic, line_text, expr_start, 1U);
    point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    return rc;
  }
  }
  rc = ensure_expr_in_reg(program, &target, 1U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ',') {
    char message[128];
    snprintf(message, sizeof(message), "expected ',' between %s arguments", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  {
    const char *expr_start = cursor;
  rc = parse_expression(&cursor, program, &attrs, 2U, line, diagnostic);
  if (rc != GINT_OK) {
    point_unknown_operand_diagnostic(diagnostic, line_text, 1U);
    point_builtin_argument_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    point_ternary_diagnostic_from_segment(diagnostic, line_text, expr_start, 1U);
    point_delimiter_diagnostic_at_cursor(diagnostic, line_text, cursor, 1U);
    return rc;
  }
  }
  rc = ensure_expr_in_reg(program, &attrs, 2U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ')') {
    char message[128];
    snprintf(message, sizeof(message), "expected ')' after %s arguments", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    char message[128];
    snprintf(message, sizeof(message), "unexpected trailing tokens after %s", name);
    return fail(diagnostic, line, source_column(line_text, cursor), message, GINT_ERR_PARSE);
  }
  rc = program_emit(program, GVM_OP_LOAD_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, opcode, 0U, 1U, 2, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, graph_index, line, diagnostic);
}

int seed_program_from_scope(graphion_runtime_program *program,
                            const graphion_runtime_scope *scope,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  size_t i;
  if (program == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  graphion_runtime_program_init(program);
  if (graphion_runtime_program_reserve_globals(program, scope->global_count, line, diagnostic) != GINT_OK) {
    return GINT_ERR_CAPACITY;
  }
  program->global_count = scope->global_count;
  for (i = 0U; i < scope->global_count; ++i) {
    copy_name(program->global_names[i], scope->global_names[i]);
  }
  return GINT_OK;
}

int parse_statement_line(const char *line_text,
                                const graphion_runtime_scope *scope,
                                graphion_runtime_program *program,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  const char *line_cursor = line_text;
  int rc;
  skip_spaces(&line_cursor);
  if (*line_cursor == '\0') {
    return GINT_OK;
  }
  if (strncmp(line_cursor, "print", 5U) == 0 && !is_ident_char(line_cursor[5])) {
    rc = parse_print(line_cursor, scope, program, line, diagnostic);
  } else if (strncmp(line_cursor, "struct", 6U) == 0 && !is_ident_char(line_cursor[6])) {
    rc = fail(diagnostic, line, 1U, "struct declaration requires ':' and an indented field block", GINT_ERR_PARSE);
  } else if (strncmp(line_cursor, "hypergraph", 10U) == 0 && !is_ident_char(line_cursor[10])) {
    rc = parse_hypergraph_declaration(line_cursor, program, line, diagnostic);
  } else if (strncmp(line_cursor, "graph", 5U) == 0 && !is_ident_char(line_cursor[5])) {
    rc = parse_graph_declaration(line_cursor, program, line, diagnostic);
  } else if ((strncmp(line_cursor, "add_node", 8U) == 0 && !is_ident_char(line_cursor[8])) ||
             (strncmp(line_cursor, "add_edge", 8U) == 0 && !is_ident_char(line_cursor[8])) ||
             (strncmp(line_cursor, "remove_node", 11U) == 0 && !is_ident_char(line_cursor[11])) ||
             (strncmp(line_cursor, "remove_edge", 11U) == 0 && !is_ident_char(line_cursor[11]))) {
    rc = parse_graph_mutation_statement(line_cursor, program, line, diagnostic);
  } else if ((strncmp(line_cursor, "add_vertex", 10U) == 0 && !is_ident_char(line_cursor[10])) ||
             (strncmp(line_cursor, "add_hyperedge", 13U) == 0 && !is_ident_char(line_cursor[13])) ||
             (strncmp(line_cursor, "remove_vertex", 13U) == 0 && !is_ident_char(line_cursor[13])) ||
             (strncmp(line_cursor, "remove_hyperedge", 16U) == 0 && !is_ident_char(line_cursor[16]))) {
    rc = parse_hypergraph_mutation_statement(line_cursor, program, line, diagnostic);
  } else if ((strncmp(line_cursor, "set_node_attrs", 14U) == 0 && !is_ident_char(line_cursor[14])) ||
             (strncmp(line_cursor, "set_edge_attrs", 14U) == 0 && !is_ident_char(line_cursor[14])) ||
             (strncmp(line_cursor, "set_edge_weight", 15U) == 0 && !is_ident_char(line_cursor[15]))) {
    rc = parse_graph_attr_mutation_statement(line_cursor, program, line, diagnostic);
  } else if ((strncmp(line_cursor, "set_vertex_attrs", 16U) == 0 && !is_ident_char(line_cursor[16])) ||
             (strncmp(line_cursor, "set_hyperedge_attrs", 19U) == 0 && !is_ident_char(line_cursor[19]))) {
    rc = parse_hypergraph_attr_mutation_statement(line_cursor, program, line, diagnostic);
  } else {
    rc = parse_assignment(line_cursor, program, line, diagnostic);
  }
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
}
