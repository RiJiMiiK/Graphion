/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_H
#define GRAPHION_RUNTIME_INTERPRETER_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

enum {
  GRAPHION_RUNTIME_BINDING_MAX = 128,
  GRAPHION_RUNTIME_NAME_MAX = 64,
  GRAPHION_RUNTIME_STRING_MAX = 256
};

typedef enum {
  GRAPHION_VALUE_NONE = 0,
  GRAPHION_VALUE_INT = 1,
  GRAPHION_VALUE_FLOAT = 2,
  GRAPHION_VALUE_BOOL = 3,
  GRAPHION_VALUE_STRING = 4
} graphion_runtime_value_kind;

typedef struct {
  int kind;
  int64_t int_value;
  double float_value;
  int bool_value;
  char string_value[GRAPHION_RUNTIME_STRING_MAX];
} graphion_runtime_value;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  graphion_runtime_value value;
} graphion_runtime_binding;

typedef struct {
  graphion_runtime_binding bindings[GRAPHION_RUNTIME_BINDING_MAX];
  size_t count;
} graphion_runtime_scope;

typedef struct {
  size_t line;
  size_t column;
  const char *message;
} graphion_runtime_diagnostic;

typedef enum {
  GINT_OK = 0,
  GINT_ERR_INVALID_ARG = -1,
  GINT_ERR_CAPACITY = -2,
  GINT_ERR_PARSE = -3,
  GINT_ERR_UNKNOWN_VARIABLE = -4,
  GINT_ERR_RESERVED_NAME = -5,
  GINT_ERR_CALL = -6,
  GINT_ERR_RETURN = -7
} graphion_interpreter_result;

void graphion_runtime_scope_init(graphion_runtime_scope *scope);

const graphion_runtime_value *graphion_runtime_scope_find(const graphion_runtime_scope *scope,
                                                          const char *name);

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic);

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output);

#endif
