/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/source.h"

void runtime_match_case_value_init(runtime_match_case_value *entry) {
  if (entry == NULL) {
    return;
  }
  memset(entry, 0, sizeof(*entry));
  entry->value.kind = GVM_VALUE_NONE;
}

void runtime_match_case_value_dispose(runtime_match_case_value *entry) {
  if (entry == NULL) {
    return;
  }
  runtime_free_string(&entry->owned_string);
  entry->value.kind = GVM_VALUE_NONE;
  entry->value.as.string_value = NULL;
}

int runtime_match_case_value_clone(runtime_match_case_value *entry,
                                          const graphion_vm_value *value,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  runtime_match_case_value_init(entry);
  if (entry == NULL || value == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  entry->value = *value;
  if (value->kind == GVM_VALUE_STRING && value->as.string_value != NULL) {
    const size_t len = strlen(value->as.string_value);
    entry->owned_string = (char *)malloc(len + 1U);
    if (entry->owned_string == NULL) {
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    memcpy(entry->owned_string, value->as.string_value, len + 1U);
    entry->value.as.string_value = entry->owned_string;
  }
  return GINT_OK;
}

int line_is_blank(const runtime_source_line *line) {
  const char *cursor = line->text;
  skip_spaces(&cursor);
  return *cursor == '\0';
}

const char *line_content(const runtime_source_line *line) {
  const char *cursor = line->text;
  skip_spaces(&cursor);
  return cursor;
}

int line_starts_with_keyword(const runtime_source_line *line, const char *keyword) {
  const char *cursor = line_content(line);
  const size_t len = strlen(keyword);
  return strncmp(cursor, keyword, len) == 0 && !is_ident_char(cursor[len]);
}

int line_keyword_is_assignment_like(const runtime_source_line *line, const char *keyword) {
  const char *cursor = line_content(line);
  const size_t len = strlen(keyword);
  if (strncmp(cursor, keyword, len) != 0 || is_ident_char(cursor[len])) {
    return 0;
  }
  cursor += len;
  skip_spaces(&cursor);
  if (*cursor == '=') {
    return 1;
  }
  if ((cursor[0] == '+' || cursor[0] == '-' || cursor[0] == '*' || cursor[0] == '/' || cursor[0] == '%' ||
       cursor[0] == '&' || cursor[0] == '|' || cursor[0] == '^') &&
      cursor[1] == '=') {
    return 1;
  }
  if (cursor[0] == '*' && cursor[1] == '*' && cursor[2] == '=') {
    return 1;
  }
  if (cursor[0] == '/' && cursor[1] == '/' && cursor[2] == '=') {
    return 1;
  }
  if (cursor[0] == '<' && cursor[1] == '<' && cursor[2] == '=') {
    return 1;
  }
  return 0;
}

int line_is_if_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "if") && !line_keyword_is_assignment_like(line, "if");
}

int line_is_elif_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "elif") && !line_keyword_is_assignment_like(line, "elif");
}

int line_is_else_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "else") && !line_keyword_is_assignment_like(line, "else");
}

int line_is_match_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "match") && !line_keyword_is_assignment_like(line, "match");
}

int line_is_default_clause(const runtime_source_line *line) {
  return line_starts_with_keyword(line, "default") && !line_keyword_is_assignment_like(line, "default");
}

size_t find_next_nonblank_line(const runtime_source_line *lines, size_t count, size_t start);

size_t scan_block_end(const runtime_source_line *lines, size_t count, size_t start, unsigned int block_indent);

int collect_match_expression_text(const runtime_source_line *lines,
                                         size_t count,
                                         size_t start_index,
                                         char *buffer,
                                         size_t buffer_size,
                                         size_t *header_end_index_out,
                                         unsigned int line,
                                         graphion_runtime_diagnostic *diagnostic);

int parse_match_case_header(const char *cursor,
                                   graphion_vm_value *value_out,
                                   runtime_match_case_value *owned_value,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic);

const char *scalar_kind_name(const graphion_vm_value *value);

