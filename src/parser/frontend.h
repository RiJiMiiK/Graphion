/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_PARSER_FRONTEND_H
#define GRAPHION_PARSER_FRONTEND_H

#include <stddef.h>

#include "compiler/ir.h"

typedef enum {
  GFE_OK = 0,
  GFE_ERR_INVALID_ARG = -1,
  GFE_ERR_CAPACITY = -2,
  GFE_ERR_PARSE = -3
} graphion_frontend_result;

int graphion_parse_source_to_ir(const char *source,
                                graphion_ir_insn *out_ir,
                                size_t out_capacity,
                                size_t *out_count);

#endif
