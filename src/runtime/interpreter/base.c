/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/base.h"

static int is_scalar_builtin_name(const char *name) {
  static const char *const names[] = {
      "abs",       "min",      "max",       "clamp",    "sqrt",     "cbrt",   "sin",   "csc",
      "sec",       "cot",      "acsc",      "asec",     "acot",     "sech",   "csch",  "coth",
      "sinh",      "asinh",    "acosh",     "cosh",     "tanh",     "atanh",  "cos",   "tan",
      "asin",      "acos",     "atan",      "atan2",    "hypot",    "copysign",
      "fma",       "fdim",     "remainder", "rint",     "degrees",  "radians",
      "isnan",     "isinf",    "isfinite",  "expm1",    "exp2",     "log1p",
      "erf",       "erfc",     "gamma",     "lgamma",   "fract",    "exp",
      "ln",        "log",      "log10",     "log2",     "floor",    "ceil",
      "round",      "trunc",      "sign",                  "len",
      "contains",   "node_count", "edge_count",            "is_directed",
      "is_weighted", "orientation", "node_attrs",           "edge_attrs",
      "edge_weight", "vertex_count", "hyperedge_count",     "vertex_attr_count",
      "hyperedge_attr_count",        "vertex_ids",          "vertices",
      "hyperedges", "vertex_attrs", "hyperedge_vertices",  "hyperedge_attrs",
      "has_vertex", "has_hyperedge", "incident_hyperedges", "has_node",
      "has_edge",   "neighbors",   "indegree",             "outdegree",
      "node_ids",   "nodes",       "edges",                "add_node",
      "add_edge",   "set_node_attrs", "set_edge_attrs",    "set_edge_weight",
      "remove_node", "remove_edge", "add_vertex",          "add_hyperedge",
      "set_vertex_attrs",          "set_hyperedge_attrs",  "remove_vertex",
      "remove_hyperedge"
  };
  size_t i;

  for (i = 0U; i < sizeof(names) / sizeof(names[0]); ++i) {
    if (strcmp(name, names[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

void clear_diagnostic(graphion_runtime_diagnostic *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = 0U;
  diagnostic->column = 0U;
  diagnostic->message_storage[0] = '\0';
  diagnostic->message = NULL;
}

void graphion_runtime_warning_report_init(graphion_runtime_warning_report *report) {
  if (report == NULL) {
    return;
  }
  memset(report, 0, sizeof(*report));
  report->enabled = 1;
}

void graphion_runtime_warning_report_clear(graphion_runtime_warning_report *report) {
  graphion_runtime_warning_report_init(report);
}

int fail(graphion_runtime_diagnostic *diagnostic,
                unsigned int line,
                unsigned int column,
                const char *message,
                int code) {
  if (diagnostic != NULL) {
    diagnostic->line = line;
    diagnostic->column = column;
    if (message == NULL) {
      diagnostic->message_storage[0] = '\0';
      diagnostic->message = NULL;
    } else {
      size_t len;
      len = strlen(message);
      if (len >= sizeof(diagnostic->message_storage)) {
        len = sizeof(diagnostic->message_storage) - 1U;
      }
      memcpy(diagnostic->message_storage, message, len);
      diagnostic->message_storage[len] = '\0';
      diagnostic->message = diagnostic->message_storage;
    }
  }
  return code;
}

int add_warning(graphion_runtime_warning_report *report,
                       unsigned int line,
                       unsigned int column,
                       const char *message,
                       graphion_runtime_diagnostic *diagnostic) {
  graphion_runtime_warning *warning;
  size_t len;

  if (report == NULL || message == NULL) {
    return fail(diagnostic, line, column, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (!report->enabled) {
    return GINT_OK;
  }
  if (report->count >= GRAPHION_RUNTIME_WARNING_MAX) {
    return fail(diagnostic, line, column, "warning capacity exceeded", GINT_ERR_CAPACITY);
  }
  warning = &report->items[report->count++];
  warning->line = line;
  warning->column = column;
  len = strlen(message);
  if (len >= sizeof(warning->message)) {
    len = sizeof(warning->message) - 1U;
  }
  memcpy(warning->message, message, len);
  warning->message[len] = '\0';
  return GINT_OK;
}

void vm_value_set_none(graphion_vm_value *value) {
  if (value == NULL) {
    return;
  }
  memset(value, 0, sizeof(*value));
  value->kind = GVM_VALUE_NONE;
}

void runtime_free_string(char **text) {
  if (text == NULL || *text == NULL) {
    return;
  }
  free(*text);
  *text = NULL;
}


void skip_spaces(const char **cursor) {
  while (cursor != NULL && *cursor != NULL && (**cursor == ' ' || **cursor == '\t' || **cursor == '\r')) {
    (*cursor)++;
  }
}

int is_ident_start_char(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_';
}

int is_ident_char(char ch) {
  return is_ident_start_char(ch) || (ch >= '0' && ch <= '9');
}

void point_unknown_operand_diagnostic(graphion_runtime_diagnostic *diagnostic,
                                      const char *source_text,
                                      unsigned int base_column) {
  const char *prefix = "unknown operand '";
  const char *name_start;
  const char *name_end;
  const char *scan;
  size_t prefix_len = strlen(prefix);
  size_t name_len;

  if (diagnostic == NULL || diagnostic->message == NULL || source_text == NULL ||
      strncmp(diagnostic->message, prefix, prefix_len) != 0) {
    return;
  }
  name_start = diagnostic->message + prefix_len;
  name_end = strchr(name_start, '\'');
  if (name_end == NULL || name_end == name_start) {
    return;
  }
  name_len = (size_t)(name_end - name_start);
  for (scan = source_text; *scan != '\0'; ++scan) {
    if ((scan == source_text || !is_ident_char(scan[-1])) &&
        strncmp(scan, name_start, name_len) == 0 &&
        !is_ident_char(scan[name_len])) {
      diagnostic->column = base_column + (unsigned int)(scan - source_text);
      return;
    }
  }
}

void point_builtin_argument_diagnostic_at_cursor(graphion_runtime_diagnostic *diagnostic,
                                                 const char *source_text,
                                                 const char *cursor,
                                                 unsigned int base_column) {
  if (diagnostic == NULL || diagnostic->message == NULL || source_text == NULL ||
      cursor == NULL || cursor < source_text ||
      strncmp(diagnostic->message, "expected ", 9U) != 0 ||
      strstr(diagnostic->message, " argument") == NULL) {
    return;
  }
  diagnostic->column = base_column + (unsigned int)(cursor - source_text);
}

static int message_is_delimiter_diagnostic(const char *message) {
  return strncmp(message, "expected ')'", 12U) == 0 ||
         strncmp(message, "expected ']'", 12U) == 0 ||
         strncmp(message, "expected ','", 12U) == 0 ||
         strncmp(message, "expected ':'", 12U) == 0 ||
         strncmp(message, "expected '('", 12U) == 0 ||
         strncmp(message, "expected '['", 12U) == 0 ||
         strncmp(message, "expected '{'", 12U) == 0 ||
         strcmp(message, "empty tuple literal is not supported") == 0 ||
         strncmp(message, "trailing comma is not allowed", 29U) == 0;
}

void point_delimiter_diagnostic_at_cursor(graphion_runtime_diagnostic *diagnostic,
                                          const char *source_text,
                                          const char *cursor,
                                          unsigned int base_column) {
  if (diagnostic == NULL || diagnostic->message == NULL || source_text == NULL ||
      cursor == NULL || cursor < source_text ||
      !message_is_delimiter_diagnostic(diagnostic->message)) {
    return;
  }
  diagnostic->column = base_column + (unsigned int)(cursor - source_text);
}

int is_reserved_name(const char *name) {
  if (is_scalar_builtin_name(name)) {
    return 1;
  }
  return strcmp(name, "print") == 0 || strcmp(name, "true") == 0 || strcmp(name, "false") == 0 ||
         strcmp(name, "pi") == 0 || strcmp(name, "tau") == 0 || strcmp(name, "phi") == 0 ||
         strcmp(name, "e") == 0 ||
         strcmp(name, "nan") == 0 || strcmp(name, "inf") == 0 ||
         strcmp(name, "set") == 0 ||
         strcmp(name, "graph") == 0 ||
         strcmp(name, "hypergraph") == 0 ||
         strcmp(name, "struct") == 0 ||
         strcmp(name, "if") == 0 ||
         strcmp(name, "elif") == 0 ||
         strcmp(name, "else") == 0 || strcmp(name, "match") == 0 || strcmp(name, "default") == 0 ||
         strcmp(name, "and") == 0 || strcmp(name, "or") == 0 || strcmp(name, "nand") == 0 ||
         strcmp(name, "nor") == 0 || strcmp(name, "not") == 0;
}

void copy_name(char dst[GRAPHION_RUNTIME_NAME_MAX], const char *src) {
  size_t len = strlen(src);
  if (len >= GRAPHION_RUNTIME_NAME_MAX) {
    len = GRAPHION_RUNTIME_NAME_MAX - 1U;
  }
  memcpy(dst, src, len);
  dst[len] = '\0';
}


void graphion_emit_warning_report(const graphion_runtime_warning_report *report, FILE *stream) {
  size_t i;

  if (report == NULL || stream == NULL || !report->enabled) {
    return;
  }
  for (i = 0U; i < report->count; ++i) {
    fprintf(stream,
            "warning:%u:%u: %s\n",
            report->items[i].line,
            report->items[i].column,
            report->items[i].message);
  }
}
