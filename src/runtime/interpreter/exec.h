/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_EXEC_H
#define GRAPHION_RUNTIME_INTERPRETER_EXEC_H

#include "runtime/interpreter/source.h"
#include "runtime/interpreter/stmt.h"

int execute_block(const runtime_source_line *lines,
                  size_t count,
                  size_t *index,
                  unsigned int block_indent,
                  graphion_runtime_scope *scope,
                  graphion_runtime_diagnostic *diagnostic,
                  FILE *output);

#endif
