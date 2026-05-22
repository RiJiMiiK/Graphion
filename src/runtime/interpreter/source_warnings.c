/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/source.h"

static unsigned int source_column(const char *line_text, const char *cursor) {
  if (line_text == NULL || cursor == NULL || cursor < line_text) {
    return 1U;
  }
  return (unsigned int)(cursor - line_text) + 1U;
}

typedef struct {
  unsigned int line;
  unsigned int column;
  int set;
} warning_source_position;

int process_file_level_directives(const char *source,
                                  graphion_runtime_warning_report *report,
                                  graphion_runtime_diagnostic *diagnostic) {
  if (source == NULL || report == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  (void)source;
  (void)report;
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
          rc = add_warning(report,
                           lines[clause_index].line,
                           source_column(lines[clause_index].text, line_content(&lines[clause_index])),
                           message,
                           diagnostic);
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

static int line_is_graph_block_header(const runtime_source_line *line) {
  const char *cursor;
  const char *end;

  if (line == NULL || !line_starts_with_keyword(line, "graph")) {
    return 0;
  }
  cursor = line_content(line);
  while (*cursor != '\0') {
    cursor++;
  }
  end = cursor;
  while (end > line->text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
    end--;
  }
  return end > line->text && end[-1] == ':';
}

static void add_graph_warning_name(char names[GRAPHION_RUNTIME_PROGRAM_MAX][GRAPHION_RUNTIME_NAME_MAX],
                                   size_t *name_count,
                                   const char *name,
                                   size_t name_len) {
  size_t i;

  if (names == NULL || name_count == NULL || name == NULL || name_len == 0U ||
      name_len >= GRAPHION_RUNTIME_NAME_MAX || *name_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return;
  }
  for (i = 0U; i < *name_count; ++i) {
    if (strlen(names[i]) == name_len && strncmp(names[i], name, name_len) == 0) {
      return;
    }
  }
  memcpy(names[*name_count], name, name_len);
  names[*name_count][name_len] = '\0';
  *name_count += 1U;
}

static void collect_one_graph_ref_for_warning(const char **cursor,
                                              const runtime_source_line *line,
                                              unsigned char used_ids[GRAPHION_RUNTIME_PROGRAM_MAX],
                                              char names[GRAPHION_RUNTIME_PROGRAM_MAX][GRAPHION_RUNTIME_NAME_MAX],
                                              size_t *name_count,
                                              warning_source_position *first_numeric_ref) {
  if (cursor == NULL || *cursor == NULL) {
    return;
  }
  skip_spaces(cursor);
  if (**cursor == '"') {
    const char *name = *cursor + 1;
    size_t name_len = 0U;
    while (name[name_len] != '\0' && name[name_len] != '"') {
      name_len++;
    }
    if (name[name_len] == '"') {
      add_graph_warning_name(names, name_count, name, name_len);
      *cursor = name + name_len + 1U;
    }
    return;
  }
  if (**cursor >= '0' && **cursor <= '9') {
    const char *id_start = *cursor;
    char *end = NULL;
    const long id = strtol(*cursor, &end, 10);
    if (end != *cursor && (*end == '\0' || !is_ident_char(*end)) && id >= 0 &&
        (unsigned long)id < GRAPHION_RUNTIME_PROGRAM_MAX) {
      used_ids[(size_t)id] = 1U;
      if (first_numeric_ref != NULL && !first_numeric_ref->set && line != NULL) {
        first_numeric_ref->line = line->line;
        first_numeric_ref->column = source_column(line->text, id_start);
        first_numeric_ref->set = 1;
      }
    }
    if (end != NULL && end > *cursor) {
      *cursor = end;
    }
    return;
  }
  if (is_ident_start_char(**cursor)) {
    const char *name = *cursor;
    size_t name_len = 0U;
    while (is_ident_char(name[name_len])) {
      name_len++;
    }
    add_graph_warning_name(names, name_count, name, name_len);
    *cursor = name + name_len;
  }
}

static void collect_graph_refs_from_graph_line(const runtime_source_line *line,
                                               unsigned char used_ids[GRAPHION_RUNTIME_PROGRAM_MAX],
                                               char names[GRAPHION_RUNTIME_PROGRAM_MAX][GRAPHION_RUNTIME_NAME_MAX],
                                               size_t *name_count,
                                               warning_source_position *first_numeric_ref) {
  const char *cursor;

  if (line == NULL) {
    return;
  }
  cursor = line->text;
  skip_spaces(&cursor);
  if (strncmp(cursor, "defaults", 8U) == 0 && !is_ident_char(cursor[8])) {
    return;
  }
  collect_one_graph_ref_for_warning(&cursor, line, used_ids, names, name_count, first_numeric_ref);
  skip_spaces(&cursor);
  if (*cursor == '-') {
    cursor++;
    if (*cursor == '>') {
      cursor++;
    }
    collect_one_graph_ref_for_warning(&cursor, line, used_ids, names, name_count, first_numeric_ref);
  } else if (cursor[0] == '<' && cursor[1] == '-' && cursor[2] == '>') {
    cursor += 3;
    collect_one_graph_ref_for_warning(&cursor, line, used_ids, names, name_count, first_numeric_ref);
  }
}

static void fill_named_graph_warning_ids(unsigned char used_ids[GRAPHION_RUNTIME_PROGRAM_MAX], size_t name_count) {
  size_t name_index;
  size_t id;

  for (name_index = 0U; name_index < name_count; ++name_index) {
    for (id = 0U; id < GRAPHION_RUNTIME_PROGRAM_MAX; ++id) {
      if (!used_ids[id]) {
        used_ids[id] = 1U;
        break;
      }
    }
  }
}

static int append_missing_graph_id(char *message, size_t message_size, size_t id, size_t shown_count) {
  char fragment[32];
  const int written = snprintf(fragment, sizeof(fragment), "%s%lu", shown_count == 0U ? "" : ", ", (unsigned long)id);
  size_t message_len;

  if (written < 0 || (size_t)written >= sizeof(fragment)) {
    return 0;
  }
  message_len = strlen(message);
  if (message_len + (size_t)written + 1U >= message_size) {
    return 0;
  }
  memcpy(message + message_len, fragment, (size_t)written + 1U);
  return 1;
}

static int append_graph_warning_suffix(char *message, size_t message_size, const char *suffix) {
  size_t message_len;
  size_t suffix_len;

  if (message == NULL || suffix == NULL) {
    return 0;
  }
  message_len = strlen(message);
  suffix_len = strlen(suffix);
  if (message_len + suffix_len + 1U >= message_size) {
    return 0;
  }
  memcpy(message + message_len, suffix, suffix_len + 1U);
  return 1;
}

static int build_graph_numeric_gap_warning(const unsigned char used_ids[GRAPHION_RUNTIME_PROGRAM_MAX],
                                           char *message,
                                           size_t message_size) {
  size_t max_id = 0U;
  size_t i;
  int has_id = 0;
  size_t missing_count = 0U;
  size_t shown_count = 0U;
  int truncated = 0;

  if (message == NULL || message_size == 0U) {
    return 0;
  }
  message[0] = '\0';
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (used_ids[i]) {
      has_id = 1;
      max_id = i;
    }
  }
  if (!has_id) {
    return 0;
  }
  for (i = 0U; i <= max_id; ++i) {
    if (!used_ids[i]) {
      missing_count += 1U;
    }
  }
  if (missing_count == 0U) {
    return 0;
  }
  snprintf(message,
           message_size,
           "graph numeric node ids have gaps; missing %s: ",
           missing_count == 1U ? "id" : "ids");
  for (i = 0U; i <= max_id; ++i) {
    if (!used_ids[i]) {
      if (shown_count >= 4U || !append_missing_graph_id(message, message_size, i, shown_count)) {
        truncated = 1;
        break;
      }
      shown_count += 1U;
    }
  }
  if (truncated) {
    (void)append_graph_warning_suffix(message, message_size, ", ...");
  }
  return 1;
}

int collect_graph_warnings(const runtime_source_line *lines,
                           size_t count,
                           graphion_runtime_warning_report *report,
                           graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  if (lines == NULL || report == NULL) {
    return fail(diagnostic, 1U, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  for (i = 0U; i < count; ++i) {
    unsigned char used_ids[GRAPHION_RUNTIME_PROGRAM_MAX];
    char named_refs[GRAPHION_RUNTIME_PROGRAM_MAX][GRAPHION_RUNTIME_NAME_MAX];
    warning_source_position first_numeric_ref = {0U, 0U, 0};
    size_t named_ref_count = 0U;
    size_t body_start;
    size_t body_end;
    size_t j;

    if (!line_is_graph_block_header(&lines[i])) {
      continue;
    }
    memset(used_ids, 0, sizeof(used_ids));
    memset(named_refs, 0, sizeof(named_refs));
    body_start = find_next_nonblank_line(lines, count, i + 1U);
    if (body_start >= count || lines[body_start].indent <= lines[i].indent) {
      continue;
    }
    body_end = scan_block_end(lines, count, body_start, lines[body_start].indent);
    for (j = body_start; j < body_end; ++j) {
      if (!line_is_blank(&lines[j]) && lines[j].indent == lines[body_start].indent) {
        collect_graph_refs_from_graph_line(&lines[j], used_ids, named_refs, &named_ref_count, &first_numeric_ref);
      }
    }
    fill_named_graph_warning_ids(used_ids, named_ref_count);
    {
      char warning_message[128];
      if (build_graph_numeric_gap_warning(used_ids, warning_message, sizeof(warning_message))) {
        const int rc = add_warning(report,
                                   first_numeric_ref.set ? first_numeric_ref.line : lines[i].line,
                                   first_numeric_ref.set ? first_numeric_ref.column : 1U,
                                   warning_message,
                                   diagnostic);
        if (rc != GINT_OK) {
          return rc;
        }
      }
    }
    i = body_end > 0U ? body_end - 1U : i;
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
  rc = collect_graph_warnings(lines, line_count, report, diagnostic);
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
    return fail(diagnostic, line, source_column(line_content(start_line), cursor), "expected expression after match", GINT_ERR_PARSE);
  }

  multiline_allowed = *cursor == '(' ? 1 : 0;
  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (!multiline_allowed) {
        const char *start_content = line_content(start_line);
        const char *start_end = start_content;
        while (*start_end != '\0') {
          start_end++;
        }
        return fail(diagnostic,
                    line,
                    source_column(start_content, start_end),
                    "multiline match expression requires grouping parentheses",
                    GINT_ERR_PARSE);
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
          return fail(diagnostic,
                      lines[i].line,
                      source_column(line_content(&lines[i]), tail),
                      "unexpected trailing tokens after match",
                      GINT_ERR_PARSE);
        }
        if (write_index == 0U) {
          return fail(diagnostic,
                      lines[i].line,
                      source_column(line_content(&lines[i]), scan),
                      "expected expression after match",
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
        if (condition_line_looks_incomplete(buffer, write_index) && find_next_nonblank_line(lines, count, i + 1U) < count) {
          return fail(diagnostic,
                      line,
                      source_column(line_content(start_line), scan),
                      "multiline match expression requires grouping parentheses",
                      GINT_ERR_PARSE);
        }
        return fail(diagnostic,
                    line,
                    source_column(line_content(start_line), scan),
                    "expected ':' after match expression",
                    GINT_ERR_PARSE);
      }
      return fail(diagnostic,
                  lines[i].line,
                  source_column(line_content(&lines[i]), scan),
                  "expected ':' after match expression",
                  GINT_ERR_PARSE);
    }
  }

  return fail(diagnostic,
              line,
              source_column(line_content(start_line), cursor),
              "expected ':' after match expression",
              GINT_ERR_PARSE);
}

int parse_match_case_header(const char *cursor,
                            graphion_vm_value *value_out,
                            runtime_match_case_value *owned_value,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic) {
  graphion_runtime_program program;
  const char *line_text = cursor;
  int rc;

  if (cursor == NULL || value_out == NULL || owned_value == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  graphion_runtime_program_init(&program);
  skip_spaces(&cursor);
  rc = parse_scalar_literal(&program, &cursor, value_out, line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    if (rc == GINT_ERR_PARSE && diagnostic != NULL &&
        diagnostic->message != NULL &&
        strcmp(diagnostic->message, "expected scalar literal") == 0) {
      return fail(diagnostic, line, source_column(line_text, cursor), "expected match case literal", GINT_ERR_PARSE);
    }
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != ':') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, source_column(line_text, cursor), "expected ':' after match case", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic,
                line,
                source_column(line_text, cursor),
                "unexpected trailing tokens after match case",
                GINT_ERR_PARSE);
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
  const char *line_text = cursor;
  if (strncmp(cursor, "default", 7U) != 0 || is_ident_char(cursor[7])) {
    return fail(diagnostic, line, 1U, "invalid default header", GINT_ERR_PARSE);
  }
  cursor += 7;
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected ':' after default", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, source_column(line_text, cursor), "unexpected trailing tokens after default", GINT_ERR_PARSE);
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
