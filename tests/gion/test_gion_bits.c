/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_bits_literals(void) {
  const char *source =
      "bits_two = 0b10\n"
      "bits_four = 0b0010\n"
      "bits_copy = bits_four\n"
      "print(bits_two)\n"
      "print(bits_four)\n"
      "print(bits_copy)\n";
  const char *path = "gion_bits_literals.txt";
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *bits_two;
  const graphion_runtime_value *bits_four;
  const graphion_runtime_value *bits_copy;
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

  bits_two = graphion_runtime_scope_find(&scope, "bits_two");
  bits_four = graphion_runtime_scope_find(&scope, "bits_four");
  bits_copy = graphion_runtime_scope_find(&scope, "bits_copy");

  if (bits_two == NULL || bits_two->kind != GVM_VALUE_BITS || bits_two->reserved[0] != 2U || (uint64_t)bits_two->as.int_value != 2U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (bits_four == NULL || bits_four->kind != GVM_VALUE_BITS || bits_four->reserved[0] != 4U ||
      (uint64_t)bits_four->as.int_value != 2U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (bits_copy == NULL || bits_copy->kind != GVM_VALUE_BITS || bits_copy->reserved[0] != 4U ||
      (uint64_t)bits_copy->as.int_value != 2U) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  remove(path);
  if (strcmp(output, "0b10\n0b0010\n0b0010\n") != 0) {
    return finish_scope_test(&scope, 7);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_literal_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 0b\n", "expected binary digits after 0b"},
      {"value = 0b2\n", "expected binary digits after 0b"},
      {"value = 0b102\n", "invalid bits literal"},
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

int test_gion_bits_equality(void) {
  const char *source =
      "same_value = 0b10 == 0b0010\n"
      "same_copy = 0b0010 == 0b0010\n"
      "print(same_value)\n"
      "print(same_copy)\n";
  const char *path = "gion_bits_equality.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *same_value;
  const graphion_runtime_value *same_copy;
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

  same_value = graphion_runtime_scope_find(&scope, "same_value");
  same_copy = graphion_runtime_scope_find(&scope, "same_copy");
  if (same_value == NULL || same_value->kind != GVM_VALUE_BOOL || same_value->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (same_copy == NULL || same_copy->kind != GVM_VALUE_BOOL || same_copy->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_inequality(void) {
  const char *source =
      "different_value = 0b10 != 0b0011\n"
      "same_value = 0b10 != 0b0010\n"
      "print(different_value)\n"
      "print(same_value)\n";
  const char *path = "gion_bits_inequality.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *different_value;
  const graphion_runtime_value *same_value;
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

  different_value = graphion_runtime_scope_find(&scope, "different_value");
  same_value = graphion_runtime_scope_find(&scope, "same_value");
  if (different_value == NULL || different_value->kind != GVM_VALUE_BOOL || different_value->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (same_value == NULL || same_value->kind != GVM_VALUE_BOOL || same_value->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\nfalse\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_mixed_type_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 0b10 == 1\n", "incompatible operand types"},
      {"value = 0b10 != 1.0\n", "incompatible operand types"},
      {"value = 0b10 == true\n", "incompatible operand types"},
      {"value = 0b10 + 1\n", "incompatible operand types"},
      {"value = 0b10 - 1\n", "incompatible operand types"},
      {"value = 0b10 * 1\n", "incompatible operand types"},
      {"value = 0b10 / 1\n", "incompatible operand types"},
      {"value = 0b10 // 1\n", "incompatible operand types"},
      {"value = 0b10 % 1\n", "incompatible operand types"},
      {"value = 0b10 ** 1\n", "incompatible operand types"},
      {"value = \"x\" + 0b10\n", "incompatible operand types"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_bits_and(void) {
  const char *source =
      "masked_value = 0b1100 & 0b1010\n"
      "print(masked_value)\n";
  const char *path = "gion_bits_and.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *masked_value;
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

  masked_value = graphion_runtime_scope_find(&scope, "masked_value");
  if (masked_value == NULL || masked_value->kind != GVM_VALUE_BITS || masked_value->reserved[0] != 4U ||
      (uint64_t)masked_value->as.int_value != 8U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "0b1000\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_and_runtime_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 0b10 & 0b0010\n", "incompatible operand types"},
      {"value = 0b10 & 1\n", "incompatible operand types"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_bits_or(void) {
  const char *source =
      "merged_value = 0b1100 | 0b1010\n"
      "print(merged_value)\n";
  const char *path = "gion_bits_or.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *merged_value;
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

  merged_value = graphion_runtime_scope_find(&scope, "merged_value");
  if (merged_value == NULL || merged_value->kind != GVM_VALUE_BITS || merged_value->reserved[0] != 4U ||
      (uint64_t)merged_value->as.int_value != 14U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "0b1110\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_or_runtime_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 0b10 | 0b0010\n", "incompatible operand types"},
      {"value = 0b10 | 1\n", "incompatible operand types"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_bits_xor(void) {
  const char *source =
      "xor_value = 0b1100 ^ 0b1010\n"
      "print(xor_value)\n";
  const char *path = "gion_bits_xor.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *xor_value;
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

  xor_value = graphion_runtime_scope_find(&scope, "xor_value");
  if (xor_value == NULL || xor_value->kind != GVM_VALUE_BITS || xor_value->reserved[0] != 4U ||
      (uint64_t)xor_value->as.int_value != 6U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "0b0110\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_xor_runtime_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 0b10 ^ 0b0010\n", "incompatible operand types"},
      {"value = 0b10 ^ 1\n", "incompatible operand types"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_bits_not(void) {
  const char *source =
      "not_wide = ~0b0010\n"
      "not_short = ~0b10\n"
      "print(not_wide)\n"
      "print(not_short)\n";
  const char *path = "gion_bits_not.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *not_wide;
  const graphion_runtime_value *not_short;
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

  not_wide = graphion_runtime_scope_find(&scope, "not_wide");
  not_short = graphion_runtime_scope_find(&scope, "not_short");
  if (not_wide == NULL || not_wide->kind != GVM_VALUE_BITS || not_wide->reserved[0] != 4U ||
      (uint64_t)not_wide->as.int_value != 13U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (not_short == NULL || not_short->kind != GVM_VALUE_BITS || not_short->reserved[0] != 2U ||
      (uint64_t)not_short->as.int_value != 1U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "0b1101\n0b01\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_not_runtime_errors(void) {
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source("value = ~1\n", &scope, &diagnostic);
  if (rc != GINT_ERR_RUN) {
    return finish_scope_test(&scope, 1);
  }
  if (diagnostic.message == NULL || strcmp(diagnostic.message, "incompatible operand types") != 0) {
    return finish_scope_test(&scope, 2);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_shl(void) {
  const char *source =
      "shifted_value = 0b0011 << 1\n"
      "truncated_value = 0b1111 << 1\n"
      "print(shifted_value)\n"
      "print(truncated_value)\n";
  const char *path = "gion_bits_shl.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *shifted_value;
  const graphion_runtime_value *truncated_value;
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

  shifted_value = graphion_runtime_scope_find(&scope, "shifted_value");
  truncated_value = graphion_runtime_scope_find(&scope, "truncated_value");
  if (shifted_value == NULL || shifted_value->kind != GVM_VALUE_BITS || shifted_value->reserved[0] != 4U ||
      (uint64_t)shifted_value->as.int_value != 6U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (truncated_value == NULL || truncated_value->kind != GVM_VALUE_BITS || truncated_value->reserved[0] != 4U ||
      (uint64_t)truncated_value->as.int_value != 14U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "0b0110\n0b1110\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_shl_runtime_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 0b10 << 0b0010\n", "incompatible operand types"},
      {"value = 0b10 << 1.0\n", "incompatible operand types"},
      {"value = 0b10 << -1\n", "incompatible operand types"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_bits_shr(void) {
  const char *source =
      "shifted_value = 0b1010 >> 1\n"
      "cleared_value = 0b1010 >> 4\n"
      "print(shifted_value)\n"
      "print(cleared_value)\n";
  const char *path = "gion_bits_shr.txt";
  char output[32];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *shifted_value;
  const graphion_runtime_value *cleared_value;
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

  shifted_value = graphion_runtime_scope_find(&scope, "shifted_value");
  cleared_value = graphion_runtime_scope_find(&scope, "cleared_value");
  if (shifted_value == NULL || shifted_value->kind != GVM_VALUE_BITS || shifted_value->reserved[0] != 4U ||
      (uint64_t)shifted_value->as.int_value != 5U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (cleared_value == NULL || cleared_value->kind != GVM_VALUE_BITS || cleared_value->reserved[0] != 4U ||
      (uint64_t)cleared_value->as.int_value != 0U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "0b0101\n0b0000\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_bits_shr_runtime_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 0b10 >> 0b0010\n", "incompatible operand types"},
      {"value = 0b10 >> 1.0\n", "incompatible operand types"},
      {"value = 0b10 >> -1\n", "incompatible operand types"},
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
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}
