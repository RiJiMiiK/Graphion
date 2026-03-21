/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_PARSER_STDLIB_H
#define GRAPHION_PARSER_STDLIB_H

#include <stddef.h>

#include "parser/frontend.h"

typedef struct {
  const char *name;
  const char *description;
  const char *source;
  int requires_csr;
  int requires_hypergraph;
  int requires_frontier;
} graphion_stdlib_program;

size_t graphion_stdlib_program_count(void);

const graphion_stdlib_program *graphion_stdlib_program_at(size_t index);

const graphion_stdlib_program *graphion_stdlib_find_program(const char *name);

int graphion_stdlib_lower_program_to_ir(const char *name,
                                        graphion_ir_insn *out_ir,
                                        size_t out_capacity,
                                        size_t *out_count,
                                        graphion_frontend_diagnostic *diagnostic);

#endif
