/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/source.h"

int process_file_level_directives(const char *source,
                                  graphion_runtime_warning_report *report,
                                  graphion_runtime_diagnostic *diagnostic) {
  const char *cursor;
  unsigned int line;

  if (source == NULL || report == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }

  cursor = source;
  line = 1U;
  for (;;) {
    const char *line_start = cursor;
    const char *line_end = cursor;
    const char *trimmed;

    while (*line_end != '\0' && *line_end != '\n') {
      line_end++;
    }
    trimmed = line_start;
    while (trimmed < line_end && (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\r')) {
      trimmed++;
    }

    if (trimmed == line_end) {
      /* blank line before code is allowed */
    } else if (*trimmed == '#') {
      static const char directive_prefix[] = "# graphion:";
      static const char warnings_off_value[] = "warnings=off";
      const size_t directive_prefix_len = sizeof(directive_prefix) - 1U;

      if ((size_t)(line_end - trimmed) >= directive_prefix_len &&
          strncmp(trimmed, directive_prefix, directive_prefix_len) == 0) {
        const char *payload = trimmed + directive_prefix_len;
        const char *payload_end = line_end;
        char directive_value[32];
        size_t payload_len;
        while (payload < payload_end && (*payload == ' ' || *payload == '\t' || *payload == '\r')) {
          payload++;
        }
        while (payload_end > payload && (payload_end[-1] == ' ' || payload_end[-1] == '\t' || payload_end[-1] == '\r')) {
          payload_end--;
        }
        payload_len = (size_t)(payload_end - payload);
        if (payload_len < sizeof(directive_value)) {
          memcpy(directive_value, payload, payload_len);
          directive_value[payload_len] = '\0';
        } else {
          directive_value[0] = '\0';
        }
        if (strcmp(directive_value, warnings_off_value) == 0) {
          report->enabled = 0;
          report->count = 0U;
        } else {
          int rc = add_warning(report, line, 1U, "unknown graphion directive", diagnostic);
          if (rc != GINT_OK) {
            return rc;
          }
        }
      }
    } else {
      break;
    }

    if (*line_end == '\0') {
      break;
    }
    cursor = line_end + 1;
    line++;
  }
  return GINT_OK;
}

int collect_match_warnings(const runtime_source_line *lines,
                           size_t count,
                           graphion_runtime_warning_report *report,
                           graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  for (i = 0U; i < count; ++i) {
    char match_expression[512];
    size_t header_end_index = i;
    graphion_runtime_program program;
    const char *cursor;
    graphion_vm_value match_literal;
    size_t clause_index;
    unsigned int branch_indent;
    int rc;

    if (!line_is_match_clause(&lines[i])) {
      continue;
    }

    rc = collect_match_expression_text(lines,
                                       count,
                                       i,
                                       match_expression,
                                       sizeof(match_expression),
                                       &header_end_index,
                                       lines[i].line,
                                       diagnostic);
    if (rc != GINT_OK) {
      continue;
    }

    graphion_runtime_program_init(&program);
    cursor = match_expression;
    rc = parse_scalar_literal(&program, &cursor, &match_literal, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      graphion_runtime_program_dispose(&program);
      i = header_end_index;
      continue;
    }
    skip_spaces(&cursor);
    if (*cursor != '\0') {
      graphion_runtime_program_dispose(&program);
      i = header_end_index;
      continue;
    }

    clause_index = find_next_nonblank_line(lines, count, header_end_index + 1U);
    if (clause_index >= count || lines[clause_index].indent <= lines[i].indent) {
      graphion_runtime_program_dispose(&program);
      i = header_end_index;
      continue;
    }
    branch_indent = lines[clause_index].indent;

    while (clause_index < count) {
      runtime_match_case_value case_value;
      graphion_vm_value parsed_value;
      int compatible = 0;
      int equal = 0;

      if (line_is_blank(&lines[clause_index])) {
        clause_index++;
        continue;
      }
      if (lines[clause_index].indent < branch_indent) {
        break;
      }
      if (lines[clause_index].indent > branch_indent) {
        clause_index++;
        continue;
      }
      if (line_is_default_clause(&lines[clause_index])) {
        clause_index = scan_block_end(lines,
                                      count,
                                      find_next_nonblank_line(lines, count, clause_index + 1U),
                                      find_next_nonblank_line(lines, count, clause_index + 1U) < count
                                          ? lines[find_next_nonblank_line(lines, count, clause_index + 1U)].indent
                                          : branch_indent + 1U);
        continue;
      }

      runtime_match_case_value_init(&case_value);
      rc = parse_match_case_header(line_content(&lines[clause_index]),
                                   &parsed_value,
                                   &case_value,
                                   lines[clause_index].line,
                                   diagnostic);
      if (rc == GINT_OK) {
        scalar_values_match_equal(&match_literal, &parsed_value, &compatible, &equal);
        if (!compatible) {
          char message[GRAPHION_RUNTIME_WARNING_MESSAGE_MAX];
          snprintf(message,
                   sizeof(message),
                   "match case can never match a %s value",
                   scalar_kind_name(&match_literal));
          rc = add_warning(report, lines[clause_index].line, 1U, message, diagnostic);
          runtime_match_case_value_dispose(&case_value);
          if (rc != GINT_OK) {
            graphion_runtime_program_dispose(&program);
            return rc;
          }
        } else {
          runtime_match_case_value_dispose(&case_value);
        }
      }
      clause_index++;
    }

    graphion_runtime_program_dispose(&program);
    i = clause_index > 0U ? clause_index - 1U : i;
  }

  return GINT_OK;
}

int graphion_collect_source_warnings(const char *source,
                                     graphion_runtime_warning_report *report,
                                     graphion_runtime_diagnostic *diagnostic) {
  runtime_source_line lines[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t line_count = 0U;
  int rc;

  if (report == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  graphion_runtime_warning_report_init(report);
  if (diagnostic != NULL) {
    clear_diagnostic(diagnostic);
  }
  if (source == NULL) {
    return fail(diagnostic, 1U, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  rc = process_file_level_directives(source, report, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_warning_report_clear(report);
    return rc;
  }
  if (!report->enabled) {
    return GINT_OK;
  }
  rc = split_source_lines(source, lines, GRAPHION_RUNTIME_PROGRAM_MAX, &line_count, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_warning_report_clear(report);
    return rc;
  }
  rc = collect_match_warnings(lines, line_count, report, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_warning_report_clear(report);
    return rc;
  }
  return GINT_OK;
}

int collect_match_expression_text(const runtime_source_line *lines,
                                  size_t count,
                                  size_t start_index,
                                  char *buffer,
                                  size_t buffer_size,
                                  size_t *header_end_index_out,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic) {
  const runtime_source_line *start_line;
  const char *cursor;
  size_t write_index = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int multiline_allowed = 0;

  if (lines == NULL || start_index >= count || buffer == NULL || buffer_size == 0U || header_end_index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  start_line = &lines[start_index];
  cursor = line_content(start_line);
  if (strncmp(cursor, "match", 5U) != 0 || is_ident_char(cursor[5])) {
    return fail(diagnostic, line, 1U, "invalid match header", GINT_ERR_PARSE);
  }
  cursor += 5;
  skip_spaces(&cursor);
  if (*cursor == '\0') {
    return fail(diagnostic, line, 1U, "expected expression after match", GINT_ERR_PARSE);
  }

  multiline_allowed = *cursor == '(' ? 1 : 0;
  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (!multiline_allowed) {
        return fail(diagnostic, line, 1U, "multiline match expression requires grouping parentheses", GINT_ERR_PARSE);
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
      if (*scan == ':' && depth == 0) {
        const char *tail = scan + 1;
        while (write_index > 0U &&
               (buffer[write_index - 1U] == ' ' || buffer[write_index - 1U] == '\t' || buffer[write_index - 1U] == '\r')) {
          write_index--;
        }
        skip_spaces(&tail);
        if (*tail != '\0') {
          return fail(diagnostic, lines[i].line, 1U, "unexpected trailing tokens after match", GINT_ERR_PARSE);
        }
        if (write_index == 0U) {
          return fail(diagnostic, line, 1U, "expected expression after match", GINT_ERR_PARSE);
        }
        buffer[write_index] = '\0';
        *header_end_index_out = i;
        return GINT_OK;
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      buffer[write_index++] = *scan;
      scan++;
    }

    if (depth == 0) {
      if (i == start_index) {
        return fail(diagnostic, line, 1U, "expected ':' after match expression", GINT_ERR_PARSE);
      }
      return fail(diagnostic, lines[i].line, 1U, "expected ':' after match expression", GINT_ERR_PARSE);
    }
  }

  return fail(diagnostic, line, 1U, "expected ':' after match expression", GINT_ERR_PARSE);
}

int parse_match_case_header(const char *cursor,
                            graphion_vm_value *value_out,
                            runtime_match_case_value *owned_value,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  graphion_runtime_program program;
  int rc;

  if (cursor == NULL || value_out == NULL || owned_value == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  graphion_runtime_program_init(&program);
  skip_spaces(&cursor);
  rc = parse_scalar_literal(&program, &cursor, value_out, line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ':') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, 1U, "expected ':' after match case", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after match case", GINT_ERR_PARSE);
  }
  rc = runtime_match_case_value_clone(owned_value, value_out, line, diagnostic);
  graphion_runtime_program_dispose(&program);
  if (rc != GINT_OK) {
    return rc;
  }
  *value_out = owned_value->value;
  return GINT_OK;
}

int parse_default_header(const char *cursor,
                         unsigned int line,
                         graphion_runtime_diagnostic *diagnostic) {
  if (strncmp(cursor, "default", 7U) != 0 || is_ident_char(cursor[7])) {
    return fail(diagnostic, line, 1U, "invalid default header", GINT_ERR_PARSE);
  }
  cursor += 7;
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after default", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after default", GINT_ERR_PARSE);
  }
  return GINT_OK;
}

const char *scalar_kind_name(const graphion_vm_value *value) {
  if (value == NULL) {
    return "unknown";
  }
  switch (value->kind) {
    case GVM_VALUE_INT:
      return "int";
    case GVM_VALUE_FLOAT:
      return "float";
    case GVM_VALUE_BOOL:
      return "bool";
    case GVM_VALUE_STRING:
      return "string";
    default:
      return "unknown";
  }
}
