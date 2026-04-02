/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_PROGRAM_H
#define GRAPHION_RUNTIME_INTERPRETER_PROGRAM_H

#include "runtime/interpreter/base.h"

int scope_find_global_index(const graphion_runtime_scope *scope, const char *name);
int program_find_global_index(const graphion_runtime_program *program, const char *name);
int scope_find_index(const graphion_runtime_scope *scope, const char *name);
int program_find_or_add_global(graphion_runtime_program *program,
                               const char *name,
                               unsigned int line,
                               graphion_runtime_diagnostic *diagnostic,
                               size_t *index_out);
int program_add_const(graphion_runtime_program *program,
                      const graphion_vm_value *value,
                      unsigned int line,
                      graphion_runtime_diagnostic *diagnostic,
                      size_t *index_out);
int program_emit(graphion_runtime_program *program,
                 graphion_opcode op,
                 uint8_t a,
                 uint8_t b,
                 int32_t imm,
                 unsigned int line,
                 graphion_runtime_diagnostic *diagnostic);
int program_patch_imm(graphion_runtime_program *program,
                      size_t insn_index,
                      int32_t imm,
                      unsigned int line,
                      graphion_runtime_diagnostic *diagnostic);
int program_emit_load_bool(graphion_runtime_program *program,
                           uint8_t reg,
                           int bool_value,
                           unsigned int line,
                           graphion_runtime_diagnostic *diagnostic);
int program_emit_load_int(graphion_runtime_program *program,
                          uint8_t reg,
                          int64_t int_value,
                          unsigned int line,
                          graphion_runtime_diagnostic *diagnostic);
int runtime_value_get_numeric(const graphion_vm_value *value,
                              int64_t *int_out,
                              double *float_out,
                              int *is_float_out);
int scalar_values_match_equal(const graphion_vm_value *lhs,
                              const graphion_vm_value *rhs,
                              int *compatible_out,
                              int *equal_out);

#endif
