/* SPDX-License-Identifier: MIT */

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "test_gion_scalar_math_case.h"
#include "test_parser_helpers.h"

typedef struct {
  const char *name;
  int expected;
  int error_code;
} int_expectation;

typedef struct {
  const char *name;
  int expected;
  int error_code;
} bool_expectation;

typedef struct {
  const char *name;
  double expected;
  int error_code;
} exact_float_expectation;

typedef struct {
  const char *name;
  double min_value;
  double max_value;
  int error_code;
} ranged_float_expectation;

typedef enum {
  TEST_FLOAT_NAN,
  TEST_FLOAT_POS_INF,
} special_float_kind;

typedef struct {
  const char *name;
  special_float_kind kind;
  int error_code;
} special_float_expectation;

static size_t split_lines_inplace(char *text, const char **lines, size_t capacity) {
  size_t count = 0U;
  char *cursor = text;

  if (text == NULL || lines == NULL || capacity == 0U) {
    return 0U;
  }

  while (*cursor != '\0' && count < capacity) {
    char *newline = strchr(cursor, '\n');
    lines[count++] = cursor;
    if (newline == NULL) {
      break;
    }
    *newline = '\0';
    cursor = newline + 1;
  }
  return count;
}

static int parse_double_strict(const char *text, double *value_out) {
  char *end = NULL;
  double value;

  if (text == NULL || *text == '\0' || value_out == NULL) {
    return 0;
  }
  errno = 0;
  value = strtod(text, &end);
  if (errno != 0 || end == text || *end != '\0') {
    return 0;
  }
  *value_out = value;
  return 1;
}

static int double_is_nan(double value) { return value != value; }

static int double_is_infinite(double value) { return value > DBL_MAX || value < -DBL_MAX; }

static int double_is_negative(double value) { return copysign(1.0, value) < 0.0; }

static int lines_match_with_tolerance(const char *actual, const char *expected) {
  double actual_value;
  double expected_value;
  double diff;
  double scale;

  if (strcmp(actual, expected) == 0) {
    return 1;
  }
  if (!parse_double_strict(actual, &actual_value) || !parse_double_strict(expected, &expected_value)) {
    return 0;
  }
  if (double_is_nan(actual_value) && double_is_nan(expected_value)) {
    return 1;
  }
  if (double_is_infinite(actual_value) && double_is_infinite(expected_value) &&
      (double_is_negative(actual_value) == double_is_negative(expected_value))) {
    return 1;
  }
  diff = fabs(actual_value - expected_value);
  scale = fmax(fabs(actual_value), fabs(expected_value));
  if (diff <= 1e-4) {
    return 1;
  }
  return diff <= (1e-6 * (scale > 1.0 ? scale : 1.0));
}

