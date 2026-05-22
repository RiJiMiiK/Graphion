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

static unsigned int source_column(const char *line_text, const char *cursor) {
  if (line_text == NULL || cursor == NULL || cursor < line_text) {
    return 1U;
  }
  return (unsigned int)(cursor - line_text) + 1U;
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
  if (cursor[0] == '>' && cursor[1] == '>' && cursor[2] == '=') {
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
                source_column(line_content(start_line), cursor),
                strcmp(keyword, "if") == 0 ? "expected condition after if" : "expected condition after elif",
                GINT_ERR_PARSE);
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
                    "multiline condition requires grouping parentheses",
                    GINT_ERR_PARSE);
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
          return fail(diagnostic,
                      lines[i].line,
                      source_column(line_content(&lines[i]), tail),
                      "unexpected trailing tokens after condition",
                      GINT_ERR_PARSE);
        }
        if (write_index == 0U) {
          return fail(diagnostic,
                      lines[i].line,
                      source_column(line_content(&lines[i]), scan),
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
          return fail(diagnostic,
                      line,
                      source_column(line_content(start_line), scan),
                      "multiline condition requires grouping parentheses",
                      GINT_ERR_PARSE);
        }
        return fail(diagnostic,
                    line,
                    source_column(line_content(start_line), scan),
                    strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
                    GINT_ERR_PARSE);
      }
      return fail(diagnostic,
                  lines[i].line,
                  source_column(line_content(&lines[i]), scan),
                  strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
                  GINT_ERR_PARSE);
    }
  }

  return fail(diagnostic,
              line,
              source_column(line_content(start_line), cursor),
              strcmp(keyword, "if") == 0 ? "expected ':' after if condition" : "expected ':' after elif condition",
              GINT_ERR_PARSE);
}

int parse_else_header(const char *cursor,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  const char *line_text = cursor;
  if (strncmp(cursor, "else", 4U) != 0 || is_ident_char(cursor[4])) {
    return fail(diagnostic, line, 1U, "invalid else header", GINT_ERR_PARSE);
  }
  cursor += 4;
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, source_column(line_text, cursor), "expected ':' after else", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, source_column(line_text, cursor), "unexpected trailing tokens after else", GINT_ERR_PARSE);
  }
  return GINT_OK;
}
