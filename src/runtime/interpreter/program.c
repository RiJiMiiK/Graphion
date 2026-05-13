/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/program.h"

#include "vm/internal/core/value.h"

static size_t next_global_capacity(size_t current, size_t min_capacity) {
  size_t capacity = current == 0U ? GRAPHION_RUNTIME_GLOBAL_INITIAL_CAPACITY : current;
  while (capacity < min_capacity) {
    if (capacity > ((size_t)-1) / 2U) {
      capacity = min_capacity;
      break;
    }
    capacity *= 2U;
  }
  return capacity;
}

int graphion_runtime_scope_reserve_globals(graphion_runtime_scope *scope,
                                                  size_t min_capacity,
                                                  unsigned int line,
                                                  graphion_runtime_diagnostic *diagnostic) {
  char(*new_names)[GRAPHION_RUNTIME_NAME_MAX];
  char **new_owners;
  graphion_runtime_value *new_globals;
  size_t i;
  size_t new_capacity;

  if (scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (min_capacity <= scope->global_capacity) {
    return GINT_OK;
  }

  new_capacity = next_global_capacity(scope->global_capacity, min_capacity);
  new_names = (char(*)[GRAPHION_RUNTIME_NAME_MAX])realloc(
      scope->global_names, new_capacity * sizeof(*new_names));
  if (new_names == NULL) {
    return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  scope->global_names = new_names;

  new_owners = (char **)realloc(scope->owned_string_values, new_capacity * sizeof(*new_owners));
  if (new_owners == NULL) {
    return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  scope->owned_string_values = new_owners;

  new_globals = (graphion_runtime_value *)realloc(scope->globals, new_capacity * sizeof(*new_globals));
  if (new_globals == NULL) {
    return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  scope->globals = new_globals;

  for (i = scope->global_capacity; i < new_capacity; ++i) {
    scope->global_names[i][0] = '\0';
    scope->owned_string_values[i] = NULL;
    vm_value_set_none(&scope->globals[i]);
  }
  scope->global_capacity = new_capacity;
  return GINT_OK;
}

int graphion_runtime_program_reserve_globals(graphion_runtime_program *program,
                                                    size_t min_capacity,
                                                    unsigned int line,
                                                    graphion_runtime_diagnostic *diagnostic) {
  char(*new_names)[GRAPHION_RUNTIME_NAME_MAX];
  size_t i;
  size_t new_capacity;

  if (program == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (min_capacity <= program->global_capacity) {
    return GINT_OK;
  }

  new_capacity = next_global_capacity(program->global_capacity, min_capacity);
  new_names = (char(*)[GRAPHION_RUNTIME_NAME_MAX])realloc(
      program->global_names, new_capacity * sizeof(*new_names));
  if (new_names == NULL) {
    return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  program->global_names = new_names;
  for (i = program->global_capacity; i < new_capacity; ++i) {
    program->global_names[i][0] = '\0';
  }
  program->global_capacity = new_capacity;
  return GINT_OK;
}

int scope_find_global_index(const graphion_runtime_scope *scope, const char *name) {
  size_t i;
  if (scope == NULL || name == NULL) {
    return -1;
  }
  for (i = 0U; i < scope->global_count; ++i) {
    if (strcmp(scope->global_names[i], name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

int program_find_global_index(const graphion_runtime_program *program, const char *name) {
  size_t i;
  if (program == NULL || name == NULL) {
    return -1;
  }
  for (i = 0U; i < program->global_count; ++i) {
    if (strcmp(program->global_names[i], name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

int scope_find_index(const graphion_runtime_scope *scope, const char *name) {
  return scope_find_global_index(scope, name);
}

int program_find_or_add_global(graphion_runtime_program *program,
                                      const char *name,
                                      unsigned int line,
                                      graphion_runtime_diagnostic *diagnostic,
                                      size_t *index_out) {
  int existing;
  if (program == NULL || name == NULL || index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  existing = program_find_global_index(program, name);
  if (existing >= 0) {
    *index_out = (size_t)existing;
    return GINT_OK;
  }
  if (graphion_runtime_program_reserve_globals(program, program->global_count + 1U, line, diagnostic) != GINT_OK) {
    return GINT_ERR_CAPACITY;
  }
  copy_name(program->global_names[program->global_count], name);
  *index_out = program->global_count;
  program->global_count += 1U;
  return GINT_OK;
}

int program_add_const(graphion_runtime_program *program,
                             const graphion_vm_value *value,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic,
                             size_t *index_out) {
  if (program == NULL || value == NULL || index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (program->const_count >= GRAPHION_RUNTIME_CONST_MAX) {
    return fail(diagnostic, line, 1U, "constant pool capacity exceeded", GINT_ERR_CAPACITY);
  }
  program->const_pool[program->const_count] = *value;
  *index_out = program->const_count;
  program->const_count += 1U;
  return GINT_OK;
}

int program_emit(graphion_runtime_program *program,
                        graphion_opcode op,
                        uint8_t a,
                        uint8_t b,
                        int32_t imm,
                        unsigned int line,
                        graphion_runtime_diagnostic *diagnostic) {
  graphion_insn *out;
  if (program == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (program->program_len >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "bytecode capacity exceeded", GINT_ERR_CAPACITY);
  }
  out = &program->program[program->program_len++];
  out->op = (uint8_t)op;
  out->a = a;
  out->b = b;
  out->imm = imm;
  return GINT_OK;
}

int program_patch_imm(graphion_runtime_program *program,
                             size_t insn_index,
                             int32_t imm,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic) {
  if (program == NULL || insn_index >= program->program_len) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  program->program[insn_index].imm = imm;
  return GINT_OK;
}

int program_emit_load_bool(graphion_runtime_program *program,
                                  uint8_t reg,
                                  int bool_value,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value value;
  size_t const_index;
  int rc;

  vm_value_set_none(&value);
  value.kind = GVM_VALUE_BOOL;
  value.as.bool_value = bool_value != 0 ? 1 : 0;
  rc = program_add_const(program, &value, line, diagnostic, &const_index);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_LOAD_CONST, reg, 0U, (int32_t)const_index, line, diagnostic);
}

int program_emit_load_int(graphion_runtime_program *program,
                                 uint8_t reg,
                                 int64_t int_value,
                                 unsigned int line,
                                 graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value value;
  size_t const_index;
  int rc;

  vm_value_set_none(&value);
  value.kind = GVM_VALUE_INT;
  value.as.int_value = int_value;
  rc = program_add_const(program, &value, line, diagnostic, &const_index);
  if (rc != GINT_OK) {
    return rc;
  }
  return program_emit(program, GVM_OP_LOAD_CONST, reg, 0U, (int32_t)const_index, line, diagnostic);
}


int runtime_value_get_numeric(const graphion_vm_value *value,
                                     int64_t *int_out,
                                     double *float_out,
                                     int *is_float_out) {
  if (value == NULL || int_out == NULL || float_out == NULL || is_float_out == NULL) {
    return 0;
  }
  if (value->kind == GVM_VALUE_INT) {
    *int_out = value->as.int_value;
    *float_out = (double)value->as.int_value;
    *is_float_out = 0;
    return 1;
  }
  if (value->kind == GVM_VALUE_FLOAT) {
    *int_out = (int64_t)value->as.float_value;
    *float_out = value->as.float_value;
    *is_float_out = 1;
    return 1;
  }
  return 0;
}

int scalar_values_match_equal(const graphion_vm_value *lhs,
                                     const graphion_vm_value *rhs,
                                     int *compatible_out,
                                     int *equal_out) {
  if (compatible_out == NULL || equal_out == NULL || lhs == NULL || rhs == NULL) {
    return 0;
  }
  *compatible_out = 1;
  *equal_out = 0;

  if ((lhs->kind == GVM_VALUE_INT || lhs->kind == GVM_VALUE_FLOAT) &&
      (rhs->kind == GVM_VALUE_INT || rhs->kind == GVM_VALUE_FLOAT)) {
    int64_t lhs_i = 0;
    int64_t rhs_i = 0;
    double lhs_f = 0.0;
    double rhs_f = 0.0;
    int lhs_is_float = 0;
    int rhs_is_float = 0;

    if (!runtime_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
        !runtime_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
      *compatible_out = 0;
      return 1;
    }
    *equal_out = lhs_f == rhs_f;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_INT) {
    if (rhs->as.int_value != 0 && rhs->as.int_value != 1) {
      *compatible_out = 0;
      return 1;
    }
    *equal_out = rhs->as.int_value == (int64_t)lhs->as.bool_value;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_INT && rhs->kind == GVM_VALUE_BOOL) {
    if (lhs->as.int_value != 0 && lhs->as.int_value != 1) {
      *compatible_out = 0;
      return 1;
    }
    *equal_out = lhs->as.int_value == (int64_t)rhs->as.bool_value;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_BOOL) {
    *equal_out = lhs->as.bool_value == rhs->as.bool_value;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_STRING && rhs->kind == GVM_VALUE_STRING) {
    const char *lhs_text = lhs->as.string_value != NULL ? lhs->as.string_value : "";
    const char *rhs_text = rhs->as.string_value != NULL ? rhs->as.string_value : "";
    *equal_out = strcmp(lhs_text, rhs_text) == 0;
    return 1;
  }
  if (lhs->kind == GVM_VALUE_BITS && rhs->kind == GVM_VALUE_BITS) {
    *equal_out = (uint64_t)lhs->as.int_value == (uint64_t)rhs->as.int_value;
    return 1;
  }

  *compatible_out = 0;
  return 1;
}

void graphion_runtime_scope_init(graphion_runtime_scope *scope) {
  if (scope == NULL) {
    return;
  }
  scope->global_names = NULL;
  scope->owned_string_values = NULL;
  scope->globals = NULL;
  scope->global_count = 0U;
  scope->global_capacity = 0U;
}

void graphion_runtime_scope_dispose(graphion_runtime_scope *scope) {
  size_t i;
  if (scope == NULL) {
    return;
  }
  for (i = 0U; i < scope->global_capacity; ++i) {
    runtime_free_string(&scope->owned_string_values[i]);
    if (scope->globals[i].kind == GVM_VALUE_LIST || scope->globals[i].kind == GVM_VALUE_DICT ||
        scope->globals[i].kind == GVM_VALUE_TUPLE || scope->globals[i].kind == GVM_VALUE_SET ||
        scope->globals[i].kind == GVM_VALUE_GRAPH_REF || scope->globals[i].kind == GVM_VALUE_HYPERGRAPH_REF) {
      vm_value_dispose_owned(&scope->globals[i]);
    }
  }
  free(scope->global_names);
  free(scope->owned_string_values);
  free(scope->globals);
  scope->global_names = NULL;
  scope->owned_string_values = NULL;
  scope->globals = NULL;
  scope->global_count = 0U;
  scope->global_capacity = 0U;
}

const graphion_runtime_value *graphion_runtime_scope_find(const graphion_runtime_scope *scope,
                                                          const char *name) {
  int index = scope_find_index(scope, name);
  if (index < 0) {
    return NULL;
  }
  return &scope->globals[index];
}

void graphion_runtime_program_init(graphion_runtime_program *program) {
  size_t i;
  if (program == NULL) {
    return;
  }
  program->global_names = NULL;
  program->global_count = 0U;
  program->global_capacity = 0U;
  program->const_count = 0U;
  program->program_len = 0U;
  for (i = 0U; i < GRAPHION_RUNTIME_CONST_MAX; ++i) {
    program->owned_const_strings[i] = NULL;
    vm_value_set_none(&program->const_pool[i]);
  }
  memset(program->program, 0, sizeof(program->program));
}

void graphion_runtime_program_dispose(graphion_runtime_program *program) {
  size_t i;
  if (program == NULL) {
    return;
  }
  for (i = 0U; i < GRAPHION_RUNTIME_CONST_MAX; ++i) {
    runtime_free_string(&program->owned_const_strings[i]);
    if (program->const_pool[i].kind == GVM_VALUE_LIST || program->const_pool[i].kind == GVM_VALUE_DICT ||
        program->const_pool[i].kind == GVM_VALUE_TUPLE || program->const_pool[i].kind == GVM_VALUE_SET ||
        program->const_pool[i].kind == GVM_VALUE_GRAPH_REF || program->const_pool[i].kind == GVM_VALUE_HYPERGRAPH_REF) {
      vm_value_dispose_owned(&program->const_pool[i]);
    }
    vm_value_set_none(&program->const_pool[i]);
  }
  free(program->global_names);
  program->global_names = NULL;
  program->global_count = 0U;
  program->global_capacity = 0U;
  program->const_count = 0U;
  program->program_len = 0U;
  memset(program->program, 0, sizeof(program->program));
}
