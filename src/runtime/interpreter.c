/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum {
  GINT_LINE_MAX = 512,
  GINT_SOURCE_LINE_MAX = 256,
  GINT_FUNCTION_MAX = 32,
  GINT_PARAM_MAX = 8,
  GINT_ARG_MAX = 8
};

typedef struct {
  size_t line_no;
  size_t indent;
  char text[GINT_LINE_MAX];
} graphion_source_line;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  char params[GINT_PARAM_MAX][GRAPHION_RUNTIME_NAME_MAX];
  size_t param_count;
  size_t body_start;
  size_t body_end;
} graphion_runtime_function;

typedef struct {
  graphion_source_line lines[GINT_SOURCE_LINE_MAX];
  size_t line_count;
  graphion_runtime_function functions[GINT_FUNCTION_MAX];
  size_t function_count;
} graphion_program;

static void set_diagnostic(graphion_runtime_diagnostic *diagnostic,
                           size_t line,
                           size_t column,
                           const char *message);
static void trim_in_place(char *s);
static int is_valid_identifier(const char *name);
static int is_reserved_name(const char *name);
static int parse_int_literal(const char *token, graphion_runtime_value *value);

static int parse_graph_header(const char *text,
                              char *name_out,
                              graphion_runtime_diagnostic *diagnostic,
                              size_t line_no) {
  size_t name_len;
  const char *cursor;
  if (strncmp(text, "graph ", 6U) != 0) {
    return 0;
  }
  cursor = text + 6U;
  name_len = strlen(cursor);
  if (name_len == 0U || cursor[name_len - 1U] != ':') {
    set_diagnostic(diagnostic, line_no, 1U, "invalid graph declaration");
    return GINT_ERR_PARSE;
  }
  if (name_len >= GRAPHION_RUNTIME_NAME_MAX) {
    set_diagnostic(diagnostic, line_no, 1U, "graph name too long");
    return GINT_ERR_PARSE;
  }
  memcpy(name_out, cursor, name_len - 1U);
  name_out[name_len - 1U] = '\0';
  trim_in_place(name_out);
  if (!is_valid_identifier(name_out)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid graph name");
    return GINT_ERR_PARSE;
  }
  if (is_reserved_name(name_out)) {
    set_diagnostic(diagnostic, line_no, 1U, "reserved name cannot be used as a graph");
    return GINT_ERR_RESERVED_NAME;
  }
  return GINT_OK;
}

