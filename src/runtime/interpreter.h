/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_H
#define GRAPHION_RUNTIME_INTERPRETER_H

#include <stdio.h>

#include "vm/vm.h"

enum {
  GRAPHION_RUNTIME_BINDING_MAX = 128,
  GRAPHION_RUNTIME_NAME_MAX = 64,
  GRAPHION_RUNTIME_CONST_MAX = 256,
  GRAPHION_RUNTIME_PROGRAM_MAX = 512
};

typedef struct {
  unsigned int line;
  unsigned int column;
  const char *message;
} graphion_runtime_diagnostic;

typedef graphion_vm_value graphion_runtime_value;

typedef struct {
  char global_names[GRAPHION_RUNTIME_BINDING_MAX][GRAPHION_RUNTIME_NAME_MAX];
  char *owned_string_values[GRAPHION_RUNTIME_BINDING_MAX];
  graphion_runtime_value globals[GRAPHION_RUNTIME_BINDING_MAX];
  size_t global_count;
} graphion_runtime_scope;

typedef struct {
  char global_names[GRAPHION_RUNTIME_BINDING_MAX][GRAPHION_RUNTIME_NAME_MAX];
  size_t global_count;
  char *owned_const_strings[GRAPHION_RUNTIME_CONST_MAX];
  graphion_vm_value const_pool[GRAPHION_RUNTIME_CONST_MAX];
  size_t const_count;
  graphion_insn program[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t program_len;
} graphion_runtime_program;

typedef enum {
  GINT_OK = 0,
  GINT_ERR_INVALID_ARG = -1,
  GINT_ERR_CAPACITY = -2,
  GINT_ERR_PARSE = -3,
  GINT_ERR_UNKNOWN_VARIABLE = -4,
  GINT_ERR_RESERVED_NAME = -5,
  GINT_ERR_CALL = -6,
  GINT_ERR_RETURN = -7,
  GINT_ERR_RUN = -8
} graphion_interpreter_result;

void graphion_runtime_scope_init(graphion_runtime_scope *scope);
void graphion_runtime_scope_dispose(graphion_runtime_scope *scope);

const graphion_runtime_value *graphion_runtime_scope_find(const graphion_runtime_scope *scope,
                                                          const char *name);

void graphion_runtime_program_init(graphion_runtime_program *program);
void graphion_runtime_program_dispose(graphion_runtime_program *program);

int graphion_prepare_source(const char *source,
                            graphion_runtime_program *program,
                            graphion_runtime_diagnostic *diagnostic);

int graphion_execute_program(const graphion_runtime_program *program,
                             graphion_runtime_scope *scope,
                             graphion_runtime_diagnostic *diagnostic,
                             FILE *output);

int graphion_execute_prepared_program_with_sink(const graphion_runtime_program *program,
                                                graphion_runtime_scope *scope,
                                                graphion_runtime_diagnostic *diagnostic,
                                                const graphion_output_sink *output);

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic);

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output);

#endif