static int build_scalar_math_source(char *source, size_t capacity) {
  static const char *source_parts[] = {
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
      "csc_right_angle = csc(pi / 2)\n"
      "csc_negative_right_angle = csc(-pi / 2)\n"
      "csc_pi_sixth = csc(0.5235987755982988)\n"
      "sec_zero = sec(0)\n"
      "sec_pi = sec(pi)\n"
      "sec_pi_third = sec(1.0471975511965976)\n"
      "cot_pi_fourth = cot(0.7853981633974483)\n"
      "cot_negative_pi_fourth = cot(-0.7853981633974483)\n"
      "cot_pi_sixth = cot(0.5235987755982988)\n"
      "acsc_one = acsc(1)\n"
      "acsc_two = acsc(2)\n"
      "acsc_negative_two = acsc(-2)\n"
      "asec_one = asec(1)\n"
      "asec_two = asec(2)\n"
      "asec_negative_two = asec(-2)\n"
      "acot_one = acot(1)\n"
      "acot_zero = acot(0)\n"
      "acot_negative_one = acot(-1)\n"
      "sech_zero = sech(0)\n"
      "sech_one = sech(1)\n"
      "sech_negative = sech(-1)\n"
      "csch_one = csch(1)\n"
      "csch_negative = csch(-1)\n"
      "csch_two = csch(2)\n"
      "coth_one = coth(1)\n"
      "coth_negative = coth(-1)\n"
      "coth_two = coth(2)\n"
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
      "copysign_negative = copysign(3, -2)\n"
      "copysign_positive = copysign(-3.5, 2)\n"
      "copysign_same_negative = copysign(-3, -2)\n"
      "fma_basic = fma(2, 3, 4)\n"
      "fma_mixed = fma(0.5, 8, -1)\n"
      "fma_negative = fma(-2, 4, 1.5)\n"
      "fdim_positive = fdim(7, 3)\n"
      "fdim_zero = fdim(3, 7)\n"
      "fdim_equal = fdim(5, 5)\n"
      "remainder_nearest = remainder(7, 4)\n"
      "remainder_fractional = remainder(5.5, 2)\n"
      "remainder_negative = remainder(-7, 4)\n"
      "rint_int = rint(7)\n"
      "rint_float = rint(7.4)\n"
      "rint_negative = rint(-3.2)\n"
      "degrees_zero = degrees(0)\n"
      "degrees_right_angle = degrees(pi / 2)\n"
      "degrees_negative_quarter = degrees(-0.7853981633974483)\n"
      "radians_zero = radians(0)\n"
      "radians_straight = radians(180)\n"
      "radians_negative_quarter = radians(-45)\n"
      "isnan_nan = isnan(nan)\n"
      "isnan_one = isnan(1.0)\n"
      "isnan_count = isnan(7)\n"
      "isinf_inf = isinf(inf)\n"
      "isinf_one = isinf(1.0)\n"
      "isinf_count = isinf(7)\n"
      "isfinite_inf = isfinite(inf)\n"
      "isfinite_nan = isfinite(nan)\n"
      "isfinite_one = isfinite(1.0)\n"
      "isfinite_count = isfinite(7)\n"
      "expm1_int = expm1(1)\n"
      "expm1_float = expm1(0.0)\n"
      "expm1_expr = expm1(1 + 1)\n"
      "exp2_int = exp2(1)\n"
      "exp2_float = exp2(0.0)\n"
      "exp2_expr = exp2(1 + 1)\n"
      "log1p_int = log1p(1)\n"
      "log1p_float = log1p(0.0)\n"
      "log1p_expr = log1p(2 - 1.5)\n"
      "erf_zero = erf(0)\n"
      "erf_one = erf(1)\n"
      "erf_negative_one = erf(-1)\n"
      "erfc_zero = erfc(0)\n"
      "erfc_one = erfc(1)\n"
      "erfc_negative_one = erfc(-1)\n"
      "gamma_one = gamma(1)\n"
      "gamma_five = gamma(5)\n"
      "gamma_half = gamma(0.5)\n"
      "lgamma_one = lgamma(1)\n"
      "lgamma_five = lgamma(5)\n"
      "lgamma_half = lgamma(0.5)\n",
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
      "fract_int = fract(7)\n"
      "fract_float = fract(7.25)\n"
      "fract_negative = fract(-3.75)\n"
      "sign_positive = sign(7)\n"
      "sign_negative = sign(-3.9)\n"
      "sign_zero = sign(0)\n"
      "pi_value = pi\n"
      "tau_value = tau\n"
      "phi_value = phi\n"
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
      "modulo_value = 10 % 4\n"
      "modulo_negative = -10 % 4\n"
      "modulo_float = 7.5 % 2\n"
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
      "print(csc_right_angle)\n"
      "print(csc_negative_right_angle)\n"
      "print(csc_pi_sixth)\n"
      "print(sec_zero)\n"
      "print(sec_pi)\n"
      "print(sec_pi_third)\n"
      "print(cot_pi_fourth)\n"
      "print(cot_negative_pi_fourth)\n"
      "print(cot_pi_sixth)\n"
      "print(acsc_one)\n"
      "print(acsc_two)\n"
      "print(acsc_negative_two)\n"
      "print(asec_one)\n"
      "print(asec_two)\n"
      "print(asec_negative_two)\n"
      "print(acot_one)\n"
      "print(acot_zero)\n"
      "print(acot_negative_one)\n"
      "print(sech_zero)\n"
      "print(sech_one)\n"
      "print(sech_negative)\n"
      "print(csch_one)\n"
      "print(csch_negative)\n"
      "print(csch_two)\n",
      "print(coth_one)\n"
      "print(coth_negative)\n"
      "print(coth_two)\n"
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
      "print(copysign_negative)\n"
      "print(copysign_positive)\n"
      "print(copysign_same_negative)\n"
      "print(fma_basic)\n"
      "print(fma_mixed)\n"
      "print(fma_negative)\n"
      "print(fdim_positive)\n"
      "print(fdim_zero)\n"
      "print(fdim_equal)\n"
      "print(remainder_nearest)\n"
      "print(remainder_fractional)\n"
      "print(remainder_negative)\n"
      "print(rint_int)\n"
      "print(rint_float)\n"
      "print(rint_negative)\n"
      "print(degrees_zero)\n"
      "print(degrees_right_angle)\n"
      "print(degrees_negative_quarter)\n"
      "print(radians_zero)\n"
      "print(radians_straight)\n"
      "print(radians_negative_quarter)\n"
      "print(isnan_nan)\n"
      "print(isnan_one)\n"
      "print(isnan_count)\n"
      "print(isinf_inf)\n"
      "print(isinf_one)\n"
      "print(isinf_count)\n"
      "print(isfinite_inf)\n"
      "print(isfinite_nan)\n"
      "print(isfinite_one)\n"
      "print(isfinite_count)\n"
      "print(expm1_int)\n"
      "print(expm1_float)\n"
      "print(expm1_expr)\n"
      "print(exp2_int)\n"
      "print(exp2_float)\n"
      "print(exp2_expr)\n"
      "print(log1p_int)\n"
      "print(log1p_float)\n"
      "print(log1p_expr)\n"
      "print(erf_zero)\n"
      "print(erf_one)\n"
      "print(erf_negative_one)\n"
      "print(erfc_zero)\n"
      "print(erfc_one)\n"
      "print(erfc_negative_one)\n"
      "print(gamma_one)\n"
      "print(gamma_five)\n"
      "print(gamma_half)\n"
      "print(lgamma_one)\n"
      "print(lgamma_five)\n"
      "print(lgamma_half)\n",
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
      "print(fract_int)\n"
      "print(fract_float)\n"
      "print(fract_negative)\n"
      "print(sign_positive)\n"
      "print(sign_negative)\n"
      "print(sign_zero)\n"
      "print(pi_value)\n"
      "print(tau_value)\n"
      "print(phi_value)\n"
      "print(e_value)\n"
      "print(factorial_zero)\n"
      "print(factorial_int)\n"
      "print(factorial_group)\n"
      "print(len_empty)\n"
      "print(len_text)\n"
      "print(len_concat)\n"
      "print(modulo_value)\n"
      "print(modulo_negative)\n"
      "print(modulo_float)\n"
      "print(3 + 4 * 2)\n"
      "print((3 + 4) * 2)\n"
      "print(10 % 4)\n",
  };
  size_t source_len = 0U;
  size_t i;

  if (source == NULL || capacity == 0U) {
    return 250;
  }

  source[0] = '\0';
  for (i = 0U; i < (sizeof(source_parts) / sizeof(source_parts[0])); ++i) {
    size_t part_len = strlen(source_parts[i]);
    if (source_len + part_len + 1U > capacity) {
      return 250;
    }
    memcpy(source + source_len, source_parts[i], part_len);
    source_len += part_len;
    source[source_len] = '\0';
  }
  return 0;
}

