/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_tuple_literals_and_prints(void) {
  const char *source =
      "pair = (1, 2)\n"
      "mixed = (pair, \"x\", true)\n"
      "second = pair[1]\n"
      "same = pair == (1, 2)\n"
      "different = pair != (2, 1)\n"
      "grouped = (1 + 2) * 3\n"
      "size = len(pair)\n"
      "print(pair)\n"
      "print(mixed)\n"
      "print(second)\n"
      "print(same)\n"
      "print(different)\n"
      "print(grouped)\n"
      "print(size)\n";
  char path[512];
  char output[256];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *pair;
  const graphion_runtime_value *mixed;
  const graphion_runtime_value *second;
  const graphion_runtime_value *same;
  const graphion_runtime_value *different;
  const graphion_runtime_value *grouped;
  const graphion_runtime_value *size;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_tuple_literals_and_prints.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  pair = graphion_runtime_scope_find(&scope, "pair");
  mixed = graphion_runtime_scope_find(&scope, "mixed");
  second = graphion_runtime_scope_find(&scope, "second");
  same = graphion_runtime_scope_find(&scope, "same");
  different = graphion_runtime_scope_find(&scope, "different");
  grouped = graphion_runtime_scope_find(&scope, "grouped");
  size = graphion_runtime_scope_find(&scope, "size");
  if (pair == NULL || pair->kind != GVM_VALUE_TUPLE) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (mixed == NULL || mixed->kind != GVM_VALUE_TUPLE) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (second == NULL || second->kind != GVM_VALUE_INT || second->as.int_value != 2) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (same == NULL || same->kind != GVM_VALUE_BOOL || same->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (different == NULL || different->kind != GVM_VALUE_BOOL || different->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (grouped == NULL || grouped->kind != GVM_VALUE_INT || grouped->as.int_value != 9) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (size == NULL || size->kind != GVM_VALUE_INT || size->as.int_value != 2) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  remove(path);
  if (strcmp(output,
             "(1, 2)\n"
             "((1, 2), \"x\", true)\n"
             "2\n"
             "true\n"
             "true\n"
             "9\n"
             "2\n") != 0) {
    return finish_scope_test(&scope, 11);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_tuple_contains_conditions(void) {
  const char *source =
      "items = (1, 2, (3, 4))\n"
      "if_result = \"unset\"\n"
      "elif_result = \"unset\"\n"
      "if contains(items, 2):\n"
      "    if_result = \"present\"\n"
      "else:\n"
      "    if_result = \"missing\"\n"
      "if contains(items, 9):\n"
      "    elif_result = \"bad\"\n"
      "elif contains(items, (3, 4)):\n"
      "    elif_result = \"nested\"\n"
      "else:\n"
      "    elif_result = \"missing\"\n"
      "ternary_result = \"yes\" if contains(items, (3, 4)) else \"no\"\n"
      "and_result = contains(items, 1) and contains(items, 2)\n"
      "or_result = contains(items, 9) or contains(items, (3, 4))\n"
      "print(if_result)\n"
      "print(elif_result)\n"
      "print(ternary_result)\n"
      "print(and_result)\n"
      "print(or_result)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_tuple_contains_conditions.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output,
             "present\n"
             "nested\n"
             "yes\n"
             "true\n"
             "true\n") != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_tuple_runtime_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"pair = (1, 2)\nprint(pair[2])\n", GINT_ERR_RUN, "list index out of range"},
      {"pair = (1, 2)\nprint(pair[\"0\"])\n", GINT_ERR_RUN, "incompatible operand types"},
      {"pair = (1, 2)\nprint(pair == [1, 2])\n", GINT_ERR_RUN, "incompatible operand types"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != cases[i].expected_rc) {
      return (int)(100 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(200 + i);
    }
  }
  return 0;
}

int test_gion_tuple_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"pair = ()\n", GINT_ERR_PARSE, "empty tuple literal is not supported"},
      {"pair = (1,)\n", GINT_ERR_PARSE, "trailing comma is not allowed in tuple literal"},
      {"pair = (1 2)\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"pair = (1, 2\n", GINT_ERR_PARSE, "expected ',' or ')' after tuple element"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != cases[i].expected_rc) {
      return (int)(300 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(400 + i);
    }
  }
  return 0;
}
