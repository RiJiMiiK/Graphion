/* SPDX-License-Identifier: MIT */

#include "test_parser_helpers.h"

int test_gion_equality_expressions(void) {
  const char *source =
      "same_int = 42 == 42\n"
      "mixed_numeric = 42 == 42.0\n"
      "int_true_bool = 1 == true\n"
      "int_false_bool = 0 == false\n"
      "bool_true_int = true == 1\n"
      "bool_false_int = false == 0\n"
      "different_numeric = 42 == 41\n"
      "same_bool = true == true\n"
      "different_bool = true == false\n"
      "same_string = \"ok\" == \"ok\"\n"
      "different_string = \"ok\" == \"no\"\n"
      "grouped = (1 + 2) == 3\n"
      "precedence = 1 + 2 == 3\n"
      "power_cmp = 2 ** 3 == 8\n"
      "print(same_int)\n"
      "print(mixed_numeric)\n"
      "print(int_true_bool)\n"
      "print(int_false_bool)\n"
      "print(bool_true_int)\n"
      "print(bool_false_int)\n"
      "print(different_numeric)\n"
      "print(same_bool)\n"
      "print(different_bool)\n"
      "print(same_string)\n"
      "print(different_string)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  const char *path = "gion_equality_expressions.txt";
  char output[160];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *same_int;
  const graphion_runtime_value *mixed_numeric;
  const graphion_runtime_value *int_true_bool;
  const graphion_runtime_value *int_false_bool;
  const graphion_runtime_value *bool_true_int;
  const graphion_runtime_value *bool_false_int;
  const graphion_runtime_value *different_numeric;
  const graphion_runtime_value *same_string;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  same_int = graphion_runtime_scope_find(&scope, "same_int");
  mixed_numeric = graphion_runtime_scope_find(&scope, "mixed_numeric");
  int_true_bool = graphion_runtime_scope_find(&scope, "int_true_bool");
  int_false_bool = graphion_runtime_scope_find(&scope, "int_false_bool");
  bool_true_int = graphion_runtime_scope_find(&scope, "bool_true_int");
  bool_false_int = graphion_runtime_scope_find(&scope, "bool_false_int");
  different_numeric = graphion_runtime_scope_find(&scope, "different_numeric");
  same_string = graphion_runtime_scope_find(&scope, "same_string");
  if (same_int == NULL || same_int->kind != GVM_VALUE_BOOL || same_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (mixed_numeric == NULL || mixed_numeric->kind != GVM_VALUE_BOOL || mixed_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (int_true_bool == NULL || int_true_bool->kind != GVM_VALUE_BOOL || int_true_bool->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (int_false_bool == NULL || int_false_bool->kind != GVM_VALUE_BOOL || int_false_bool->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (bool_true_int == NULL || bool_true_int->kind != GVM_VALUE_BOOL || bool_true_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (bool_false_int == NULL || bool_false_int->kind != GVM_VALUE_BOOL || bool_false_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (different_numeric == NULL || different_numeric->kind != GVM_VALUE_BOOL ||
      different_numeric->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (same_string == NULL || same_string->kind != GVM_VALUE_BOOL || same_string->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 11);
  }
  remove(path);
  if (strcmp(output, "true\ntrue\ntrue\ntrue\ntrue\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 12);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_equality_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 1 == \"1\"\n", 1U},
      {"if 1 == \"1\":\n    print(1)\n", 1U},
      {"value = \"true\" == true\n", 1U},
      {"value = \"x\" == 1.5\n", 1U},
      {"value = 2 == true\n", 1U},
      {"value = true == 2\n", 1U},
      {"value = 1.0 == true\n", 1U},
      {"value = false == 0.0\n", 1U},
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

int test_gion_equality_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 ==\n", "expected scalar literal"},
      {"value = == 1\n", "expected scalar literal"},
      {"print(1 == )\n", "expected scalar literal"},
      {"if 1 ==:\n    print(1)\n", "expected scalar literal"},
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
  const char *path = "gion_inequality_expressions.txt";
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
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
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

int test_gion_less_than_expressions(void) {
  const char *source =
      "smaller_int = 1 < 2\n"
      "same_numeric = 2 < 2.0\n"
      "mixed_numeric = 2 < 2.5\n"
      "reverse_numeric = 3.0 < 2\n"
      "grouped = (1 + 2) < 4\n"
      "precedence = 1 + 2 < 4\n"
      "power_cmp = 2 ** 3 < 9\n"
      "print(smaller_int)\n"
      "print(same_numeric)\n"
      "print(mixed_numeric)\n"
      "print(reverse_numeric)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  const char *path = "gion_less_than_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *smaller_int;
  const graphion_runtime_value *mixed_numeric;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  smaller_int = graphion_runtime_scope_find(&scope, "smaller_int");
  mixed_numeric = graphion_runtime_scope_find(&scope, "mixed_numeric");
  if (smaller_int == NULL || smaller_int->kind != GVM_VALUE_BOOL || smaller_int->as.bool_value != 1) {
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

int test_gion_less_than_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = true < 1\n", 1U},
      {"if true < 1:\n    print(1)\n", 1U},
      {"value = \"x\" < \"y\"\n", 1U},
      {"value = \"x\" < 1.5\n", 1U},
      {"value = 0b10 < 0b0010\n", 1U},
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

int test_gion_less_than_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 <\n", "expected scalar literal"},
      {"value = < 1\n", "expected scalar literal"},
      {"print(1 < )\n", "expected scalar literal"},
      {"if 1 <:\n    print(1)\n", "expected scalar literal"},
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

int test_gion_less_equal_expressions(void) {
  const char *source =
      "smaller_int = 1 <= 2\n"
      "same_numeric = 2 <= 2.0\n"
      "mixed_numeric = 2 <= 2.5\n"
      "reverse_numeric = 3.0 <= 2\n"
      "grouped = (1 + 2) <= 3\n"
      "precedence = 1 + 2 <= 3\n"
      "power_cmp = 2 ** 3 <= 8\n"
      "print(smaller_int)\n"
      "print(same_numeric)\n"
      "print(mixed_numeric)\n"
      "print(reverse_numeric)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  const char *path = "gion_less_equal_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *smaller_int;
  const graphion_runtime_value *same_numeric;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  smaller_int = graphion_runtime_scope_find(&scope, "smaller_int");
  same_numeric = graphion_runtime_scope_find(&scope, "same_numeric");
  if (smaller_int == NULL || smaller_int->kind != GVM_VALUE_BOOL || smaller_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (same_numeric == NULL || same_numeric->kind != GVM_VALUE_BOOL || same_numeric->as.bool_value != 1) {
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

int test_gion_less_equal_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = true <= 1\n", 1U},
      {"if true <= 1:\n    print(1)\n", 1U},
      {"value = \"x\" <= \"y\"\n", 1U},
      {"value = \"x\" <= 1.5\n", 1U},
      {"value = 0b10 <= 0b0010\n", 1U},
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

int test_gion_less_equal_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 <=\n", "expected scalar literal"},
      {"value = <= 1\n", "expected scalar literal"},
      {"print(1 <= )\n", "expected scalar literal"},
      {"if 1 <=:\n    print(1)\n", "expected scalar literal"},
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
  const char *path = "gion_greater_than_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *greater_int;
  const graphion_runtime_value *mixed_numeric;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
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
  const char *path = "gion_greater_equal_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *same_numeric;
  const graphion_runtime_value *power_cmp;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
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

int test_gion_and_expressions(void) {
  const char *source =
      "both_true = true and true\n"
      "true_false = true and false\n"
      "int_bool = 1 and true\n"
      "bool_int = false and 1\n"
      "ints = 1 and 0\n"
      "comparisons = 1 == 1 and 2 < 3\n"
      "precedence = 1 == 1 and 2 == 3\n"
      "precedence_over_or = true or false and false\n"
      "grouped = (1 == 1) and (3 >= 3)\n"
      "print(both_true)\n"
      "print(true_false)\n"
      "print(int_bool)\n"
      "print(bool_int)\n"
      "print(ints)\n"
      "print(comparisons)\n"
      "print(precedence)\n"
      "print(precedence_over_or)\n"
      "print(grouped)\n";
  const char *path = "gion_and_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *both_true;
  const graphion_runtime_value *precedence;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  both_true = graphion_runtime_scope_find(&scope, "both_true");
  precedence = graphion_runtime_scope_find(&scope, "precedence");
  if (both_true == NULL || both_true->kind != GVM_VALUE_BOOL || both_true->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (precedence == NULL || precedence->kind != GVM_VALUE_BOOL || precedence->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\nfalse\ntrue\nfalse\nfalse\ntrue\nfalse\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_and_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 2 and true\n", 1U},
      {"value = true and 2\n", 1U},
      {"value = 1 and 2\n", 1U},
      {"value = 1.0 and true\n", 1U},
      {"value = \"x\" and true\n", 1U},
      {"value = 0b10 and true\n", 1U},
      {"value = true and 0b10\n", 1U},
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

int test_gion_and_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    int alternate_rc;
    const char *message;
  } cases[] = {
      {"value = true and\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"value = and true\n", GINT_ERR_UNKNOWN_OPERAND, GINT_ERR_PARSE, "unknown operand"},
      {"print(true and )\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"if true and:\n    print(1)\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc && rc != cases[i].alternate_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_or_expressions(void) {
  const char *source =
      "both_true = true or true\n"
      "true_false = true or false\n"
      "int_bool = 1 or false\n"
      "bool_int = false or 1\n"
      "ints = 0 or 0\n"
      "comparisons = 1 == 2 or 2 < 3\n"
      "precedence = 1 == 2 or 2 == 3\n"
      "precedence_under_and = false or true and false\n"
      "grouped = (1 == 2) or (3 >= 3)\n"
      "print(both_true)\n"
      "print(true_false)\n"
      "print(int_bool)\n"
      "print(bool_int)\n"
      "print(ints)\n"
      "print(comparisons)\n"
      "print(precedence)\n"
      "print(precedence_under_and)\n"
      "print(grouped)\n";
  const char *path = "gion_or_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *both_true;
  const graphion_runtime_value *precedence;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  both_true = graphion_runtime_scope_find(&scope, "both_true");
  precedence = graphion_runtime_scope_find(&scope, "precedence");
  if (both_true == NULL || both_true->kind != GVM_VALUE_BOOL || both_true->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (precedence == NULL || precedence->kind != GVM_VALUE_BOOL || precedence->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\ntrue\ntrue\ntrue\nfalse\ntrue\nfalse\nfalse\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_or_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 2 or true\n", 1U},
      {"value = false or 2\n", 1U},
      {"value = 0 or 2\n", 1U},
      {"value = 1.0 or true\n", 1U},
      {"value = \"x\" or true\n", 1U},
      {"value = 0b10 or true\n", 1U},
      {"value = false or 0b10\n", 1U},
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

int test_gion_or_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    int alternate_rc;
    const char *message;
  } cases[] = {
      {"value = true or\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"value = or true\n", GINT_ERR_UNKNOWN_OPERAND, GINT_ERR_PARSE, "unknown operand"},
      {"print(true or )\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"if true or:\n    print(1)\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc && rc != cases[i].alternate_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_not_expressions(void) {
  const char *source =
      "invert_true = not true\n"
      "invert_false = not false\n"
      "int_true = not 1\n"
      "int_false = not 0\n"
      "comparison = not 1 == 2\n"
      "double_not = not not true\n"
      "mixed_precedence = not false and false\n"
      "under_or = false or not false\n"
      "grouped = not (1 == 1 and false)\n"
      "print(invert_true)\n"
      "print(invert_false)\n"
      "print(int_true)\n"
      "print(int_false)\n"
      "print(comparison)\n"
      "print(double_not)\n"
      "print(mixed_precedence)\n"
      "print(under_or)\n"
      "print(grouped)\n";
  const char *path = "gion_not_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *invert_true;
  const graphion_runtime_value *comparison;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  invert_true = graphion_runtime_scope_find(&scope, "invert_true");
  comparison = graphion_runtime_scope_find(&scope, "comparison");
  if (invert_true == NULL || invert_true->kind != GVM_VALUE_BOOL || invert_true->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (comparison == NULL || comparison->kind != GVM_VALUE_BOOL || comparison->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "false\ntrue\nfalse\ntrue\ntrue\ntrue\nfalse\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_not_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = not 2\n", 1U},
      {"value = not 1.0\n", 1U},
      {"value = not \"x\"\n", 1U},
      {"value = true and not 2\n", 1U},
      {"value = not 0b10\n", 1U},
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

int test_gion_not_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    int alternate_rc;
    const char *message;
  } cases[] = {
      {"value = not\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"print(not )\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"if not:\n    print(1)\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc && rc != cases[i].alternate_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_nand_expressions(void) {
  const char *source =
      "both_true = true nand true\n"
      "true_false = true nand false\n"
      "int_bool = 1 nand true\n"
      "bool_int = false nand 1\n"
      "ints = 1 nand 0\n"
      "comparisons = 1 == 1 nand 2 < 3\n"
      "precedence_over_or = true or true nand true\n"
      "grouped = (1 == 1) nand (3 >= 3)\n"
      "print(both_true)\n"
      "print(true_false)\n"
      "print(int_bool)\n"
      "print(bool_int)\n"
      "print(ints)\n"
      "print(comparisons)\n"
      "print(precedence_over_or)\n"
      "print(grouped)\n";
  const char *path = "gion_nand_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *both_true;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  both_true = graphion_runtime_scope_find(&scope, "both_true");
  if (both_true == NULL || both_true->kind != GVM_VALUE_BOOL || both_true->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "false\ntrue\nfalse\ntrue\ntrue\nfalse\ntrue\nfalse\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_nand_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 2 nand true\n", 1U},
      {"value = true nand 2\n", 1U},
      {"value = 1 nand 2\n", 1U},
      {"value = 1.0 nand true\n", 1U},
      {"value = \"x\" nand true\n", 1U},
      {"value = 0b10 nand true\n", 1U},
      {"value = true nand 0b10\n", 1U},
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

int test_gion_nand_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    int alternate_rc;
    const char *message;
  } cases[] = {
      {"value = true nand\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"value = nand true\n", GINT_ERR_UNKNOWN_OPERAND, GINT_ERR_PARSE, "unknown operand"},
      {"print(true nand )\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"if true nand:\n    print(1)\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc && rc != cases[i].alternate_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_nor_expressions(void) {
  const char *source =
      "both_false = false nor false\n"
      "true_false = true nor false\n"
      "int_bool = 0 nor false\n"
      "bool_int = false nor 1\n"
      "ints = 0 nor 1\n"
      "comparisons = 1 == 2 nor 2 == 3\n"
      "precedence_with_and = false nor true and false\n"
      "grouped = (1 == 2) nor (3 < 2)\n"
      "print(both_false)\n"
      "print(true_false)\n"
      "print(int_bool)\n"
      "print(bool_int)\n"
      "print(ints)\n"
      "print(comparisons)\n"
      "print(precedence_with_and)\n"
      "print(grouped)\n";
  const char *path = "gion_nor_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *both_false;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  both_false = graphion_runtime_scope_find(&scope, "both_false");
  if (both_false == NULL || both_false->kind != GVM_VALUE_BOOL || both_false->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "true\nfalse\ntrue\nfalse\nfalse\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_nor_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 2 nor false\n", 1U},
      {"value = false nor 2\n", 1U},
      {"value = 0 nor 2\n", 1U},
      {"value = 1.0 nor false\n", 1U},
      {"value = \"x\" nor false\n", 1U},
      {"value = 0b10 nor false\n", 1U},
      {"value = false nor 0b10\n", 1U},
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

int test_gion_nor_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    int alternate_rc;
    const char *message;
  } cases[] = {
      {"value = false nor\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"value = nor false\n", GINT_ERR_UNKNOWN_OPERAND, GINT_ERR_PARSE, "unknown operand"},
      {"print(false nor )\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
      {"if false nor:\n    print(1)\n", GINT_ERR_PARSE, 0, "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc && rc != cases[i].alternate_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

