/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_print_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"print = 42\n", GINT_ERR_PARSE, 7U, "expected '(' after print"},
      {"print\n", GINT_ERR_PARSE, 6U, "expected '(' after print"},
      {"print(\n", GINT_ERR_PARSE, 7U, "expected print argument"},
      {"print()\n", GINT_ERR_PARSE, 7U, "expected print argument"},
      {"print(count\n", GINT_ERR_PARSE, 12U, "expected ')' after print argument"},
      {"print(count) extra\n", GINT_ERR_PARSE, 14U, "unexpected trailing tokens after print"},
      {"print(count)\n", GINT_ERR_UNKNOWN_OPERAND, 7U, "unknown operand 'count'"},
      {"print(\"x\" + missing\n", GINT_ERR_PARSE, 20U, "expected ')' after print argument"},
      {"print(\"x\" + missing) extra\n", GINT_ERR_PARSE, 22U, "unexpected trailing tokens after print"},
      {"print(\"x\" + missing)\n", GINT_ERR_UNKNOWN_OPERAND, 13U, "unknown operand 'missing'"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != 1U || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("count = 42\nprint(count\n", &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, 100);
    }
    if (diagnostic.line != 2U) {
      return finish_scope_test(&scope, 101);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "expected ')' after print argument") != 0) {
      return finish_scope_test(&scope, 102);
    }
    graphion_runtime_scope_dispose(&scope);
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("count = 42\nprint(count) extra\n", &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, 110);
    }
    if (diagnostic.line != 2U) {
      return finish_scope_test(&scope, 111);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "unexpected trailing tokens after print") != 0) {
      return finish_scope_test(&scope, 112);
    }
    graphion_runtime_scope_dispose(&scope);
  }

  return 0;
}

