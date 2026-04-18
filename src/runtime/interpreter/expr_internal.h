/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_EXPR_INTERNAL_H
#define GRAPHION_RUNTIME_INTERPRETER_EXPR_INTERNAL_H

#include "runtime/interpreter/expr.h"

int try_parse_direct_builtin(const char **cursor,
                             graphion_runtime_program *program,
                             parsed_expr_result *result_out,
                             uint8_t base_reg,
                             unsigned int line,
                             graphion_runtime_diagnostic *diagnostic);

#endif
