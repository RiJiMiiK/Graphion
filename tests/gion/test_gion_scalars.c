/* SPDX-License-Identifier: MIT */

#include "test_parser_helpers.h"

#include <math.h>

int test_gion_scalar_assignments_and_prints(void) {
  const char *source =
      "count = 42\n"
      "ratio = 3.5\n"
      "name = \"graphion\"\n"
      "ready = true\n"
      "copy = count\n"
      "print(7)\n"
      "print(\"raw\")\n"
      "print(false)\n"
      "print(count)\n"
      "print(ratio)\n"
      "print(name)\n"
      "print(ready)\n"
      "print(copy)\n";
  const char *path = "gion_scalar_assignments_and_prints.txt";
  char output[3072];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *count;
  const graphion_runtime_value *ratio;
  const graphion_runtime_value *name;
  const graphion_runtime_value *ready;
  const graphion_runtime_value *copy;
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
  count = graphion_runtime_scope_find(&scope, "count");
  ratio = graphion_runtime_scope_find(&scope, "ratio");
  name = graphion_runtime_scope_find(&scope, "name");
  ready = graphion_runtime_scope_find(&scope, "ready");
  copy = graphion_runtime_scope_find(&scope, "copy");
  if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (ratio == NULL || ratio->kind != GVM_VALUE_FLOAT || ratio->as.float_value != 3.5) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (name == NULL || name->kind != GVM_VALUE_STRING || strcmp(name->as.string_value, "graphion") != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (ready == NULL || ready->kind != GVM_VALUE_BOOL || ready->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (copy == NULL || copy->kind != GVM_VALUE_INT || copy->as.int_value != 42) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  remove(path);
  if (strcmp(output, "7\nraw\nfalse\n42\n3.5\ngraphion\ntrue\n42\n") != 0) {
    return finish_scope_test(&scope, 9);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_scalar_feature_varied_names(void) {
  const char *source =
      "alpha_1 = \"ok\"\n"
      "z_value = false\n"
      "n2 = -7\n"
      "copied_name = alpha_1\n"
      "shadow_0 = n2\n"
      "print(copied_name)\n"
      "print(z_value)\n"
      "print(shadow_0)\n";
  const char *path = "gion_scalar_feature_varied_names.txt";
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *copied_name;
  const graphion_runtime_value *shadow_0;
  const graphion_runtime_value *z_value;
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
  copied_name = graphion_runtime_scope_find(&scope, "copied_name");
  shadow_0 = graphion_runtime_scope_find(&scope, "shadow_0");
  z_value = graphion_runtime_scope_find(&scope, "z_value");
  if (copied_name == NULL || copied_name->kind != GVM_VALUE_STRING ||
      strcmp(copied_name->as.string_value, "ok") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (shadow_0 == NULL || shadow_0->kind != GVM_VALUE_INT || shadow_0->as.int_value != -7) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (z_value == NULL || z_value->kind != GVM_VALUE_BOOL || z_value->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  remove(path);
  if (strcmp(output, "ok\nfalse\n-7\n") != 0) {
    return finish_scope_test(&scope, 7);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_unknown_variable_errors(void) {
  const char *source = "copy = missing\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source(source, &scope, &diagnostic);
  if (rc != GINT_ERR_UNKNOWN_OPERAND) {
    return 1;
  }
  if (diagnostic.message == NULL) {
    return 2;
  }
  return strcmp(diagnostic.message, "unknown operand") == 0 ? 0 : 3;
}

int test_gion_partial_execution_stops_at_first_unsupported_line(void) {
  const char *source =
      "count = 42\n"
      "print(count)\n"
      "graph G:\n"
      "print(count)\n";
  const char *path = "gion_partial_execution.txt";
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
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
    return 1;
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_ERR_PARSE) {
    remove(path);
    return 2;
  }
  if (diagnostic.line != 3U) {
    remove(path);
    return 3;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 4;
  }
  remove(path);
  if (strcmp(output, "42\n") != 0) {
    return 5;
  }
  return 0;
}

int test_gion_reserved_name_errors(void) {
  static const struct {
    const char *source;
    const char *message;
    const char *path;
  } cases[] = {
      {"true = 1\n", "reserved name cannot be assigned", "gion_reserved_true.gion"},
      {"false = 0\n", "reserved name cannot be assigned", "gion_reserved_false.gion"},
      {"abs = 1\n", "reserved name cannot be assigned", "gion_reserved_abs.gion"},
      {"min = 1\n", "reserved name cannot be assigned", "gion_reserved_min.gion"},
      {"max = 1\n", "reserved name cannot be assigned", "gion_reserved_max.gion"},
      {"clamp = 1\n", "reserved name cannot be assigned", "gion_reserved_clamp.gion"},
      {"sqrt = 1\n", "reserved name cannot be assigned", "gion_reserved_sqrt.gion"},
      {"cbrt = 1\n", "reserved name cannot be assigned", "gion_reserved_cbrt.gion"},
      {"sin = 1\n", "reserved name cannot be assigned", "gion_reserved_sin.gion"},
      {"sinh = 1\n", "reserved name cannot be assigned", "gion_reserved_sinh.gion"},
      {"asinh = 1\n", "reserved name cannot be assigned", "gion_reserved_asinh.gion"},
      {"acosh = 1\n", "reserved name cannot be assigned", "gion_reserved_acosh.gion"},
      {"cosh = 1\n", "reserved name cannot be assigned", "gion_reserved_cosh.gion"},
      {"tanh = 1\n", "reserved name cannot be assigned", "gion_reserved_tanh.gion"},
      {"atanh = 1\n", "reserved name cannot be assigned", "gion_reserved_atanh.gion"},
      {"cos = 1\n", "reserved name cannot be assigned", "gion_reserved_cos.gion"},
      {"tan = 1\n", "reserved name cannot be assigned", "gion_reserved_tan.gion"},
      {"asin = 1\n", "reserved name cannot be assigned", "gion_reserved_asin.gion"},
      {"acos = 1\n", "reserved name cannot be assigned", "gion_reserved_acos.gion"},
      {"atan = 1\n", "reserved name cannot be assigned", "gion_reserved_atan.gion"},
      {"atan2 = 1\n", "reserved name cannot be assigned", "gion_reserved_atan2.gion"},
      {"hypot = 1\n", "reserved name cannot be assigned", "gion_reserved_hypot.gion"},
      {"degrees = 1\n", "reserved name cannot be assigned", "gion_reserved_degrees.gion"},
      {"radians = 1\n", "reserved name cannot be assigned", "gion_reserved_radians.gion"},
      {"isnan = 1\n", "reserved name cannot be assigned", "gion_reserved_isnan.gion"},
      {"exp = 1\n", "reserved name cannot be assigned", "gion_reserved_exp.gion"},
      {"ln = 1\n", "reserved name cannot be assigned", "gion_reserved_ln.gion"},
      {"log = 1\n", "reserved name cannot be assigned", "gion_reserved_log.gion"},
      {"log10 = 1\n", "reserved name cannot be assigned", "gion_reserved_log10.gion"},
      {"log2 = 1\n", "reserved name cannot be assigned", "gion_reserved_log2.gion"},
      {"floor = 1\n", "reserved name cannot be assigned", "gion_reserved_floor.gion"},
      {"ceil = 1\n", "reserved name cannot be assigned", "gion_reserved_ceil.gion"},
      {"round = 1\n", "reserved name cannot be assigned", "gion_reserved_round.gion"},
      {"trunc = 1\n", "reserved name cannot be assigned", "gion_reserved_trunc.gion"},
      {"sign = 1\n", "reserved name cannot be assigned", "gion_reserved_sign.gion"},
      {"len = 1\n", "reserved name cannot be assigned", "gion_reserved_len.gion"},
      {"pi = 1\n", "reserved name cannot be assigned", "gion_reserved_pi.gion"},
      {"e = 1\n", "reserved name cannot be assigned", "gion_reserved_e.gion"},
      {"nan = 1\n", "reserved name cannot be assigned", "gion_reserved_nan.gion"},
      {"inf = 1\n", "reserved name cannot be assigned", "gion_reserved_inf.gion"},
      {"if = true\n", "reserved name cannot be assigned", "gion_reserved_if.gion"},
      {"elif = false\n", "reserved name cannot be assigned", "gion_reserved_elif.gion"},
      {"else = true\n", "reserved name cannot be assigned", "gion_reserved_else.gion"},
      {"match = true\n", "reserved name cannot be assigned", "gion_reserved_match.gion"},
      {"default = true\n", "reserved name cannot be assigned", "gion_reserved_default.gion"},
      {"and = true\n", "reserved name cannot be assigned", "gion_reserved_and.gion"},
      {"nand = true\n", "reserved name cannot be assigned", "gion_reserved_nand.gion"},
      {"or = true\n", "reserved name cannot be assigned", "gion_reserved_or.gion"},
      {"nor = true\n", "reserved name cannot be assigned", "gion_reserved_nor.gion"},
      {"not = true\n", "reserved name cannot be assigned", "gion_reserved_not.gion"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    FILE *fp = NULL;
    int rc;

    graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
    if (fopen_s(&fp, cases[i].path, "wb") != 0) {
      fp = NULL;
    }
#else
    fp = fopen(cases[i].path, "wb");
#endif
    if (fp == NULL) {
      return (int)(1 + i * 10U);
    }
    fputs(cases[i].source, fp);
    fclose(fp);
    rc = graphion_run_gion_path(cases[i].path, &scope, &diagnostic);
    remove(cases[i].path);
    if (rc != GENTRY_ERR_PARSE) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.line != 1U || diagnostic.column != 1U) {
      return (int)(3 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(4 + i * 10U);
    }
  }
  return 0;
}

int test_gion_assignment_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"count 42\n", GINT_ERR_PARSE, "expected '='"},
      {"= 42\n", GINT_ERR_PARSE, "expected identifier"},
      {"count =\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 42 +\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = nope\n", GINT_ERR_UNKNOWN_OPERAND, "unknown operand"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.line != 1U) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(3 + i * 10U);
    }
  }
  return 0;
}

int test_gion_arithmetic_expressions(void) {
  const char *source =
      "base = 8\n"
      "sum = 40 + 2\n"
      "mixed = 1 + 2 * 3\n"
      "grouped = (1 + 2) * 3\n"
      "delta = base - 3\n"
      "ratio = 7 / 2\n"
      "scaled = ratio * 2\n"
      "negative_add = -5 + 2\n"
      "negative_sub = 5 - -2\n"
      "negative_mul = -3 * 4\n"
      "negative_div = -7 / 2\n"
      "floor_half = 7 // 2\n"
      "negative_floor = -7 // 2\n"
      "float_floor = 7.5 // 2\n"
      "power = 2 ** 3\n"
      "negative_power = (-2) ** 3\n"
      "negative_exponent = 2 ** -1\n"
      "count = 5\n"
      "neg_count = -count\n"
      "neg_group = -(1 + 2)\n"
      "neg_abs = -abs(-3)\n"
      "right_assoc = 2 ** 3 ** 2\n"
      "powered_group = (1 + 2) ** 2\n"
      "abs_int = abs(-42)\n"
      "abs_float = abs(-3.5)\n"
      "abs_expr = abs(-5 + 2)\n"
      "min_int = min(7, 3)\n"
      "min_float = min(3.5, 2)\n"
      "min_expr = min(10 - 2, 3 * 3)\n"
      "max_int = max(7, 3)\n"
      "max_float = max(3.5, 2)\n"
      "max_expr = max(10 - 2, 3 * 3)\n"
      "clamp_low = clamp(-2, 0, 10)\n"
      "clamp_mid = clamp(5, 0, 10)\n"
      "clamp_high = clamp(17, 0, 10)\n"
      "clamp_float = clamp(12.5, 0, 10)\n"
      "sqrt_int = sqrt(9)\n"
      "sqrt_float = sqrt(2.25)\n"
      "sqrt_expr = sqrt(1 + 8)\n"
      "cbrt_int = cbrt(27)\n"
      "cbrt_negative = cbrt(-8)\n"
      "cbrt_expr = cbrt(1 + 26)\n"
      "sin_zero = sin(0)\n"
      "sin_half_turn = sin(pi / 2)\n"
      "sin_expr = sin(1.5707963267948966)\n"
      "sinh_zero = sinh(0)\n"
      "sinh_one = sinh(1)\n"
      "sinh_negative = sinh(-1)\n"
      "asinh_zero = asinh(0)\n"
      "asinh_one = asinh(1)\n"
      "asinh_negative = asinh(-1)\n"
      "acosh_one = acosh(1)\n"
      "acosh_two = acosh(2)\n"
      "acosh_four = acosh(4)\n"
      "cosh_zero = cosh(0)\n"
      "cosh_one = cosh(1)\n"
      "cosh_negative = cosh(-1)\n"
      "tanh_zero = tanh(0)\n"
      "tanh_one = tanh(1)\n"
      "tanh_negative = tanh(-1)\n"
      "atanh_zero = atanh(0)\n"
      "atanh_half = atanh(0.5)\n"
      "atanh_negative_half = atanh(-0.5)\n"
      "cos_zero = cos(0)\n"
      "cos_pi = cos(pi)\n"
      "cos_expr = cos(3.14159265358979323846)\n"
      "tan_zero = tan(0)\n"
      "tan_quarter_turn = tan(pi / 4)\n"
      "tan_expr = tan(0.7853981633974483)\n"
      "asin_zero = asin(0)\n"
      "asin_one = asin(1)\n"
      "asin_half = asin(0.5)\n"
      "acos_one = acos(1)\n"
      "acos_zero = acos(0)\n"
      "acos_half = acos(0.5)\n"
      "atan_zero = atan(0)\n"
      "atan_one = atan(1)\n"
      "atan_negative_one = atan(-1)\n"
      "atan2_diag = atan2(1, 1)\n"
      "atan2_quadrant_two = atan2(1, -1)\n"
      "atan2_quadrant_three = atan2(-1, -1)\n"
      "hypot_diag = hypot(3, 4)\n"
      "hypot_large = hypot(5, 12)\n"
      "hypot_negative = hypot(-3, 4)\n"
      "degrees_zero = degrees(0)\n"
      "degrees_right_angle = degrees(pi / 2)\n"
      "degrees_negative_quarter = degrees(-0.7853981633974483)\n"
      "radians_zero = radians(0)\n"
      "radians_straight = radians(180)\n"
      "radians_negative_quarter = radians(-45)\n"
      "isnan_nan = isnan(nan)\n"
      "isnan_one = isnan(1.0)\n"
      "isnan_count = isnan(7)\n"
      "exp_int = exp(1)\n"
      "exp_float = exp(0.0)\n"
      "exp_expr = exp(1 + 1)\n"
      "ln_int = ln(1)\n"
      "ln_float = ln(e)\n"
      "ln_expr = ln(e ** 2)\n"
      "log_int = log(8, 2)\n"
      "log_float = log(100, 10)\n"
      "log_expr = log(2 ** 5, 2)\n"
      "log10_int = log10(1000)\n"
      "log10_float = log10(10.0)\n"
      "log10_expr = log10(10 ** 4)\n"
      "log2_int = log2(8)\n"
      "log2_float = log2(2.0)\n"
      "log2_expr = log2(2 ** 6)\n"
      "floor_int = floor(7)\n"
      "floor_float = floor(7.5)\n"
      "floor_negative = floor(-3.2)\n"
      "ceil_int = ceil(7)\n"
      "ceil_float = ceil(7.5)\n"
      "ceil_negative = ceil(-3.2)\n"
      "round_int = round(7)\n"
      "round_float = round(7.4)\n"
      "round_half = round(7.5)\n"
      "round_negative = round(-3.2)\n"
      "round_negative_half = round(-3.5)\n"
      "trunc_int = trunc(7)\n"
      "trunc_float = trunc(7.9)\n"
      "trunc_negative = trunc(-3.9)\n"
      "trunc_small_negative = trunc(-0.4)\n"
      "sign_positive = sign(7)\n"
      "sign_negative = sign(-3.9)\n"
      "sign_zero = sign(0)\n"
      "pi_value = pi\n"
      "e_value = e\n"
      "nan_value = nan\n"
      "inf_value = inf\n"
      "factorial_zero = 0!\n"
      "factorial_int = 5!\n"
      "factorial_group = (1 + 2)!\n"
      "len_empty = len(\"\")\n"
      "len_text = len(\"graphion\")\n"
      "len_concat = len(\"graph\" + \"ion\")\n"
      "total = base + ratio * 2\n"
      "remainder = 10 % 4\n"
      "negative_remainder = -10 % 4\n"
      "float_remainder = 7.5 % 2\n"
      "print(sum)\n"
      "print(mixed)\n"
      "print(grouped)\n"
      "print(delta)\n"
      "print(ratio)\n"
      "print(total)\n"
      "print(negative_add)\n"
      "print(negative_sub)\n"
      "print(negative_mul)\n"
      "print(negative_div)\n"
      "print(floor_half)\n"
      "print(negative_floor)\n"
      "print(float_floor)\n"
      "print(power)\n"
      "print(negative_power)\n"
      "print(negative_exponent)\n"
      "print(neg_count)\n"
      "print(neg_group)\n"
      "print(neg_abs)\n"
      "print(right_assoc)\n"
      "print(powered_group)\n"
      "print(abs_int)\n"
      "print(abs_float)\n"
      "print(abs_expr)\n"
      "print(min_int)\n"
      "print(min_float)\n"
      "print(min_expr)\n"
      "print(max_int)\n"
      "print(max_float)\n"
      "print(max_expr)\n"
      "print(clamp_low)\n"
      "print(clamp_mid)\n"
      "print(clamp_high)\n"
      "print(clamp_float)\n"
      "print(sqrt_int)\n"
      "print(sqrt_float)\n"
      "print(sqrt_expr)\n"
      "print(cbrt_int)\n"
      "print(cbrt_negative)\n"
      "print(cbrt_expr)\n"
      "print(sin_zero)\n"
      "print(sin_half_turn)\n"
      "print(sin_expr)\n"
      "print(sinh_zero)\n"
      "print(sinh_one)\n"
      "print(sinh_negative)\n"
      "print(asinh_zero)\n"
      "print(asinh_one)\n"
      "print(asinh_negative)\n"
      "print(acosh_one)\n"
      "print(acosh_two)\n"
      "print(acosh_four)\n"
      "print(cosh_zero)\n"
      "print(cosh_one)\n"
      "print(cosh_negative)\n"
      "print(tanh_zero)\n"
      "print(tanh_one)\n"
      "print(tanh_negative)\n"
      "print(atanh_zero)\n"
      "print(atanh_half)\n"
      "print(atanh_negative_half)\n"
      "print(cos_zero)\n"
      "print(cos_pi)\n"
      "print(cos_expr)\n"
      "print(tan_zero)\n"
      "print(tan_quarter_turn)\n"
      "print(tan_expr)\n"
      "print(asin_zero)\n"
      "print(asin_one)\n"
      "print(asin_half)\n"
      "print(acos_one)\n"
      "print(acos_zero)\n"
      "print(acos_half)\n"
      "print(atan_zero)\n"
      "print(atan_one)\n"
      "print(atan_negative_one)\n"
      "print(atan2_diag)\n"
      "print(atan2_quadrant_two)\n"
      "print(atan2_quadrant_three)\n"
      "print(hypot_diag)\n"
      "print(hypot_large)\n"
      "print(hypot_negative)\n"
      "print(degrees_zero)\n"
      "print(degrees_right_angle)\n"
      "print(degrees_negative_quarter)\n"
      "print(radians_zero)\n"
      "print(radians_straight)\n"
      "print(radians_negative_quarter)\n"
      "print(isnan_nan)\n"
      "print(isnan_one)\n"
      "print(isnan_count)\n"
      "print(exp_int)\n"
      "print(exp_float)\n"
      "print(exp_expr)\n"
      "print(ln_int)\n"
      "print(ln_float)\n"
      "print(ln_expr)\n"
      "print(log_int)\n"
      "print(log_float)\n"
      "print(log_expr)\n"
      "print(log10_int)\n"
      "print(log10_float)\n"
      "print(log10_expr)\n"
      "print(log2_int)\n"
      "print(log2_float)\n"
      "print(log2_expr)\n"
      "print(floor_int)\n"
      "print(floor_float)\n"
      "print(floor_negative)\n"
      "print(ceil_int)\n"
      "print(ceil_float)\n"
      "print(ceil_negative)\n"
      "print(round_int)\n"
      "print(round_float)\n"
      "print(round_half)\n"
      "print(round_negative)\n"
      "print(round_negative_half)\n"
      "print(trunc_int)\n"
      "print(trunc_float)\n"
      "print(trunc_negative)\n"
      "print(trunc_small_negative)\n"
      "print(sign_positive)\n"
      "print(sign_negative)\n"
      "print(sign_zero)\n"
      "print(pi_value)\n"
      "print(e_value)\n"
      "print(factorial_zero)\n"
      "print(factorial_int)\n"
      "print(factorial_group)\n"
      "print(len_empty)\n"
      "print(len_text)\n"
      "print(len_concat)\n"
      "print(remainder)\n"
      "print(negative_remainder)\n"
      "print(float_remainder)\n"
      "print(3 + 4 * 2)\n"
      "print((3 + 4) * 2)\n"
      "print(10 % 4)\n";
  const char *path = "gion_arithmetic_expressions.txt";
  char output[3072];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *sum;
  const graphion_runtime_value *mixed;
  const graphion_runtime_value *grouped;
  const graphion_runtime_value *delta;
  const graphion_runtime_value *ratio;
  const graphion_runtime_value *scaled;
  const graphion_runtime_value *negative_add;
  const graphion_runtime_value *negative_sub;
  const graphion_runtime_value *negative_mul;
  const graphion_runtime_value *negative_div;
  const graphion_runtime_value *floor_half;
  const graphion_runtime_value *negative_floor;
  const graphion_runtime_value *float_floor;
  const graphion_runtime_value *power;
  const graphion_runtime_value *negative_power;
  const graphion_runtime_value *negative_exponent;
  const graphion_runtime_value *neg_count;
  const graphion_runtime_value *neg_group;
  const graphion_runtime_value *neg_abs;
  const graphion_runtime_value *right_assoc;
  const graphion_runtime_value *powered_group;
  const graphion_runtime_value *abs_int;
  const graphion_runtime_value *abs_float;
  const graphion_runtime_value *abs_expr;
  const graphion_runtime_value *min_int;
  const graphion_runtime_value *min_float;
  const graphion_runtime_value *min_expr;
  const graphion_runtime_value *max_int;
  const graphion_runtime_value *max_float;
  const graphion_runtime_value *max_expr;
  const graphion_runtime_value *clamp_low;
  const graphion_runtime_value *clamp_mid;
  const graphion_runtime_value *clamp_high;
  const graphion_runtime_value *clamp_float;
  const graphion_runtime_value *sqrt_int;
  const graphion_runtime_value *sqrt_float;
  const graphion_runtime_value *sqrt_expr;
  const graphion_runtime_value *cbrt_int;
  const graphion_runtime_value *cbrt_negative;
  const graphion_runtime_value *cbrt_expr;
  const graphion_runtime_value *sin_zero;
  const graphion_runtime_value *sin_half_turn;
  const graphion_runtime_value *sin_expr;
  const graphion_runtime_value *sinh_zero;
  const graphion_runtime_value *sinh_one;
  const graphion_runtime_value *sinh_negative;
  const graphion_runtime_value *asinh_zero;
  const graphion_runtime_value *asinh_one;
  const graphion_runtime_value *asinh_negative;
  const graphion_runtime_value *acosh_one;
  const graphion_runtime_value *acosh_two;
  const graphion_runtime_value *acosh_four;
  const graphion_runtime_value *cosh_zero;
  const graphion_runtime_value *cosh_one;
  const graphion_runtime_value *cosh_negative;
  const graphion_runtime_value *tanh_zero;
  const graphion_runtime_value *tanh_one;
  const graphion_runtime_value *tanh_negative;
  const graphion_runtime_value *atanh_zero;
  const graphion_runtime_value *atanh_half;
  const graphion_runtime_value *atanh_negative_half;
  const graphion_runtime_value *cos_zero;
  const graphion_runtime_value *cos_pi;
  const graphion_runtime_value *cos_expr;
  const graphion_runtime_value *tan_zero;
  const graphion_runtime_value *tan_quarter_turn;
  const graphion_runtime_value *tan_expr;
  const graphion_runtime_value *asin_zero;
  const graphion_runtime_value *asin_one;
  const graphion_runtime_value *asin_half;
  const graphion_runtime_value *atan_zero;
  const graphion_runtime_value *atan_one;
  const graphion_runtime_value *atan_negative_one;
  const graphion_runtime_value *atan2_diag;
  const graphion_runtime_value *atan2_quadrant_two;
  const graphion_runtime_value *atan2_quadrant_three;
  const graphion_runtime_value *hypot_diag;
  const graphion_runtime_value *hypot_large;
  const graphion_runtime_value *hypot_negative;
  const graphion_runtime_value *degrees_zero;
  const graphion_runtime_value *degrees_right_angle;
  const graphion_runtime_value *degrees_negative_quarter;
  const graphion_runtime_value *radians_zero;
  const graphion_runtime_value *radians_straight;
  const graphion_runtime_value *radians_negative_quarter;
  const graphion_runtime_value *isnan_nan;
  const graphion_runtime_value *isnan_one;
  const graphion_runtime_value *isnan_count;
  const graphion_runtime_value *exp_int;
  const graphion_runtime_value *exp_float;
  const graphion_runtime_value *exp_expr;
  const graphion_runtime_value *ln_int;
  const graphion_runtime_value *ln_float;
  const graphion_runtime_value *ln_expr;
  const graphion_runtime_value *log_int;
  const graphion_runtime_value *log_float;
  const graphion_runtime_value *log_expr;
  const graphion_runtime_value *log10_int;
  const graphion_runtime_value *log10_float;
  const graphion_runtime_value *log10_expr;
  const graphion_runtime_value *log2_int;
  const graphion_runtime_value *log2_float;
  const graphion_runtime_value *log2_expr;
  const graphion_runtime_value *floor_int;
  const graphion_runtime_value *floor_float;
  const graphion_runtime_value *floor_negative;
  const graphion_runtime_value *ceil_int;
  const graphion_runtime_value *ceil_float;
  const graphion_runtime_value *ceil_negative;
  const graphion_runtime_value *round_int;
  const graphion_runtime_value *round_float;
  const graphion_runtime_value *round_half;
  const graphion_runtime_value *round_negative;
  const graphion_runtime_value *round_negative_half;
  const graphion_runtime_value *trunc_int;
  const graphion_runtime_value *trunc_float;
  const graphion_runtime_value *trunc_negative;
  const graphion_runtime_value *trunc_small_negative;
  const graphion_runtime_value *sign_positive;
  const graphion_runtime_value *sign_negative;
  const graphion_runtime_value *sign_zero;
  const graphion_runtime_value *pi_value;
  const graphion_runtime_value *e_value;
  const graphion_runtime_value *nan_value;
  const graphion_runtime_value *inf_value;
  const graphion_runtime_value *factorial_zero;
  const graphion_runtime_value *factorial_int;
  const graphion_runtime_value *factorial_group;
  const graphion_runtime_value *len_empty;
  const graphion_runtime_value *len_text;
  const graphion_runtime_value *len_concat;
  const graphion_runtime_value *total;
  const graphion_runtime_value *remainder;
  const graphion_runtime_value *negative_remainder;
  const graphion_runtime_value *float_remainder;
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
    return 1;
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return 2;
  }

  sum = graphion_runtime_scope_find(&scope, "sum");
  mixed = graphion_runtime_scope_find(&scope, "mixed");
  grouped = graphion_runtime_scope_find(&scope, "grouped");
  delta = graphion_runtime_scope_find(&scope, "delta");
  ratio = graphion_runtime_scope_find(&scope, "ratio");
  scaled = graphion_runtime_scope_find(&scope, "scaled");
  negative_add = graphion_runtime_scope_find(&scope, "negative_add");
  negative_sub = graphion_runtime_scope_find(&scope, "negative_sub");
  negative_mul = graphion_runtime_scope_find(&scope, "negative_mul");
  negative_div = graphion_runtime_scope_find(&scope, "negative_div");
  floor_half = graphion_runtime_scope_find(&scope, "floor_half");
  negative_floor = graphion_runtime_scope_find(&scope, "negative_floor");
  float_floor = graphion_runtime_scope_find(&scope, "float_floor");
  power = graphion_runtime_scope_find(&scope, "power");
  negative_power = graphion_runtime_scope_find(&scope, "negative_power");
  negative_exponent = graphion_runtime_scope_find(&scope, "negative_exponent");
  neg_count = graphion_runtime_scope_find(&scope, "neg_count");
  neg_group = graphion_runtime_scope_find(&scope, "neg_group");
  neg_abs = graphion_runtime_scope_find(&scope, "neg_abs");
  right_assoc = graphion_runtime_scope_find(&scope, "right_assoc");
  powered_group = graphion_runtime_scope_find(&scope, "powered_group");
  abs_int = graphion_runtime_scope_find(&scope, "abs_int");
  abs_float = graphion_runtime_scope_find(&scope, "abs_float");
  abs_expr = graphion_runtime_scope_find(&scope, "abs_expr");
  min_int = graphion_runtime_scope_find(&scope, "min_int");
  min_float = graphion_runtime_scope_find(&scope, "min_float");
  min_expr = graphion_runtime_scope_find(&scope, "min_expr");
  max_int = graphion_runtime_scope_find(&scope, "max_int");
  max_float = graphion_runtime_scope_find(&scope, "max_float");
  max_expr = graphion_runtime_scope_find(&scope, "max_expr");
  clamp_low = graphion_runtime_scope_find(&scope, "clamp_low");
  clamp_mid = graphion_runtime_scope_find(&scope, "clamp_mid");
  clamp_high = graphion_runtime_scope_find(&scope, "clamp_high");
  clamp_float = graphion_runtime_scope_find(&scope, "clamp_float");
  sqrt_int = graphion_runtime_scope_find(&scope, "sqrt_int");
  sqrt_float = graphion_runtime_scope_find(&scope, "sqrt_float");
  sqrt_expr = graphion_runtime_scope_find(&scope, "sqrt_expr");
  cbrt_int = graphion_runtime_scope_find(&scope, "cbrt_int");
  cbrt_negative = graphion_runtime_scope_find(&scope, "cbrt_negative");
  cbrt_expr = graphion_runtime_scope_find(&scope, "cbrt_expr");
  sin_zero = graphion_runtime_scope_find(&scope, "sin_zero");
  sin_half_turn = graphion_runtime_scope_find(&scope, "sin_half_turn");
  sin_expr = graphion_runtime_scope_find(&scope, "sin_expr");
  sinh_zero = graphion_runtime_scope_find(&scope, "sinh_zero");
  sinh_one = graphion_runtime_scope_find(&scope, "sinh_one");
  sinh_negative = graphion_runtime_scope_find(&scope, "sinh_negative");
  asinh_zero = graphion_runtime_scope_find(&scope, "asinh_zero");
  asinh_one = graphion_runtime_scope_find(&scope, "asinh_one");
  asinh_negative = graphion_runtime_scope_find(&scope, "asinh_negative");
  acosh_one = graphion_runtime_scope_find(&scope, "acosh_one");
  acosh_two = graphion_runtime_scope_find(&scope, "acosh_two");
  acosh_four = graphion_runtime_scope_find(&scope, "acosh_four");
  cosh_zero = graphion_runtime_scope_find(&scope, "cosh_zero");
  cosh_one = graphion_runtime_scope_find(&scope, "cosh_one");
  cosh_negative = graphion_runtime_scope_find(&scope, "cosh_negative");
  tanh_zero = graphion_runtime_scope_find(&scope, "tanh_zero");
  tanh_one = graphion_runtime_scope_find(&scope, "tanh_one");
  tanh_negative = graphion_runtime_scope_find(&scope, "tanh_negative");
  atanh_zero = graphion_runtime_scope_find(&scope, "atanh_zero");
  atanh_half = graphion_runtime_scope_find(&scope, "atanh_half");
  atanh_negative_half = graphion_runtime_scope_find(&scope, "atanh_negative_half");
  cos_zero = graphion_runtime_scope_find(&scope, "cos_zero");
  cos_pi = graphion_runtime_scope_find(&scope, "cos_pi");
  cos_expr = graphion_runtime_scope_find(&scope, "cos_expr");
  tan_zero = graphion_runtime_scope_find(&scope, "tan_zero");
  tan_quarter_turn = graphion_runtime_scope_find(&scope, "tan_quarter_turn");
  tan_expr = graphion_runtime_scope_find(&scope, "tan_expr");
  asin_zero = graphion_runtime_scope_find(&scope, "asin_zero");
  asin_one = graphion_runtime_scope_find(&scope, "asin_one");
  asin_half = graphion_runtime_scope_find(&scope, "asin_half");
  atan_zero = graphion_runtime_scope_find(&scope, "atan_zero");
  atan_one = graphion_runtime_scope_find(&scope, "atan_one");
  atan_negative_one = graphion_runtime_scope_find(&scope, "atan_negative_one");
  atan2_diag = graphion_runtime_scope_find(&scope, "atan2_diag");
  atan2_quadrant_two = graphion_runtime_scope_find(&scope, "atan2_quadrant_two");
  atan2_quadrant_three = graphion_runtime_scope_find(&scope, "atan2_quadrant_three");
  hypot_diag = graphion_runtime_scope_find(&scope, "hypot_diag");
  hypot_large = graphion_runtime_scope_find(&scope, "hypot_large");
  hypot_negative = graphion_runtime_scope_find(&scope, "hypot_negative");
  degrees_zero = graphion_runtime_scope_find(&scope, "degrees_zero");
  degrees_right_angle = graphion_runtime_scope_find(&scope, "degrees_right_angle");
  degrees_negative_quarter = graphion_runtime_scope_find(&scope, "degrees_negative_quarter");
  radians_zero = graphion_runtime_scope_find(&scope, "radians_zero");
  radians_straight = graphion_runtime_scope_find(&scope, "radians_straight");
  radians_negative_quarter = graphion_runtime_scope_find(&scope, "radians_negative_quarter");
  isnan_nan = graphion_runtime_scope_find(&scope, "isnan_nan");
  isnan_one = graphion_runtime_scope_find(&scope, "isnan_one");
  isnan_count = graphion_runtime_scope_find(&scope, "isnan_count");
  exp_int = graphion_runtime_scope_find(&scope, "exp_int");
  exp_float = graphion_runtime_scope_find(&scope, "exp_float");
  exp_expr = graphion_runtime_scope_find(&scope, "exp_expr");
  ln_int = graphion_runtime_scope_find(&scope, "ln_int");
  ln_float = graphion_runtime_scope_find(&scope, "ln_float");
  ln_expr = graphion_runtime_scope_find(&scope, "ln_expr");
  log_int = graphion_runtime_scope_find(&scope, "log_int");
  log_float = graphion_runtime_scope_find(&scope, "log_float");
  log_expr = graphion_runtime_scope_find(&scope, "log_expr");
  log10_int = graphion_runtime_scope_find(&scope, "log10_int");
  log10_float = graphion_runtime_scope_find(&scope, "log10_float");
  log10_expr = graphion_runtime_scope_find(&scope, "log10_expr");
  log2_int = graphion_runtime_scope_find(&scope, "log2_int");
  log2_float = graphion_runtime_scope_find(&scope, "log2_float");
  log2_expr = graphion_runtime_scope_find(&scope, "log2_expr");
  floor_int = graphion_runtime_scope_find(&scope, "floor_int");
  floor_float = graphion_runtime_scope_find(&scope, "floor_float");
  floor_negative = graphion_runtime_scope_find(&scope, "floor_negative");
  ceil_int = graphion_runtime_scope_find(&scope, "ceil_int");
  ceil_float = graphion_runtime_scope_find(&scope, "ceil_float");
  ceil_negative = graphion_runtime_scope_find(&scope, "ceil_negative");
  round_int = graphion_runtime_scope_find(&scope, "round_int");
  round_float = graphion_runtime_scope_find(&scope, "round_float");
  round_half = graphion_runtime_scope_find(&scope, "round_half");
  round_negative = graphion_runtime_scope_find(&scope, "round_negative");
  round_negative_half = graphion_runtime_scope_find(&scope, "round_negative_half");
  trunc_int = graphion_runtime_scope_find(&scope, "trunc_int");
  trunc_float = graphion_runtime_scope_find(&scope, "trunc_float");
  trunc_negative = graphion_runtime_scope_find(&scope, "trunc_negative");
  trunc_small_negative = graphion_runtime_scope_find(&scope, "trunc_small_negative");
  sign_positive = graphion_runtime_scope_find(&scope, "sign_positive");
  sign_negative = graphion_runtime_scope_find(&scope, "sign_negative");
  sign_zero = graphion_runtime_scope_find(&scope, "sign_zero");
  pi_value = graphion_runtime_scope_find(&scope, "pi_value");
  e_value = graphion_runtime_scope_find(&scope, "e_value");
  nan_value = graphion_runtime_scope_find(&scope, "nan_value");
  inf_value = graphion_runtime_scope_find(&scope, "inf_value");
  factorial_zero = graphion_runtime_scope_find(&scope, "factorial_zero");
  factorial_int = graphion_runtime_scope_find(&scope, "factorial_int");
  factorial_group = graphion_runtime_scope_find(&scope, "factorial_group");
  len_empty = graphion_runtime_scope_find(&scope, "len_empty");
  len_text = graphion_runtime_scope_find(&scope, "len_text");
  len_concat = graphion_runtime_scope_find(&scope, "len_concat");
  total = graphion_runtime_scope_find(&scope, "total");
  remainder = graphion_runtime_scope_find(&scope, "remainder");
  negative_remainder = graphion_runtime_scope_find(&scope, "negative_remainder");
  float_remainder = graphion_runtime_scope_find(&scope, "float_remainder");
  if (sum == NULL || sum->kind != GVM_VALUE_INT || sum->as.int_value != 42) {
    remove(path);
    return 3;
  }
  if (mixed == NULL || mixed->kind != GVM_VALUE_INT || mixed->as.int_value != 7) {
    remove(path);
    return 4;
  }
  if (grouped == NULL || grouped->kind != GVM_VALUE_INT || grouped->as.int_value != 9) {
    remove(path);
    return 5;
  }
  if (delta == NULL || delta->kind != GVM_VALUE_INT || delta->as.int_value != 5) {
    remove(path);
    return 6;
  }
  if (ratio == NULL || ratio->kind != GVM_VALUE_FLOAT || ratio->as.float_value != 3.5) {
    remove(path);
    return 7;
  }
  if (scaled == NULL || scaled->kind != GVM_VALUE_FLOAT || scaled->as.float_value != 7.0) {
    remove(path);
    return 8;
  }
  if (negative_add == NULL || negative_add->kind != GVM_VALUE_INT || negative_add->as.int_value != -3) {
    remove(path);
    return 9;
  }
  if (negative_sub == NULL || negative_sub->kind != GVM_VALUE_INT || negative_sub->as.int_value != 7) {
    remove(path);
    return 10;
  }
  if (negative_mul == NULL || negative_mul->kind != GVM_VALUE_INT || negative_mul->as.int_value != -12) {
    remove(path);
    return 11;
  }
  if (negative_div == NULL || negative_div->kind != GVM_VALUE_FLOAT || negative_div->as.float_value != -3.5) {
    remove(path);
    return 12;
  }
  if (floor_half == NULL || floor_half->kind != GVM_VALUE_INT || floor_half->as.int_value != 3) {
    remove(path);
    return 13;
  }
  if (negative_floor == NULL || negative_floor->kind != GVM_VALUE_INT || negative_floor->as.int_value != -4) {
    remove(path);
    return 14;
  }
  if (float_floor == NULL || float_floor->kind != GVM_VALUE_FLOAT || float_floor->as.float_value != 3.0) {
    remove(path);
    return 15;
  }
  if (power == NULL || power->kind != GVM_VALUE_FLOAT || power->as.float_value != 8.0) {
    remove(path);
    return 16;
  }
  if (negative_power == NULL || negative_power->kind != GVM_VALUE_FLOAT || negative_power->as.float_value != -8.0) {
    remove(path);
    return 17;
  }
  if (negative_exponent == NULL || negative_exponent->kind != GVM_VALUE_FLOAT || negative_exponent->as.float_value != 0.5) {
    remove(path);
    return 18;
  }
  if (neg_count == NULL || neg_count->kind != GVM_VALUE_INT || neg_count->as.int_value != -5) {
    remove(path);
    return 181;
  }
  if (neg_group == NULL || neg_group->kind != GVM_VALUE_INT || neg_group->as.int_value != -3) {
    remove(path);
    return 182;
  }
  if (neg_abs == NULL || neg_abs->kind != GVM_VALUE_INT || neg_abs->as.int_value != -3) {
    remove(path);
    return 183;
  }
  if (right_assoc == NULL || right_assoc->kind != GVM_VALUE_FLOAT || right_assoc->as.float_value != 512.0) {
    remove(path);
    return 19;
  }
  if (powered_group == NULL || powered_group->kind != GVM_VALUE_FLOAT || powered_group->as.float_value != 9.0) {
    remove(path);
    return 20;
  }
  if (abs_int == NULL || abs_int->kind != GVM_VALUE_INT || abs_int->as.int_value != 42) {
    remove(path);
    return 21;
  }
  if (abs_float == NULL || abs_float->kind != GVM_VALUE_FLOAT || abs_float->as.float_value != 3.5) {
    remove(path);
    return 22;
  }
  if (abs_expr == NULL || abs_expr->kind != GVM_VALUE_INT || abs_expr->as.int_value != 3) {
    remove(path);
    return 23;
  }
  if (min_int == NULL || min_int->kind != GVM_VALUE_INT || min_int->as.int_value != 3) {
    remove(path);
    return 231;
  }
  if (min_float == NULL || min_float->kind != GVM_VALUE_FLOAT || min_float->as.float_value != 2.0) {
    remove(path);
    return 232;
  }
  if (min_expr == NULL || min_expr->kind != GVM_VALUE_INT || min_expr->as.int_value != 8) {
    remove(path);
    return 233;
  }
  if (max_int == NULL || max_int->kind != GVM_VALUE_INT || max_int->as.int_value != 7) {
    remove(path);
    return 234;
  }
  if (max_float == NULL || max_float->kind != GVM_VALUE_FLOAT || max_float->as.float_value != 3.5) {
    remove(path);
    return 235;
  }
  if (max_expr == NULL || max_expr->kind != GVM_VALUE_INT || max_expr->as.int_value != 9) {
    remove(path);
    return 236;
  }
  if (clamp_low == NULL || clamp_low->kind != GVM_VALUE_INT || clamp_low->as.int_value != 0) {
    remove(path);
    return 237;
  }
  if (clamp_mid == NULL || clamp_mid->kind != GVM_VALUE_INT || clamp_mid->as.int_value != 5) {
    remove(path);
    return 238;
  }
  if (clamp_high == NULL || clamp_high->kind != GVM_VALUE_INT || clamp_high->as.int_value != 10) {
    remove(path);
    return 239;
  }
  if (clamp_float == NULL || clamp_float->kind != GVM_VALUE_FLOAT || clamp_float->as.float_value != 10.0) {
    remove(path);
    return 240;
  }
  if (sqrt_int == NULL || sqrt_int->kind != GVM_VALUE_FLOAT || sqrt_int->as.float_value != 3.0) {
    remove(path);
    return 241;
  }
  if (sqrt_float == NULL || sqrt_float->kind != GVM_VALUE_FLOAT || sqrt_float->as.float_value != 1.5) {
    remove(path);
    return 242;
  }
  if (sqrt_expr == NULL || sqrt_expr->kind != GVM_VALUE_FLOAT || sqrt_expr->as.float_value != 3.0) {
    remove(path);
    return 243;
  }
  if (cbrt_int == NULL || cbrt_int->kind != GVM_VALUE_FLOAT || cbrt_int->as.float_value != 3.0) {
    remove(path);
    return 2431;
  }
  if (cbrt_negative == NULL || cbrt_negative->kind != GVM_VALUE_FLOAT || cbrt_negative->as.float_value != -2.0) {
    remove(path);
    return 2432;
  }
  if (cbrt_expr == NULL || cbrt_expr->kind != GVM_VALUE_FLOAT || cbrt_expr->as.float_value != 3.0) {
    remove(path);
    return 24325;
  }
  if (sin_zero == NULL || sin_zero->kind != GVM_VALUE_FLOAT || sin_zero->as.float_value != 0.0) {
    remove(path);
    return 24326;
  }
  if (sin_half_turn == NULL || sin_half_turn->kind != GVM_VALUE_FLOAT || sin_half_turn->as.float_value < 0.999999999 ||
      sin_half_turn->as.float_value > 1.000000001) {
    remove(path);
    return 24327;
  }
  if (sin_expr == NULL || sin_expr->kind != GVM_VALUE_FLOAT || sin_expr->as.float_value < 0.999999999 ||
      sin_expr->as.float_value > 1.000000001) {
    remove(path);
    return 24328;
  }
  if (sinh_zero == NULL || sinh_zero->kind != GVM_VALUE_FLOAT || sinh_zero->as.float_value != 0.0) {
    remove(path);
    return 243281;
  }
  if (sinh_one == NULL || sinh_one->kind != GVM_VALUE_FLOAT || sinh_one->as.float_value < 1.175201193 ||
      sinh_one->as.float_value > 1.175201195) {
    remove(path);
    return 243282;
  }
  if (sinh_negative == NULL || sinh_negative->kind != GVM_VALUE_FLOAT ||
      sinh_negative->as.float_value < -1.175201195 || sinh_negative->as.float_value > -1.175201193) {
    remove(path);
    return 243283;
  }
  if (asinh_zero == NULL || asinh_zero->kind != GVM_VALUE_FLOAT ||
      asinh_zero->as.float_value < -0.000000001 || asinh_zero->as.float_value > 0.000000001) {
    remove(path);
    return 2432831;
  }
  if (asinh_one == NULL || asinh_one->kind != GVM_VALUE_FLOAT || asinh_one->as.float_value < 0.881373586 ||
      asinh_one->as.float_value > 0.881373588) {
    remove(path);
    return 2432832;
  }
  if (asinh_negative == NULL || asinh_negative->kind != GVM_VALUE_FLOAT ||
      asinh_negative->as.float_value < -0.881373588 || asinh_negative->as.float_value > -0.881373586) {
    remove(path);
    return 2432833;
  }
  if (acosh_one == NULL || acosh_one->kind != GVM_VALUE_FLOAT ||
      acosh_one->as.float_value < -0.000000001 || acosh_one->as.float_value > 0.000000001) {
    remove(path);
    return 2432834;
  }
  if (acosh_two == NULL || acosh_two->kind != GVM_VALUE_FLOAT ||
      acosh_two->as.float_value < 1.316957895 || acosh_two->as.float_value > 1.316957897) {
    remove(path);
    return 2432835;
  }
  if (acosh_four == NULL || acosh_four->kind != GVM_VALUE_FLOAT ||
      acosh_four->as.float_value < 2.063437067 || acosh_four->as.float_value > 2.063437069) {
    remove(path);
    return 2432836;
  }
  if (cosh_zero == NULL || cosh_zero->kind != GVM_VALUE_FLOAT || cosh_zero->as.float_value != 1.0) {
    remove(path);
    return 243284;
  }
  if (cosh_one == NULL || cosh_one->kind != GVM_VALUE_FLOAT || cosh_one->as.float_value < 1.543080634 ||
      cosh_one->as.float_value > 1.543080636) {
    remove(path);
    return 243285;
  }
  if (cosh_negative == NULL || cosh_negative->kind != GVM_VALUE_FLOAT ||
      cosh_negative->as.float_value < 1.543080634 || cosh_negative->as.float_value > 1.543080636) {
    remove(path);
    return 243286;
  }
  if (tanh_zero == NULL || tanh_zero->kind != GVM_VALUE_FLOAT || tanh_zero->as.float_value < -0.000000001 ||
      tanh_zero->as.float_value > 0.000000001) {
    remove(path);
    return 243287;
  }
  if (tanh_one == NULL || tanh_one->kind != GVM_VALUE_FLOAT || tanh_one->as.float_value < 0.761594155 ||
      tanh_one->as.float_value > 0.761594157) {
    remove(path);
    return 243288;
  }
  if (tanh_negative == NULL || tanh_negative->kind != GVM_VALUE_FLOAT ||
      tanh_negative->as.float_value < -0.761594157 || tanh_negative->as.float_value > -0.761594155) {
    remove(path);
    return 243289;
  }
  if (atanh_zero == NULL || atanh_zero->kind != GVM_VALUE_FLOAT || atanh_zero->as.float_value < -0.000000001 ||
      atanh_zero->as.float_value > 0.000000001) {
    remove(path);
    return 2432891;
  }
  if (atanh_half == NULL || atanh_half->kind != GVM_VALUE_FLOAT ||
      atanh_half->as.float_value < 0.549306143 || atanh_half->as.float_value > 0.549306145) {
    remove(path);
    return 2432892;
  }
  if (atanh_negative_half == NULL || atanh_negative_half->kind != GVM_VALUE_FLOAT ||
      atanh_negative_half->as.float_value < -0.549306145 || atanh_negative_half->as.float_value > -0.549306143) {
    remove(path);
    return 2432893;
  }
  if (cos_zero == NULL || cos_zero->kind != GVM_VALUE_FLOAT || cos_zero->as.float_value < 0.999999999 ||
      cos_zero->as.float_value > 1.000000001) {
    remove(path);
    return 24329;
  }
  if (cos_pi == NULL || cos_pi->kind != GVM_VALUE_FLOAT || cos_pi->as.float_value > -0.999999999 ||
      cos_pi->as.float_value < -1.000000001) {
    remove(path);
    return 24330;
  }
  if (cos_expr == NULL || cos_expr->kind != GVM_VALUE_FLOAT || cos_expr->as.float_value > -0.999999999 ||
      cos_expr->as.float_value < -1.000000001) {
    remove(path);
    return 24331;
  }
  if (tan_zero == NULL || tan_zero->kind != GVM_VALUE_FLOAT || tan_zero->as.float_value < -0.000000001 ||
      tan_zero->as.float_value > 0.000000001) {
    remove(path);
    return 24332;
  }
  if (tan_quarter_turn == NULL || tan_quarter_turn->kind != GVM_VALUE_FLOAT ||
      tan_quarter_turn->as.float_value < 0.999999999 || tan_quarter_turn->as.float_value > 1.000000001) {
    remove(path);
    return 24333;
  }
  if (tan_expr == NULL || tan_expr->kind != GVM_VALUE_FLOAT || tan_expr->as.float_value < 0.999999999 ||
      tan_expr->as.float_value > 1.000000001) {
    remove(path);
    return 24334;
  }
  if (asin_zero == NULL || asin_zero->kind != GVM_VALUE_FLOAT || asin_zero->as.float_value < -0.000000001 ||
      asin_zero->as.float_value > 0.000000001) {
    remove(path);
    return 24335;
  }
  if (asin_one == NULL || asin_one->kind != GVM_VALUE_FLOAT || asin_one->as.float_value < 1.570796326 ||
      asin_one->as.float_value > 1.570796328) {
    remove(path);
    return 24336;
  }
  if (asin_half == NULL || asin_half->kind != GVM_VALUE_FLOAT || asin_half->as.float_value < 0.523598775 ||
      asin_half->as.float_value > 0.523598777) {
    remove(path);
    return 24337;
  }
  if (atan_zero == NULL || atan_zero->kind != GVM_VALUE_FLOAT || atan_zero->as.float_value < -0.000000001 ||
      atan_zero->as.float_value > 0.000000001) {
    remove(path);
    return 24341;
  }
  if (atan_one == NULL || atan_one->kind != GVM_VALUE_FLOAT || atan_one->as.float_value < 0.785398163 ||
      atan_one->as.float_value > 0.785398164) {
    remove(path);
    return 24342;
  }
  if (atan_negative_one == NULL || atan_negative_one->kind != GVM_VALUE_FLOAT ||
      atan_negative_one->as.float_value < -0.785398164 || atan_negative_one->as.float_value > -0.785398163) {
    remove(path);
    return 24343;
  }
  if (atan2_diag == NULL || atan2_diag->kind != GVM_VALUE_FLOAT || atan2_diag->as.float_value < 0.785398163 ||
      atan2_diag->as.float_value > 0.785398164) {
    remove(path);
    return 24344;
  }
  if (atan2_quadrant_two == NULL || atan2_quadrant_two->kind != GVM_VALUE_FLOAT ||
      atan2_quadrant_two->as.float_value < 2.356194489 || atan2_quadrant_two->as.float_value > 2.356194491) {
    remove(path);
    return 24345;
  }
  if (atan2_quadrant_three == NULL || atan2_quadrant_three->kind != GVM_VALUE_FLOAT ||
      atan2_quadrant_three->as.float_value < -2.356194491 || atan2_quadrant_three->as.float_value > -2.356194489) {
    remove(path);
    return 24346;
  }
  if (hypot_diag == NULL || hypot_diag->kind != GVM_VALUE_FLOAT || hypot_diag->as.float_value < 4.999999999 ||
      hypot_diag->as.float_value > 5.000000001) {
    remove(path);
    return 24347;
  }
  if (hypot_large == NULL || hypot_large->kind != GVM_VALUE_FLOAT || hypot_large->as.float_value < 12.999999999 ||
      hypot_large->as.float_value > 13.000000001) {
    remove(path);
    return 24348;
  }
  if (hypot_negative == NULL || hypot_negative->kind != GVM_VALUE_FLOAT ||
      hypot_negative->as.float_value < 4.999999999 || hypot_negative->as.float_value > 5.000000001) {
    remove(path);
    return 24349;
  }
  if (degrees_zero == NULL || degrees_zero->kind != GVM_VALUE_FLOAT || degrees_zero->as.float_value < -0.000000001 ||
      degrees_zero->as.float_value > 0.000000001) {
    remove(path);
    return 24350;
  }
  if (degrees_right_angle == NULL || degrees_right_angle->kind != GVM_VALUE_FLOAT ||
      degrees_right_angle->as.float_value < 89.999999999 || degrees_right_angle->as.float_value > 90.000000001) {
    remove(path);
    return 24351;
  }
  if (degrees_negative_quarter == NULL || degrees_negative_quarter->kind != GVM_VALUE_FLOAT ||
      degrees_negative_quarter->as.float_value < -45.000000001 ||
      degrees_negative_quarter->as.float_value > -44.999999999) {
    remove(path);
    return 24352;
  }
  if (radians_zero == NULL || radians_zero->kind != GVM_VALUE_FLOAT || radians_zero->as.float_value < -0.000000001 ||
      radians_zero->as.float_value > 0.000000001) {
    remove(path);
    return 24353;
  }
  if (radians_straight == NULL || radians_straight->kind != GVM_VALUE_FLOAT ||
      radians_straight->as.float_value < 3.141592652589793 || radians_straight->as.float_value > 3.141592654589793) {
    remove(path);
    return 24354;
  }
  if (radians_negative_quarter == NULL || radians_negative_quarter->kind != GVM_VALUE_FLOAT ||
      radians_negative_quarter->as.float_value < -0.7853981643974483 ||
      radians_negative_quarter->as.float_value > -0.7853981623974483) {
    remove(path);
    return 24355;
  }
  if (isnan_nan == NULL || isnan_nan->kind != GVM_VALUE_BOOL || isnan_nan->as.bool_value != 1) {
    remove(path);
    return 24356;
  }
  if (isnan_one == NULL || isnan_one->kind != GVM_VALUE_BOOL || isnan_one->as.bool_value != 0) {
    remove(path);
    return 24357;
  }
  if (isnan_count == NULL || isnan_count->kind != GVM_VALUE_BOOL || isnan_count->as.bool_value != 0) {
    remove(path);
    return 24358;
  }
  if (exp_int == NULL || exp_int->kind != GVM_VALUE_FLOAT || exp_int->as.float_value < 2.718281828 ||
      exp_int->as.float_value > 2.718281829) {
    remove(path);
    return 2433;
  }
  if (exp_float == NULL || exp_float->kind != GVM_VALUE_FLOAT || exp_float->as.float_value != 1.0) {
    remove(path);
    return 2434;
  }
  if (exp_expr == NULL || exp_expr->kind != GVM_VALUE_FLOAT || exp_expr->as.float_value < 7.389056098 ||
      exp_expr->as.float_value > 7.389056100) {
    remove(path);
    return 2435;
  }
  if (ln_int == NULL || ln_int->kind != GVM_VALUE_FLOAT || ln_int->as.float_value != 0.0) {
    remove(path);
    return 2436;
  }
  if (ln_float == NULL || ln_float->kind != GVM_VALUE_FLOAT || ln_float->as.float_value < 0.999999999 ||
      ln_float->as.float_value > 1.000000001) {
    remove(path);
    return 2437;
  }
  if (ln_expr == NULL || ln_expr->kind != GVM_VALUE_FLOAT || ln_expr->as.float_value < 1.999999999 ||
      ln_expr->as.float_value > 2.000000001) {
    remove(path);
    return 2438;
  }
  if (log_int == NULL || log_int->kind != GVM_VALUE_FLOAT || log_int->as.float_value < 2.999999999 ||
      log_int->as.float_value > 3.000000001) {
    remove(path);
    return 2439;
  }
  if (log_float == NULL || log_float->kind != GVM_VALUE_FLOAT || log_float->as.float_value < 1.999999999 ||
      log_float->as.float_value > 2.000000001) {
    remove(path);
    return 2440;
  }
  if (log_expr == NULL || log_expr->kind != GVM_VALUE_FLOAT || log_expr->as.float_value < 4.999999999 ||
      log_expr->as.float_value > 5.000000001) {
    remove(path);
    return 2441;
  }
  if (log10_int == NULL || log10_int->kind != GVM_VALUE_FLOAT || log10_int->as.float_value < 2.999999999 ||
      log10_int->as.float_value > 3.000000001) {
    remove(path);
    return 24411;
  }
  if (log10_float == NULL || log10_float->kind != GVM_VALUE_FLOAT || log10_float->as.float_value < 0.999999999 ||
      log10_float->as.float_value > 1.000000001) {
    remove(path);
    return 24412;
  }
  if (log10_expr == NULL || log10_expr->kind != GVM_VALUE_FLOAT || log10_expr->as.float_value < 3.999999999 ||
      log10_expr->as.float_value > 4.000000001) {
    remove(path);
    return 24413;
  }
  if (log2_int == NULL || log2_int->kind != GVM_VALUE_FLOAT || log2_int->as.float_value < 2.999999999 ||
      log2_int->as.float_value > 3.000000001) {
    remove(path);
    return 24414;
  }
  if (log2_float == NULL || log2_float->kind != GVM_VALUE_FLOAT || log2_float->as.float_value < 0.999999999 ||
      log2_float->as.float_value > 1.000000001) {
    remove(path);
    return 24415;
  }
  if (log2_expr == NULL || log2_expr->kind != GVM_VALUE_FLOAT || log2_expr->as.float_value < 5.999999999 ||
      log2_expr->as.float_value > 6.000000001) {
    remove(path);
    return 24416;
  }
  if (floor_int == NULL || floor_int->kind != GVM_VALUE_INT || floor_int->as.int_value != 7) {
    remove(path);
    return 24417;
  }
  if (floor_float == NULL || floor_float->kind != GVM_VALUE_FLOAT || floor_float->as.float_value != 7.0) {
    remove(path);
    return 24418;
  }
  if (floor_negative == NULL || floor_negative->kind != GVM_VALUE_FLOAT || floor_negative->as.float_value != -4.0) {
    remove(path);
    return 24419;
  }
  if (ceil_int == NULL || ceil_int->kind != GVM_VALUE_INT || ceil_int->as.int_value != 7) {
    remove(path);
    return 24420;
  }
  if (ceil_float == NULL || ceil_float->kind != GVM_VALUE_FLOAT || ceil_float->as.float_value != 8.0) {
    remove(path);
    return 24421;
  }
  if (ceil_negative == NULL || ceil_negative->kind != GVM_VALUE_FLOAT || ceil_negative->as.float_value != -3.0) {
    remove(path);
    return 24422;
  }
  if (round_int == NULL || round_int->kind != GVM_VALUE_INT || round_int->as.int_value != 7) {
    remove(path);
    return 24423;
  }
  if (round_float == NULL || round_float->kind != GVM_VALUE_FLOAT || round_float->as.float_value != 7.0) {
    remove(path);
    return 24424;
  }
  if (round_half == NULL || round_half->kind != GVM_VALUE_FLOAT || round_half->as.float_value != 8.0) {
    remove(path);
    return 24425;
  }
  if (round_negative == NULL || round_negative->kind != GVM_VALUE_FLOAT || round_negative->as.float_value != -3.0) {
    remove(path);
    return 24426;
  }
  if (round_negative_half == NULL || round_negative_half->kind != GVM_VALUE_FLOAT ||
      round_negative_half->as.float_value != -4.0) {
    remove(path);
    return 24427;
  }
  if (trunc_int == NULL || trunc_int->kind != GVM_VALUE_INT || trunc_int->as.int_value != 7) {
    remove(path);
    return 24428;
  }
  if (trunc_float == NULL || trunc_float->kind != GVM_VALUE_FLOAT || trunc_float->as.float_value != 7.0) {
    remove(path);
    return 24429;
  }
  if (trunc_negative == NULL || trunc_negative->kind != GVM_VALUE_FLOAT || trunc_negative->as.float_value != -3.0) {
    remove(path);
    return 24430;
  }
  if (trunc_small_negative == NULL || trunc_small_negative->kind != GVM_VALUE_FLOAT ||
      trunc_small_negative->as.float_value != 0.0) {
    remove(path);
    return 24431;
  }
  if (sign_positive == NULL || sign_positive->kind != GVM_VALUE_INT || sign_positive->as.int_value != 1) {
    remove(path);
    return 24432;
  }
  if (sign_negative == NULL || sign_negative->kind != GVM_VALUE_INT || sign_negative->as.int_value != -1) {
    remove(path);
    return 24433;
  }
  if (sign_zero == NULL || sign_zero->kind != GVM_VALUE_INT || sign_zero->as.int_value != 0) {
    remove(path);
    return 24434;
  }
  if (pi_value == NULL || pi_value->kind != GVM_VALUE_FLOAT || pi_value->as.float_value != 3.14159265358979323846) {
    remove(path);
    return 2431;
  }
  if (e_value == NULL || e_value->kind != GVM_VALUE_FLOAT || e_value->as.float_value != 2.71828182845904523536) {
    remove(path);
    return 2432;
  }
  if (nan_value == NULL || nan_value->kind != GVM_VALUE_FLOAT || !isnan(nan_value->as.float_value)) {
    remove(path);
    return 2433;
  }
  if (inf_value == NULL || inf_value->kind != GVM_VALUE_FLOAT || !isinf(inf_value->as.float_value) ||
      inf_value->as.float_value <= 0.0) {
    remove(path);
    return 2434;
  }
  if (factorial_zero == NULL || factorial_zero->kind != GVM_VALUE_INT || factorial_zero->as.int_value != 1) {
    remove(path);
    return 244;
  }
  if (factorial_int == NULL || factorial_int->kind != GVM_VALUE_INT || factorial_int->as.int_value != 120) {
    remove(path);
    return 245;
  }
  if (factorial_group == NULL || factorial_group->kind != GVM_VALUE_INT || factorial_group->as.int_value != 6) {
    remove(path);
    return 246;
  }
  if (len_empty == NULL || len_empty->kind != GVM_VALUE_INT || len_empty->as.int_value != 0) {
    remove(path);
    return 247;
  }
  if (len_text == NULL || len_text->kind != GVM_VALUE_INT || len_text->as.int_value != 8) {
    remove(path);
    return 248;
  }
  if (len_concat == NULL || len_concat->kind != GVM_VALUE_INT || len_concat->as.int_value != 8) {
    remove(path);
    return 249;
  }
  if (total == NULL || total->kind != GVM_VALUE_FLOAT || total->as.float_value != 15.0) {
    remove(path);
    return 24;
  }
  if (remainder == NULL || remainder->kind != GVM_VALUE_INT || remainder->as.int_value != 2) {
    remove(path);
    return 25;
  }
  if (negative_remainder == NULL || negative_remainder->kind != GVM_VALUE_INT || negative_remainder->as.int_value != -2) {
    remove(path);
    return 26;
  }
  if (float_remainder == NULL || float_remainder->kind != GVM_VALUE_FLOAT || float_remainder->as.float_value != 1.5) {
    remove(path);
    return 27;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 28;
  }
  remove(path);
  normalize_text_newlines(output);
  if (strcmp(output, "42\n7\n9\n5\n3.5\n15\n-3\n7\n-12\n-3.5\n3\n-4\n3\n8\n-8\n0.5\n-5\n-3\n-3\n512\n9\n42\n3.5\n3\n3\n2\n8\n7\n3.5\n9\n0\n5\n10\n10\n3\n1.5\n3\n3\n-2\n3\n0\n1\n1\n0\n1.1752\n-1.1752\n0\n0.881374\n-0.881374\n0\n1.31696\n2.06344\n1\n1.54308\n1.54308\n0\n0.761594\n-0.761594\n0\n0.549306\n-0.549306\n1\n-1\n-1\n0\n1\n1\n0\n1.5708\n0.523599\n0\n1.5708\n1.0472\n0\n0.785398\n-0.785398\n0.785398\n2.35619\n-2.35619\n5\n13\n5\n0\n90\n-45\n0\n3.14159\n-0.785398\ntrue\nfalse\nfalse\n2.71828\n1\n7.38906\n0\n1\n2\n3\n2\n5\n3\n1\n4\n3\n1\n6\n7\n7\n-4\n7\n8\n-3\n7\n7\n8\n-3\n-4\n7\n7\n-3\n0\n1\n-1\n0\n3.14159\n2.71828\n1\n120\n6\n0\n8\n8\n2\n-2\n1.5\n11\n14\n2\n") != 0) {
    return 29;
  }
  return 0;
}

int test_gion_string_concatenation(void) {
  const char *source =
      "label = \"debut\" + \"fin\"\n"
      "full = label + \"!\"\n"
      "print(label)\n"
      "print(full)\n";
  const char *path = "gion_string_concatenation.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *label;
  const graphion_runtime_value *full;
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
  const char *path = "gion_print_string_coercion.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
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
  const char *path = "gion_compound_assignments.txt";
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
    const char *message;
  } cases[] = {
      {"count += 1\n", GINT_ERR_UNKNOWN_VARIABLE, "unknown variable"},
      {"count = 1\ncount +=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount -=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount *=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount /=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount //=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount %=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount **=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"mask = 0b11\nmask &=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"merge = 0b11\nmerge |=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"flip = 0b11\nflip ^=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"shift = 0b11\nshift <<=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"shift = 0b11\nshift >>=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount /= 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 1\ncount //= 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 1\ncount = count // 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 1\ncount %= 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 2\ncount **= \"x\"\n", GINT_ERR_RUN, "incompatible operand types"},
      {"count = 1\ncount += \"x\"\n", GINT_ERR_RUN, "incompatible operand types"},
      {"mask = 0b10\nmask &= 0b0010\n", GINT_ERR_RUN, "incompatible operand types"},
      {"mask = 0b10\nmask &= 1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"merge = 0b10\nmerge |= 0b0010\n", GINT_ERR_RUN, "incompatible operand types"},
      {"merge = 0b10\nmerge |= 1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"flip = 0b10\nflip ^= 0b0010\n", GINT_ERR_RUN, "incompatible operand types"},
      {"flip = 0b10\nflip ^= 1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"shift = 0b10\nshift <<= 0b0010\n", GINT_ERR_RUN, "incompatible operand types"},
      {"shift = 0b10\nshift <<= 1.0\n", GINT_ERR_RUN, "incompatible operand types"},
      {"shift = 0b10\nshift <<= -1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"shift = 0b10\nshift >>= 0b0010\n", GINT_ERR_RUN, "incompatible operand types"},
      {"shift = 0b10\nshift >>= 1.0\n", GINT_ERR_RUN, "incompatible operand types"},
      {"shift = 0b10\nshift >>= -1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = \"Test \" + 7\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext -= \"y\"\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext *= 2\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext /= 2\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext %= 2\n", GINT_ERR_RUN, "incompatible operand types"},
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

int test_gion_arithmetic_precedence_and_associativity(void) {
  const char *source =
      "a = 20 - 5 - 3\n"
      "b = 20 / 5 / 2\n"
      "c = 2 * 3 + 4 * 5\n"
      "d = 2 + 3 * 4 - 5\n"
      "expr_e = 10 - 2 * 3\n"
      "f = 2 * 3 / 4\n"
      "g = -7 + 2\n"
      "h = 2 + -3 * 4\n"
      "i = (2 + 3) * 4\n"
      "j = 2 * (3 + 4)\n"
      "k = (20 - 5) - 3\n"
      "l = 20 - (5 - 3)\n"
      "print(a)\n"
      "print(b)\n"
      "print(c)\n"
      "print(d)\n"
      "print(expr_e)\n"
      "print(f)\n"
      "print(g)\n"
      "print(h)\n"
      "print(i)\n"
      "print(j)\n"
      "print(k)\n"
      "print(l)\n";
  const char *path = "gion_arithmetic_precedence.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
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
    return 1;
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return 2;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 3;
  }
  remove(path);
  if (strcmp(output, "12\n2\n26\n9\n4\n1.5\n-5\n-10\n20\n14\n12\n18\n") != 0) {
    return 4;
  }
  return 0;
}

int test_gion_arithmetic_runtime_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"value = 1 / 0\n", GINT_ERR_RUN, "division by zero"},
      {"value = \"x\" + 1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = true + 1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = abs(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = min(\"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = min(1, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = max(\"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = max(1, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = clamp(\"x\", 0, 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = clamp(1, \"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = clamp(1, 0, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = sqrt(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = cbrt(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = sin(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = sinh(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = asinh(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = acosh(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = cosh(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = tanh(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = atanh(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = cos(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = tan(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = asin(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = atan(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = atan2(\"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = atan2(1, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = hypot(\"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = hypot(1, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = degrees(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = radians(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = isnan(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = exp(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = ln(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = log(\"x\", 2)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = log(8, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = log10(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = log2(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = floor(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = ceil(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = round(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = trunc(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = sign(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = sqrt(-1)\n", GINT_ERR_RUN, "sqrt requires non-negative input"},
      {"value = ln(0)\n", GINT_ERR_RUN, "ln requires strictly positive input"},
      {"value = ln(-1)\n", GINT_ERR_RUN, "ln requires strictly positive input"},
      {"value = log(0, 10)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log(-1, 10)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log(8, 0)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log(8, -2)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log(8, 1)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log10(0)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log10(-1)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log2(0)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = log2(-1)\n", GINT_ERR_RUN, "log requires x > 0 and base > 0 with base != 1"},
      {"value = asin(2)\n", GINT_ERR_RUN, "asin requires input in [-1, 1]"},
      {"value = asin(-2)\n", GINT_ERR_RUN, "asin requires input in [-1, 1]"},
        {"value = acos(2)\n", GINT_ERR_RUN, "acos requires input in [-1, 1]"},
        {"value = acos(-2)\n", GINT_ERR_RUN, "acos requires input in [-1, 1]"},
      {"value = acosh(0)\n", GINT_ERR_RUN, "acosh requires input >= 1"},
      {"value = acosh(0.5)\n", GINT_ERR_RUN, "acosh requires input >= 1"},
      {"value = atanh(1)\n", GINT_ERR_RUN, "atanh requires input in (-1, 1)"},
      {"value = atanh(-1)\n", GINT_ERR_RUN, "atanh requires input in (-1, 1)"},
      {"value = atanh(2)\n", GINT_ERR_RUN, "atanh requires input in (-1, 1)"},
      {"value = (-1)!\n", GINT_ERR_RUN, "factorial requires non-negative integer input"},
      {"value = 1.5!\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = len(1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"print(\"x\" / 2)\n", GINT_ERR_RUN, "incompatible operand types"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(2 + i * 10U);
    }
  }
  return 0;
}

int test_gion_arithmetic_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"value = 1 + * 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 / / 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 ** * 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 % % 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 2 **\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 2 //\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 + 2 3\n", GINT_ERR_PARSE, "unsupported assignment expression"},
      {"value = 1\nvalue +=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value + 2\n", GINT_ERR_PARSE, "expected '='"},
      {"value = abs()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = abs(1 + 2\n", GINT_ERR_PARSE, "expected ')' after abs argument"},
      {"value = min()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = min(1)\n", GINT_ERR_PARSE, "expected ',' between min arguments"},
      {"value = min(1,)\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = min(1, 2\n", GINT_ERR_PARSE, "expected ')' after min arguments"},
      {"value = min(1 2)\n", GINT_ERR_PARSE, "expected ',' between min arguments"},
      {"value = max()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = max(1)\n", GINT_ERR_PARSE, "expected ',' between max arguments"},
      {"value = max(1,)\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = max(1, 2\n", GINT_ERR_PARSE, "expected ')' after max arguments"},
      {"value = max(1 2)\n", GINT_ERR_PARSE, "expected ',' between max arguments"},
      {"value = clamp()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = clamp(1)\n", GINT_ERR_PARSE, "expected ',' after clamp value"},
      {"value = clamp(1,)\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = clamp(1, 2)\n", GINT_ERR_PARSE, "expected ',' after clamp lower bound"},
      {"value = clamp(1, 2,)\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = clamp(1, 2, 3\n", GINT_ERR_PARSE, "expected ')' after clamp arguments"},
      {"value = clamp(1 2, 3)\n", GINT_ERR_PARSE, "expected ',' after clamp value"},
      {"value = sqrt()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = sqrt(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sqrt argument"},
      {"value = cbrt()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = cbrt(1 + 2\n", GINT_ERR_PARSE, "expected ')' after cbrt argument"},
      {"value = sin()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = sin(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sin argument"},
      {"value = sinh()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = sinh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sinh argument"},
      {"value = asinh()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = asinh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after asinh argument"},
      {"value = acosh()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = acosh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after acosh argument"},
      {"value = cosh()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = cosh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after cosh argument"},
      {"value = tanh()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = tanh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after tanh argument"},
      {"value = atanh()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = atanh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after atanh argument"},
      {"value = cos()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = cos(1 + 2\n", GINT_ERR_PARSE, "expected ')' after cos argument"},
      {"value = tan()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = tan(1 + 2\n", GINT_ERR_PARSE, "expected ')' after tan argument"},
      {"value = asin()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = asin(1 + 2\n", GINT_ERR_PARSE, "expected ')' after asin argument"},
      {"value = atan()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = atan(1 + 2\n", GINT_ERR_PARSE, "expected ')' after atan argument"},
      {"value = atan2()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = atan2(1)\n", GINT_ERR_PARSE, "expected ',' between atan2 arguments"},
      {"value = atan2(1,)\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = atan2(1, 1\n", GINT_ERR_PARSE, "expected ')' after atan2 arguments"},
      {"value = atan2(1 1)\n", GINT_ERR_PARSE, "expected ',' between atan2 arguments"},
      {"value = hypot()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = hypot(1)\n", GINT_ERR_PARSE, "expected ',' between hypot arguments"},
      {"value = hypot(1,)\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = hypot(1, 1\n", GINT_ERR_PARSE, "expected ')' after hypot arguments"},
      {"value = hypot(1 1)\n", GINT_ERR_PARSE, "expected ',' between hypot arguments"},
      {"value = degrees()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = degrees(1 + 2\n", GINT_ERR_PARSE, "expected ')' after degrees argument"},
      {"value = radians()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = radians(1 + 2\n", GINT_ERR_PARSE, "expected ')' after radians argument"},
      {"value = isnan()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = isnan(1 + 2\n", GINT_ERR_PARSE, "expected ')' after isnan argument"},
      {"value = exp()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = exp(1 + 2\n", GINT_ERR_PARSE, "expected ')' after exp argument"},
      {"value = ln()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = ln(1 + 2\n", GINT_ERR_PARSE, "expected ')' after ln argument"},
      {"value = log()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = log(8)\n", GINT_ERR_PARSE, "expected ',' between log arguments"},
      {"value = log(8,)\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = log(8, 2\n", GINT_ERR_PARSE, "expected ')' after log arguments"},
      {"value = log(8 2)\n", GINT_ERR_PARSE, "expected ',' between log arguments"},
      {"value = log10()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = log10(1 + 2\n", GINT_ERR_PARSE, "expected ')' after log10 argument"},
      {"value = log2()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = log2(1 + 2\n", GINT_ERR_PARSE, "expected ')' after log2 argument"},
      {"value = floor()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = floor(1 + 2\n", GINT_ERR_PARSE, "expected ')' after floor argument"},
      {"value = ceil()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = ceil(1 + 2\n", GINT_ERR_PARSE, "expected ')' after ceil argument"},
      {"value = round()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = round(1 + 2\n", GINT_ERR_PARSE, "expected ')' after round argument"},
      {"value = trunc()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = trunc(1 + 2\n", GINT_ERR_PARSE, "expected ')' after trunc argument"},
      {"value = sign()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = sign(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sign argument"},
      {"value = !\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = len()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = len(\"x\"\n", GINT_ERR_PARSE, "expected ')' after len argument"},
      {"value = (1 + 2\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = -\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = -(1 + 2\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = 1 + (2 * 3\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = ()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print(1 + )\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print((1 + 2)\n", GINT_ERR_PARSE, "expected ')' after print argument"},
      {"print(()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print(1 + 2\n", GINT_ERR_PARSE, "expected ')' after print argument"},
      {"print(1 + 2 3)\n", GINT_ERR_PARSE, "expected ')' after print argument"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(2 + i * 10U);
    }
  }
  return 0;
}

int test_gion_mixed_scalar_values(void) {
  const char *source =
      "i = -7\n"
      "f = -3.25\n"
      "b = false\n"
      "s = \"hello\"\n"
      "copy_i = i\n"
      "print(i)\n"
      "print(f)\n"
      "print(b)\n"
      "print(s)\n"
      "print(copy_i)\n";
  const char *path = "gion_mixed_scalar_values.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *copy_i;
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
  copy_i = graphion_runtime_scope_find(&scope, "copy_i");
  if (copy_i == NULL || copy_i->kind != GVM_VALUE_INT || copy_i->as.int_value != -7) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "-7\n-3.25\nfalse\nhello\n-7\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_capacity_errors(void) {
  {
    char source[GRAPHION_RUNTIME_NAME_MAX + 32U];
    memset(source, 'a', GRAPHION_RUNTIME_NAME_MAX);
    source[GRAPHION_RUNTIME_NAME_MAX] = '\0';
    memcpy(source + GRAPHION_RUNTIME_NAME_MAX, " = 1\n", 6U);

    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(source, &scope, &diagnostic);
    if (rc != GINT_ERR_CAPACITY) {
      return 1;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "identifier too long") != 0) {
      return 2;
    }
  }

  {
    char source[640];
    memset(source, 'a', sizeof(source));
    source[0] = 'n';
    source[1] = 'a';
    source[2] = 'm';
    source[3] = 'e';
    source[4] = ' ';
    source[5] = '=';
    source[6] = ' ';
    source[7] = '"';
    memset(source + 8, 'x', 620U);
    source[628] = '"';
    source[629] = '\n';
    source[630] = '\0';

    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(source, &scope, &diagnostic);
    if (rc != GINT_ERR_CAPACITY) {
      return 10;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "source line too long") != 0) {
      return 11;
    }
  }

  {
    char source[4096];
    size_t offset = 0U;
    unsigned int i;
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    source[0] = '\0';
    for (i = 0U; i < 160U; ++i) {
      int written = snprintf(source + offset, sizeof(source) - offset, "v%u = %u\n", i, i);
      if (written <= 0) {
        return 20;
      }
      offset += (size_t)written;
      if (offset >= sizeof(source)) {
        return 21;
      }
    }

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(source, &scope, &diagnostic);
    if (rc != GINT_OK) {
      return 22;
    }
    if (scope.global_count != 160U) {
      graphion_runtime_scope_dispose(&scope);
      return 23;
    }
    graphion_runtime_scope_dispose(&scope);
  }

  return 0;
}