static const int_expectation int_expectations[] = {
    {"sum", 42, 3},
    {"mixed", 7, 4},
    {"grouped", 9, 5},
    {"delta", 5, 6},
    {"negative_add", -3, 9},
    {"negative_sub", 7, 10},
    {"negative_mul", -12, 11},
    {"floor_half", 3, 13},
    {"negative_floor", -4, 14},
    {"neg_count", -5, 181},
    {"neg_group", -3, 182},
    {"neg_abs", -3, 183},
    {"abs_int", 42, 21},
    {"abs_expr", 3, 23},
    {"min_int", 3, 231},
    {"min_expr", 8, 233},
    {"max_int", 7, 234},
    {"max_expr", 9, 236},
    {"clamp_low", 0, 237},
    {"clamp_mid", 5, 238},
    {"clamp_high", 10, 239},
    {"floor_int", 7, 24417},
    {"ceil_int", 7, 24420},
    {"round_int", 7, 24423},
    {"trunc_int", 7, 24428},
    {"sign_positive", 1, 24432},
    {"sign_negative", -1, 24433},
    {"sign_zero", 0, 24434},
    {"factorial_zero", 1, 244},
    {"factorial_int", 120, 245},
    {"factorial_group", 6, 246},
    {"len_empty", 0, 247},
    {"len_text", 8, 248},
    {"len_concat", 8, 249},
    {"modulo_value", 2, 25},
    {"modulo_negative", -2, 26},
};

