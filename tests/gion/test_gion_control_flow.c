/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_if_elif_else_control_flow(void) {
  const char *source =
      "flag = true\n"
      "other = false\n"
      "nested = false\n"
      "if false:\n"
      "    selected = \"bad\"\n"
      "elif flag:\n"
      "    selected = \"if branch\"\n"
      "else:\n"
      "    selected = \"fallback\"\n"
      "if other:\n"
      "    optional = \"bad\"\n"
      "else:\n"
      "    optional = \"if else without elif\"\n"
      "if false:\n"
      "    single = \"bad\"\n"
      "single = \"if without else stays optional\"\n"
      "if 1:\n"
      "    int_true_branch = \"int one acts like true\"\n"
      "if 0:\n"
      "    int_false_branch = \"bad\"\n"
      "else:\n"
      "    int_false_branch = \"int zero acts like false\"\n"
      "if 1 + 1 == 2:\n"
      "    equality_branch = \"equality condition works\"\n"
      "else:\n"
      "    equality_branch = \"bad\"\n"
      "if 2 + 2 != 5:\n"
      "    inequality_branch = \"inequality condition works\"\n"
      "else:\n"
      "    inequality_branch = \"bad\"\n"
      "if 2 < 3:\n"
      "    less_than_branch = \"less-than condition works\"\n"
      "else:\n"
      "    less_than_branch = \"bad\"\n"
      "if 3 <= 3:\n"
      "    less_equal_branch = \"less-equal condition works\"\n"
      "else:\n"
      "    less_equal_branch = \"bad\"\n"
      "if 4 > 3:\n"
      "    greater_than_branch = \"greater-than condition works\"\n"
      "else:\n"
      "    greater_than_branch = \"bad\"\n"
      "if 4 >= 4:\n"
      "    greater_equal_branch = \"greater-equal condition works\"\n"
      "else:\n"
      "    greater_equal_branch = \"bad\"\n"
      "if true and 1:\n"
      "    and_branch = \"and condition works\"\n"
      "else:\n"
      "    and_branch = \"bad\"\n"
      "if true nand 1:\n"
      "    nand_branch = \"bad\"\n"
      "else:\n"
      "    nand_branch = \"nand condition works\"\n"
      "if false or 1:\n"
      "    or_branch = \"or condition works\"\n"
      "else:\n"
      "    or_branch = \"bad\"\n"
      "if false nor 0:\n"
      "    nor_branch = \"nor condition works\"\n"
      "else:\n"
      "    nor_branch = \"bad\"\n"
      "if not false:\n"
      "    not_branch = \"not condition works\"\n"
      "else:\n"
      "    not_branch = \"bad\"\n"
      "if (\n"
      "    flag and\n"
      "    2 < 3 and\n"
      "    not false\n"
      "):\n"
      "    multiline_branch = \"multiline condition works\"\n"
      "else:\n"
      "    multiline_branch = \"bad\"\n"
      "if nested:\n"
      "    nested_result = \"bad\"\n"
      "elif false:\n"
      "    nested_result = \"bad2\"\n"
      "elif true:\n"
      "    if flag:\n"
      "        nested_result = \"nested if branch\"\n"
      "    else:\n"
      "        nested_result = \"nested else branch\"\n"
      "else:\n"
      "    nested_result = \"bad3\"\n"
      "print(selected)\n"
      "print(optional)\n"
      "print(single)\n"
      "print(equality_branch)\n"
      "print(inequality_branch)\n"
      "print(less_than_branch)\n"
      "print(less_equal_branch)\n"
      "print(greater_than_branch)\n"
      "print(greater_equal_branch)\n"
      "print(and_branch)\n"
      "print(nand_branch)\n"
      "print(or_branch)\n"
      "print(nor_branch)\n"
      "print(not_branch)\n"
      "print(multiline_branch)\n"
      "print(nested_result)\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *selected;
  const graphion_runtime_value *optional;
  const graphion_runtime_value *single;
  const graphion_runtime_value *int_true_branch;
  const graphion_runtime_value *int_false_branch;
  const graphion_runtime_value *equality_branch;
  const graphion_runtime_value *inequality_branch;
  const graphion_runtime_value *less_than_branch;
  const graphion_runtime_value *less_equal_branch;
  const graphion_runtime_value *greater_than_branch;
  const graphion_runtime_value *greater_equal_branch;
  const graphion_runtime_value *and_branch;
  const graphion_runtime_value *nand_branch;
  const graphion_runtime_value *or_branch;
  const graphion_runtime_value *nor_branch;
  const graphion_runtime_value *not_branch;
  const graphion_runtime_value *multiline_branch;
  const graphion_runtime_value *nested_result;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_if_elif_else_control_flow.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  selected = graphion_runtime_scope_find(&scope, "selected");
  optional = graphion_runtime_scope_find(&scope, "optional");
  single = graphion_runtime_scope_find(&scope, "single");
  int_true_branch = graphion_runtime_scope_find(&scope, "int_true_branch");
  int_false_branch = graphion_runtime_scope_find(&scope, "int_false_branch");
  equality_branch = graphion_runtime_scope_find(&scope, "equality_branch");
  inequality_branch = graphion_runtime_scope_find(&scope, "inequality_branch");
  less_than_branch = graphion_runtime_scope_find(&scope, "less_than_branch");
  less_equal_branch = graphion_runtime_scope_find(&scope, "less_equal_branch");
  greater_than_branch = graphion_runtime_scope_find(&scope, "greater_than_branch");
  greater_equal_branch = graphion_runtime_scope_find(&scope, "greater_equal_branch");
  and_branch = graphion_runtime_scope_find(&scope, "and_branch");
  nand_branch = graphion_runtime_scope_find(&scope, "nand_branch");
  or_branch = graphion_runtime_scope_find(&scope, "or_branch");
  nor_branch = graphion_runtime_scope_find(&scope, "nor_branch");
  not_branch = graphion_runtime_scope_find(&scope, "not_branch");
  multiline_branch = graphion_runtime_scope_find(&scope, "multiline_branch");
  nested_result = graphion_runtime_scope_find(&scope, "nested_result");
  if (selected == NULL || selected->kind != GVM_VALUE_STRING || strcmp(selected->as.string_value, "if branch") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (optional == NULL || optional->kind != GVM_VALUE_STRING ||
      strcmp(optional->as.string_value, "if else without elif") != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (single == NULL || single->kind != GVM_VALUE_STRING ||
      strcmp(single->as.string_value, "if without else stays optional") != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (int_true_branch == NULL || int_true_branch->kind != GVM_VALUE_STRING ||
      strcmp(int_true_branch->as.string_value, "int one acts like true") != 0) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (int_false_branch == NULL || int_false_branch->kind != GVM_VALUE_STRING ||
      strcmp(int_false_branch->as.string_value, "int zero acts like false") != 0) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (equality_branch == NULL || equality_branch->kind != GVM_VALUE_STRING ||
      strcmp(equality_branch->as.string_value, "equality condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (inequality_branch == NULL || inequality_branch->kind != GVM_VALUE_STRING ||
      strcmp(inequality_branch->as.string_value, "inequality condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (less_than_branch == NULL || less_than_branch->kind != GVM_VALUE_STRING ||
      strcmp(less_than_branch->as.string_value, "less-than condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  if (less_equal_branch == NULL || less_equal_branch->kind != GVM_VALUE_STRING ||
      strcmp(less_equal_branch->as.string_value, "less-equal condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 11);
  }
  if (greater_than_branch == NULL || greater_than_branch->kind != GVM_VALUE_STRING ||
      strcmp(greater_than_branch->as.string_value, "greater-than condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 12);
  }
  if (greater_equal_branch == NULL || greater_equal_branch->kind != GVM_VALUE_STRING ||
      strcmp(greater_equal_branch->as.string_value, "greater-equal condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 13);
  }
  if (and_branch == NULL || and_branch->kind != GVM_VALUE_STRING ||
      strcmp(and_branch->as.string_value, "and condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 14);
  }
  if (nand_branch == NULL || nand_branch->kind != GVM_VALUE_STRING ||
      strcmp(nand_branch->as.string_value, "nand condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 15);
  }
  if (or_branch == NULL || or_branch->kind != GVM_VALUE_STRING ||
      strcmp(or_branch->as.string_value, "or condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 16);
  }
  if (nor_branch == NULL || nor_branch->kind != GVM_VALUE_STRING ||
      strcmp(nor_branch->as.string_value, "nor condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 17);
  }
  if (not_branch == NULL || not_branch->kind != GVM_VALUE_STRING ||
      strcmp(not_branch->as.string_value, "not condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 18);
  }
  if (multiline_branch == NULL || multiline_branch->kind != GVM_VALUE_STRING ||
      strcmp(multiline_branch->as.string_value, "multiline condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 19);
  }
  if (nested_result == NULL || nested_result->kind != GVM_VALUE_STRING ||
      strcmp(nested_result->as.string_value, "nested if branch") != 0) {
    remove(path);
    return finish_scope_test(&scope, 20);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 21);
  }
  remove(path);
  if (strcmp(output, "if branch\nif else without elif\nif without else stays optional\nequality condition works\ninequality condition works\nless-than condition works\nless-equal condition works\ngreater-than condition works\ngreater-equal condition works\nand condition works\nnand condition works\nor condition works\nnor condition works\nnot condition works\nmultiline condition works\nnested if branch\n") != 0) {
    return finish_scope_test(&scope, 22);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_if_elif_else_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_line;
    const char *message;
  } cases[] = {
      {"if true\n    print(1)\n", GINT_ERR_PARSE, 1U, "expected ':' after if condition"},
      {"if :\n    print(1)\n", GINT_ERR_PARSE, 1U, "expected condition after if"},
      {"if true: extra\n    print(1)\n", GINT_ERR_PARSE, 1U, "unexpected trailing tokens after condition"},
      {"elif true:\n    print(1)\n", GINT_ERR_PARSE, 1U, "elif without matching if"},
      {"elif :\n    print(1)\n", GINT_ERR_PARSE, 1U, "elif without matching if"},
      {"else:\n    print(1)\n", GINT_ERR_PARSE, 1U, "else without matching if"},
      {"if true:\nprint(1)\n", GINT_ERR_PARSE, 1U, "expected indented block after if"},
      {"if false:\n    print(1)\nelif true:\nprint(2)\n", GINT_ERR_PARSE, 3U, "expected indented block after elif"},
      {"if false:\n    print(1)\nelse:\nprint(2)\n", GINT_ERR_PARSE, 3U, "expected indented block after else"},
      {"if true:\n    print(1)\n  print(2)\n", GINT_ERR_PARSE, 3U, "unexpected indentation"},
      {"if true:\n    if false:\n        print(1)\n      print(2)\n", GINT_ERR_PARSE, 4U, "unexpected indentation"},
      {"if true:\n    elif false:\n        print(1)\n", GINT_ERR_PARSE, 2U, "elif without matching if"},
      {"if true:\n    else:\n        print(1)\n", GINT_ERR_PARSE, 2U, "else without matching if"},
      {"if 2:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if -1:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if 0.0:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if 1.5:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if \"x\":\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if 0b10:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if abs(2):\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if true and\n    false:\n    print(1)\n", GINT_ERR_PARSE, 1U, "multiline condition requires grouping parentheses"},
      {"if (\n    true and\n    false:\n    print(1)\n", GINT_ERR_PARSE, 1U, "expected ':' after if condition"},
      {"if false:\n    print(1)\nelif true and\n    false:\n    print(2)\n", GINT_ERR_PARSE, 3U, "multiline condition requires grouping parentheses"},
      {"if false:\n    print(1)\nelif (\n    true and\n    false:\n    print(2)\n", GINT_ERR_PARSE, 3U, "expected ':' after elif condition"},
      {"flag = true\nif flag:\n    print(1)\nelse:\n    print(2)\nelif false:\n    print(3)\n", GINT_ERR_PARSE, 6U, "else must be last in if chain"},
      {"if false:\n    print(1)\nelse:\n    print(2)\nelse:\n    print(3)\n", GINT_ERR_PARSE, 5U, "else must be last in if chain"},
      {"if false:\n    print(1)\nelif true\n    print(2)\n", GINT_ERR_PARSE, 3U, "expected ':' after elif condition"},
      {"if false:\n    print(1)\nelif :\n    print(2)\n", GINT_ERR_PARSE, 3U, "expected condition after elif"},
      {"if false:\n    print(1)\nelif true: extra\n    print(2)\n", GINT_ERR_PARSE, 3U, "unexpected trailing tokens after condition"},
      {"if false:\n    print(1)\nelse\n    print(2)\n", GINT_ERR_PARSE, 3U, "expected ':' after else"},
      {"if false:\n    print(1)\nelse: extra\n    print(2)\n", GINT_ERR_PARSE, 3U, "unexpected trailing tokens after else"},
      {"if nope:\n    print(1)\n", GINT_ERR_UNKNOWN_OPERAND, 1U, "unknown operand 'nope'"},
      {"flag = false\nif flag:\n    print(1)\nelif nope:\n    print(2)\nelse:\n    print(3)\n", GINT_ERR_UNKNOWN_OPERAND, 4U, "unknown operand 'nope'"},
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
    if (cases[i].expected_rc == GINT_OK) {
      graphion_runtime_scope_dispose(&scope);
      continue;
    }
    if (diagnostic.line != cases[i].expected_line) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_match_control_flow(void) {
  const char *source =
      "status = \"ready\"\n"
      "level = 2\n"
      "flag = true\n"
      "branch_string = \"unset\"\n"
      "grouped_number = \"unset\"\n"
      "bool_branch = \"unset\"\n"
      "nested = \"unset\"\n"
      "no_default = \"unchanged\"\n"
      "numeric_match = \"unset\"\n"
      "bool_int_match = \"unset\"\n"
      "match status:\n"
      "    \"waiting\":\n"
      "        branch_string = \"hold\"\n"
      "    \"ready\":\n"
      "        branch_string = \"go\"\n"
      "    default:\n"
      "        branch_string = \"other\"\n"
      "match level:\n"
      "    1:\n"
      "    2:\n"
      "        grouped_number = \"small\"\n"
      "    3:\n"
      "        grouped_number = \"three\"\n"
      "    default:\n"
      "        grouped_number = \"other\"\n"
      "match flag:\n"
      "    false:\n"
      "        bool_branch = \"false\"\n"
      "    true:\n"
      "        bool_branch = \"true\"\n"
      "if true:\n"
      "    match 1:\n"
      "        0:\n"
      "            nested = \"bad\"\n"
      "        1:\n"
      "            nested = \"match in if\"\n"
      "match 0:\n"
      "    1:\n"
      "        no_default = \"bad\"\n"
      "match 1:\n"
      "    1.0:\n"
      "        numeric_match = \"float matches int\"\n"
      "match true:\n"
      "    1:\n"
      "        bool_int_match = \"bool matches one\"\n"
      "print(branch_string)\n"
      "print(grouped_number)\n"
      "print(bool_branch)\n"
      "print(nested)\n"
      "print(no_default)\n"
      "print(numeric_match)\n"
      "print(bool_int_match)\n";
  char path[512];
  char output[256];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *branch_string;
  const graphion_runtime_value *grouped_number;
  const graphion_runtime_value *bool_branch;
  const graphion_runtime_value *nested;
  const graphion_runtime_value *no_default;
  const graphion_runtime_value *numeric_match;
  const graphion_runtime_value *bool_int_match;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_match_control_flow.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }

  branch_string = graphion_runtime_scope_find(&scope, "branch_string");
  grouped_number = graphion_runtime_scope_find(&scope, "grouped_number");
  bool_branch = graphion_runtime_scope_find(&scope, "bool_branch");
  nested = graphion_runtime_scope_find(&scope, "nested");
  no_default = graphion_runtime_scope_find(&scope, "no_default");
  numeric_match = graphion_runtime_scope_find(&scope, "numeric_match");
  bool_int_match = graphion_runtime_scope_find(&scope, "bool_int_match");

  if (branch_string == NULL || branch_string->kind != GVM_VALUE_STRING ||
      strcmp(branch_string->as.string_value, "go") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (grouped_number == NULL || grouped_number->kind != GVM_VALUE_STRING ||
      strcmp(grouped_number->as.string_value, "small") != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (bool_branch == NULL || bool_branch->kind != GVM_VALUE_STRING ||
      strcmp(bool_branch->as.string_value, "true") != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (nested == NULL || nested->kind != GVM_VALUE_STRING || strcmp(nested->as.string_value, "match in if") != 0) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (no_default == NULL || no_default->kind != GVM_VALUE_STRING ||
      strcmp(no_default->as.string_value, "unchanged") != 0) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (numeric_match == NULL || numeric_match->kind != GVM_VALUE_STRING ||
      strcmp(numeric_match->as.string_value, "float matches int") != 0) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (bool_int_match == NULL || bool_int_match->kind != GVM_VALUE_STRING ||
      strcmp(bool_int_match->as.string_value, "bool matches one") != 0) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  remove(path);
  if (strcmp(output, "go\nsmall\ntrue\nmatch in if\nunchanged\nfloat matches int\nbool matches one\n") != 0) {
    return finish_scope_test(&scope, 11);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_match_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_line;
    const char *message;
  } cases[] = {
      {"match:\n    1:\n        print(1)\n", GINT_ERR_PARSE, 1U, "expected expression after match"},
      {"match 1\n    1:\n        print(1)\n", GINT_ERR_PARSE, 1U, "expected ':' after match expression"},
      {"match 1: extra\n    1:\n        print(1)\n", GINT_ERR_PARSE, 1U, "unexpected trailing tokens after match"},
      {"match 1:\nprint(1)\n", GINT_ERR_PARSE, 1U, "expected indented match block"},
      {"default:\n    print(1)\n", GINT_ERR_PARSE, 1U, "default without matching match"},
      {"match 1:\n    1\n        print(1)\n", GINT_ERR_PARSE, 2U, "expected ':' after match case"},
      {"match 1:\n    1: extra\n        print(1)\n", GINT_ERR_PARSE, 2U, "unexpected trailing tokens after match case"},
      {"match 1:\n    default\n        print(1)\n", GINT_ERR_PARSE, 2U, "expected ':' after default"},
      {"match 1:\n    1:\n    2:\nprint(1)\n", GINT_ERR_PARSE, 2U, "expected indented block after match case"},
      {"match 1:\n    1:\n        print(1)\n    1:\n        print(2)\n", GINT_ERR_PARSE, 4U, "duplicate match case"},
      {"match 1:\n    1:\n        print(1)\n    1.0:\n        print(2)\n", GINT_ERR_PARSE, 4U, "duplicate match case"},
      {"match 1:\n    default:\n        print(1)\n    2:\n        print(2)\n", GINT_ERR_PARSE, 2U, "default must be last in match"},
      {"match 1:\n    default:\n        print(1)\n    default:\n        print(2)\n", GINT_ERR_PARSE, 2U, "default must be last in match"},
      {"match nope:\n    1:\n        print(1)\n", GINT_ERR_UNKNOWN_OPERAND, 1U, "unknown operand 'nope'"},
      {"match 1:\n    abs(1):\n        print(1)\n", GINT_ERR_PARSE, 2U, "expected scalar literal"},
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
    if (diagnostic.line != cases[i].expected_line) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_ternary_expressions(void) {
  const char *source =
      "ready = true\n"
      "fallback = false\n"
      "label = \"ready\" if ready else \"not ready\"\n"
      "int_label = \"int true\" if 1 else \"bad\"\n"
      "compare_label = \"compare true\" if 2 < 3 else \"bad\"\n"
      "logic_label = \"logic true\" if true and 1 else \"bad\"\n"
      "nested = \"outer true\" if true else \"inner true\" if false else \"inner false\"\n"
      "grouped = (\"grouped true\" if false else \"grouped false\")\n"
      "multiline = (\n"
      "    \"ready multi\"\n"
      "    if ready\n"
      "    else \"not ready multi\"\n"
      ")\n"
      "print(label)\n"
      "print(int_label)\n"
      "print(compare_label)\n"
      "print(logic_label)\n"
      "print(nested)\n"
      "print(grouped)\n"
      "print(multiline)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *label;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_ternary_expressions.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  label = graphion_runtime_scope_find(&scope, "label");
  if (label == NULL || label->kind != GVM_VALUE_STRING || strcmp(label->as.string_value, "ready") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "ready\nint true\ncompare true\nlogic true\nouter true\ngrouped false\nready multi\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_ternary_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = \"ready\" if 2 else \"bad\"\n", 1U},
      {"value = \"ready\" if 1.0 else \"bad\"\n", 1U},
      {"value = \"ready\" if \"x\" else \"bad\"\n", 1U},
      {"value = \"ready\" if 0b10 else \"bad\"\n", 1U},
      {"value = \"ready\" if 2 and true else \"bad\"\n", 1U},
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

int test_gion_ternary_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"value = \"ready\" if ready\n", GINT_ERR_PARSE, "expected else in ternary expression"},
      {"value = if ready else \"bad\"\n", GINT_ERR_PARSE, "expected expression before ternary if"},
      {"value = \"ready\" if else \"bad\"\n", GINT_ERR_PARSE, "expected condition after ternary if"},
      {"value = \"ready\" if ready else\n", GINT_ERR_PARSE, "expected expression after ternary else"},
      {"value = \"ready\" if\n    ready else \"bad\"\n", GINT_ERR_PARSE, "multiline assignment expression requires grouping parentheses"},
      {"value = \"ready\" if ready else\n    \"bad\"\n", GINT_ERR_PARSE, "multiline assignment expression requires grouping parentheses"},
      {"value = (\n    \"ready\"\n    if ready\n    else \"bad\"\n", GINT_ERR_PARSE, "expected ')' after expression"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_boolean_short_circuit(void) {
  const char *source =
      "safe_and = false and 2\n"
      "safe_or = true or 2\n"
      "safe_nand = false nand 2\n"
      "safe_nor = true nor 2\n"
      "mixed_safe = true or false and 2\n"
      "print(safe_and)\n"
      "print(safe_or)\n"
      "print(safe_nand)\n"
      "print(safe_nor)\n"
      "print(mixed_safe)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *safe_and;
  const graphion_runtime_value *safe_or;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_boolean_short_circuit.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  safe_and = graphion_runtime_scope_find(&scope, "safe_and");
  safe_or = graphion_runtime_scope_find(&scope, "safe_or");
  if (safe_and == NULL || safe_and->kind != GVM_VALUE_BOOL || safe_and->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (safe_or == NULL || safe_or->kind != GVM_VALUE_BOOL || safe_or->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "false\ntrue\ntrue\nfalse\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_boolean_short_circuit_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = true and 2\n", 1U},
      {"value = false or 2\n", 1U},
      {"value = true nand 2\n", 1U},
      {"value = false nor 2\n", 1U},
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
