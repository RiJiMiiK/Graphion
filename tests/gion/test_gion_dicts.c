/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_dict_literals_and_prints(void) {
  const char *source =
      "name = \"answer\"\n"
      "data = {\"a\": 1, \"b\": 2, \"name\": name}\n"
      "data[\"c\"] = 3\n"
      "alias = \"name\"\n"
      "data[alias] = \"updated\"\n"
      "nested = {\"inner\": data, \"empty\": {}}\n"
      "second = data[\"b\"]\n"
      "named = data[\"name\"]\n"
      "same = data == {\"b\": 2, \"name\": \"updated\", \"a\": 1, \"c\": 3}\n"
      "different = data != {\"a\": 1}\n"
      "size = len(data)\n"
      "print(data)\n"
      "print(nested)\n"
      "print(second)\n"
      "print(named)\n"
      "print(same)\n"
      "print(different)\n"
      "print(size)\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *data;
  const graphion_runtime_value *nested;
  const graphion_runtime_value *second;
  const graphion_runtime_value *named;
  const graphion_runtime_value *same;
  const graphion_runtime_value *different;
  const graphion_runtime_value *size;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_dict_literals_and_prints.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  data = graphion_runtime_scope_find(&scope, "data");
  nested = graphion_runtime_scope_find(&scope, "nested");
  second = graphion_runtime_scope_find(&scope, "second");
  named = graphion_runtime_scope_find(&scope, "named");
  same = graphion_runtime_scope_find(&scope, "same");
  different = graphion_runtime_scope_find(&scope, "different");
  size = graphion_runtime_scope_find(&scope, "size");
  if (data == NULL || data->kind != GVM_VALUE_DICT) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (nested == NULL || nested->kind != GVM_VALUE_DICT) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (second == NULL || second->kind != GVM_VALUE_INT || second->as.int_value != 2) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (named == NULL || named->kind != GVM_VALUE_STRING || named->as.string_value == NULL ||
      strcmp(named->as.string_value, "updated") != 0) {
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
  if (size == NULL || size->kind != GVM_VALUE_INT || size->as.int_value != 4) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  remove(path);
  if (strcmp(output,
             "{\"a\": 1, \"b\": 2, \"name\": \"updated\", \"c\": 3}\n"
             "{\"inner\": {\"a\": 1, \"b\": 2, \"name\": \"updated\", \"c\": 3}, \"empty\": {}}\n"
             "2\n"
             "updated\n"
             "true\n"
             "true\n"
             "4\n") != 0) {
    return finish_scope_test(&scope, 11);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_dict_runtime_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"data = {\"a\": 1}\nprint(data[\"missing\"])\n", GINT_ERR_RUN, "dict key not found"},
      {"data = {\"a\": 1}\nprint(data[1])\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = 1\nprint(value[\"a\"])\n", GINT_ERR_RUN, "incompatible operand types"},
      {"data = {\"a\": 1}\nprint(data == [1])\n", GINT_ERR_RUN, "incompatible operand types"},
      {"data = {\"a\": 1}\ndata[1] = 2\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = 1\nvalue[\"a\"] = 2\n", GINT_ERR_RUN, "incompatible operand types"},
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
      return (int)(500 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(600 + i);
    }
  }
  return 0;
}

int test_gion_dict_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"data = {a: 1}\n", GINT_ERR_PARSE, "dict literal keys must be string literals"},
      {"data = {\"a\" 1}\n", GINT_ERR_PARSE, "expected ':' after dict key"},
      {"data = {\"a\": 1,}\n", GINT_ERR_PARSE, "trailing comma is not allowed in dict literal"},
      {"data = {\"a\": 1 \"b\": 2}\n", GINT_ERR_PARSE, "expected ',' or '}' after dict entry"},
      {"data = {\"a\": 1}\ndata[\"a\" = 2\n", GINT_ERR_PARSE, "expected ']' after assignment target index"},
      {"data = {\"a\": 1}\ndata[\"a\"] += 2\n", GINT_ERR_PARSE, "compound indexed assignment is not supported"},
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
      return (int)(700 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(800 + i);
    }
  }
  return 0;
}
