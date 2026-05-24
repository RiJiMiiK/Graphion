/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_list_literals_and_prints(void) {
  const char *source =
      "items = [1, 2, 3]\n"
      "nested = [items, [4, \"x\"], []]\n"
      "second = items[1]\n"
      "same = items == [1, 2, 3]\n"
      "different = items != [1, 2]\n"
      "size = len(items)\n"
      "print(items)\n"
      "print(nested)\n"
      "print(second)\n"
      "print(same)\n"
      "print(different)\n"
      "print(size)\n";
  char path[512];
  char output[256];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *items;
  const graphion_runtime_value *nested;
  const graphion_runtime_value *second;
  const graphion_runtime_value *same;
  const graphion_runtime_value *different;
  const graphion_runtime_value *size;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_list_literals_and_prints.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  items = graphion_runtime_scope_find(&scope, "items");
  nested = graphion_runtime_scope_find(&scope, "nested");
  second = graphion_runtime_scope_find(&scope, "second");
  same = graphion_runtime_scope_find(&scope, "same");
  different = graphion_runtime_scope_find(&scope, "different");
  size = graphion_runtime_scope_find(&scope, "size");
  if (items == NULL || items->kind != GVM_VALUE_LIST) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (nested == NULL || nested->kind != GVM_VALUE_LIST) {
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
  if (size == NULL || size->kind != GVM_VALUE_INT || size->as.int_value != 3) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  remove(path);
  if (strcmp(output,
             "[1, 2, 3]\n"
             "[[1, 2, 3], [4, \"x\"], []]\n"
             "2\n"
             "true\n"
             "true\n"
             "3\n") != 0) {
    return finish_scope_test(&scope, 10);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_list_contains_conditions(void) {
  const char *source =
      "items = [1, 2, [3]]\n"
      "if_result = \"unset\"\n"
      "elif_result = \"unset\"\n"
      "if contains(items, 2):\n"
      "    if_result = \"present\"\n"
      "else:\n"
      "    if_result = \"missing\"\n"
      "if contains(items, 9):\n"
      "    elif_result = \"bad\"\n"
      "elif contains(items, [3]):\n"
      "    elif_result = \"nested\"\n"
      "else:\n"
      "    elif_result = \"missing\"\n"
      "ternary_result = \"yes\" if contains(items, [3]) else \"no\"\n"
      "and_result = contains(items, 1) and contains(items, 2)\n"
      "or_result = contains(items, 9) or contains(items, [3])\n"
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
  fp = test_open_temp_output(path, sizeof(path), "gion_list_contains_conditions.txt");
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

int test_gion_list_runtime_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"items = [1, 2]\nprint(items[2])\n", GINT_ERR_RUN, "list index out of range"},
      {"items = [1, 2]\nprint(items[-1])\n", GINT_ERR_RUN, "list index out of range"},
      {"items = [1, 2]\nprint(items[\"0\"])\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = 1\nprint(value[0])\n", GINT_ERR_RUN, "incompatible operand types"},
      {"items = [1, 2]\nprint(items == 1)\n", GINT_ERR_RUN, "incompatible operand types"},
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

int test_gion_list_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"items = [1, 2\n", GINT_ERR_PARSE, "expected ',' or ']' after list element"},
      {"items = [1,]\n", GINT_ERR_PARSE, "trailing comma is not allowed in list literal"},
      {"items = [1 2]\n", GINT_ERR_PARSE, "expected ',' or ']' after list element"},
      {"items = [1, 2]\nprint(items[0)\n", GINT_ERR_PARSE, "expected ']' after index expression"},
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