static const bool_expectation bool_expectations[] = {
    {"isnan_nan", 1, 24356},
    {"isnan_one", 0, 24357},
    {"isnan_count", 0, 24358},
    {"isinf_inf", 1, 24359},
    {"isinf_one", 0, 24360},
    {"isinf_count", 0, 24361},
    {"isfinite_inf", 0, 24362},
    {"isfinite_nan", 0, 24363},
    {"isfinite_one", 1, 24364},
    {"isfinite_count", 1, 24365},
};

static const exact_float_expectation exact_float_expectations[] = {
    {"ratio", 3.5, 7},
    {"scaled", 7.0, 8},
    {"negative_div", -3.5, 12},
    {"float_floor", 3.0, 15},
    {"power", 8.0, 16},
    {"negative_power", -8.0, 17},
    {"negative_exponent", 0.5, 18},
    {"right_assoc", 512.0, 19},
    {"powered_group", 9.0, 20},
    {"abs_float", 3.5, 22},
    {"min_float", 2.0, 232},
    {"max_float", 3.5, 235},
    {"clamp_float", 10.0, 240},
    {"sqrt_int", 3.0, 241},
    {"sqrt_float", 1.5, 242},
    {"sqrt_expr", 3.0, 243},
    {"sin_zero", 0.0, 24326},
    {"sinh_zero", 0.0, 2432821},
    {"cosh_zero", 1.0, 243284},
    {"copysign_negative", -3.0, 24378},
    {"copysign_positive", 3.5, 24379},
    {"copysign_same_negative", -3.0, 24380},
    {"fma_basic", 10.0, 24381},
    {"fma_mixed", 3.0, 24382},
    {"fma_negative", -6.5, 24383},
    {"fdim_positive", 4.0, 24384},
    {"fdim_zero", 0.0, 24385},
    {"fdim_equal", 0.0, 24386},
    {"remainder_nearest", -1.0, 24387},
    {"remainder_fractional", -0.5, 24388},
    {"remainder_negative", 1.0, 24389},
    {"rint_int", 7.0, 24390},
    {"rint_float", 7.0, 24391},
    {"rint_negative", -3.0, 24392},
    {"expm1_float", 0.0, 24367},
    {"exp2_float", 1.0, 24370},
    {"log1p_float", 0.0, 24373},
    {"erf_zero", 0.0, 24375},
    {"erfc_zero", 1.0, 24378},
    {"gamma_one", 1.0, 24381},
    {"gamma_five", 24.0, 24382},
    {"lgamma_one", 0.0, 24435},
    {"exp_float", 1.0, 2434},
    {"ln_int", 0.0, 2436},
    {"floor_float", 7.0, 24418},
    {"floor_negative", -4.0, 24419},
    {"ceil_float", 8.0, 24421},
    {"ceil_negative", -3.0, 24422},
    {"round_float", 7.0, 24424},
    {"round_half", 8.0, 24425},
    {"round_negative", -3.0, 24426},
    {"round_negative_half", -4.0, 24427},
    {"trunc_float", 7.0, 24429},
    {"trunc_negative", -3.0, 24430},
    {"trunc_small_negative", 0.0, 24431},
    {"fract_int", 0.0, 244315},
    {"pi_value", 3.14159265358979323846, 2431},
    {"tau_value", 6.28318530717958647692, 24315},
    {"phi_value", 1.61803398874989484820, 24316},
    {"e_value", 2.71828182845904523536, 2432},
    {"total", 15.0, 24},
    {"modulo_float", 1.5, 27},
};

