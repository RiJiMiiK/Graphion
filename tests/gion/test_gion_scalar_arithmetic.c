/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

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
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_arithmetic_precedence.txt");
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
  if (strcmp(output, "12\n2\n26\n9\n4\n1.5\n-5\n-10\n20\n14\n12\n18\n") != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
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
      {"value = csc(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = sec(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = cot(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = acsc(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = acsc(0.5)\n", GINT_ERR_RUN, "acsc requires input <= -1 or >= 1"},
      {"value = asec(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = asec(0.5)\n", GINT_ERR_RUN, "asec requires input <= -1 or >= 1"},
      {"value = acot(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = sech(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = csch(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = coth(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
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
      {"value = copysign(\"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = copysign(1, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = fma(\"x\", 1, 2)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = fma(1, \"x\", 2)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = fma(1, 2, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = fdim(\"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = fdim(1, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = remainder(\"x\", 1)\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = remainder(1, \"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = remainder(1, 0)\n", GINT_ERR_RUN, "remainder requires non-zero divisor"},
      {"value = rint(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = degrees(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = radians(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = isnan(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = isinf(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = isfinite(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = expm1(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = exp2(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = log1p(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = erf(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = erfc(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = gamma(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = lgamma(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = fract(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
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
      {"value = log1p(-1)\n", GINT_ERR_RUN, "log1p requires input > -1"},
      {"value = log1p(-2)\n", GINT_ERR_RUN, "log1p requires input > -1"},
      {"value = gamma(0)\n", GINT_ERR_RUN, "gamma is undefined at 0 and negative integers"},
      {"value = gamma(-1)\n", GINT_ERR_RUN, "gamma is undefined at 0 and negative integers"},
      {"value = lgamma(0)\n", GINT_ERR_RUN, "lgamma is undefined at 0 and negative integers"},
      {"value = lgamma(-1)\n", GINT_ERR_RUN, "lgamma is undefined at 0 and negative integers"},
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
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_arithmetic_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"value = 1 + * 2\n", GINT_ERR_PARSE, "expected expression after '+'"},
      {"value = 1 / / 2\n", GINT_ERR_PARSE, "expected expression after '/'"},
      {"value = 1 ** * 2\n", GINT_ERR_PARSE, "expected expression after '**'"},
      {"value = 1 % % 2\n", GINT_ERR_PARSE, "expected expression after '%'"},
      {"value = 2 **\n", GINT_ERR_PARSE, "expected expression after '**'"},
      {"value = 2 //\n", GINT_ERR_PARSE, "expected expression after '//'"},
      {"value = 1 + 2 3\n", GINT_ERR_PARSE, "unexpected trailing tokens after assignment"},
      {"value = 1\nvalue +=\n", GINT_ERR_PARSE, "expected expression after '+='"},
      {"value + 2\n", GINT_ERR_PARSE, "expected '='"},
      {"value = abs()\n", GINT_ERR_PARSE, "expected abs argument"},
      {"value = abs(1 + 2\n", GINT_ERR_PARSE, "expected ')' after abs argument"},
      {"value = min()\n", GINT_ERR_PARSE, "expected min first argument"},
      {"value = min(1)\n", GINT_ERR_PARSE, "expected ',' between min arguments"},
      {"value = min(1,)\n", GINT_ERR_PARSE, "expected min second argument"},
      {"value = min(1, 2\n", GINT_ERR_PARSE, "expected ')' after min arguments"},
      {"value = min(1 2)\n", GINT_ERR_PARSE, "expected ',' between min arguments"},
      {"value = max()\n", GINT_ERR_PARSE, "expected max first argument"},
      {"value = max(1)\n", GINT_ERR_PARSE, "expected ',' between max arguments"},
      {"value = max(1,)\n", GINT_ERR_PARSE, "expected max second argument"},
      {"value = max(1, 2\n", GINT_ERR_PARSE, "expected ')' after max arguments"},
      {"value = max(1 2)\n", GINT_ERR_PARSE, "expected ',' between max arguments"},
      {"value = clamp()\n", GINT_ERR_PARSE, "expected clamp value"},
      {"value = clamp(1)\n", GINT_ERR_PARSE, "expected ',' after clamp value"},
      {"value = clamp(1,)\n", GINT_ERR_PARSE, "expected clamp lower bound"},
      {"value = clamp(1, 2)\n", GINT_ERR_PARSE, "expected ',' after clamp lower bound"},
      {"value = clamp(1, 2,)\n", GINT_ERR_PARSE, "expected clamp upper bound"},
      {"value = clamp(1, 2, 3\n", GINT_ERR_PARSE, "expected ')' after clamp arguments"},
      {"value = clamp(1 2, 3)\n", GINT_ERR_PARSE, "expected ',' after clamp value"},
      {"value = sqrt()\n", GINT_ERR_PARSE, "expected sqrt argument"},
      {"value = sqrt(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sqrt argument"},
      {"value = cbrt()\n", GINT_ERR_PARSE, "expected cbrt argument"},
      {"value = cbrt(1 + 2\n", GINT_ERR_PARSE, "expected ')' after cbrt argument"},
      {"value = sin()\n", GINT_ERR_PARSE, "expected sin argument"},
      {"value = sin(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sin argument"},
      {"value = csc()\n", GINT_ERR_PARSE, "expected csc argument"},
      {"value = csc(1 + 2\n", GINT_ERR_PARSE, "expected ')' after csc argument"},
      {"value = sec()\n", GINT_ERR_PARSE, "expected sec argument"},
      {"value = sec(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sec argument"},
      {"value = cot()\n", GINT_ERR_PARSE, "expected cot argument"},
      {"value = cot(1 + 2\n", GINT_ERR_PARSE, "expected ')' after cot argument"},
      {"value = acsc()\n", GINT_ERR_PARSE, "expected acsc argument"},
      {"value = acsc(1 + 2\n", GINT_ERR_PARSE, "expected ')' after acsc argument"},
      {"value = asec()\n", GINT_ERR_PARSE, "expected asec argument"},
      {"value = asec(1 + 2\n", GINT_ERR_PARSE, "expected ')' after asec argument"},
      {"value = acot()\n", GINT_ERR_PARSE, "expected acot argument"},
      {"value = acot(1 + 2\n", GINT_ERR_PARSE, "expected ')' after acot argument"},
      {"value = sech()\n", GINT_ERR_PARSE, "expected sech argument"},
      {"value = sech(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sech argument"},
      {"value = csch()\n", GINT_ERR_PARSE, "expected csch argument"},
      {"value = csch(1 + 2\n", GINT_ERR_PARSE, "expected ')' after csch argument"},
      {"value = coth()\n", GINT_ERR_PARSE, "expected coth argument"},
      {"value = coth(1 + 2\n", GINT_ERR_PARSE, "expected ')' after coth argument"},
      {"value = sinh()\n", GINT_ERR_PARSE, "expected sinh argument"},
      {"value = sinh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sinh argument"},
      {"value = asinh()\n", GINT_ERR_PARSE, "expected asinh argument"},
      {"value = asinh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after asinh argument"},
      {"value = acosh()\n", GINT_ERR_PARSE, "expected acosh argument"},
      {"value = acosh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after acosh argument"},
      {"value = cosh()\n", GINT_ERR_PARSE, "expected cosh argument"},
      {"value = cosh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after cosh argument"},
      {"value = tanh()\n", GINT_ERR_PARSE, "expected tanh argument"},
      {"value = tanh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after tanh argument"},
      {"value = atanh()\n", GINT_ERR_PARSE, "expected atanh argument"},
      {"value = atanh(1 + 2\n", GINT_ERR_PARSE, "expected ')' after atanh argument"},
      {"value = cos()\n", GINT_ERR_PARSE, "expected cos argument"},
      {"value = cos(1 + 2\n", GINT_ERR_PARSE, "expected ')' after cos argument"},
      {"value = tan()\n", GINT_ERR_PARSE, "expected tan argument"},
      {"value = tan(1 + 2\n", GINT_ERR_PARSE, "expected ')' after tan argument"},
      {"value = asin()\n", GINT_ERR_PARSE, "expected asin argument"},
      {"value = asin(1 + 2\n", GINT_ERR_PARSE, "expected ')' after asin argument"},
      {"value = atan()\n", GINT_ERR_PARSE, "expected atan argument"},
      {"value = atan(1 + 2\n", GINT_ERR_PARSE, "expected ')' after atan argument"},
      {"value = atan2()\n", GINT_ERR_PARSE, "expected atan2 first argument"},
      {"value = atan2(1)\n", GINT_ERR_PARSE, "expected ',' between atan2 arguments"},
      {"value = atan2(1,)\n", GINT_ERR_PARSE, "expected atan2 second argument"},
      {"value = atan2(1, 1\n", GINT_ERR_PARSE, "expected ')' after atan2 arguments"},
      {"value = atan2(1 1)\n", GINT_ERR_PARSE, "expected ',' between atan2 arguments"},
      {"value = hypot()\n", GINT_ERR_PARSE, "expected hypot first argument"},
      {"value = hypot(1)\n", GINT_ERR_PARSE, "expected ',' between hypot arguments"},
      {"value = hypot(1,)\n", GINT_ERR_PARSE, "expected hypot second argument"},
      {"value = hypot(1, 1\n", GINT_ERR_PARSE, "expected ')' after hypot arguments"},
      {"value = hypot(1 1)\n", GINT_ERR_PARSE, "expected ',' between hypot arguments"},
      {"value = copysign()\n", GINT_ERR_PARSE, "expected copysign first argument"},
      {"value = copysign(1)\n", GINT_ERR_PARSE, "expected ',' between copysign arguments"},
      {"value = copysign(1,)\n", GINT_ERR_PARSE, "expected copysign second argument"},
      {"value = copysign(1, 1\n", GINT_ERR_PARSE, "expected ')' after copysign arguments"},
      {"value = copysign(1 1)\n", GINT_ERR_PARSE, "expected ',' between copysign arguments"},
      {"value = fma()\n", GINT_ERR_PARSE, "expected fma first argument"},
      {"value = fma(1)\n", GINT_ERR_PARSE, "expected ',' after fma first argument"},
      {"value = fma(1,)\n", GINT_ERR_PARSE, "expected fma second argument"},
      {"value = fma(1, 2)\n", GINT_ERR_PARSE, "expected ',' after fma second argument"},
      {"value = fma(1, 2,)\n", GINT_ERR_PARSE, "expected fma third argument"},
      {"value = fma(1, 2, 3\n", GINT_ERR_PARSE, "expected ')' after fma arguments"},
      {"value = fma(1 2, 3)\n", GINT_ERR_PARSE, "expected ',' after fma first argument"},
      {"value = fdim()\n", GINT_ERR_PARSE, "expected fdim first argument"},
      {"value = fdim(1)\n", GINT_ERR_PARSE, "expected ',' between fdim arguments"},
      {"value = fdim(1,)\n", GINT_ERR_PARSE, "expected fdim second argument"},
      {"value = fdim(1, 1\n", GINT_ERR_PARSE, "expected ')' after fdim arguments"},
      {"value = fdim(1 1)\n", GINT_ERR_PARSE, "expected ',' between fdim arguments"},
      {"value = remainder()\n", GINT_ERR_PARSE, "expected remainder first argument"},
      {"value = remainder(1)\n", GINT_ERR_PARSE, "expected ',' between remainder arguments"},
      {"value = remainder(1,)\n", GINT_ERR_PARSE, "expected remainder second argument"},
      {"value = remainder(1, 1\n", GINT_ERR_PARSE, "expected ')' after remainder arguments"},
      {"value = remainder(1 1)\n", GINT_ERR_PARSE, "expected ',' between remainder arguments"},
      {"value = rint()\n", GINT_ERR_PARSE, "expected rint argument"},
      {"value = rint(1 + 2\n", GINT_ERR_PARSE, "expected ')' after rint argument"},
      {"value = degrees()\n", GINT_ERR_PARSE, "expected degrees argument"},
      {"value = degrees(1 + 2\n", GINT_ERR_PARSE, "expected ')' after degrees argument"},
      {"value = radians()\n", GINT_ERR_PARSE, "expected radians argument"},
      {"value = radians(1 + 2\n", GINT_ERR_PARSE, "expected ')' after radians argument"},
      {"value = isnan()\n", GINT_ERR_PARSE, "expected isnan argument"},
      {"value = isnan(1 + 2\n", GINT_ERR_PARSE, "expected ')' after isnan argument"},
      {"value = isinf()\n", GINT_ERR_PARSE, "expected isinf argument"},
      {"value = isinf(1 + 2\n", GINT_ERR_PARSE, "expected ')' after isinf argument"},
      {"value = isfinite()\n", GINT_ERR_PARSE, "expected isfinite argument"},
      {"value = isfinite(1 + 2\n", GINT_ERR_PARSE, "expected ')' after isfinite argument"},
      {"value = expm1()\n", GINT_ERR_PARSE, "expected expm1 argument"},
      {"value = expm1(1 + 2\n", GINT_ERR_PARSE, "expected ')' after expm1 argument"},
      {"value = exp2()\n", GINT_ERR_PARSE, "expected exp2 argument"},
      {"value = exp2(1 + 2\n", GINT_ERR_PARSE, "expected ')' after exp2 argument"},
      {"value = log1p()\n", GINT_ERR_PARSE, "expected log1p argument"},
      {"value = log1p(1 + 2\n", GINT_ERR_PARSE, "expected ')' after log1p argument"},
      {"value = erf()\n", GINT_ERR_PARSE, "expected erf argument"},
      {"value = erf(1 + 2\n", GINT_ERR_PARSE, "expected ')' after erf argument"},
      {"value = erfc()\n", GINT_ERR_PARSE, "expected erfc argument"},
      {"value = erfc(1 + 2\n", GINT_ERR_PARSE, "expected ')' after erfc argument"},
      {"value = gamma()\n", GINT_ERR_PARSE, "expected gamma argument"},
      {"value = gamma(1 + 2\n", GINT_ERR_PARSE, "expected ')' after gamma argument"},
      {"value = lgamma()\n", GINT_ERR_PARSE, "expected lgamma argument"},
      {"value = lgamma(1 + 2\n", GINT_ERR_PARSE, "expected ')' after lgamma argument"},
      {"value = fract()\n", GINT_ERR_PARSE, "expected fract argument"},
      {"value = fract(1 + 2\n", GINT_ERR_PARSE, "expected ')' after fract argument"},
      {"value = exp()\n", GINT_ERR_PARSE, "expected exp argument"},
      {"value = exp(1 + 2\n", GINT_ERR_PARSE, "expected ')' after exp argument"},
      {"value = ln()\n", GINT_ERR_PARSE, "expected ln argument"},
      {"value = ln(1 + 2\n", GINT_ERR_PARSE, "expected ')' after ln argument"},
      {"value = log()\n", GINT_ERR_PARSE, "expected log first argument"},
      {"value = log(8)\n", GINT_ERR_PARSE, "expected ',' between log arguments"},
      {"value = log(8,)\n", GINT_ERR_PARSE, "expected log second argument"},
      {"value = log(8, 2\n", GINT_ERR_PARSE, "expected ')' after log arguments"},
      {"value = log(8 2)\n", GINT_ERR_PARSE, "expected ',' between log arguments"},
      {"value = log10()\n", GINT_ERR_PARSE, "expected log10 argument"},
      {"value = log10(1 + 2\n", GINT_ERR_PARSE, "expected ')' after log10 argument"},
      {"value = log2()\n", GINT_ERR_PARSE, "expected log2 argument"},
      {"value = log2(1 + 2\n", GINT_ERR_PARSE, "expected ')' after log2 argument"},
      {"value = floor()\n", GINT_ERR_PARSE, "expected floor argument"},
      {"value = floor(1 + 2\n", GINT_ERR_PARSE, "expected ')' after floor argument"},
      {"value = ceil()\n", GINT_ERR_PARSE, "expected ceil argument"},
      {"value = ceil(1 + 2\n", GINT_ERR_PARSE, "expected ')' after ceil argument"},
      {"value = round()\n", GINT_ERR_PARSE, "expected round argument"},
      {"value = round(1 + 2\n", GINT_ERR_PARSE, "expected ')' after round argument"},
      {"value = trunc()\n", GINT_ERR_PARSE, "expected trunc argument"},
      {"value = trunc(1 + 2\n", GINT_ERR_PARSE, "expected ')' after trunc argument"},
      {"value = sign()\n", GINT_ERR_PARSE, "expected sign argument"},
      {"value = sign(1 + 2\n", GINT_ERR_PARSE, "expected ')' after sign argument"},
      {"value = !\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = len()\n", GINT_ERR_PARSE, "expected len argument"},
      {"value = len(\"x\"\n", GINT_ERR_PARSE, "expected ')' after len argument"},
      {"value = (1 + 2\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = -\n", GINT_ERR_PARSE, "expected expression after '-'"},
      {"value = -(1 + 2\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = 1 + (2 * 3\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = ()\n", GINT_ERR_PARSE, "empty tuple literal is not supported"},
      {"print(1 + )\n", GINT_ERR_PARSE, "expected expression after '+'"},
      {"print((1 + 2)\n", GINT_ERR_PARSE, "expected ')' after print argument"},
      {"print(()\n", GINT_ERR_PARSE, "empty tuple literal is not supported"},
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
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}
