/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "runtime/interpreter/exec_internal.h"
#include "vm/internal/core/value.h"

static int scope_sync_to_program(graphion_runtime_scope *scope,
                                 const graphion_runtime_program *program,
                                 unsigned int line,
                                 graphion_runtime_diagnostic *diagnostic) {
  size_t i;
  int rc;

  if (scope == NULL || program == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  rc = graphion_runtime_scope_reserve_globals(scope, program->global_count, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (i = scope->global_count; i < program->global_count; ++i) {
    copy_name(scope->global_names[i], program->global_names[i]);
    vm_value_set_none(&scope->globals[i]);
  }
  scope->global_count = program->global_count;
  return GINT_OK;
}

static int fail_for_vm_runtime_error(graphion_runtime_diagnostic *diagnostic,
                                     unsigned int line,
                                     unsigned int column,
                                     int vm_rc) {
  char message[GRAPHION_RUNTIME_DIAGNOSTIC_MESSAGE_MAX];

  if (vm_rc == GVM_ERR_DIVIDE_BY_ZERO) {
    return fail(diagnostic, line, column, "division by zero", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_DOMAIN) {
    return fail(diagnostic, line, column, "sqrt requires non-negative input", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_LN_DOMAIN) {
    return fail(diagnostic, line, column, "ln requires strictly positive input", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_LOG_DOMAIN) {
    return fail(diagnostic,
                line,
                column,
                "log requires x > 0 and base > 0 with base != 1",
                GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_ASIN_DOMAIN) {
    return fail(diagnostic, line, column, "asin requires input in [-1, 1]", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_ACOS_DOMAIN) {
    return fail(diagnostic, line, column, "acos requires input in [-1, 1]", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_ACSC_DOMAIN) {
    return fail(diagnostic, line, column, "acsc requires input <= -1 or >= 1", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_ASEC_DOMAIN) {
    return fail(diagnostic, line, column, "asec requires input <= -1 or >= 1", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_ACOSH_DOMAIN) {
    return fail(diagnostic, line, column, "acosh requires input >= 1", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_ATANH_DOMAIN) {
    return fail(diagnostic, line, column, "atanh requires input in (-1, 1)", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_LOG1P_DOMAIN) {
    return fail(diagnostic, line, column, "log1p requires input > -1", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_REMAINDER_DOMAIN) {
    return fail(diagnostic, line, column, "remainder requires non-zero divisor", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_GAMMA_DOMAIN) {
    return fail(diagnostic,
                line,
                column,
                "gamma is undefined at 0 and negative integers",
                GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_LGAMMA_DOMAIN) {
    return fail(diagnostic,
                line,
                column,
                "lgamma is undefined at 0 and negative integers",
                GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_FACTORIAL_DOMAIN) {
    return fail(diagnostic, line, column, "factorial requires non-negative integer input", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_BITS_WIDTH_MISMATCH) {
    return fail(diagnostic,
                line,
                column,
                "bitwise operations require matching bits widths",
                GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_NEGATIVE_SHIFT) {
    return fail(diagnostic,
                line,
                column,
                "bit shifts require non-negative integer counts",
                GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_INDEX_OUT_OF_RANGE) {
    return fail(diagnostic, line, column, "list index out of range", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_INVALID_NODE_ID) {
    return fail(diagnostic, line, column, "invalid node id", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_INVALID_HYPEREDGE_ID) {
    return fail(diagnostic, line, column, "invalid hyperedge id", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_MISSING_KEY) {
    return fail(diagnostic, line, column, "dict key not found", GINT_ERR_RUN);
  }
  if (vm_rc == GVM_ERR_TYPE_MISMATCH) {
    return fail(diagnostic, line, column, "incompatible operand types", GINT_ERR_RUN);
  }

  switch (vm_rc) {
    case GVM_ERR_INVALID_ARG:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_INVALID_ARG");
      break;
    case GVM_ERR_INVALID_MOV_IMM_REG:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_INVALID_MOV_IMM_REG");
      break;
    case GVM_ERR_INVALID_REG:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_INVALID_REG");
      break;
    case GVM_ERR_UNKNOWN_OPCODE:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_UNKNOWN_OPCODE");
      break;
    case GVM_ERR_CSR_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_CSR_UNBOUND");
      break;
    case GVM_ERR_INVALID_BFS_SOURCE:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_INVALID_BFS_SOURCE");
      break;
    case GVM_ERR_BFS_RUNTIME:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_BFS_RUNTIME");
      break;
    case GVM_ERR_HYPERGRAPH_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_HYPERGRAPH_UNBOUND");
      break;
    case GVM_ERR_FRONTIER_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_FRONTIER_UNBOUND");
      break;
    case GVM_ERR_FRONTIER_OVERFLOW:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_FRONTIER_OVERFLOW");
      break;
    case GVM_ERR_INVALID_FRONTIER_VALUE:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_INVALID_FRONTIER_VALUE");
      break;
    case GVM_ERR_CSR_WEIGHTS_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_CSR_WEIGHTS_UNBOUND");
      break;
    case GVM_ERR_CSR_EDGE_ATTRS_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_CSR_EDGE_ATTRS_UNBOUND");
      break;
    case GVM_ERR_CONST_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_CONST_UNBOUND");
      break;
    case GVM_ERR_GLOBALS_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_GLOBALS_UNBOUND");
      break;
    case GVM_ERR_INVALID_CONST_INDEX:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_INVALID_CONST_INDEX");
      break;
    case GVM_ERR_INVALID_GLOBAL_INDEX:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_INVALID_GLOBAL_INDEX");
      break;
    case GVM_ERR_OUTPUT_UNBOUND:
      snprintf(message, sizeof(message), "unmapped VM runtime error: GVM_ERR_OUTPUT_UNBOUND");
      break;
    default:
      snprintf(message, sizeof(message), "unmapped VM runtime error: code %d", vm_rc);
      break;
  }
  return fail(diagnostic, line, column, message, GINT_ERR_RUN);
}

static void bind_scope_to_vm(graphion_vm *vm, graphion_runtime_scope *scope) {
  static graphion_vm_value empty_globals[1];
  static char *empty_global_owners[1];

  if (vm == NULL || scope == NULL) {
    return;
  }

  graphion_vm_bind_globals(vm,
                           scope->global_count > 0U ? scope->globals : empty_globals,
                           scope->global_count);
  graphion_vm_bind_global_string_owners(vm,
                                        scope->global_count > 0U ? scope->owned_string_values : empty_global_owners,
                                        scope->global_count);
}

static int execute_condition_program(const graphion_runtime_program *program,
                                     graphion_runtime_scope *scope,
                                     uint8_t reg_index,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic,
                                     graphion_vm_value *value_out) {
  graphion_vm vm;
  int rc;

  if (program == NULL || scope == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  rc = scope_sync_to_program(scope, program, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, program->const_pool, program->const_count);
  bind_scope_to_vm(&vm, scope);
  rc = graphion_vm_load(&vm, program->program, program->program_len);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return fail(diagnostic, line, 1U, "failed to load VM program", GINT_ERR_PARSE);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return fail_for_vm_runtime_error(diagnostic, line, 1U, rc);
  }
  rc = vm_value_clone(value_out, &vm.regs[reg_index]);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return fail(diagnostic, line, 1U, "failed to clone expression value", GINT_ERR_CAPACITY);
  }
  graphion_vm_dispose(&vm);
  return GINT_OK;
}

int evaluate_expression_text_to_value(const char *expression_text,
                                      size_t expression_len,
                                      graphion_runtime_scope *scope,
                                      unsigned int line,
                                      graphion_runtime_diagnostic *diagnostic,
                                      graphion_vm_value *value_out) {
  char expression_buffer[512];
  const char *cursor = expression_buffer;
  parsed_expr_result expr;
  graphion_runtime_program program;
  int rc;

  if (scope == NULL || expression_text == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (expression_len >= sizeof(expression_buffer)) {
    return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
  }
  memcpy(expression_buffer, expression_text, expression_len);
  expression_buffer[expression_len] = '\0';
  rc = seed_program_from_scope(&program, scope, line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  rc = parse_expression(&cursor, &program, &expr, 0U, line, diagnostic);
  if (rc != GINT_OK) {
    point_unknown_operand_diagnostic(diagnostic, expression_text, 1U);
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    graphion_runtime_program_dispose(&program);
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after expression", GINT_ERR_PARSE);
  }
  if (expr.kind == EXPR_RESULT_LITERAL) {
    rc = vm_value_clone(value_out, &program.const_pool[expr.const_index]);
    graphion_runtime_program_dispose(&program);
    if (rc != GVM_OK) {
      return fail(diagnostic, line, 1U, "failed to clone expression value", GINT_ERR_CAPACITY);
    }
  } else if (expr.kind == EXPR_RESULT_GLOBAL) {
    rc = vm_value_clone(value_out, &scope->globals[expr.global_index]);
    graphion_runtime_program_dispose(&program);
    if (rc != GVM_OK) {
      return fail(diagnostic, line, 1U, "failed to clone expression value", GINT_ERR_CAPACITY);
    }
  } else {
    rc = program_emit(&program, GVM_OP_HALT, 0U, 0U, 0, line, diagnostic);
    if (rc != GINT_OK) {
      graphion_runtime_program_dispose(&program);
      return rc;
    }
    rc = execute_condition_program(&program, scope, expr.reg_index, line, diagnostic, value_out);
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  return GINT_OK;
}

int evaluate_condition_text(const char *condition_text,
                            size_t condition_len,
                            graphion_runtime_scope *scope,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic,
                            int *result_out) {
  graphion_vm_value value;
  int rc;

  rc = evaluate_expression_text_to_value(condition_text, condition_len, scope, line, diagnostic, &value);
  if (rc != GINT_OK) {
    return rc;
  }
  if (value.kind == GVM_VALUE_BOOL) {
    *result_out = value.as.bool_value != 0;
    vm_value_dispose_owned(&value);
    return GINT_OK;
  }
  if (value.kind == GVM_VALUE_INT && (value.as.int_value == 0 || value.as.int_value == 1)) {
    *result_out = value.as.int_value != 0;
    vm_value_dispose_owned(&value);
    return GINT_OK;
  }
  vm_value_dispose_owned(&value);
  return fail(diagnostic, line, 1U, "if condition must be boolean or 0/1", GINT_ERR_RUN);
}

int graphion_execute_prepared_program_with_sink(const graphion_runtime_program *program,
                                                graphion_runtime_scope *scope,
                                                graphion_runtime_diagnostic *diagnostic,
                                                const graphion_output_sink *output) {
  graphion_vm vm;
  int rc;

  if (program == NULL || scope == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  rc = scope_sync_to_program(scope, program, 1U, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, program->const_pool, program->const_count);
  bind_scope_to_vm(&vm, scope);
  if (output != NULL) {
    graphion_vm_bind_output_sink(&vm, output);
  }
  rc = graphion_vm_load(&vm, program->program, program->program_len);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return fail(diagnostic, 1U, 1U, "failed to load VM program", GINT_ERR_PARSE);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return fail_for_vm_runtime_error(diagnostic, 1U, 1U, rc);
  }
  graphion_vm_dispose(&vm);
  return GINT_OK;
}

int graphion_execute_program(const graphion_runtime_program *program,
                             graphion_runtime_scope *scope,
                             graphion_runtime_diagnostic *diagnostic,
                             FILE *output) {
  graphion_output_sink sink;

  graphion_output_sink_from_file(&sink, output);
  return graphion_execute_prepared_program_with_sink(program, scope, diagnostic, &sink);
}
