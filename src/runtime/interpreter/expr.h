/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_EXPR_H
#define GRAPHION_RUNTIME_INTERPRETER_EXPR_H

#include "runtime/interpreter/operands.h"

int parse_expression(const char **cursor,
                     graphion_runtime_program *program,
                     parsed_expr_result *result_out,
                     uint8_t base_reg,
                     unsigned int line,
                     graphion_runtime_diagnostic *diagnostic);

#endif
