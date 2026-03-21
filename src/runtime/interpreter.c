/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum { GINT_LINE_MAX = 512 };

static void clear_diagnostic(graphion_runtime_diagnostic *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = 0U;
  diagnostic->column = 0U;
  diagnostic->message = NULL;
}

static void set_diagnostic(graphion_runtime_diagnostic *diagnostic,
                           size_t line,
                           size_t column,
                           const char *message) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = line;
  diagnostic->column = column;
  diagnostic->message = message;
}

static void trim_in_place(char *s) {
  size_t start = 0U;
  size_t end;
  while (s[start] != '\0' && isspace((unsigned char)s[start]) != 0) {
    ++start;
  }
  if (start > 0U) {
    size_t i = 0U;
    while (s[start + i] != '\0') {
      s[i] = s[start + i];
      ++i;
    }
    s[i] = '\0';
  }
  end = strlen(s);
  while (end > 0U && isspace((unsigned char)s[end - 1U]) != 0) {
    --end;
  }
  s[end] = '\0';
}

static void strip_comments(char *s) {
  size_t i = 0U;
  int in_string = 0;
  while (s[i] != '\0') {
    if (s[i] == '"' && (i == 0U || s[i - 1U] != '\\')) {
      in_string = !in_string;
    }
    if (!in_string && s[i] == '#') {
      s[i] = '\0';
      break;
    }
    if (!in_string && s[i] == '/' && s[i + 1U] == '/') {
      s[i] = '\0';
      break;
    }
    ++i;
  }
}

static int is_identifier_start(char c) {
  return isalpha((unsigned char)c) != 0 || c == '_';
}

static int is_identifier_char(char c) {
  return isalnum((unsigned char)c) != 0 || c == '_';
}

static int is_valid_identifier(const char *name) {
  size_t i;
  if (name == NULL || name[0] == '\0' || !is_identifier_start(name[0])) {
    return 0;
  }
  for (i = 1U; name[i] != '\0'; ++i) {
    if (!is_identifier_char(name[i])) {
      return 0;
    }
  }
  return 1;
}