static int parse_graph_edge(const char *text,
                            graphion_runtime_graph_edge *edge,
                            graphion_runtime_diagnostic *diagnostic,
                            size_t line_no) {
  const char *arrow;
  char lhs[GRAPHION_RUNTIME_NAME_MAX];
  char rhs[GRAPHION_RUNTIME_NAME_MAX];
  size_t lhs_len;
  size_t rhs_len;
  graphion_runtime_value source_value;
  graphion_runtime_value target_value;

  if (text == NULL || edge == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  arrow = strstr(text, "->");
  if (arrow == NULL) {
    set_diagnostic(diagnostic, line_no, 1U, "expected graph edge using a -> b syntax");
    return GINT_ERR_PARSE;
  }
  lhs_len = (size_t)(arrow - text);
  rhs_len = strlen(arrow + 2U);
  if (lhs_len == 0U || lhs_len >= sizeof(lhs) || rhs_len == 0U || rhs_len >= sizeof(rhs)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid graph edge");
    return GINT_ERR_PARSE;
  }
  memcpy(lhs, text, lhs_len);
  lhs[lhs_len] = '\0';
  memcpy(rhs, arrow + 2U, rhs_len + 1U);
  trim_in_place(lhs);
  trim_in_place(rhs);
  if (!parse_int_literal(lhs, &source_value) || !parse_int_literal(rhs, &target_value)) {
    set_diagnostic(diagnostic, line_no, 1U, "graph node ids must be integers");
    return GINT_ERR_PARSE;
  }
  edge->source = source_value.int_value;
  edge->target = target_value.int_value;
  return GINT_OK;
}

static int graph_contains_node(const graphion_runtime_value *graph, int64_t node_id) {
  size_t i;
  if (graph == NULL) {
    return 0;
  }
  for (i = 0U; i < graph->graph_edge_count; ++i) {
    if (graph->graph_edges[i].source == node_id || graph->graph_edges[i].target == node_id) {
      return 1;
    }
  }
  return 0;
}

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
  if (s == NULL) {
    return;
  }
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

static const graphion_runtime_value *find_value(const graphion_runtime_scope *local_scope,
                                                const graphion_runtime_scope *global_scope,
                                                const char *name) {
  const graphion_runtime_value *value = graphion_runtime_scope_find(local_scope, name);
  if (value != NULL) {
    return value;
  }
  return graphion_runtime_scope_find(global_scope, name);
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
  binding = &scope->bindings[scope->count];
  memset(binding, 0, sizeof(*binding));
  memcpy(binding->name, name, strlen(name) + 1U);
  binding->value = *value;
  scope->count += 1U;
  return GINT_OK;
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
  memset(value, 0, sizeof(*value));
  value->kind = GRAPHION_VALUE_STRING;
  memcpy(value->string_value, token + 1U, len - 2U);
  value->string_value[len - 2U] = '\0';
  return 1;
}

static int parse_bool_literal(const char *token, graphion_runtime_value *value) {
  if (token == NULL || value == NULL) {
    return 0;
  }
  memset(value, 0, sizeof(*value));
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
  memset(value, 0, sizeof(*value));
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
  memset(value, 0, sizeof(*value));
  value->kind = GRAPHION_VALUE_FLOAT;
  value->float_value = parsed;
  return 1;
}

static int parse_source_lines(const char *source,
                              graphion_program *program,
                              graphion_runtime_diagnostic *diagnostic) {
  const char *cursor;
  size_t line_no = 1U;
  if (source == NULL || program == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  memset(program, 0, sizeof(*program));
  cursor = source;
  while (*cursor != '\0') {
    char raw[GINT_LINE_MAX];
    size_t raw_len = 0U;
    size_t indent = 0U;
    graphion_source_line *line;

    while (*cursor != '\0' && *cursor != '\n' && raw_len < (GINT_LINE_MAX - 1U)) {
      raw[raw_len++] = *cursor++;
    }
    if (*cursor == '\n') {
      ++cursor;
    }
    raw[raw_len] = '\0';
    strip_comments(raw);
    while (raw[indent] == ' ' || raw[indent] == '\t') {
      ++indent;
    }
    trim_in_place(raw);
    if (raw[0] == '\0') {
      ++line_no;
      continue;
    }
    if (program->line_count >= GINT_SOURCE_LINE_MAX) {
      set_diagnostic(diagnostic, line_no, 1U, "source contains too many statements");
      return GINT_ERR_CAPACITY;
    }
    line = &program->lines[program->line_count];
    memset(line, 0, sizeof(*line));
    line->line_no = line_no;
    line->indent = indent;
    memcpy(line->text, raw, strlen(raw) + 1U);
    program->line_count += 1U;
    ++line_no;
  }
  return GINT_OK;
}

static int parse_def_header(const char *text,
                            graphion_runtime_function *function,
                            graphion_runtime_diagnostic *diagnostic,
                            size_t line_no) {
  const char *cursor;
  const char *open_paren;
  const char *close_paren;
  size_t name_len;
  char params_buf[GINT_LINE_MAX];
  if (strncmp(text, "def ", 4U) != 0) {
    return 0;
  }
  cursor = text + 4U;
  open_paren = strchr(cursor, '(');
  close_paren = strrchr(cursor, ')');
  if (open_paren == NULL || close_paren == NULL || close_paren < open_paren ||
      close_paren[1] != ':') {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function definition");
    return GINT_ERR_PARSE;
  }
  name_len = (size_t)(open_paren - cursor);
  if (name_len == 0U || name_len >= GRAPHION_RUNTIME_NAME_MAX) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function name");
    return GINT_ERR_PARSE;
  }
  memset(function, 0, sizeof(*function));
  memcpy(function->name, cursor, name_len);
  function->name[name_len] = '\0';
  trim_in_place(function->name);
  if (!is_valid_identifier(function->name)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function name");
    return GINT_ERR_PARSE;
  }
  if (is_reserved_name(function->name)) {
    set_diagnostic(diagnostic, line_no, 1U, "reserved name cannot be used as a function");
    return GINT_ERR_RESERVED_NAME;
  }
  if ((size_t)(close_paren - open_paren - 1) >= sizeof(params_buf)) {
    set_diagnostic(diagnostic, line_no, 1U, "function parameter list too long");
    return GINT_ERR_PARSE;
  }
  memcpy(params_buf, open_paren + 1, (size_t)(close_paren - open_paren - 1));
  params_buf[close_paren - open_paren - 1] = '\0';
  trim_in_place(params_buf);
  if (params_buf[0] != '\0') {
    size_t start = 0U;
    size_t i = 0U;
    while (1) {
      const char current = params_buf[i];
      if (current == ',' || current == '\0') {
        char token[GRAPHION_RUNTIME_NAME_MAX];
        size_t len = i - start;
        if (len == 0U || len >= sizeof(token)) {
          set_diagnostic(diagnostic, line_no, 1U, "invalid function parameter");
          return GINT_ERR_PARSE;
        }
        memcpy(token, params_buf + start, len);
        token[len] = '\0';
        trim_in_place(token);
        if (!is_valid_identifier(token)) {
          set_diagnostic(diagnostic, line_no, 1U, "invalid function parameter");
          return GINT_ERR_PARSE;
        }
        if (function->param_count >= GINT_PARAM_MAX) {
          set_diagnostic(diagnostic, line_no, 1U, "too many function parameters");
          return GINT_ERR_CAPACITY;
        }
        memcpy(function->params[function->param_count], token, strlen(token) + 1U);
        function->param_count += 1U;
        if (current == '\0') {
          break;
        }
        start = i + 1U;
      }
      ++i;
    }
  }
  return GINT_OK;
}

static const graphion_runtime_function *find_function(const graphion_program *program, const char *name) {
  size_t i;
  if (program == NULL || name == NULL) {
    return NULL;
  }
  for (i = 0U; i < program->function_count; ++i) {
    if (strcmp(program->functions[i].name, name) == 0) {
      return &program->functions[i];
    }
  }
  return NULL;
}

static int index_functions(graphion_program *program, graphion_runtime_diagnostic *diagnostic) {
  size_t i = 0U;
  while (i < program->line_count) {
    const graphion_source_line *line = &program->lines[i];
    graphion_runtime_function function;
    int rc;
    if (strncmp(line->text, "def ", 4U) != 0) {
      if (strncmp(line->text, "graph ", 6U) == 0) {
        if (line->indent != 0U) {
          set_diagnostic(diagnostic, line->line_no, 1U, "nested graph declarations are not supported");
          return GINT_ERR_PARSE;
        }
        ++i;
        while (i < program->line_count && program->lines[i].indent > line->indent) {
          ++i;
        }
        continue;
      }
      if (line->indent != 0U) {
        set_diagnostic(diagnostic, line->line_no, 1U, "unexpected indentation outside function body");
        return GINT_ERR_PARSE;
      }
      ++i;
      continue;
    }
    if (line->indent != 0U) {
      set_diagnostic(diagnostic, line->line_no, 1U, "nested functions are not supported");
      return GINT_ERR_PARSE;
    }
    if (program->function_count >= GINT_FUNCTION_MAX) {
      set_diagnostic(diagnostic, line->line_no, 1U, "too many functions");
      return GINT_ERR_CAPACITY;
    }
    rc = parse_def_header(line->text, &function, diagnostic, line->line_no);
    if (rc != GINT_OK) {
      return rc;
    }
    if (find_function(program, function.name) != NULL) {
      set_diagnostic(diagnostic, line->line_no, 1U, "duplicate function definition");
      return GINT_ERR_PARSE;
    }
    function.body_start = i + 1U;
    function.body_end = function.body_start;
    while (function.body_end < program->line_count &&
           program->lines[function.body_end].indent > line->indent) {
      function.body_end += 1U;
    }
    if (function.body_end == function.body_start) {
      set_diagnostic(diagnostic, line->line_no, 1U, "function body cannot be empty");
      return GINT_ERR_PARSE;
    }
    program->functions[program->function_count] = function;
    program->function_count += 1U;
    i = function.body_end;
  }
  return GINT_OK;
}

static int split_assignment(char *line, char **lhs, char **rhs) {
  size_t i = 0U;
  int in_string = 0;
  int depth = 0;
  if (line == NULL || lhs == NULL || rhs == NULL) {
    return 0;
  }
  while (line[i] != '\0') {
    if (line[i] == '"' && (i == 0U || line[i - 1U] != '\\')) {
      in_string = !in_string;
    } else if (!in_string && line[i] == '(') {
      depth += 1;
    } else if (!in_string && line[i] == ')' && depth > 0) {
      depth -= 1;
    } else if (!in_string && depth == 0 && line[i] == '=') {
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

static int split_call(const char *expr, char *name_out, char *args_out) {
  const char *open_paren;
  const char *close_paren;
  size_t name_len;
  if (expr == NULL || name_out == NULL || args_out == NULL) {
    return 0;
  }
  open_paren = strchr(expr, '(');
  close_paren = strrchr(expr, ')');
  if (open_paren == NULL || close_paren == NULL || close_paren < open_paren || close_paren[1] != '\0') {
    return 0;
  }
  name_len = (size_t)(open_paren - expr);
  if (name_len == 0U || name_len >= GRAPHION_RUNTIME_NAME_MAX) {
    return 0;
  }
  memcpy(name_out, expr, name_len);
  name_out[name_len] = '\0';
  trim_in_place(name_out);
  if (!is_valid_identifier(name_out)) {
    return 0;
  }
  if ((size_t)(close_paren - open_paren - 1) >= GINT_LINE_MAX) {
    return 0;
  }
  memcpy(args_out, open_paren + 1, (size_t)(close_paren - open_paren - 1));
  args_out[close_paren - open_paren - 1] = '\0';
  trim_in_place(args_out);
  return 1;
}

static int split_arguments(const char *args_buf, char args[GINT_ARG_MAX][GINT_LINE_MAX], size_t *arg_count) {
  size_t i = 0U;
  size_t start = 0U;
  int in_string = 0;
  int depth = 0;
  if (arg_count == NULL) {
    return 0;
  }
  *arg_count = 0U;
  if (args_buf == NULL || args_buf[0] == '\0') {
    return 1;
  }
  while (1) {
    const char current = args_buf[i];
    if (current == '"' && (i == 0U || args_buf[i - 1U] != '\\')) {
      in_string = !in_string;
    } else if (!in_string && current == '(') {
      depth += 1;
    } else if (!in_string && current == ')' && depth > 0) {
      depth -= 1;
    }
    if (!in_string && depth == 0 && (current == ',' || current == '\0')) {
      size_t len;
      if (*arg_count >= GINT_ARG_MAX) {
        return 0;
      }
      len = i - start;
      if (len >= GINT_LINE_MAX) {
        return 0;
      }
      memcpy(args[*arg_count], args_buf + start, len);
      args[*arg_count][len] = '\0';
      trim_in_place(args[*arg_count]);
      if (args[*arg_count][0] == '\0') {
        return 0;
      }
      *arg_count += 1U;
      if (current == '\0') {
        break;
      }
      start = i + 1U;
    }
    if (current == '\0') {
      break;
    }
    ++i;
  }
  return 1;
}

static int print_value(FILE *output, const graphion_runtime_value *value) {
  if (output == NULL || value == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  switch (value->kind) {
    case GRAPHION_VALUE_INT:
      fprintf(output, "%lld\n", (long long)value->int_value);
      break;
    case GRAPHION_VALUE_FLOAT:
      fprintf(output, "%g\n", value->float_value);
      break;
    case GRAPHION_VALUE_BOOL:
      fprintf(output, "%s\n", value->bool_value != 0 ? "true" : "false");
      break;
    case GRAPHION_VALUE_STRING:
      fprintf(output, "%s\n", value->string_value);
      break;
    case GRAPHION_VALUE_GRAPH:
      fprintf(output,
              "<graph name=%s nodes=%zu edges=%zu>\n",
              value->graph_name,
              value->graph_node_count,
              value->graph_edge_count);
      break;
    default:
      fprintf(output, "none\n");
      break;
  }
  return GINT_OK;
}

static int eval_expression(const char *expr,
                           const graphion_program *program,
                           graphion_runtime_scope *global_scope,
                           graphion_runtime_scope *local_scope,
                           graphion_runtime_diagnostic *diagnostic,
                           FILE *output,
                           size_t line_no,
                           graphion_runtime_value *value);

static int execute_block(const graphion_program *program,
                         size_t start,
                         size_t end,
                         graphion_runtime_scope *global_scope,
                         graphion_runtime_scope *local_scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output,
                         int allow_return,
                         int *did_return,
                         graphion_runtime_value *return_value);

static int call_function(const graphion_program *program,
                         const graphion_runtime_function *function,
                         graphion_runtime_scope *global_scope,
                         graphion_runtime_scope *caller_scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output,
                         size_t line_no,
                         const char *args_expr,
                         graphion_runtime_value *value) {
  char args_buf[GINT_LINE_MAX];
  char args[GINT_ARG_MAX][GINT_LINE_MAX];
  graphion_runtime_scope local_scope;
  graphion_runtime_value arg_values[GINT_ARG_MAX];
  graphion_runtime_value return_value;
  size_t arg_count = 0U;
  size_t i;
  int did_return = 0;
  int rc;

  if (args_expr != NULL) {
    memcpy(args_buf, args_expr, strlen(args_expr) + 1U);
  } else {
    args_buf[0] = '\0';
  }
  if (!split_arguments(args_buf, args, &arg_count)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function call arguments");
    return GINT_ERR_PARSE;
  }
  if (arg_count != function->param_count) {
    set_diagnostic(diagnostic, line_no, 1U, "function call argument count mismatch");
    return GINT_ERR_CALL;
  }
  graphion_runtime_scope_init(&local_scope);
  for (i = 0U; i < arg_count; ++i) {
    rc = eval_expression(args[i], program, global_scope, caller_scope, diagnostic, output, line_no, &arg_values[i]);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = assign_value(&local_scope, function->params[i], &arg_values[i]);
    if (rc != GINT_OK) {
      set_diagnostic(diagnostic, line_no, 1U, "runtime scope capacity exceeded");
      return rc;
    }
  }
  memset(&return_value, 0, sizeof(return_value));
  rc = execute_block(program,
                     function->body_start,
                     function->body_end,
                     global_scope,
                     &local_scope,
                     diagnostic,
                     output,
                     1,
                     &did_return,
                     &return_value);
  if (rc != GINT_OK) {
    return rc;
  }
  if (did_return) {
    *value = return_value;
  } else {
    memset(value, 0, sizeof(*value));
    value->kind = GRAPHION_VALUE_NONE;
  }
  return GINT_OK;
}

static int eval_expression(const char *expr,
                           const graphion_program *program,
                           graphion_runtime_scope *global_scope,
                           graphion_runtime_scope *local_scope,
                           graphion_runtime_diagnostic *diagnostic,
                           FILE *output,
                           size_t line_no,
                           graphion_runtime_value *value) {
  const graphion_runtime_value *existing;

  if (parse_string_literal(expr, value) || parse_bool_literal(expr, value) ||
      parse_float_literal(expr, value) || parse_int_literal(expr, value)) {
    return GINT_OK;
  }
  {
    char name[GRAPHION_RUNTIME_NAME_MAX];
    char args[GINT_LINE_MAX];
    if (split_call(expr, name, args)) {
      const graphion_runtime_function *function;
      if (strcmp(name, "print") == 0) {
        set_diagnostic(diagnostic, line_no, 1U, "print cannot be used as an expression");
        return GINT_ERR_CALL;
      }
      function = find_function(program, name);
      if (function == NULL) {
        set_diagnostic(diagnostic, line_no, 1U, "unknown function call");
        return GINT_ERR_CALL;
      }
      return call_function(program, function, global_scope, local_scope, diagnostic, output, line_no, args, value);
    }
  }
  if (!is_valid_identifier(expr)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid expression");
    return GINT_ERR_PARSE;
  }
  existing = find_value(local_scope, global_scope, expr);
  if (existing == NULL) {
    set_diagnostic(diagnostic, line_no, 1U, "unknown variable in expression");
    return GINT_ERR_UNKNOWN_VARIABLE;
  }
  *value = *existing;
  return GINT_OK;
}

static int execute_statement(const graphion_program *program,
                             const graphion_source_line *line,
                             graphion_runtime_scope *global_scope,
                             graphion_runtime_scope *local_scope,
                             graphion_runtime_diagnostic *diagnostic,
                             FILE *output,
                             int allow_return,
                             int *did_return,
                             graphion_runtime_value *return_value) {
  char buffer[GINT_LINE_MAX];
  char *lhs;
  char *rhs;
  graphion_runtime_value value;
  int rc;

  memcpy(buffer, line->text, strlen(line->text) + 1U);
  if (strncmp(buffer, "print(", 6U) == 0 && buffer[strlen(buffer) - 1U] == ')') {
    buffer[strlen(buffer) - 1U] = '\0';
    trim_in_place(buffer + 6U);
    rc = eval_expression(buffer + 6U, program, global_scope, local_scope, diagnostic, output, line->line_no, &value);
    if (rc != GINT_OK) {
      return rc;
    }
    return print_value(output, &value);
  }
  if (strncmp(buffer, "return", 6U) == 0 &&
      (buffer[6] == '\0' || isspace((unsigned char)buffer[6]) != 0)) {
    if (!allow_return) {
      set_diagnostic(diagnostic, line->line_no, 1U, "return is only valid inside a function");
      return GINT_ERR_RETURN;
    }
    if (buffer[6] == '\0') {
      memset(return_value, 0, sizeof(*return_value));
      return_value->kind = GRAPHION_VALUE_NONE;
    } else {
      char *expr = buffer + 6U;
      trim_in_place(expr);
      rc = eval_expression(expr, program, global_scope, local_scope, diagnostic, output, line->line_no, return_value);
      if (rc != GINT_OK) {
        return rc;
      }
    }
    *did_return = 1;
    return GINT_OK;
  }
  if (strncmp(buffer, "def ", 4U) == 0) {
    set_diagnostic(diagnostic, line->line_no, 1U, "nested functions are not supported");
    return GINT_ERR_PARSE;
  }
  if (!split_assignment(buffer, &lhs, &rhs)) {
    set_diagnostic(diagnostic, line->line_no, 1U, "expected assignment, print, or return statement");
    return GINT_ERR_PARSE;
  }
  if (!is_valid_identifier(lhs)) {
    set_diagnostic(diagnostic, line->line_no, 1U, "invalid variable name");
    return GINT_ERR_PARSE;
  }
  if (is_reserved_name(lhs)) {
    set_diagnostic(diagnostic, line->line_no, 1U, "reserved name cannot be assigned");
    return GINT_ERR_RESERVED_NAME;
  }
  memset(&value, 0, sizeof(value));
  rc = eval_expression(rhs, program, global_scope, local_scope, diagnostic, output, line->line_no, &value);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = assign_value(local_scope != NULL ? local_scope : global_scope, lhs, &value);
  if (rc != GINT_OK) {
    set_diagnostic(diagnostic, line->line_no, 1U, "runtime scope capacity exceeded");
    return rc;
  }
  return GINT_OK;
}

static int execute_block(const graphion_program *program,
                         size_t start,
                         size_t end,
                         graphion_runtime_scope *global_scope,
                         graphion_runtime_scope *local_scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output,
                         int allow_return,
                         int *did_return,
                         graphion_runtime_value *return_value) {
  size_t i = start;
  if (did_return != NULL) {
    *did_return = 0;
  }
  while (i < end) {
    const graphion_source_line *line = &program->lines[i];
    int rc;
    if (line->indent == 0U && strncmp(line->text, "graph ", 6U) == 0) {
      graphion_runtime_value graph_value;
      char graph_name[GRAPHION_RUNTIME_NAME_MAX];
      size_t body_end = i + 1U;
      rc = parse_graph_header(line->text, graph_name, diagnostic, line->line_no);
      if (rc != GINT_OK) {
        return rc;
      }
      memset(&graph_value, 0, sizeof(graph_value));
      graph_value.kind = GRAPHION_VALUE_GRAPH;
      memcpy(graph_value.graph_name, graph_name, strlen(graph_name) + 1U);
      while (body_end < end && program->lines[body_end].indent > line->indent) {
        graphion_runtime_graph_edge edge;
        if (graph_value.graph_edge_count >= GRAPHION_RUNTIME_GRAPH_EDGE_MAX) {
          set_diagnostic(diagnostic, program->lines[body_end].line_no, 1U, "graph edge capacity exceeded");
          return GINT_ERR_CAPACITY;
        }
        rc = parse_graph_edge(program->lines[body_end].text, &edge, diagnostic, program->lines[body_end].line_no);
        if (rc != GINT_OK) {
          return rc;
        }
        if (!graph_contains_node(&graph_value, edge.source)) {
          graph_value.graph_node_count += 1U;
        }
        if (!graph_contains_node(&graph_value, edge.target)) {
          graph_value.graph_node_count += 1U;
        }
        graph_value.graph_edges[graph_value.graph_edge_count] = edge;
        graph_value.graph_edge_count += 1U;
        body_end += 1U;
      }
      if (body_end == i + 1U) {
        set_diagnostic(diagnostic, line->line_no, 1U, "graph body cannot be empty");
        return GINT_ERR_PARSE;
      }
      rc = assign_value(global_scope, graph_name, &graph_value);
      if (rc != GINT_OK) {
        set_diagnostic(diagnostic, line->line_no, 1U, "runtime scope capacity exceeded");
        return rc;
      }
      i = body_end;
      continue;
    }
    if (line->indent == 0U && strncmp(line->text, "def ", 4U) == 0) {
      const graphion_runtime_function *function = find_function(program, line->text + 4U);
      size_t skip_to = i + 1U;
      size_t j;
      (void)function;
      for (j = 0U; j < program->function_count; ++j) {
        if (program->functions[j].body_start == i + 1U) {
          skip_to = program->functions[j].body_end;
          break;
        }
      }
      i = skip_to;
      continue;
    }
    rc = execute_statement(program, line, global_scope, local_scope, diagnostic, output, allow_return, did_return, return_value);
    if (rc != GINT_OK) {
      return rc;
    }
    if (did_return != NULL && *did_return) {
      return GINT_OK;
    }
    i += 1U;
  }
  return GINT_OK;
}

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output) {
  graphion_program program;
  graphion_runtime_value return_value;
  int did_return = 0;
  int rc;

  clear_diagnostic(diagnostic);
  if (source == NULL || scope == NULL || output == NULL) {
    return GINT_ERR_INVALID_ARG;
  }

  rc = parse_source_lines(source, &program, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = index_functions(&program, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  memset(&return_value, 0, sizeof(return_value));
  return execute_block(&program,
                       0U,
                       program.line_count,
                       scope,
                       scope,
                       diagnostic,
                       output,
                       0,
                       &did_return,
                       &return_value);
}

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic) {
  return graphion_interpret_source_with_output(source, scope, diagnostic, stdout);
}
