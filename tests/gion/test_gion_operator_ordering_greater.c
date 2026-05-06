/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_greater_than_expressions(void) {
  const char *source =
      "greater_int = 2 > 1\n"
      "same_numeric = 2 > 2.0\n"
      "mixed_numeric = 2.5 > 2\n"
      "reverse_numeric = 2 > 3.0\n"
      "grouped = (1 + 3) > 3\n"
      "precedence = 1 + 3 > 3\n"
      "power_cmp = 2 ** 3 > 7\n"
      "print(greater_int)\n"
      "print(same_numeric)\n"
      "print(mixed_numeric)\n"
      "print(reverse_numeric)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *greater_int;
  const graphion_runtime_value *mixed_numeric;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_greater_than_expressions.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  greater_int = graphion_runtime_scope_find(&scope, "greater_int");
  mixed_numeric = graphion_runtime_scope_find(&scope, "mixed_numeric");
  if (greater_int == NULL || greater_int->kind != GVM_VALUE_BOOL || greater_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (mixed_numeric == NULL || mixed_numeric->kind != GVM_VALUE_BOOL || mixed_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_greater_than_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = true > 1\n", 1U},
      {"if true > 1:\n    print(1)\n", 1U},
      {"value = \"x\" > \"y\"\n", 1U},
      {"value = \"x\" > 1.5\n", 1U},
      {"value = 0b10 > 0b0010\n", 1U},
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

int test_gion_greater_than_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 >\n", "expected scalar literal"},
      {"value = > 1\n", "expected scalar literal"},
      {"print(1 > )\n", "expected scalar literal"},
      {"if 1 >:\n    print(1)\n", "expected scalar literal"},
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

int test_gion_greater_equal_expressions(void) {
  const char *source =
      "greater_int = 2 >= 1\n"
      "same_numeric = 2 >= 2.0\n"
      "mixed_numeric = 2.5 >= 2\n"
      "reverse_numeric = 2 >= 3.0\n"
      "grouped = (1 + 2) >= 3\n"
      "precedence = 1 + 2 >= 3\n"
      "power_cmp = 2 ** 3 >= 8\n"
      "print(greater_int)\n"
      "print(same_numeric)\n"
      "print(mixed_numeric)\n"
      "print(reverse_numeric)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *same_numeric;
  const graphion_runtime_value *power_cmp;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_greater_equal_expressions.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  same_numeric = graphion_runtime_scope_find(&scope, "same_numeric");
  power_cmp = graphion_runtime_scope_find(&scope, "power_cmp");
  if (same_numeric == NULL || same_numeric->kind != GVM_VALUE_BOOL || same_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (power_cmp == NULL || power_cmp->kind != GVM_VALUE_BOOL || power_cmp->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\ntrue\ntrue\nfalse\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_greater_equal_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = true >= 1\n", 1U},
      {"if true >= 1:\n    print(1)\n", 1U},
      {"value = \"x\" >= \"y\"\n", 1U},
      {"value = \"x\" >= 1.5\n", 1U},
      {"value = 0b10 >= 0b0010\n", 1U},
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

int test_gion_greater_equal_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 >=\n", "expected scalar literal"},
      {"value = >= 1\n", "expected scalar literal"},
      {"print(1 >= )\n", "expected scalar literal"},
      {"if 1 >=:\n    print(1)\n", "expected scalar literal"},
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