static const ranged_float_expectation ranged_float_expectations[] = {
    {"cbrt_int", 2.999999999999, 3.000000000001, 2431},
    {"cbrt_negative", -2.000000000001, -1.999999999999, 2432},
    {"cbrt_expr", 2.999999999999, 3.000000000001, 24325},
    {"sin_half_turn", 0.999999999, 1.000000001, 24327},
    {"sin_expr", 0.999999999, 1.000000001, 24328},
    {"csc_right_angle", 0.999999999, 1.000000001, 243280},
    {"csc_negative_right_angle", -1.000000001, -0.999999999, 2432801},
    {"csc_pi_sixth", 1.999999999, 2.000000001, 2432802},
    {"sec_zero", 0.999999999, 1.000000001, 2432803},
    {"sec_pi", -1.000000001, -0.999999999, 2432804},
    {"sec_pi_third", 1.999999999, 2.000000001, 2432805},
    {"cot_pi_fourth", 0.999999999, 1.000000001, 2432806},
    {"cot_negative_pi_fourth", -1.000000001, -0.999999999, 2432807},
    {"cot_pi_sixth", 1.732050806, 1.732050809, 2432808},
    {"acsc_one", 1.570796326, 1.570796328, 2432809},
    {"acsc_two", 0.523598775, 0.523598777, 2432810},
    {"acsc_negative_two", -0.523598777, -0.523598775, 2432811},
    {"asec_one", -1e-09, 1e-09, 2432812},
    {"asec_two", 1.047197551, 1.047197553, 2432813},
    {"asec_negative_two", 2.094395101, 2.094395103, 2432814},
    {"acot_one", 0.785398163, 0.785398164, 2432815},
    {"acot_zero", 1.570796326, 1.570796328, 2432816},
    {"acot_negative_one", 2.356194489, 2.356194491, 2432817},
    {"sech_zero", 0.999999999, 1.000000001, 2432818},
    {"sech_one", 0.648054273, 0.648054274, 2432819},
    {"sech_negative", 0.648054273, 0.648054274, 2432820},
    {"csch_one", 0.850918127, 0.850918129, 24328201},
    {"csch_negative", -0.850918129, -0.850918127, 24328202},
    {"csch_two", 0.275720564, 0.275720566, 24328203},
    {"coth_one", 1.313035284, 1.313035286, 24328204},
    {"coth_negative", -1.313035286, -1.313035284, 24328205},
    {"coth_two", 1.037314719, 1.037314721, 24328206},
    {"sinh_one", 1.175201193, 1.175201195, 243282},
    {"sinh_negative", -1.175201195, -1.175201193, 243283},
    {"asinh_zero", -1e-09, 1e-09, 2432831},
    {"asinh_one", 0.881373586, 0.881373588, 2432832},
    {"asinh_negative", -0.881373588, -0.881373586, 2432833},
    {"acosh_one", -1e-09, 1e-09, 2432834},
    {"acosh_two", 1.316957895, 1.316957897, 2432835},
    {"acosh_four", 2.063437067, 2.063437069, 2432836},
    {"cosh_one", 1.543080634, 1.543080636, 243285},
    {"cosh_negative", 1.543080634, 1.543080636, 243286},
    {"tanh_zero", -1e-09, 1e-09, 243287},
    {"tanh_one", 0.761594155, 0.761594157, 243288},
    {"tanh_negative", -0.761594157, -0.761594155, 243289},
    {"atanh_zero", -1e-09, 1e-09, 2432891},
    {"atanh_half", 0.549306143, 0.549306145, 2432892},
    {"atanh_negative_half", -0.549306145, -0.549306143, 2432893},
    {"cos_zero", 0.999999999, 1.000000001, 24329},
    {"cos_pi", -1.000000001, -0.999999999, 24330},
    {"cos_expr", -1.000000001, -0.999999999, 24331},
    {"tan_zero", -1e-09, 1e-09, 24332},
    {"tan_quarter_turn", 0.999999999, 1.000000001, 24333},
    {"tan_expr", 0.999999999, 1.000000001, 24334},
    {"asin_zero", -1e-09, 1e-09, 24335},
    {"asin_one", 1.570796326, 1.570796328, 24336},
    {"asin_half", 0.523598775, 0.523598777, 24337},
    {"atan_zero", -1e-09, 1e-09, 24341},
    {"atan_one", 0.785398163, 0.785398164, 24342},
    {"atan_negative_one", -0.785398164, -0.785398163, 24343},
    {"atan2_diag", 0.785398163, 0.785398164, 24344},
    {"atan2_quadrant_two", 2.356194489, 2.356194491, 24345},
    {"atan2_quadrant_three", -2.356194491, -2.356194489, 24346},
    {"hypot_diag", 4.999999999, 5.000000001, 24347},
    {"hypot_large", 12.999999999, 13.000000001, 24348},
    {"hypot_negative", 4.999999999, 5.000000001, 24349},
    {"degrees_zero", -1e-09, 1e-09, 24350},
    {"degrees_right_angle", 89.999999999, 90.000000001, 24351},
    {"degrees_negative_quarter", -45.000000001, -44.999999999, 24352},
    {"radians_zero", -1e-09, 1e-09, 24353},
    {"radians_straight", 3.14159265258979, 3.14159265458979, 24354},
    {"radians_negative_quarter", -0.785398164397448, -0.785398162397448, 24355},
    {"expm1_int", 1.718281828, 1.718281829, 24366},
    {"expm1_expr", 6.389056098, 6.3890561, 24368},
    {"exp2_int", 1.999999999, 2.000000001, 24369},
    {"exp2_expr", 3.999999999, 4.000000001, 24371},
    {"log1p_int", 0.69314718, 0.693147181, 24372},
    {"log1p_expr", 0.405465108, 0.405465109, 24374},
    {"erf_one", 0.842700792, 0.842700793, 24376},
    {"erf_negative_one", -0.842700793, -0.842700792, 24377},
    {"erfc_one", 0.157299207, 0.157299208, 24379},
    {"erfc_negative_one", 1.842700792, 1.842700793, 24380},
    {"gamma_half", 1.772453849, 1.772453851, 24383},
    {"lgamma_five", 3.178053829, 3.178053831, 24436},
    {"lgamma_half", 0.572364941, 0.572364943, 24437},
    {"exp_int", 2.718281828, 2.718281829, 2433},
    {"exp_expr", 7.389056098, 7.3890561, 2435},
    {"ln_float", 0.999999999, 1.000000001, 2437},
    {"ln_expr", 1.999999999, 2.000000001, 2438},
    {"log_int", 2.999999999, 3.000000001, 2439},
    {"log_float", 1.999999999, 2.000000001, 2440},
    {"log_expr", 4.999999999, 5.000000001, 2441},
    {"log10_int", 2.999999999, 3.000000001, 24411},
    {"log10_float", 0.999999999, 1.000000001, 24412},
    {"log10_expr", 3.999999999, 4.000000001, 24413},
    {"log2_int", 2.999999999, 3.000000001, 24414},
    {"log2_float", 0.999999999, 1.000000001, 24415},
    {"log2_expr", 5.999999999, 6.000000001, 24416},
    {"fract_float", 0.249999999, 0.250000001, 244316},
    {"fract_negative", 0.249999999, 0.250000001, 244317},
};

