/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/stmt.h"

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
      return fail(diagnostic, line, 1U, "unknown variable", GINT_ERR_UNKNOWN_VARIABLE);
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
      return fail(diagnostic, line, 1U, "expected ']' after assignment target index", GINT_ERR_PARSE);
    }
    cursor++;
    skip_spaces(&cursor);
  }
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
    return fail(diagnostic, line, 1U, "expected '='", GINT_ERR_PARSE);
  }
  if (indexed_target && assign_op != '=') {
    return fail(diagnostic, line, 1U, "compound indexed assignment is not supported", GINT_ERR_PARSE);
  }
  if (!indexed_target && assign_op == '=') {
    rc = program_find_or_add_global(program, target, line, diagnostic, &target_index);
    if (rc != GINT_OK) {
      return rc;
    }
  } else if (!indexed_target) {
    int existing = program_find_global_index(program, target);
    if (existing < 0) {
      return fail(diagnostic, line, 1U, "unknown variable", GINT_ERR_UNKNOWN_VARIABLE);
    }
    target_index = (size_t)existing;
  }
  rc = parse_expression(&cursor, program, &expr, indexed_target ? 2U : 0U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unsupported assignment expression", GINT_ERR_PARSE);
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
    return fail(diagnostic, line, 1U, "expected '(' after print", GINT_ERR_PARSE);
  }
  cursor++;
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
        return fail(diagnostic, line, 1U, "expected ')' after print argument", GINT_ERR_PARSE);
      }
      cursor++;
      skip_spaces(&cursor);
      if (*cursor != '\0') {
        return fail(diagnostic, line, 1U, "unexpected trailing tokens after print", GINT_ERR_PARSE);
      }
      return program_emit(program, GVM_OP_PRINT_NEWLINE, 0U, 0U, 0, line, diagnostic);
    }
  }
  rc = parse_expression(&cursor, program, &expr, 0U, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ')') {
    return fail(diagnostic, line, 1U, "expected ')' after print argument", GINT_ERR_PARSE);
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

int parse_graph_declaration(const char *line_text,
                            graphion_runtime_program *program,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = line_text;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  size_t target_index = 0U;
  int rc;

  skip_spaces(&cursor);
  if (strncmp(cursor, "graph", 5U) != 0 || is_ident_char(cursor[5])) {
    return fail(diagnostic, line, 1U, "expected 'graph'", GINT_ERR_PARSE);
  }
  cursor += 5;
  if (*cursor != ' ' && *cursor != '\t') {
    return fail(diagnostic, line, 1U, "expected graph name", GINT_ERR_PARSE);
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
    return fail(diagnostic, line, 1U, "expected ';' after graph declaration", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after graph declaration", GINT_ERR_PARSE);
  }
  rc = program_find_or_add_global(program, target, line, diagnostic, &target_index);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = program_emit(program, GVM_OP_GRAPH_NEW, 0U, 0U, 0, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)target_index, line, diagnostic);
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
  } else if (strncmp(line_cursor, "graph", 5U) == 0 && !is_ident_char(line_cursor[5])) {
    rc = parse_graph_declaration(line_cursor, program, line, diagnostic);
  } else {
    rc = parse_assignment(line_cursor, program, line, diagnostic);
  }
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
}
