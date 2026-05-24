/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_set_literals_and_prints(void) {
  const char *source =
      "frontier = set(1, 2, 2, \"a\")\n"
      "other = set(\"a\", 2, 1)\n"
      "empty = set()\n"
      "has_two = contains(frontier, 2)\n"
      "has_three = contains(frontier, 3)\n"
      "same = frontier == other\n"
      "different = frontier != set(1, 2)\n"
      "size = len(frontier)\n"
      "print(frontier)\n"
      "print(empty)\n"
      "print(has_two)\n"
      "print(has_three)\n"
      "print(same)\n"
      "print(different)\n"
      "print(size)\n";
  char path[512];
  char output[256];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *frontier;
  const graphion_runtime_value *empty;
  const graphion_runtime_value *has_two;
  const graphion_runtime_value *has_three;
  const graphion_runtime_value *same;
  const graphion_runtime_value *different;
  const graphion_runtime_value *size;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_set_literals_and_prints.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  frontier = graphion_runtime_scope_find(&scope, "frontier");
  empty = graphion_runtime_scope_find(&scope, "empty");
  has_two = graphion_runtime_scope_find(&scope, "has_two");
  has_three = graphion_runtime_scope_find(&scope, "has_three");
  same = graphion_runtime_scope_find(&scope, "same");
  different = graphion_runtime_scope_find(&scope, "different");
  size = graphion_runtime_scope_find(&scope, "size");
  if (frontier == NULL || frontier->kind != GVM_VALUE_SET) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (empty == NULL || empty->kind != GVM_VALUE_SET) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (has_two == NULL || has_two->kind != GVM_VALUE_BOOL || has_two->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (has_three == NULL || has_three->kind != GVM_VALUE_BOOL || has_three->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (same == NULL || same->kind != GVM_VALUE_BOOL || same->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (different == NULL || different->kind != GVM_VALUE_BOOL || different->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (size == NULL || size->kind != GVM_VALUE_INT || size->as.int_value != 3) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  remove(path);
  if (strcmp(output,
             "set(1, 2, \"a\")\n"
             "set()\n"
             "true\n"
             "false\n"
             "true\n"
             "true\n"
             "3\n") != 0) {
    return finish_scope_test(&scope, 11);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_set_runtime_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"items = {\"a\": 1}\nprint(contains(items, 1))\n", GINT_ERR_RUN, "incompatible operand types"},
      {"items = set(1, 2)\nprint(items[0])\n", GINT_ERR_RUN, "incompatible operand types"},
      {"items = set(1, 2)\nprint(items == [1, 2])\n", GINT_ERR_RUN, "incompatible operand types"},
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

int test_gion_set_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"items = set\n", GINT_ERR_PARSE, "expected '(' after set"},
      {"items = set(1,)\n", GINT_ERR_PARSE, "trailing comma is not allowed in set literal"},
      {"items = set(1 2)\n", GINT_ERR_PARSE, "expected ',' or ')' after set element"},
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