static const special_float_expectation special_float_expectations[] = {
    {"nan_value", TEST_FLOAT_NAN, 2433},
    {"inf_value", TEST_FLOAT_POS_INF, 2434},
};

static int validate_int_expectations(graphion_runtime_scope *scope) {
  size_t i;

  for (i = 0U; i < sizeof(int_expectations) / sizeof(int_expectations[0]); ++i) {
    const graphion_runtime_value *value = graphion_runtime_scope_find(scope, int_expectations[i].name);
    if (value == NULL || value->kind != GVM_VALUE_INT || value->as.int_value != int_expectations[i].expected) {
      return int_expectations[i].error_code;
    }
  }
  return 0;
}

static int validate_bool_expectations(graphion_runtime_scope *scope) {
  size_t i;

  for (i = 0U; i < sizeof(bool_expectations) / sizeof(bool_expectations[0]); ++i) {
    const graphion_runtime_value *value = graphion_runtime_scope_find(scope, bool_expectations[i].name);
    if (value == NULL || value->kind != GVM_VALUE_BOOL || value->as.bool_value != bool_expectations[i].expected) {
      return bool_expectations[i].error_code;
    }
  }
  return 0;
}

static int validate_exact_float_expectations(graphion_runtime_scope *scope) {
  size_t i;

  for (i = 0U; i < sizeof(exact_float_expectations) / sizeof(exact_float_expectations[0]); ++i) {
    const graphion_runtime_value *value = graphion_runtime_scope_find(scope, exact_float_expectations[i].name);
    if (value == NULL || value->kind != GVM_VALUE_FLOAT ||
        value->as.float_value != exact_float_expectations[i].expected) {
      return exact_float_expectations[i].error_code;
    }
  }
  return 0;
}

