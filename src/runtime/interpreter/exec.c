/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/exec_internal.h"

static int graph_header_ends_with_colon(const char *text) {
  const char *cursor = text;
  skip_spaces(&cursor);
  if (strncmp(cursor, "graph", 5U) != 0 || is_ident_char(cursor[5])) {
    return 0;
  }
  while (*cursor != '\0') {
    cursor++;
  }
  while (cursor > text && (cursor[-1] == ' ' || cursor[-1] == '\t' || cursor[-1] == '\r')) {
    cursor--;
  }
  return cursor > text && cursor[-1] == ':';
}

static int parse_graph_node_line(const char *text,
                                 int64_t *explicit_id_out,
                                 int *has_explicit_id_out,
                                 unsigned int line,
                                 graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  char *end = NULL;
  int64_t id;

  if (text == NULL || explicit_id_out == NULL || has_explicit_id_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(&cursor);
  *has_explicit_id_out = 0;
  *explicit_id_out = 0;

  if (*cursor == '"') {
    cursor++;
    while (*cursor != '\0' && *cursor != '"') {
      cursor++;
    }
    if (*cursor != '"') {
      return fail(diagnostic, line, 1U, "unterminated string literal", GINT_ERR_PARSE);
    }
    cursor++;
    skip_spaces(&cursor);
    if (*cursor != '\0') {
      return fail(diagnostic, line, 1U, "unexpected trailing tokens after graph node", GINT_ERR_PARSE);
    }
    return GINT_OK;
  }

  if (*cursor == '-' || (*cursor >= '0' && *cursor <= '9')) {
    id = strtoll(cursor, &end, 10);
    if (end == cursor) {
      return fail(diagnostic, line, 1U, "expected graph node name or id", GINT_ERR_PARSE);
    }
    if (id < 0) {
      return fail(diagnostic, line, 1U, "graph node id must be non-negative", GINT_ERR_PARSE);
    }
    cursor = end;
    skip_spaces(&cursor);
    if (*cursor != '\0') {
      return fail(diagnostic, line, 1U, "unexpected trailing tokens after graph node", GINT_ERR_PARSE);
    }
    *has_explicit_id_out = 1;
    *explicit_id_out = id;
    return GINT_OK;
  }

  if (is_ident_start_char(*cursor)) {
    cursor++;
    while (is_ident_char(*cursor)) {
      cursor++;
    }
    skip_spaces(&cursor);
    if (*cursor != '\0') {
      return fail(diagnostic, line, 1U, "unexpected trailing tokens after graph node", GINT_ERR_PARSE);
    }
    return GINT_OK;
  }

  return fail(diagnostic, line, 1U, "expected graph node name or id", GINT_ERR_PARSE);
}

static int collect_graph_node_count(const runtime_source_line *lines,
                                    size_t count,
                                    size_t start_index,
                                    unsigned int current_indent,
                                    size_t *end_index_out,
                                    size_t *node_count_out,
                                    unsigned int line,
                                    graphion_runtime_diagnostic *diagnostic) {
  unsigned char used_ids[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t body_start;
  size_t body_end;
  unsigned int body_indent;
  size_t named_count = 0U;
  size_t explicit_count = 0U;
  size_t i;

  if (lines == NULL || end_index_out == NULL || node_count_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  memset(used_ids, 0, sizeof(used_ids));

  body_start = find_next_nonblank_line(lines, count, start_index + 1U);
  if (body_start >= count || lines[body_start].indent <= current_indent) {
    return fail(diagnostic, line, 1U, "expected indented graph node block", GINT_ERR_PARSE);
  }
  body_indent = lines[body_start].indent;
  body_end = scan_block_end(lines, count, body_start, body_indent);

  for (i = body_start; i < body_end; ++i) {
    int has_explicit_id = 0;
    int64_t explicit_id = 0;
    int rc;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    rc = parse_graph_node_line(line_content(&lines[i]),
                               &explicit_id,
                               &has_explicit_id,
                               lines[i].line,
                               diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    if (has_explicit_id) {
      if ((uint64_t)explicit_id >= GRAPHION_RUNTIME_PROGRAM_MAX) {
        return fail(diagnostic, lines[i].line, 1U, "graph node id is too large", GINT_ERR_CAPACITY);
      }
      if (used_ids[(size_t)explicit_id]) {
        return fail(diagnostic, lines[i].line, 1U, "duplicate graph node id", GINT_ERR_PARSE);
      }
      used_ids[(size_t)explicit_id] = 1U;
      explicit_count += 1U;
    } else {
      named_count += 1U;
    }
  }

  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX && named_count > 0U; ++i) {
    if (!used_ids[i]) {
      used_ids[i] = 1U;
      named_count -= 1U;
    }
  }
  if (named_count > 0U) {
    return fail(diagnostic, line, 1U, "too many graph nodes", GINT_ERR_CAPACITY);
  }
  for (i = GRAPHION_RUNTIME_PROGRAM_MAX; i > 0U; --i) {
    if (used_ids[i - 1U]) {
      *node_count_out = i;
      *end_index_out = body_end - 1U;
      return GINT_OK;
    }
  }
  *node_count_out = explicit_count;
  *end_index_out = body_end - 1U;
  return GINT_OK;
}

static int collect_assignment_statement_text(const runtime_source_line *lines,
                                             size_t count,
                                             size_t start_index,
                                             char *buffer,
                                             size_t buffer_size,
                                             size_t *end_index_out,
                                             unsigned int line,
                                             graphion_runtime_diagnostic *diagnostic) {
  const runtime_source_line *start_line;
  const char *cursor;
  const char *rhs_cursor;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  size_t write_index = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int multiline_allowed = 0;
  int saw_nonblank_continuation = 0;
  int rc;

  if (lines == NULL || start_index >= count || buffer == NULL || buffer_size == 0U || end_index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  start_line = &lines[start_index];
  cursor = line_content(start_line);
  rhs_cursor = cursor;
  rc = parse_identifier_token(&rhs_cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&rhs_cursor);
  if (*rhs_cursor == '[') {
    int bracket_depth = 0;
    int bracket_in_string = 0;
    do {
      if (*rhs_cursor == '\0') {
        return fail(diagnostic, line, 1U, "expected ']' after assignment target index", GINT_ERR_PARSE);
      }
      if (bracket_in_string) {
        if (*rhs_cursor == '"') {
          bracket_in_string = 0;
        }
      } else if (*rhs_cursor == '"') {
        bracket_in_string = 1;
      } else if (*rhs_cursor == '[') {
        bracket_depth++;
      } else if (*rhs_cursor == ']') {
        bracket_depth--;
      }
      rhs_cursor++;
    } while (bracket_depth > 0);
    skip_spaces(&rhs_cursor);
  }
  if (rhs_cursor[0] == '*' && rhs_cursor[1] == '*' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if (rhs_cursor[0] == '/' && rhs_cursor[1] == '/' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if (rhs_cursor[0] == '<' && rhs_cursor[1] == '<' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if (rhs_cursor[0] == '>' && rhs_cursor[1] == '>' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if ((rhs_cursor[0] == '+' || rhs_cursor[0] == '-' || rhs_cursor[0] == '*' || rhs_cursor[0] == '/' ||
              rhs_cursor[0] == '%' || rhs_cursor[0] == '&' || rhs_cursor[0] == '|' || rhs_cursor[0] == '^') &&
             rhs_cursor[1] == '=') {
    rhs_cursor += 2;
  } else if (*rhs_cursor == '=') {
    rhs_cursor++;
  } else {
    return fail(diagnostic, line, 1U, "expected '='", GINT_ERR_PARSE);
  }
  skip_spaces(&rhs_cursor);
  multiline_allowed = *rhs_cursor == '(' ? 1 : 0;

  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (line_is_blank(&lines[i])) {
        continue;
      }
      saw_nonblank_continuation = 1;
      if (!multiline_allowed) {
        return fail(diagnostic, line, 1U, "multiline assignment expression requires grouping parentheses", GINT_ERR_PARSE);
      }
      if (write_index > 0U && buffer[write_index - 1U] != ' ') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = ' ';
      }
    }

    while (*scan != '\0') {
      if (in_string) {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (*scan == '"') {
          in_string = 0;
        }
        scan++;
        continue;
      }
      if (*scan == '"') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        in_string = 1;
        scan++;
        continue;
      }
      if (*scan == '(') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        depth++;
        scan++;
        continue;
      }
      if (*scan == ')') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (depth > 0) {
          depth--;
        }
        scan++;
        continue;
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      buffer[write_index++] = *scan;
      scan++;
    }

    while (write_index > 0U &&
           (buffer[write_index - 1U] == ' ' || buffer[write_index - 1U] == '\t' || buffer[write_index - 1U] == '\r')) {
      write_index--;
    }
    if (depth == 0) {
      if (i == start_index && ternary_line_looks_incomplete(buffer, write_index) &&
          find_next_nonblank_line(lines, count, i + 1U) < count) {
        return fail(diagnostic, line, 1U, "multiline assignment expression requires grouping parentheses", GINT_ERR_PARSE);
      }
      buffer[write_index] = '\0';
      *end_index_out = i;
      return GINT_OK;
    }
  }

  if (!saw_nonblank_continuation) {
    buffer[write_index] = '\0';
    *end_index_out = start_index;
    return GINT_OK;
  }
  return fail(diagnostic, line, 1U, "expected ')' after expression", GINT_ERR_PARSE);
}

static int execute_statement_source_line(const runtime_source_line *lines,
                                         size_t count,
                                         size_t *index,
                                         graphion_runtime_scope *scope,
                                         graphion_runtime_diagnostic *diagnostic,
                                         FILE *output) {
  char statement_text[512];
  const char *statement_source = line_content(&lines[*index]);
  graphion_runtime_program program;
  size_t statement_end = *index;
  size_t graph_node_count = 0U;
  int rc;

  if (strncmp(statement_source, "graph", 5U) == 0 && !is_ident_char(statement_source[5]) &&
      graph_header_ends_with_colon(statement_source)) {
    rc = collect_graph_node_count(lines,
                                  count,
                                  *index,
                                  lines[*index].indent,
                                  &statement_end,
                                  &graph_node_count,
                                  lines[*index].line,
                                  diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  } else if (!(strncmp(statement_source, "print", 5U) == 0 && !is_ident_char(statement_source[5])) &&
             !(strncmp(statement_source, "graph", 5U) == 0 && !is_ident_char(statement_source[5]))) {
    rc = collect_assignment_statement_text(lines,
                                           count,
                                           *index,
                                           statement_text,
                                           sizeof(statement_text),
                                           &statement_end,
                                           lines[*index].line,
                                           diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    statement_source = statement_text;
  }
  rc = seed_program_from_scope(&program, scope, lines[*index].line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  if (strncmp(statement_source, "graph", 5U) == 0 && !is_ident_char(statement_source[5]) &&
      graph_header_ends_with_colon(statement_source)) {
    rc = parse_graph_declaration_with_node_count(statement_source,
                                                 graph_node_count,
                                                 &program,
                                                 lines[*index].line,
                                                 diagnostic);
  } else {
    rc = parse_statement_line(statement_source, scope, &program, lines[*index].line, diagnostic);
  }
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  if (program.program_len > 0U) {
    rc = graphion_execute_program(&program, scope, diagnostic, output);
    graphion_runtime_program_dispose(&program);
    if (rc != GINT_OK) {
      return rc;
    }
  } else {
    graphion_runtime_program_dispose(&program);
  }
  *index = statement_end + 1U;
  return GINT_OK;
}

int execute_block(const runtime_source_line *lines,
                  size_t count,
                  size_t *index,
                  unsigned int block_indent,
                  graphion_runtime_scope *scope,
                  graphion_runtime_diagnostic *diagnostic,
                  FILE *output);

static int execute_if_chain(const runtime_source_line *lines,
                            size_t count,
                            size_t *index,
                            unsigned int current_indent,
                            graphion_runtime_scope *scope,
                            graphion_runtime_diagnostic *diagnostic,
                            FILE *output) {
  size_t clause_index = *index;
  int branch_taken = 0;
  int seen_else = 0;
  int first_clause = 1;
  while (clause_index < count) {
    const runtime_source_line *clause_line;
    const char *cursor;
    size_t body_start;
    size_t body_end;
    unsigned int body_indent;
    int is_else_clause;
    if (line_is_blank(&lines[clause_index])) {
      clause_index++;
      continue;
    }
    if (lines[clause_index].indent != current_indent) {
      break;
    }
    clause_line = &lines[clause_index];
    cursor = line_content(clause_line);
    is_else_clause = line_is_else_clause(clause_line);
    if (!line_is_if_clause(clause_line) && !line_is_elif_clause(clause_line) && !is_else_clause) {
      break;
    }
    if (!first_clause && line_is_if_clause(clause_line)) {
      break;
    }
    if (seen_else && line_is_if_clause(clause_line)) {
      break;
    }
    if (seen_else && (line_is_elif_clause(clause_line) || is_else_clause)) {
      return fail(diagnostic, clause_line->line, 1U, "else must be last in if chain", GINT_ERR_PARSE);
    }
    if (is_else_clause) {
      int rc = parse_else_header(cursor, clause_line->line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      body_start = find_next_nonblank_line(lines, count, clause_index + 1U);
      if (body_start >= count || lines[body_start].indent <= current_indent) {
        return fail(diagnostic, clause_line->line, 1U, "expected indented block after else", GINT_ERR_PARSE);
      }
      body_indent = lines[body_start].indent;
      body_end = scan_block_end(lines, count, body_start, body_indent);
      if (body_end < count && !line_is_blank(&lines[body_end]) && lines[body_end].indent > current_indent &&
          lines[body_end].indent < body_indent) {
        return fail(diagnostic, lines[body_end].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
      }
      seen_else = 1;
      if (!branch_taken) {
        size_t exec_index = body_start;
        rc = execute_block(lines, count, &exec_index, body_indent, scope, diagnostic, output);
        if (rc != GINT_OK) {
          return rc;
        }
        body_end = exec_index;
        branch_taken = 1;
      }
    } else {
      char condition_text[512];
      size_t header_end_index = clause_index;
      int rc = collect_control_condition_text(lines,
                                              count,
                                              clause_index,
                                              line_is_elif_clause(clause_line) ? "elif" : "if",
                                              condition_text,
                                              sizeof(condition_text),
                                              &header_end_index,
                                              clause_line->line,
                                              diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      body_start = find_next_nonblank_line(lines, count, header_end_index + 1U);
      if (body_start >= count || lines[body_start].indent <= current_indent) {
        return fail(diagnostic,
                    clause_line->line,
                    1U,
                    line_is_elif_clause(clause_line) ? "expected indented block after elif" :
                    "expected indented block after if",
                    GINT_ERR_PARSE);
      }
      body_indent = lines[body_start].indent;
      body_end = scan_block_end(lines, count, body_start, body_indent);
      if (body_end < count && !line_is_blank(&lines[body_end]) && lines[body_end].indent > current_indent &&
          lines[body_end].indent < body_indent) {
        return fail(diagnostic, lines[body_end].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
      }
      if (!branch_taken) {
        int condition_true = 0;
        rc = evaluate_condition_text(condition_text,
                                     strlen(condition_text),
                                     scope,
                                     clause_line->line,
                                     diagnostic,
                                     &condition_true);
        if (rc != GINT_OK) {
          return rc;
        }
        if (condition_true) {
          size_t exec_index = body_start;
          rc = execute_block(lines, count, &exec_index, body_indent, scope, diagnostic, output);
          if (rc != GINT_OK) {
            return rc;
          }
          body_end = exec_index;
          branch_taken = 1;
        }
      }
    }
    clause_index = body_end;
    first_clause = 0;
  }
  *index = clause_index;
  return GINT_OK;
}

static int execute_match_statement(const runtime_source_line *lines,
                                   size_t count,
                                   size_t *index,
                                   unsigned int current_indent,
                                   graphion_runtime_scope *scope,
                                   graphion_runtime_diagnostic *diagnostic,
                                   FILE *output) {
  char match_expression[512];
  graphion_vm_value match_value;
  runtime_match_case_value seen_cases[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t seen_case_count = 0U;
  size_t header_end_index = *index;
  size_t clause_index;
  unsigned int branch_indent;
  int branch_taken = 0;
  int seen_default = 0;
  int rc;
  size_t i;

  for (i = 0U; i < GRAPHION_RUNTIME_WARNING_MAX; ++i) {
    runtime_match_case_value_init(&seen_cases[i]);
  }

  rc = collect_match_expression_text(lines,
                                     count,
                                     *index,
                                     match_expression,
                                     sizeof(match_expression),
                                     &header_end_index,
                                     lines[*index].line,
                                     diagnostic);
  if (rc != GINT_OK) {
    goto cleanup;
  }

  rc = evaluate_expression_text_to_value(match_expression,
                                         strlen(match_expression),
                                         scope,
                                         lines[*index].line,
                                         diagnostic,
                                         &match_value);
  if (rc != GINT_OK) {
    goto cleanup;
  }

  clause_index = find_next_nonblank_line(lines, count, header_end_index + 1U);
  if (clause_index >= count || lines[clause_index].indent <= current_indent) {
    rc = fail(diagnostic, lines[*index].line, 1U, "expected indented match block", GINT_ERR_PARSE);
    goto cleanup;
  }
  branch_indent = lines[clause_index].indent;

  while (clause_index < count) {
    size_t label_start = clause_index;
    size_t label_index = clause_index;
    size_t body_start;
    size_t body_end;
    int label_matches = 0;
    int is_default = 0;

    if (line_is_blank(&lines[clause_index])) {
      clause_index++;
      continue;
    }
    if (lines[clause_index].indent < branch_indent) {
      break;
    }
    if (lines[clause_index].indent > branch_indent) {
      rc = fail(diagnostic, lines[clause_index].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
      goto cleanup;
    }

    if (line_is_default_clause(&lines[clause_index])) {
      is_default = 1;
      if (seen_default) {
        rc = fail(diagnostic, lines[clause_index].line, 1U, "default can only appear once", GINT_ERR_PARSE);
        goto cleanup;
      }
      rc = parse_default_header(line_content(&lines[clause_index]), lines[clause_index].line, diagnostic);
      if (rc != GINT_OK) {
        goto cleanup;
      }
      seen_default = 1;
      label_index = clause_index + 1U;
    } else {
      while (label_index < count) {
        runtime_match_case_value case_value;
        graphion_vm_value parsed_value;
        int compatible = 0;
        int equal = 0;
        size_t existing_index;

        runtime_match_case_value_init(&case_value);
        if (line_is_blank(&lines[label_index])) {
          label_index++;
          continue;
        }
        if (lines[label_index].indent != branch_indent || line_is_default_clause(&lines[label_index])) {
          break;
        }
        rc = parse_match_case_header(line_content(&lines[label_index]),
                                     &parsed_value,
                                     &case_value,
                                     lines[label_index].line,
                                     diagnostic);
        if (rc != GINT_OK) {
          runtime_match_case_value_dispose(&case_value);
          goto cleanup;
        }
        for (existing_index = 0U; existing_index < seen_case_count; ++existing_index) {
          scalar_values_match_equal(&seen_cases[existing_index].value, &parsed_value, &compatible, &equal);
          if (compatible && equal) {
            runtime_match_case_value_dispose(&case_value);
            rc = fail(diagnostic, lines[label_index].line, 1U, "duplicate match case", GINT_ERR_PARSE);
            goto cleanup;
          }
        }
        if (seen_case_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
          runtime_match_case_value_dispose(&case_value);
          rc = fail(diagnostic, lines[label_index].line, 1U, "too many match cases", GINT_ERR_CAPACITY);
          goto cleanup;
        }
        seen_cases[seen_case_count++] = case_value;
        scalar_values_match_equal(&match_value, &parsed_value, &compatible, &equal);
        if (compatible && equal) {
          label_matches = 1;
        }
        label_index++;
        if (label_index >= count || line_is_blank(&lines[label_index]) || lines[label_index].indent != branch_indent) {
          break;
        }
      }
      if (label_index == label_start) {
        rc = fail(diagnostic, lines[clause_index].line, 1U, "expected scalar literal", GINT_ERR_PARSE);
        goto cleanup;
      }
    }

    body_start = find_next_nonblank_line(lines, count, label_index);
    if (body_start >= count || lines[body_start].indent <= branch_indent) {
      rc = fail(diagnostic,
                lines[label_start].line,
                1U,
                is_default ? "expected indented block after default" : "expected indented block after match case",
                GINT_ERR_PARSE);
      goto cleanup;
    }
    body_end = scan_block_end(lines, count, body_start, lines[body_start].indent);

    if (is_default && find_next_nonblank_line(lines, count, body_end) < count &&
        lines[find_next_nonblank_line(lines, count, body_end)].indent == branch_indent) {
      rc = fail(diagnostic, lines[label_start].line, 1U, "default must be last in match", GINT_ERR_PARSE);
      goto cleanup;
    }

    if (!branch_taken && (is_default || label_matches)) {
      size_t exec_index = body_start;
      rc = execute_block(lines, count, &exec_index, lines[body_start].indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        goto cleanup;
      }
      body_end = exec_index;
      branch_taken = 1;
    }

    clause_index = body_end;
  }

  *index = clause_index;
  rc = GINT_OK;

cleanup:
  for (i = 0U; i < seen_case_count; ++i) {
    runtime_match_case_value_dispose(&seen_cases[i]);
  }
  return rc;
}

int execute_block(const runtime_source_line *lines,
                  size_t count,
                  size_t *index,
                  unsigned int block_indent,
                  graphion_runtime_scope *scope,
                  graphion_runtime_diagnostic *diagnostic,
                  FILE *output) {
  size_t i = *index;
  while (i < count) {
    if (line_is_blank(&lines[i])) {
      i++;
      continue;
    }
    if (lines[i].indent < block_indent) {
      break;
    }
    if (lines[i].indent > block_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (line_is_elif_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "elif without matching if", GINT_ERR_PARSE);
    }
    if (line_is_else_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "else without matching if", GINT_ERR_PARSE);
    }
    if (line_is_default_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "default without matching match", GINT_ERR_PARSE);
    }
    if (line_is_if_clause(&lines[i])) {
      int rc = execute_if_chain(lines, count, &i, block_indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
      continue;
    }
    if (line_is_match_clause(&lines[i])) {
      int rc = execute_match_statement(lines, count, &i, block_indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
      continue;
    }
    {
      int rc = execute_statement_source_line(lines, count, &i, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
    }
  }
  *index = i;
  return GINT_OK;
}

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output) {
  runtime_source_line lines[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t line_count = 0U;
  size_t index = 0U;
  int rc;
  if (source == NULL || scope == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  rc = split_source_lines(source, lines, GRAPHION_RUNTIME_PROGRAM_MAX, &line_count, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = execute_block(lines, line_count, &index, 0U, scope, diagnostic, output);
  if (rc != GINT_OK) {
    return rc;
  }
  while (index < line_count) {
    if (!line_is_blank(&lines[index])) {
      return fail(diagnostic, lines[index].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    index++;
  }
  return GINT_OK;
}
