/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_EXEC_INTERNAL_H
#define GRAPHION_RUNTIME_INTERPRETER_EXEC_INTERNAL_H

#include "runtime/interpreter/exec.h"

int evaluate_expression_text_to_value(const char *expression_text,
                                      size_t expression_len,
                                      graphion_runtime_scope *scope,
                                      unsigned int line,
                                      graphion_runtime_diagnostic *diagnostic,
                                      graphion_vm_value *value_out);

int evaluate_condition_text(const char *condition_text,
                            size_t condition_len,
                            graphion_runtime_scope *scope,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic,
                            int *result_out);

#endif