int copy_source_without_comments(const char *source,
                                        char **clean_out,
                                        graphion_runtime_diagnostic *diagnostic) {
  size_t len;
  char *clean;
  size_t read_index = 0U;
  size_t write_index = 0U;
  unsigned int line = 1U;
  unsigned int block_comment_line = 0U;
  int in_string = 0;
  int in_block_comment = 0;
  int in_line_comment = 0;
  if (source == NULL || clean_out == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  len = strlen(source);
  clean = (char *)malloc(len + 1U);
  if (clean == NULL) {
    return fail(diagnostic, 1U, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  while (source[read_index] != '\0') {
    const char ch = source[read_index];
    const char next = source[read_index + 1U];
    if (in_block_comment) {
      if (ch == '*' && next == '/') {
        in_block_comment = 0;
        read_index += 2U;
        continue;
      }
      if (ch == '\n') {
        clean[write_index++] = '\n';
        line++;
      }
      read_index++;
      continue;
    }
    if (in_line_comment) {
      if (ch == '\n') {
        clean[write_index++] = '\n';
        line++;
        in_line_comment = 0;
      }
      read_index++;
      continue;
    }
    if (in_string) {
      clean[write_index++] = ch;
      if (ch == '"') {
        in_string = 0;
      }
      if (ch == '\n') {
        line++;
      }
      read_index++;
      continue;
    }
    if (ch == '"') {
      in_string = 1;
      clean[write_index++] = ch;
      read_index++;
      continue;
    }
    if (ch == '#') {
      in_line_comment = 1;
      read_index++;
      continue;
    }
    if (ch == '/' && next == '*') {
      in_block_comment = 1;
      block_comment_line = line;
      read_index += 2U;
      continue;
    }
    clean[write_index++] = ch;
    if (ch == '\n') {
      line++;
    }
    read_index++;
  }
  if (in_block_comment) {
    free(clean);
    return fail(diagnostic, block_comment_line, 1U, "unterminated block comment", GINT_ERR_PARSE);
  }
  clean[write_index] = '\0';
  *clean_out = clean;
  return GINT_OK;
}

int split_source_lines(const char *source,
                              runtime_source_line *lines,
                              size_t capacity,
                              size_t *count_out,
                              graphion_runtime_diagnostic *diagnostic) {
  const char *line_start;
  const char *cursor;
  char *clean_source = NULL;
  unsigned int line_number = 1U;
  size_t count = 0U;
  int rc;
  if (source == NULL || lines == NULL || count_out == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  rc = copy_source_without_comments(source, &clean_source, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  line_start = clean_source;
  cursor = clean_source;
  for (;;) {
    if (*cursor == '\n' || *cursor == '\0') {
      size_t len = (size_t)(cursor - line_start);
      size_t indent = 0U;
      if (count >= capacity) {
        free(clean_source);
        return fail(diagnostic, line_number, 1U, "too many source lines", GINT_ERR_CAPACITY);
      }
      if (len >= sizeof(lines[count].text)) {
        free(clean_source);
        return fail(diagnostic, line_number, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      memcpy(lines[count].text, line_start, len);
      lines[count].text[len] = '\0';
      while (lines[count].text[indent] == ' ' || lines[count].text[indent] == '\t') {
        indent++;
      }
      lines[count].line = line_number;
      lines[count].indent = (unsigned int)indent;
      count++;
      if (*cursor == '\0') {
        break;
      }
      cursor++;
      line_number++;
      line_start = cursor;
      continue;
    }
    cursor++;
  }
  free(clean_source);
  *count_out = count;
  return GINT_OK;
}

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


size_t find_next_nonblank_line(const runtime_source_line *lines, size_t count, size_t start) {
  size_t i;
  for (i = start; i < count; ++i) {
    if (!line_is_blank(&lines[i])) {
      return i;
    }
  }
  return count;
}

size_t scan_block_end(const runtime_source_line *lines, size_t count, size_t start, unsigned int block_indent) {
  size_t i;
  for (i = start; i < count; ++i) {
    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent < block_indent) {
      break;
    }
  }
  return i;
}

int condition_line_looks_incomplete(const char *text, size_t length) {
  size_t end = length;

  while (end > 0U && (text[end - 1U] == ' ' || text[end - 1U] == '\t' || text[end - 1U] == '\r')) {
    end--;
  }
  if (end == 0U) {
    return 1;
  }
  {
    const char tail = text[end - 1U];
    if (tail == '(' || tail == '=' || tail == '!' || tail == '<' || tail == '>' || tail == '+' || tail == '-' ||
        tail == '*' || tail == '/' || tail == '%') {
      return 1;
    }
  }
  if (end >= 3U && strncmp(text + end - 3U, "and", 3U) == 0 && (end == 3U || !is_ident_char(text[end - 4U]))) {
    return 1;
  }
  if (end >= 2U && strncmp(text + end - 2U, "or", 2U) == 0 && (end == 2U || !is_ident_char(text[end - 3U]))) {
    return 1;
  }
  if (end >= 4U && strncmp(text + end - 4U, "nand", 4U) == 0 && (end == 4U || !is_ident_char(text[end - 5U]))) {
    return 1;
  }
  if (end >= 3U && strncmp(text + end - 3U, "nor", 3U) == 0 && (end == 3U || !is_ident_char(text[end - 4U]))) {
    return 1;
  }
  if (end >= 3U && strncmp(text + end - 3U, "not", 3U) == 0 && (end == 3U || !is_ident_char(text[end - 4U]))) {
    return 1;
  }
  return 0;
}

int ternary_line_looks_incomplete(const char *text, size_t length) {
  size_t end = length;

  while (end > 0U && (text[end - 1U] == ' ' || text[end - 1U] == '\t' || text[end - 1U] == '\r')) {
    end--;
  }
  if (end >= 2U && strncmp(text + end - 2U, "if", 2U) == 0 && (end == 2U || !is_ident_char(text[end - 3U]))) {
    return 1;
  }
  if (end >= 4U && strncmp(text + end - 4U, "else", 4U) == 0 && (end == 4U || !is_ident_char(text[end - 5U]))) {
    return 1;
  }
  return 0;
}

int collect_control_condition_text(const runtime_source_line *lines,
                                          size_t count,
                                          size_t start_index,
                                          const char *keyword,
                                          char *buffer,
                                          size_t buffer_size,
                                          size_t *header_end_index_out,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  const runtime_source_line *start_line;
  const char *cursor;
  size_t keyword_len;
  size_t write_index = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int multiline_allowed = 0;

  if (lines == NULL || start_index >= count || keyword == NULL || buffer == NULL || buffer_size == 0U ||
      header_end_index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  keyword_len = strlen(keyword);

  start_line = &lines[start_index];
  cursor = line_content(start_line);
  if (strncmp(cursor, keyword, keyword_len) != 0 || is_ident_char(cursor[keyword_len])) {
    return fail(diagnostic, line, 1U, "invalid conditional header", GINT_ERR_PARSE);
  }
  cursor += keyword_len;
  skip_spaces(&cursor);
  if (*cursor == '\0') {
    return fail(diagnostic,
                line,
                1U,
                strcmp(keyword, "if") == 0 ? "expected condition after if" : "expected condition after elif",
                GINT_ERR_PARSE);
  }

  multiline_allowed = *cursor == '(' ? 1 : 0;

  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (!multiline_allowed) {
        return fail(diagnostic, line, 1U, "multiline condition requires grouping parentheses", GINT_ERR_PARSE);
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      if (write_index > 0U && buffer[write_index - 1U] != ' ') {
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
          return fail(diagnostic, lines[i].line, 1U, "unexpected trailing tokens after condition", GINT_ERR_PARSE);
        }
        if (write_index == 0U) {
          return fail(diagnostic,
                      line,
                      1U,
                      strcmp(keyword, "if") == 0 ? "expected condition after if" : "expected condition after elif",
                      GINT_ERR_PARSE);
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
        if (condition_line_looks_incomplete(buffer, write_index)) {
          return fail(diagnostic, line, 1U, "multiline condition requires grouping parentheses", GINT_ERR_PARSE);
        }
        return fail(diagnostic,
                    line,
                    1U,
                    strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
                    GINT_ERR_PARSE);
      }
      return fail(diagnostic,
                  lines[i].line,
                  1U,
                  strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
                  GINT_ERR_PARSE);
    }
  }

  return fail(diagnostic,
              line,
              1U,
              strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
              GINT_ERR_PARSE);
}

int parse_else_header(const char *cursor,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  if (strncmp(cursor, "else", 4U) != 0 || is_ident_char(cursor[4])) {
    return fail(diagnostic, line, 1U, "invalid else header", GINT_ERR_PARSE);
  }
  cursor += 4;
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after else", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after else", GINT_ERR_PARSE);
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
