/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_inequality_expressions(void) {
  const char *source =
      "different_int = 42 != 41\n"
      "same_numeric = 42 != 42.0\n"
      "int_true_bool = 1 != true\n"
      "int_false_bool = 0 != false\n"
      "bool_true_int = true != 1\n"
      "bool_false_int = false != 0\n"
      "different_numeric = 42 != 43\n"
      "different_bool = true != false\n"
      "same_string = \"ok\" != \"ok\"\n"
      "different_string = \"ok\" != \"no\"\n"
      "grouped = (1 + 2) != 4\n"
      "precedence = 1 + 2 != 4\n"
      "power_cmp = 2 ** 3 != 9\n"
      "print(different_int)\n"
      "print(same_numeric)\n"
      "print(int_true_bool)\n"
      "print(int_false_bool)\n"
      "print(bool_true_int)\n"
      "print(bool_false_int)\n"
      "print(different_numeric)\n"
      "print(different_bool)\n"
      "print(same_string)\n"
      "print(different_string)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  char path[512];
  char output[160];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *different_int;
  const graphion_runtime_value *same_numeric;
  const graphion_runtime_value *int_true_bool;
  const graphion_runtime_value *int_false_bool;
  const graphion_runtime_value *bool_true_int;
  const graphion_runtime_value *bool_false_int;
  const graphion_runtime_value *different_numeric;
  const graphion_runtime_value *different_string;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_inequality_expressions.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  different_int = graphion_runtime_scope_find(&scope, "different_int");
  same_numeric = graphion_runtime_scope_find(&scope, "same_numeric");
  int_true_bool = graphion_runtime_scope_find(&scope, "int_true_bool");
  int_false_bool = graphion_runtime_scope_find(&scope, "int_false_bool");
  bool_true_int = graphion_runtime_scope_find(&scope, "bool_true_int");
  bool_false_int = graphion_runtime_scope_find(&scope, "bool_false_int");
  different_numeric = graphion_runtime_scope_find(&scope, "different_numeric");
  different_string = graphion_runtime_scope_find(&scope, "different_string");
  if (different_int == NULL || different_int->kind != GVM_VALUE_BOOL || different_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (same_numeric == NULL || same_numeric->kind != GVM_VALUE_BOOL || same_numeric->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (int_true_bool == NULL || int_true_bool->kind != GVM_VALUE_BOOL || int_true_bool->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (int_false_bool == NULL || int_false_bool->kind != GVM_VALUE_BOOL || int_false_bool->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (bool_true_int == NULL || bool_true_int->kind != GVM_VALUE_BOOL || bool_true_int->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (bool_false_int == NULL || bool_false_int->kind != GVM_VALUE_BOOL || bool_false_int->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (different_numeric == NULL || different_numeric->kind != GVM_VALUE_BOOL ||
      different_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (different_string == NULL || different_string->kind != GVM_VALUE_BOOL || different_string->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 11);
  }
  remove(path);
  if (strcmp(output, "true\nfalse\nfalse\nfalse\nfalse\nfalse\ntrue\ntrue\nfalse\ntrue\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 12);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_inequality_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 1 != \"1\"\n", 1U},
      {"if 1 != \"1\":\n    print(1)\n", 1U},
      {"value = \"true\" != true\n", 1U},
      {"value = \"x\" != 1.5\n", 1U},
      {"value = 2 != true\n", 1U},
      {"value = true != 2\n", 1U},
      {"value = 1.0 != true\n", 1U},
      {"value = false != 0.0\n", 1U},
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
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "incompatible operand types") != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_inequality_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 !=\n", "expected scalar literal"},
      {"value = != 1\n", "expected scalar literal"},
      {"print(1 != )\n", "expected scalar literal"},
      {"if 1 !=:\n    print(1)\n", "expected scalar literal"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}