int test_gion_builtin_call_column_diagnostics(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"value = abs()\n", 1U, 13U, "expected abs argument"},
      {"value = abs 1)\n", 1U, 13U, "expected '(' after abs"},
      {"value = abs(1 + 2\n", 1U, 18U, "expected ')' after abs argument"},
      {"value = min(1 2)\n", 1U, 15U, "expected ',' between min arguments"},
      {"value = min(1,)\n", 1U, 15U, "expected min second argument"},
      {"value = sqrt 1)\n", 1U, 14U, "expected '(' after sqrt"},
      {"value = clamp()\n", 1U, 15U, "expected clamp value"},
      {"value = clamp(1,)\n", 1U, 17U, "expected clamp lower bound"},
      {"value = clamp(1, 2,)\n", 1U, 20U, "expected clamp upper bound"},
      {"value = fma(1, 2)\n", 1U, 17U, "expected ',' after fma second argument"},
      {"value = fma 1, 2, 3)\n", 1U, 13U, "expected '(' after fma"},
      {"graph G:\n    1\nadd_node G, 2)\n", 3U, 10U, "expected '(' after add_node"},
      {"graph G:\n    1\nadd_node(G)\n", 3U, 11U, "expected ',' after add_node graph"},
      {"graph G:\n    1\n    2\nadd_edge(G, 1)\n", 4U, 14U, "expected ',' between add_edge endpoints"},
      {"graph G:\n    1\nadd_node(G, 2) extra\n", 3U, 16U, "unexpected trailing tokens after add_node"},
      {"hypergraph H:\n    1\nadd_vertex H, 2)\n", 3U, 12U, "expected '(' after add_vertex"},
      {"hypergraph H:\n    1\nadd_vertex(H)\n", 3U, 13U, "expected ',' after add_vertex hypergraph"},
      {"hypergraph H:\n    1\nadd_vertex(H, 2) extra\n", 3U, 18U, "unexpected trailing tokens after add_vertex"},
      {"graph G:\n    1\nset_node_attrs(G)\n", 3U, 17U, "expected ',' between set_node_attrs arguments"},
      {"hypergraph H:\n    1\nset_vertex_attrs(H, 1)\n",
       3U,
       22U,
       "expected ',' between set_vertex_attrs arguments"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_unterminated_string_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"name = \"graphion\n", "unterminated string literal"},
      {"print(\"x\n", "unterminated string literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != 1U) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_invalid_identifier_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"1count = 42\n", "expected identifier"},
      {"-name = 42\n", "expected identifier"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != 1U) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_trailing_token_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"count = 42 extra\n", 12U, "unexpected trailing tokens after assignment"},
      {"name = \"x\" extra\n", 12U, "unexpected trailing tokens after assignment"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != 1U || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_reference_before_definition_errors(void) {
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source("copy = count\ncount = 42\n", &scope, &diagnostic);
  if (rc != GINT_ERR_UNKNOWN_OPERAND) {
    return finish_scope_test(&scope, 1);
  }
  if (diagnostic.line != 1U || diagnostic.column != 8U) {
    return finish_scope_test(&scope, 2);
  }
  if (diagnostic.message == NULL || strcmp(diagnostic.message, "unknown operand 'count'") != 0) {
    return finish_scope_test(&scope, 3);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_unknown_identifier_column_diagnostics(void) {
  static const struct {
    const char *source;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"copy = missing\n", 8U, "unknown operand 'missing'"},
      {"print(missing)\n", 7U, "unknown operand 'missing'"},
      {"if missing:\n    print(1)\n", 4U, "unknown operand 'missing'"},
      {"match missing:\n    1:\n        print(1)\n", 7U, "unknown operand 'missing'"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_UNKNOWN_OPERAND) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != 1U || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_delimiter_column_diagnostics(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"print(1 + 2\n", 1U, 12U, "expected ')' after print argument"},
      {"value = (1 + 2\n", 1U, 15U, "expected ')' after expression"},
      {"items = [1]\nvalue = items[0\n", 2U, 16U, "expected ']' after index expression"},
      {"items = [1 2]\n", 1U, 12U, "expected ',' or ']' after list element"},
      {"data = {\"a\" 1}\n", 1U, 13U, "expected ':' after dict key"},
      {"data = {\"a\": 1 \"b\": 2}\n", 1U, 16U, "expected ',' or '}' after dict entry"},
      {"value = (1, 2 3)\n", 1U, 15U, "expected ',' or ')' after tuple element"},
      {"items = set(1 2)\n", 1U, 15U, "expected ',' or ')' after set element"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_literal_parser_column_diagnostics(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"value = 0b\n", 1U, 11U, "expected binary digits after 0b"},
      {"value = 0b102\n", 1U, 13U, "invalid bits literal"},
      {"data = {a: 1}\n", 1U, 9U, "dict literal keys must be string literals"},
      {"items = [1,]\n", 1U, 12U, "trailing comma is not allowed in list literal"},
      {"value = ()\n", 1U, 9U, "empty tuple literal is not supported"},
      {"pair = (1,)\n", 1U, 11U, "trailing comma is not allowed in tuple literal"},
      {"items = set(1,)\n", 1U, 15U, "trailing comma is not allowed in set literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_reassignment_and_type_change(void) {
  const char *source =
      "value = 1\n"
      "value = 2\n"
      "value = \"ok\"\n"
      "flag = true\n"
      "flag = false\n"
      "print(value)\n"
      "print(flag)\n";
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *value;
  const graphion_runtime_value *flag;

  graphion_runtime_scope_init(&scope);
  if (!test_capture_gion_output(source, "gion_reassignment_and_type_change.txt", &scope, &diagnostic, path,
                                sizeof(path), output, sizeof(output))) {
    return finish_scope_test(&scope, 1);
  }
  value = graphion_runtime_scope_find(&scope, "value");
  flag = graphion_runtime_scope_find(&scope, "flag");
  if (value == NULL || value->kind != GVM_VALUE_STRING || strcmp(value->as.string_value, "ok") != 0) {
    test_cleanup_temp_path(path);
    return finish_scope_test(&scope, 3);
  }
  if (flag == NULL || flag->kind != GVM_VALUE_BOOL || flag->as.bool_value != 0) {
    test_cleanup_temp_path(path);
    return finish_scope_test(&scope, 4);
  }
  test_cleanup_temp_path(path);
  if (strcmp(output, "ok\nfalse\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_copy_chains_and_blank_lines(void) {
  const char *source =
      "\n"
      "a = 1\n"
      "\n"
      "b = a\n"
      "c = b\n"
      "\n"
      "print(a)\n"
      "print(b)\n"
      "print(c)\n";
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *c;

  graphion_runtime_scope_init(&scope);
  if (!test_capture_gion_output(source, "gion_copy_chains_and_blank_lines.txt", &scope, &diagnostic, path,
                                sizeof(path), output, sizeof(output))) {
    return finish_scope_test(&scope, 1);
  }
  c = graphion_runtime_scope_find(&scope, "c");
  if (c == NULL || c->kind != GVM_VALUE_INT || c->as.int_value != 1) {
    test_cleanup_temp_path(path);
    return finish_scope_test(&scope, 3);
  }
  test_cleanup_temp_path(path);
  if (strcmp(output, "1\n1\n1\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_late_line_error_diagnostics(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"count = 42\n"
       "ratio = 3.5\n"
       "print(count)\n"
       "print(ratio)\n"
       "name =\n",
       GINT_ERR_PARSE,
       5U,
       6U,
       "expected expression after '='"},
      {"first = 1\n"
       "second = first\n"
       "third = missing\n",
       GINT_ERR_UNKNOWN_OPERAND,
       3U,
       9U,
       "unknown operand 'missing'"},
      {"ready = true\n"
       "value = \"ready\" if\n"
       "    ready else \"bad\"\n",
       GINT_ERR_PARSE,
       2U,
       19U,
       "multiline assignment expression requires grouping parentheses"},
      {"value = 1\n"
       "match value:\n"
       "    1:\n"
       "    2:\n"
       "print(value)\n",
       GINT_ERR_PARSE,
       3U,
       3U,
       "expected indented block after match case"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_runtime_diagnostic_exact_pairs(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"count =\n", GINT_ERR_PARSE, 1U, 7U, "expected expression after '='"},
      {"name = \"graphion\n", GINT_ERR_PARSE, 1U, 8U, "unterminated string literal"},
      {"first = 1\nsecond = missing\n", GINT_ERR_UNKNOWN_OPERAND, 2U, 10U, "unknown operand 'missing'"},
      {"/* open\nnext = 1\n", GINT_ERR_PARSE, 1U, 1U, "unterminated block comment"},
      {"  count = 42\n", GINT_ERR_PARSE, 1U, 1U, "unexpected indentation"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_representative_runtime_error_diagnostics(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"value = 1 / 0\n", 1U, 1U, "division by zero"},
      {"items = [1, 2]\nprint(items[2])\n", 1U, 1U, "list index out of range"},
      {"data = {\"a\": 1}\nprint(data[\"missing\"])\n", 1U, 1U, "dict key not found"},
      {"items = set(1, 2)\nprint(items[0])\n", 1U, 1U, "incompatible operand types"},
      {"pair = (1, 2)\nprint(pair[2])\n", 1U, 1U, "list index out of range"},
      {"struct Player:\n"
       "    id: int\n"
       "\n"
       "p = Player {}\n",
       4U,
       12U,
       "missing or unknown struct field"},
      {"graph G:\n"
       "    1\n"
       "remove_node(G, 2)\n",
       1U,
       1U,
       "invalid node id"},
      {"hypergraph H:\n"
       "    []\n",
       2U,
       1U,
       "hyperedge must contain at least one vertex"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_RUN) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_unexpected_indentation_errors(void) {
  static const struct {
    const char *source;
  } invalid_cases[] = {
      {"  count = 42\n"},
      {"\tprint(1)\n"},
  };
  size_t i;

  for (i = 0U; i < sizeof(invalid_cases) / sizeof(invalid_cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(invalid_cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != 1U || diagnostic.column != 1U) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "unexpected indentation") != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    const graphion_runtime_value *count;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("\t  \ncount = 42\nprint(count)\n", &scope, &diagnostic);
    if (rc != GINT_OK) {
      return finish_scope_test(&scope, 100);
    }
    count = graphion_runtime_scope_find(&scope, "count");
    if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
      return finish_scope_test(&scope, 101);
    }
    graphion_runtime_scope_dispose(&scope);
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    const graphion_runtime_value *count;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("flag = true\nif flag:\n    count = 42\nprint(count)\n", &scope, &diagnostic);
    if (rc != GINT_OK) {
      return finish_scope_test(&scope, 110);
    }
    count = graphion_runtime_scope_find(&scope, "count");
    if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
      return finish_scope_test(&scope, 111);
    }
    graphion_runtime_scope_dispose(&scope);
  }

  return 0;
}

int test_gion_comments(void) {
  const char *prepare_source_text =
      "# prepare_source should ignore comments too\n"
      "count = 40 # inline line comment\n"
      "/* block comment before an assignment */\n"
      "count += 2\n"
      "message = \"/* not a comment */\"\n"
      "ratio = /* inline block */ 7 / 2\n"
      "print(count)\n"
      "print(message)\n"
      "print(ratio)\n";
  const char *source =
      "# line comment before code\n"
      "count = 40 # inline line comment\n"
      "/* block comment before an assignment */\n"
      "count += 2\n"
      "message = \"/* not a comment */\"\n"
      "/*\n"
      "multi-line block comment\n"
      "that spans several lines\n"
      "*/\n"
      "if true: # comment after header\n"
      "    # comment inside block\n"
      "    label = \"ok\" /* inline block comment in block */\n"
      "else:\n"
      "    label = \"bad\"\n"
      "ratio = /* inline block */ 7 / 2\n"
      "print(count) # trailing print comment\n"
      "print(message)\n"
      "print(label)\n"
      "print(ratio)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  graphion_runtime_program program;
  const graphion_runtime_value *count;
  const graphion_runtime_value *message;
  const graphion_runtime_value *label;
  const graphion_runtime_value *ratio;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_prepare_source(prepare_source_text, &program, &diagnostic);
  if (rc != GINT_OK) {
    return finish_scope_test(&scope, 1);
  }
  graphion_runtime_program_dispose(&program);
  if (!test_capture_gion_output(source, "gion_comments.txt", &scope, &diagnostic, path, sizeof(path), output,
                                sizeof(output))) {
    return finish_scope_test(&scope, 3);
  }
  count = graphion_runtime_scope_find(&scope, "count");
  message = graphion_runtime_scope_find(&scope, "message");
  label = graphion_runtime_scope_find(&scope, "label");
  ratio = graphion_runtime_scope_find(&scope, "ratio");
  if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
    test_cleanup_temp_path(path);
    return finish_scope_test(&scope, 4);
  }
  if (message == NULL || message->kind != GVM_VALUE_STRING || strcmp(message->as.string_value, "/* not a comment */") != 0) {
    test_cleanup_temp_path(path);
    return finish_scope_test(&scope, 5);
  }
  if (label == NULL || label->kind != GVM_VALUE_STRING || strcmp(label->as.string_value, "ok") != 0) {
    test_cleanup_temp_path(path);
    return finish_scope_test(&scope, 6);
  }
  if (ratio == NULL || ratio->kind != GVM_VALUE_FLOAT || ratio->as.float_value != 3.5) {
    test_cleanup_temp_path(path);
    return finish_scope_test(&scope, 7);
  }
  test_cleanup_temp_path(path);
  if (strcmp(output, "42\n/* not a comment */\nok\n3.5\n") != 0) {
    return finish_scope_test(&scope, 9);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_comment_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"/* unterminated block comment\ncount = 42\n", 1U},
      {"count = 42\n/* unterminated block comment\nprint(count)\n", 2U},
      {"message = \"/* not a comment */\"\n/* unterminated\n", 2U},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    graphion_runtime_program program;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "unterminated block comment") != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);

    graphion_runtime_program_init(&program);
    rc = graphion_prepare_source(cases[i].source, &program, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      graphion_runtime_program_dispose(&program);
      return (int)(3 + i * 10U);
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "unterminated block comment") != 0) {
      graphion_runtime_program_dispose(&program);
      return (int)(4 + i * 10U);
    }
    graphion_runtime_program_dispose(&program);
  }
  return 0;
}

int test_gion_unmapped_vm_error_diagnostics(void) {
  graphion_runtime_program program;
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_program_init(&program);
  graphion_runtime_scope_init(&scope);
  program.program[0].op = GVM_OP_MOV_IMM;
  program.program[0].a = 17U;
  program.program[0].b = 0U;
  program.program[0].imm = 1;
  program.program_len = 1U;

  rc = graphion_execute_program(&program, &scope, &diagnostic, NULL);
  graphion_runtime_program_dispose(&program);
  graphion_runtime_scope_dispose(&scope);
  if (rc != GINT_ERR_RUN) {
    return 1;
  }
  if (diagnostic.line != 1U || diagnostic.column != 1U) {
    return 2;
  }
  if (diagnostic.message == NULL ||
      strcmp(diagnostic.message, "unmapped VM runtime error: GVM_ERR_INVALID_MOV_IMM_REG") != 0) {
    return 3;
  }
  return 0;
}

int test_gion_vm_load_failure_is_runtime_diagnostic(void) {
  graphion_runtime_program program;
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_program_init(&program);
  graphion_runtime_scope_init(&scope);

  rc = graphion_execute_program(&program, &scope, &diagnostic, NULL);
  graphion_runtime_program_dispose(&program);
  graphion_runtime_scope_dispose(&scope);
  if (rc != GINT_ERR_RUN) {
    return 1;
  }
  if (diagnostic.line != 1U || diagnostic.column != 1U) {
    return 2;
  }
  if (diagnostic.message == NULL || strcmp(diagnostic.message, "failed to load VM program") != 0) {
    return 3;
  }
  return 0;
}

int test_gion_warning_comments_are_ignored(void) {
  graphion_runtime_warning_report report;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  rc = graphion_collect_source_warnings("# graphion: unknown=off\nprint(1)\n", &report, &diagnostic);
  if (rc != GINT_OK) {
    return 1;
  }
  if (!report.enabled) {
    return 2;
  }
  if (report.count != 0U) {
    return 3;
  }

  rc = graphion_collect_source_warnings("# graphion: warnings=off\n# graphion: unknown=off\nprint(1)\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 6;
  }
  if (!report.enabled) {
    return 7;
  }
  if (report.count != 0U) {
    return 8;
  }

  rc = graphion_collect_source_warnings("print(1)\n# graphion: unknown=off\n", &report, &diagnostic);
  if (rc != GINT_OK) {
    return 9;
  }
  if (!report.enabled) {
    return 10;
  }
  if (report.count != 0U) {
    return 11;
  }

  rc = graphion_collect_source_warnings("match \"a\":\n    1:\n        print(1)\n    default:\n        print(2)\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 12;
  }
  if (!report.enabled) {
    return 13;
  }
  if (report.count != 1U) {
    return 14;
  }
  if (strcmp(report.items[0].message, "match case can never match a string value") != 0) {
    return 15;
  }
  if (report.items[0].line != 2U || report.items[0].column != 5U) {
    return 16;
  }

  rc = graphion_collect_source_warnings("# graphion: warnings=off\n"
                                        "match \"a\":\n"
                                        "    1:\n"
                                        "        print(1)\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 17;
  }
  if (!report.enabled || report.count != 1U) {
    return 18;
  }

  return 0;
}

int test_gion_warning_comments_are_ignored_from_path(void) {
  char path[512];
  graphion_runtime_warning_report report;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  fp = test_open_temp_output(path, sizeof(path), "gion_warning_comments_ignored_path.gion");
  if (fp == NULL) {
    return 1;
  }
  fputs("# graphion: warnings=off\n# graphion: unknown=off\nprint(1)\n", fp);
  fclose(fp);

  rc = graphion_collect_gion_path_warnings(path, &report, &diagnostic);
  remove(path);
  if (rc != GENTRY_OK) {
    return 2;
  }
  if (!report.enabled) {
    return 3;
  }
  if (report.count != 0U) {
    return 4;
  }

  return 0;
}

int test_gion_impossible_literal_match_warnings(void) {
  graphion_runtime_warning_report report;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  rc = graphion_collect_source_warnings("match \"a\":\n"
                                        "    1:\n"
                                        "        print(1)\n"
                                        "    false:\n"
                                        "        print(2)\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 1;
  }
  if (report.count != 2U) {
    return 2;
  }
  if (report.items[0].line != 2U || report.items[0].column != 5U ||
      strcmp(report.items[0].message, "match case can never match a string value") != 0) {
    return 3;
  }
  if (report.items[1].line != 4U || report.items[1].column != 5U ||
      strcmp(report.items[1].message, "match case can never match a string value") != 0) {
    return 4;
  }

  rc = graphion_collect_source_warnings("match 1:\n"
                                        "    1.0:\n"
                                        "        print(1)\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 5;
  }
  if (report.count != 0U) {
    return 6;
  }
  return 0;
}

int test_gion_warning_report_output_format(void) {
  const char *expected =
      "warning:2:5: first warning\n"
      "warning:17:23: second warning\n";
  char path[512];
  char output[256];
  graphion_runtime_warning_report report;
  FILE *fp = NULL;

  graphion_runtime_warning_report_init(&report);
  report.count = 2U;
  report.items[0].line = 2U;
  report.items[0].column = 5U;
  memcpy(report.items[0].message, "first warning", sizeof("first warning"));
  report.items[1].line = 17U;
  report.items[1].column = 23U;
  memcpy(report.items[1].message, "second warning", sizeof("second warning"));

  fp = test_open_temp_output(path, sizeof(path), "gion_warning_report_output_format.txt");
  if (fp == NULL) {
    return 1;
  }
  graphion_emit_warning_report(&report, fp);
  fclose(fp);
  if (!test_read_file_text(path, output, sizeof(output))) {
    test_cleanup_temp_path(path);
    return 2;
  }
  test_cleanup_temp_path(path);
  normalize_text_newlines(output);
  if (strcmp(output, expected) != 0) {
    return 3;
  }
  return 0;
}

int test_gion_warning_capacity_failures(void) {
  char source[4096];
  char path[512];
  size_t offset = 0U;
  size_t i;
  graphion_runtime_warning_report report;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  offset = (size_t)snprintf(source, sizeof(source), "match \"a\":\n");
  for (i = 0U; i <= GRAPHION_RUNTIME_WARNING_MAX; ++i) {
    const int written = snprintf(source + offset,
                                 sizeof(source) - offset,
                                 "    %lu:\n"
                                 "        print(1)\n",
                                 (unsigned long)i);
    if (written < 0 || (size_t)written >= sizeof(source) - offset) {
      return 1;
    }
    offset += (size_t)written;
  }

  rc = graphion_collect_source_warnings(source, &report, &diagnostic);
  if (rc != GINT_ERR_CAPACITY) {
    return 2;
  }
  if (report.count != 0U) {
    return 3;
  }
  if (diagnostic.line != 66U || diagnostic.column != 5U ||
      diagnostic.message == NULL || strcmp(diagnostic.message, "warning capacity exceeded") != 0) {
    return 4;
  }

  fp = test_open_temp_output(path, sizeof(path), "gion_warning_capacity_failure.gion");
  if (fp == NULL) {
    return 5;
  }
  fputs(source, fp);
  fclose(fp);
  rc = graphion_collect_gion_path_warnings(path, &report, &diagnostic);
  test_cleanup_temp_path(path);
  if (rc != GENTRY_ERR_RUN) {
    return 6;
  }
  if (report.count != 0U) {
    return 7;
  }
  if (diagnostic.line != 66U || diagnostic.column != 5U ||
      diagnostic.message == NULL || strcmp(diagnostic.message, "warning capacity exceeded") != 0) {
    return 8;
  }
  return 0;
}