static int validate_ranged_float_expectations(graphion_runtime_scope *scope) {
  size_t i;

  for (i = 0U; i < sizeof(ranged_float_expectations) / sizeof(ranged_float_expectations[0]); ++i) {
    const graphion_runtime_value *value = graphion_runtime_scope_find(scope, ranged_float_expectations[i].name);
    if (value == NULL || value->kind != GVM_VALUE_FLOAT ||
        value->as.float_value < ranged_float_expectations[i].min_value ||
        value->as.float_value > ranged_float_expectations[i].max_value) {
      return ranged_float_expectations[i].error_code;
    }
  }
  return 0;
}

static int validate_special_float_expectations(graphion_runtime_scope *scope) {
  size_t i;

  for (i = 0U; i < sizeof(special_float_expectations) / sizeof(special_float_expectations[0]); ++i) {
    const graphion_runtime_value *value = graphion_runtime_scope_find(scope, special_float_expectations[i].name);
    if (value == NULL || value->kind != GVM_VALUE_FLOAT) {
      return special_float_expectations[i].error_code;
    }
    if (special_float_expectations[i].kind == TEST_FLOAT_NAN) {
      if (!double_is_nan(value->as.float_value)) {
        return special_float_expectations[i].error_code;
      }
    } else if (!double_is_infinite(value->as.float_value) || value->as.float_value <= 0.0) {
      return special_float_expectations[i].error_code;
    }
  }
  return 0;
}

static int validate_scalar_math_scope(graphion_runtime_scope *scope) {
  int rc = validate_int_expectations(scope);

  if (rc != 0) {
    return rc;
  }
  rc = validate_bool_expectations(scope);
  if (rc != 0) {
    return rc;
  }
  rc = validate_exact_float_expectations(scope);
  if (rc != 0) {
    return rc;
  }
  rc = validate_ranged_float_expectations(scope);
  if (rc != 0) {
    return rc;
  }
  return validate_special_float_expectations(scope);
}

