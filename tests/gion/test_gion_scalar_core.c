/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

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
  char path[512];
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
  fp = test_open_temp_output(path, sizeof(path), "gion_scalar_assignments_and_prints.txt");
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
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *copied_name;
  const graphion_runtime_value *shadow_0;
  const graphion_runtime_value *z_value;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_scalar_feature_varied_names.txt");
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
    return finish_scope_test(&scope, 1);
  }
  if (diagnostic.message == NULL) {
    return finish_scope_test(&scope, 2);
  }
  return finish_scope_test(&scope, strcmp(diagnostic.message, "unknown operand 'missing'") == 0 ? 0 : 3);
}

int test_gion_partial_execution_stops_at_first_unsupported_line(void) {
  const char *source =
      "count = 42\n"
      "print(count)\n"
      "graph G:\n"
      "print(count)\n";
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_partial_execution.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_ERR_PARSE) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (diagnostic.line != 3U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "42\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
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
      {"csc = 1\n", "reserved name cannot be assigned", "gion_reserved_csc.gion"},
      {"sec = 1\n", "reserved name cannot be assigned", "gion_reserved_sec.gion"},
      {"cot = 1\n", "reserved name cannot be assigned", "gion_reserved_cot.gion"},
      {"acsc = 1\n", "reserved name cannot be assigned", "gion_reserved_acsc.gion"},
      {"asec = 1\n", "reserved name cannot be assigned", "gion_reserved_asec.gion"},
      {"acot = 1\n", "reserved name cannot be assigned", "gion_reserved_acot.gion"},
      {"sech = 1\n", "reserved name cannot be assigned", "gion_reserved_sech.gion"},
      {"csch = 1\n", "reserved name cannot be assigned", "gion_reserved_csch.gion"},
      {"coth = 1\n", "reserved name cannot be assigned", "gion_reserved_coth.gion"},
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
      {"copysign = 1\n", "reserved name cannot be assigned", "gion_reserved_copysign.gion"},
      {"fdim = 1\n", "reserved name cannot be assigned", "gion_reserved_fdim.gion"},
      {"remainder = 1\n", "reserved name cannot be assigned", "gion_reserved_remainder.gion"},
      {"rint = 1\n", "reserved name cannot be assigned", "gion_reserved_rint.gion"},
      {"degrees = 1\n", "reserved name cannot be assigned", "gion_reserved_degrees.gion"},
      {"radians = 1\n", "reserved name cannot be assigned", "gion_reserved_radians.gion"},
      {"isnan = 1\n", "reserved name cannot be assigned", "gion_reserved_isnan.gion"},
      {"isinf = 1\n", "reserved name cannot be assigned", "gion_reserved_isinf.gion"},
      {"isfinite = 1\n", "reserved name cannot be assigned", "gion_reserved_isfinite.gion"},
      {"expm1 = 1\n", "reserved name cannot be assigned", "gion_reserved_expm1.gion"},
      {"exp2 = 1\n", "reserved name cannot be assigned", "gion_reserved_exp2.gion"},
      {"log1p = 1\n", "reserved name cannot be assigned", "gion_reserved_log1p.gion"},
      {"erf = 1\n", "reserved name cannot be assigned", "gion_reserved_erf.gion"},
      {"erfc = 1\n", "reserved name cannot be assigned", "gion_reserved_erfc.gion"},
      {"gamma = 1\n", "reserved name cannot be assigned", "gion_reserved_gamma.gion"},
      {"lgamma = 1\n", "reserved name cannot be assigned", "gion_reserved_lgamma.gion"},
      {"fract = 1\n", "reserved name cannot be assigned", "gion_reserved_fract.gion"},
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
      {"tau = 1\n", "reserved name cannot be assigned", "gion_reserved_tau.gion"},
      {"phi = 1\n", "reserved name cannot be assigned", "gion_reserved_phi.gion"},
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
    char path[512];
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    FILE *fp = NULL;
    int rc;

    memset(&scope, 0, sizeof(scope));
    fp = test_open_temp_output(path, sizeof(path), cases[i].path);
    if (fp == NULL) {
      return (int)(1 + i * 10U);
    }
    fputs(cases[i].source, fp);
    fclose(fp);
    rc = graphion_run_gion_path(path, &scope, &diagnostic);
    remove(path);
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
      {"count = nope\n", GINT_ERR_UNKNOWN_OPERAND, "unknown operand 'nope'"},
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
    if (diagnostic.line != 1U) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
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
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *copy_i;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_mixed_scalar_values.txt");
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
      return finish_scope_test(&scope, 1);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "identifier too long") != 0) {
      return finish_scope_test(&scope, 2);
    }
    graphion_runtime_scope_dispose(&scope);
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
      return finish_scope_test(&scope, 10);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "source line too long") != 0) {
      return finish_scope_test(&scope, 11);
    }
    graphion_runtime_scope_dispose(&scope);
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
      return finish_scope_test(&scope, 22);
    }
    if (scope.global_count != 160U) {
      graphion_runtime_scope_dispose(&scope);
      return 23;
    }
    graphion_runtime_scope_dispose(&scope);
  }

  return 0;
}
