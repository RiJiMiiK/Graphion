/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_TESTS_GION_PARSER_HELPERS_H
#define GRAPHION_TESTS_GION_PARSER_HELPERS_H

#include "compiler/ir.h"
#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include "parser/bytecode.h"
#include "parser/frontend.h"
#include "runtime/entry.h"
#include "runtime/interpreter.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TEST_VM_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

int finish_scope_test(graphion_runtime_scope *scope, int code);
int test_make_temp_path(char *buffer, size_t capacity, const char *label);
FILE *test_open_temp_output(char *path_buffer, size_t capacity, const char *label);
int test_read_file_text(const char *path, char *buffer, size_t capacity);
int test_capture_gion_output(const char *source,
                             const char *label,
                             graphion_runtime_scope *scope,
                             graphion_runtime_diagnostic *diagnostic,
                             char *path_buffer,
                             size_t path_capacity,
                             char *output_buffer,
                             size_t output_capacity);
void test_cleanup_temp_path(const char *path);
void normalize_text_newlines(char *text);

#endif