static int is_reserved_name(const char *name) {
  static const char *reserved[] = {"def", "return", "print", "graph", "hypergraph", "true", "false"};
  size_t i;
  for (i = 0U; i < sizeof(reserved) / sizeof(reserved[0]); ++i) {
    if (strcmp(name, reserved[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static graphion_runtime_binding *find_binding_mut(graphion_runtime_scope *scope, const char *name) {
  size_t i;
  if (scope == NULL || name == NULL) {
    return NULL;
  }
  for (i = 0U; i < scope->count; ++i) {
    if (strcmp(scope->bindings[i].name, name) == 0) {
      return &scope->bindings[i];
    }
  }
  return NULL;
}

void graphion_runtime_scope_init(graphion_runtime_scope *scope) {
  if (scope == NULL) {
    return;
  }
  memset(scope, 0, sizeof(*scope));
}

const graphion_runtime_value *graphion_runtime_scope_find(const graphion_runtime_scope *scope,
                                                          const char *name) {
  size_t i;
  if (scope == NULL || name == NULL) {
    return NULL;
  }
  for (i = 0U; i < scope->count; ++i) {
    if (strcmp(scope->bindings[i].name, name) == 0) {
      return &scope->bindings[i].value;
    }
  }
  return NULL;
}

static int split_assignment(char *line, char **lhs, char **rhs) {
  size_t i = 0U;
  int in_string = 0;
  if (line == NULL || lhs == NULL || rhs == NULL) {
    return 0;
  }
  while (line[i] != '\0') {
    if (line[i] == '"' && (i == 0U || line[i - 1U] != '\\')) {
      in_string = !in_string;
    }
    if (!in_string && line[i] == '=') {
      line[i] = '\0';
      *lhs = line;
      *rhs = line + i + 1U;
      trim_in_place(*lhs);
      trim_in_place(*rhs);
      return (*lhs)[0] != '\0' && (*rhs)[0] != '\0';
    }
    ++i;
  }
  return 0;
}

static int parse_string_literal(const char *token, graphion_runtime_value *value) {
  size_t len;
  if (token == NULL || value == NULL) {
    return 0;
  }
  len = strlen(token);
  if (len < 2U || token[0] != '"' || token[len - 1U] != '"') {
    return 0;
  }
  if (len - 1U >= GRAPHION_RUNTIME_STRING_MAX) {
    return 0;
  }
  value->kind = GRAPHION_VALUE_STRING;
  memcpy(value->string_value, token + 1U, len - 2U);
  value->string_value[len - 2U] = '\0';
  return 1;
}

static int parse_bool_literal(const char *token, graphion_runtime_value *value) {
  if (token == NULL || value == NULL) {
    return 0;
  }
  if (strcmp(token, "true") == 0) {
    value->kind = GRAPHION_VALUE_BOOL;
    value->bool_value = 1;
    return 1;
  }
  if (strcmp(token, "false") == 0) {
    value->kind = GRAPHION_VALUE_BOOL;
    value->bool_value = 0;
    return 1;
  }
  return 0;
}

static int parse_int_literal(const char *token, graphion_runtime_value *value) {
  long long parsed;
  char *end = NULL;
  if (token == NULL || value == NULL) {
    return 0;
  }
  parsed = strtoll(token, &end, 10);
  if (end == NULL || *end != '\0') {
    return 0;
  }
  value->kind = GRAPHION_VALUE_INT;
  value->int_value = (int64_t)parsed;
  return 1;
}

static int token_looks_float(const char *token) {
  size_t i;
  for (i = 0U; token[i] != '\0'; ++i) {
    if (token[i] == '.' || token[i] == 'e' || token[i] == 'E') {
      return 1;
    }
  }
  return 0;
}

static int parse_float_literal(const char *token, graphion_runtime_value *value) {
  double parsed;
  char *end = NULL;
  if (token == NULL || value == NULL || !token_looks_float(token)) {
    return 0;
  }
  parsed = strtod(token, &end);
  if (end == NULL || *end != '\0') {
    return 0;
  }
  value->kind = GRAPHION_VALUE_FLOAT;
  value->float_value = parsed;
  return 1;
}

static int resolve_value(const char *token,
                         graphion_runtime_scope *scope,
                         graphion_runtime_value *value,
                         graphion_runtime_diagnostic *diagnostic,
                         size_t line_no) {
  const graphion_runtime_value *existing;
  if (parse_string_literal(token, value) || parse_bool_literal(token, value) ||
      parse_float_literal(token, value) || parse_int_literal(token, value)) {
    return GINT_OK;
  }
  if (!is_valid_identifier(token)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid assignment expression");
    return GINT_ERR_PARSE;
  }
  existing = graphion_runtime_scope_find(scope, token);
  if (existing == NULL) {
    set_diagnostic(diagnostic, line_no, 1U, "unknown variable in assignment expression");
    return GINT_ERR_UNKNOWN_VARIABLE;
  }
  *value = *existing;
  return GINT_OK;
}

static int assign_value(graphion_runtime_scope *scope, const char *name, const graphion_runtime_value *value) {
  graphion_runtime_binding *binding = find_binding_mut(scope, name);
  if (binding != NULL) {
    binding->value = *value;
    return GINT_OK;
  }
  if (scope->count >= GRAPHION_RUNTIME_BINDING_MAX) {
    return GINT_ERR_CAPACITY;
  }
  binding = &scope->bindings[scope->count++];
  memset(binding, 0, sizeof(*binding));
  memcpy(binding->name, name, strlen(name) + 1U);
  binding->value = *value;
  return GINT_OK;
}

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic) {
  const char *cursor;
  size_t line_no = 1U;
  clear_diagnostic(diagnostic);
  if (source == NULL || scope == NULL) {
    return GINT_ERR_INVALID_ARG;
  }

  cursor = source;
  while (*cursor != '\0') {
    char line[GINT_LINE_MAX];
    size_t len = 0U;
    char *lhs;
    char *rhs;
    graphion_runtime_value value;
    int rc;

    while (*cursor != '\0' && *cursor != '\n' && len < (GINT_LINE_MAX - 1U)) {
      line[len++] = *cursor++;
    }
    if (*cursor == '\n') {
      ++cursor;
    }
    line[len] = '\0';
    strip_comments(line);
    trim_in_place(line);
    if (line[0] == '\0') {
      ++line_no;
      continue;
    }
    if (!split_assignment(line, &lhs, &rhs)) {
      set_diagnostic(diagnostic, line_no, 1U, "expected assignment statement");
      return GINT_ERR_PARSE;
    }
    if (!is_valid_identifier(lhs)) {
      set_diagnostic(diagnostic, line_no, 1U, "invalid variable name");
      return GINT_ERR_PARSE;
    }
    if (is_reserved_name(lhs)) {
      set_diagnostic(diagnostic, line_no, 1U, "reserved name cannot be assigned");
      return GINT_ERR_RESERVED_NAME;
    }
    memset(&value, 0, sizeof(value));
    rc = resolve_value(rhs, scope, &value, diagnostic, line_no);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = assign_value(scope, lhs, &value);
    if (rc != GINT_OK) {
      set_diagnostic(diagnostic, line_no, 1U, "runtime scope capacity exceeded");
      return rc;
    }
    ++line_no;
  }

  return GINT_OK;
}
