/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_BASE_H
#define GRAPHION_RUNTIME_INTERPRETER_BASE_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runtime/interpreter.h"

void clear_diagnostic(graphion_runtime_diagnostic *diagnostic);
int fail(graphion_runtime_diagnostic *diagnostic,
         unsigned int line,
         unsigned int column,
         const char *message,
         int code);
int add_warning(graphion_runtime_warning_report *report,
                unsigned int line,
                unsigned int column,
                const char *message,
                graphion_runtime_diagnostic *diagnostic);
void vm_value_set_none(graphion_vm_value *value);
void runtime_free_string(char **text);
void skip_spaces(const char **cursor);
int is_ident_start_char(char ch);
int is_ident_char(char ch);
int is_reserved_name(const char *name);
void copy_name(char dst[GRAPHION_RUNTIME_NAME_MAX], const char *src);

#endif
