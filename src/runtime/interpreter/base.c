/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/base.h"

void clear_diagnostic(graphion_runtime_diagnostic *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = 0U;
  diagnostic->column = 0U;
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
    diagnostic->message = message;
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

int is_reserved_name(const char *name) {
  return strcmp(name, "print") == 0 || strcmp(name, "true") == 0 || strcmp(name, "false") == 0 ||
         strcmp(name, "pi") == 0 || strcmp(name, "e") == 0 || strcmp(name, "nan") == 0 ||
         strcmp(name, "inf") == 0 ||
         strcmp(name, "abs") == 0 || strcmp(name, "min") == 0 || strcmp(name, "max") == 0 ||
         strcmp(name, "clamp") == 0 || strcmp(name, "sqrt") == 0 || strcmp(name, "exp") == 0 ||
         strcmp(name, "ln") == 0 || strcmp(name, "log") == 0 || strcmp(name, "log10") == 0 ||
         strcmp(name, "log2") == 0 || strcmp(name, "floor") == 0 || strcmp(name, "ceil") == 0 ||
         strcmp(name, "round") == 0 || strcmp(name, "trunc") == 0 || strcmp(name, "fract") == 0 ||
         strcmp(name, "sign") == 0 ||
         strcmp(name, "cbrt") == 0 || strcmp(name, "sin") == 0 || strcmp(name, "sinh") == 0 ||
         strcmp(name, "asinh") == 0 || strcmp(name, "acosh") == 0 ||
         strcmp(name, "cos") == 0 || strcmp(name, "cosh") == 0 || strcmp(name, "tan") == 0 ||
         strcmp(name, "tanh") == 0 || strcmp(name, "atanh") == 0 ||
         strcmp(name, "asin") == 0 || strcmp(name, "acos") == 0 ||
         strcmp(name, "atan") == 0 || strcmp(name, "atan2") == 0 || strcmp(name, "hypot") == 0 ||
         strcmp(name, "degrees") == 0 || strcmp(name, "radians") == 0 || strcmp(name, "isnan") == 0 ||
         strcmp(name, "isinf") == 0 || strcmp(name, "isfinite") == 0 || strcmp(name, "expm1") == 0 ||
         strcmp(name, "log1p") == 0 ||
         strcmp(name, "len") == 0 ||
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
