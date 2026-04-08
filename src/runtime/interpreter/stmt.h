/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_STMT_H
#define GRAPHION_RUNTIME_INTERPRETER_STMT_H

#include "runtime/interpreter/expr.h"

int parse_assignment(const char *line_text,
                     graphion_runtime_program *program,
                     unsigned int line,
                     graphion_runtime_diagnostic *diagnostic);
int parse_print(const char *line_text,
                const graphion_runtime_scope *scope,
                graphion_runtime_program *program,
                unsigned int line,
                graphion_runtime_diagnostic *diagnostic);
int seed_program_from_scope(graphion_runtime_program *program,
                            const graphion_runtime_scope *scope,
                            unsigned int line,
                            graphion_runtime_diagnostic *diagnostic);
int parse_statement_line(const char *line_text,
                         const graphion_runtime_scope *scope,
                         graphion_runtime_program *program,
                         unsigned int line,
                         graphion_runtime_diagnostic *diagnostic);

#endif
