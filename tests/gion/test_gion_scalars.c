/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_string_concatenation(void) {
  const char *source =
      "label = \"debut\" + \"fin\"\n"
      "full = label + \"!\"\n"
      "print(label)\n"
      "print(full)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *label;
  const graphion_runtime_value *full;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_string_concatenation.txt");
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
  full = graphion_runtime_scope_find(&scope, "full");
  if (label == NULL || label->kind != GVM_VALUE_STRING || strcmp(label->as.string_value, "debutfin") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (full == NULL || full->kind != GVM_VALUE_STRING || strcmp(full->as.string_value, "debutfin!") != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "debutfin\ndebutfin!\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_print_string_coercion(void) {
  const char *source =
      "name = \"Test \"\n"
      "print(\"Test \" + 7)\n"
      "print(name + 7)\n"
      "print(\"value=\" + (3 + 4))\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_print_string_coercion.txt");
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
  if (strcmp(output, "Test 7\nTest 7\nvalue=7\n") != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_compound_assignments(void) {
  const char *source =
      "count = 10\n"
      "count += 5\n"
      "count -= 3\n"
      "count *= 4\n"
      "count /= 3\n"
      "count //= 2\n"
      "count %= 7\n"
      "count **= 3\n"
      "text = \"debut\"\n"
      "text += \"fin\"\n"
      "mask = 0b1100\n"
      "mask &= 0b1010\n"
      "merge = 0b1100\n"
      "merge |= 0b0011\n"
      "flip = 0b1100\n"
      "flip ^= 0b1010\n"
      "shift = 0b0011\n"
      "shift <<= 1\n"
      "shift_overflow = 0b1111\n"
      "shift_overflow <<= 1\n"
      "shift_right = 0b1010\n"
      "shift_right >>= 1\n"
      "shift_right_clear = 0b1010\n"
      "shift_right_clear >>= 4\n"
      "print(count)\n"
      "print(text)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *count;
  const graphion_runtime_value *text;
  const graphion_runtime_value *mask;
  const graphion_runtime_value *merge;
  const graphion_runtime_value *flip;
  const graphion_runtime_value *shift;
  const graphion_runtime_value *shift_overflow;
  const graphion_runtime_value *shift_right;
  const graphion_runtime_value *shift_right_clear;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_compound_assignments.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  count = graphion_runtime_scope_find(&scope, "count");
  text = graphion_runtime_scope_find(&scope, "text");
  mask = graphion_runtime_scope_find(&scope, "mask");
  merge = graphion_runtime_scope_find(&scope, "merge");
  flip = graphion_runtime_scope_find(&scope, "flip");
  shift = graphion_runtime_scope_find(&scope, "shift");
  shift_overflow = graphion_runtime_scope_find(&scope, "shift_overflow");
  shift_right = graphion_runtime_scope_find(&scope, "shift_right");
  shift_right_clear = graphion_runtime_scope_find(&scope, "shift_right_clear");
  if (count == NULL || count->kind != GVM_VALUE_FLOAT || count->as.float_value != 1.0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (text == NULL || text->kind != GVM_VALUE_STRING || strcmp(text->as.string_value, "debutfin") != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (mask == NULL || mask->kind != GVM_VALUE_BITS || mask->reserved[0] != 4U ||
      (uint64_t)mask->as.int_value != 0x8ULL) {
    return finish_scope_test(&scope, 6);
  }
  if (merge == NULL || merge->kind != GVM_VALUE_BITS || merge->reserved[0] != 4U ||
      (uint64_t)merge->as.int_value != 0xFULL) {
    return finish_scope_test(&scope, 7);
  }
  if (flip == NULL || flip->kind != GVM_VALUE_BITS || flip->reserved[0] != 4U ||
      (uint64_t)flip->as.int_value != 0x6ULL) {
    return finish_scope_test(&scope, 8);
  }
  if (shift == NULL || shift->kind != GVM_VALUE_BITS || shift->reserved[0] != 4U ||
      (uint64_t)shift->as.int_value != 0x6ULL) {
    return finish_scope_test(&scope, 9);
  }
  if (shift_overflow == NULL || shift_overflow->kind != GVM_VALUE_BITS || shift_overflow->reserved[0] != 4U ||
      (uint64_t)shift_overflow->as.int_value != 0xEULL) {
    return finish_scope_test(&scope, 10);
  }
  if (shift_right == NULL || shift_right->kind != GVM_VALUE_BITS || shift_right->reserved[0] != 4U ||
      (uint64_t)shift_right->as.int_value != 0x5ULL) {
    return finish_scope_test(&scope, 11);
  }
  if (shift_right_clear == NULL || shift_right_clear->kind != GVM_VALUE_BITS || shift_right_clear->reserved[0] != 4U ||
      (uint64_t)shift_right_clear->as.int_value != 0x0ULL) {
    return finish_scope_test(&scope, 12);
  }
  if (strcmp(output, "1\ndebutfin\n") != 0) {
    return finish_scope_test(&scope, 13);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_compound_assignment_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"count += 1\n", GINT_ERR_UNKNOWN_VARIABLE, 1U, "unknown variable 'count'"},
      {"count = 1\ncount +=\n", GINT_ERR_PARSE, 7U, "expected expression after '+='"},
      {"count = 1\ncount -=\n", GINT_ERR_PARSE, 7U, "expected expression after '-='"},
      {"count = 1\ncount *=\n", GINT_ERR_PARSE, 7U, "expected expression after '*='"},
      {"count = 1\ncount /=\n", GINT_ERR_PARSE, 7U, "expected expression after '/='"},
      {"count = 1\ncount //=\n", GINT_ERR_PARSE, 7U, "expected expression after '//='"},
      {"count = 1\ncount %=\n", GINT_ERR_PARSE, 7U, "expected expression after '%='"},
      {"count = 1\ncount **=\n", GINT_ERR_PARSE, 7U, "expected expression after '**='"},
      {"mask = 0b11\nmask &=\n", GINT_ERR_PARSE, 6U, "expected expression after '&='"},
      {"merge = 0b11\nmerge |=\n", GINT_ERR_PARSE, 7U, "expected expression after '|='"},
      {"flip = 0b11\nflip ^=\n", GINT_ERR_PARSE, 6U, "expected expression after '^='"},
      {"shift = 0b11\nshift <<=\n", GINT_ERR_PARSE, 7U, "expected expression after '<<='"},
      {"shift = 0b11\nshift >>=\n", GINT_ERR_PARSE, 7U, "expected expression after '>>='"},
      {"count = 1\ncount /= 0\n", GINT_ERR_RUN, 0U, "division by zero"},
      {"count = 1\ncount //= 0\n", GINT_ERR_RUN, 0U, "division by zero"},
      {"count = 1\ncount = count // 0\n", GINT_ERR_RUN, 0U, "division by zero"},
      {"count = 1\ncount %= 0\n", GINT_ERR_RUN, 0U, "division by zero"},
      {"count = 2\ncount **= \"x\"\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"count = 1\ncount += \"x\"\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"mask = 0b10\nmask &= 0b0010\n",
       GINT_ERR_RUN,
       0U,
       "bitwise operations require matching bits widths"},
      {"mask = 0b10\nmask &= 1\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"merge = 0b10\nmerge |= 0b0010\n",
       GINT_ERR_RUN,
       0U,
       "bitwise operations require matching bits widths"},
      {"merge = 0b10\nmerge |= 1\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"flip = 0b10\nflip ^= 0b0010\n",
       GINT_ERR_RUN,
       0U,
       "bitwise operations require matching bits widths"},
      {"flip = 0b10\nflip ^= 1\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"shift = 0b10\nshift <<= 0b0010\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"shift = 0b10\nshift <<= 1.0\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"shift = 0b10\nshift <<= -1\n",
       GINT_ERR_RUN,
       0U,
       "bit shifts require non-negative integer counts"},
      {"shift = 0b10\nshift >>= 0b0010\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"shift = 0b10\nshift >>= 1.0\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"shift = 0b10\nshift >>= -1\n",
       GINT_ERR_RUN,
       0U,
       "bit shifts require non-negative integer counts"},
      {"value = \"Test \" + 7\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"text = \"x\"\ntext -= \"y\"\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"text = \"x\"\ntext *= 2\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"text = \"x\"\ntext /= 2\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
      {"text = \"x\"\ntext %= 2\n", GINT_ERR_RUN, 0U, "incompatible operand types"},
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
    if (cases[i].expected_column != 0U && diagnostic.column != cases[i].expected_column) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}
