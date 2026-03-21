/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_ENTRY_H
#define GRAPHION_RUNTIME_ENTRY_H

#include <stddef.h>

#include "runtime/interpreter.h"

typedef enum {
  GENTRY_OK = 0,
  GENTRY_ERR_INVALID_ARG = -1,
  GENTRY_ERR_EXTENSION = -2,
  GENTRY_ERR_IO = -3,
  GENTRY_ERR_CAPACITY = -4,
  GENTRY_ERR_PARSE = -5,
  GENTRY_ERR_LOWER = -6,
  GENTRY_ERR_LOAD = -7,
  GENTRY_ERR_RUN = -8
} graphion_entry_result;

int graphion_source_path_is_gion(const char *path);

int graphion_run_gion_path(const char *path, graphion_runtime_scope *scope);

#endif