static int validate_scalar_math_output(const char *path) {
  static const char *expected_output_parts[] = {
      "42\n7\n9\n5\n3.5\n15\n-3\n7\n-12\n-3.5\n3\n-4\n3\n8\n-8\n0.5\n-5\n-3\n-3\n512\n9\n42\n3.5\n3\n3\n2\n8\n7\n3.5\n9\n0\n5\n10\n10\n3\n1.5\n3\n3\n-2\n3\n0\n1\n1\n1\n-1\n2\n1\n-1\n2\n1\n-1\n1.73205\n1.5708\n0.523599\n-0.523599\n0\n1.0472\n2.0944\n0.785398\n1.5708\n2.35619\n1\n0.648054\n0.648054\n0.850918\n-0.850918\n0.275721\n1.313035\n-1.313035\n1.037315\n0\n1.1752\n-1.1752\n0\n0.881374\n-0.881374\n0\n1.31696\n2.06344\n1\n1.54308\n1.54308\n0\n0.761594\n-0.761594\n0\n0.549306\n-0.549306\n1\n-1\n-1\n0\n1\n1\n0\n1.5708\n0.523599\n0\n1.5708\n1.0472\n0\n0.785398\n-0.785398\n0.785398\n2.35619\n-2.35619\n5\n13\n5\n-3\n3.5\n-3\n10\n3\n-6.5\n4\n0\n0\n-1\n-0.5\n1\n7\n7\n-3\n0\n90\n-45\n0\n3.14159\n-0.785398\ntrue\nfalse\nfalse\ntrue\nfalse\nfalse\nfalse\nfalse\ntrue\ntrue\n1.71828\n0\n6.38906\n2\n1\n4\n0.693147\n0\n0.405465\n0\n0.842701\n-0.842701\n1\n0.157299\n1.8427\n1\n24\n1.77245\n0\n3.17805\n0.572365\n2.71828\n1\n7.38906\n0\n1\n2\n3\n2\n5\n3\n1\n4\n3\n1\n6\n7\n7\n-4\n",
      "7\n8\n-3\n7\n7\n8\n-3\n-4\n7\n7\n-3\n0\n0\n0.25\n0.25\n1\n-1\n0\n3.14159\n6.28319\n1.61803\n2.71828\n1\n120\n6\n0\n8\n8\n2\n-2\n1.5\n11\n14\n2\n",
  };
  char output[3072];
  char expected_copy[3072];
  const char *expected_lines[512];
  const char *output_lines[512];
  size_t expected_len = 0U;
  size_t expected_count;
  size_t output_count;
  size_t i;

  if (!test_read_file_text(path, output, sizeof(output))) {
    return 28;
  }
  normalize_text_newlines(output);

  expected_copy[0] = '\0';
  for (i = 0U; i < sizeof(expected_output_parts) / sizeof(expected_output_parts[0]); ++i) {
    size_t part_len = strlen(expected_output_parts[i]);
    if (expected_len + part_len + 1U > sizeof(expected_copy)) {
      part_len = sizeof(expected_copy) - expected_len - 1U;
    }
    memcpy(expected_copy + expected_len, expected_output_parts[i], part_len);
    expected_len += part_len;
    expected_copy[expected_len] = '\0';
  }

  expected_count = split_lines_inplace(expected_copy, expected_lines, sizeof(expected_lines) / sizeof(expected_lines[0]));
  output_count = split_lines_inplace(output, output_lines, sizeof(output_lines) / sizeof(output_lines[0]));
  if (expected_count != output_count) {
    return 29;
  }

  for (i = 0U; i < expected_count; ++i) {
    if (!lines_match_with_tolerance(output_lines[i], expected_lines[i])) {
      return 29;
    }
  }
  return 0;
}

int run_gion_arithmetic_expressions_case(void) {
  char path[512] = {0};
  char source[12288];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;
  int result;

  graphion_runtime_scope_init(&scope);

  result = build_scalar_math_source(source, sizeof(source));
  if (result != 0) {
    goto cleanup;
  }

  fp = test_open_temp_output(path, sizeof(path), "gion_arithmetic_expressions.txt");
  if (fp == NULL) {
    result = 1;
    goto cleanup;
  }

  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  fp = NULL;
  if (rc != GINT_OK) {
    result = 2;
    goto cleanup;
  }

  result = validate_scalar_math_scope(&scope);
  if (result != 0) {
    goto cleanup;
  }

  result = validate_scalar_math_output(path);

cleanup:
  if (fp != NULL) {
    fclose(fp);
  }
  if (path[0] != '\0') {
    remove(path);
  }
  return finish_scope_test(&scope, result);
}
