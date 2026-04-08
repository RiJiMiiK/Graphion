/* SPDX-License-Identifier: MIT */

#include "parser/bytecode.h"
#include "parser/frontend.h"
#include "compiler/ir.h"
#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include "runtime/entry.h"
#include "runtime/interpreter.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_VM_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

static int finish_scope_test(graphion_runtime_scope *scope, int code) {
  graphion_runtime_scope_dispose(scope);
  return code;
}

static int test_read_file_text(const char *path, char *buffer, size_t capacity) {
  FILE *fp = NULL;
  size_t read_len;
  if (path == NULL || buffer == NULL || capacity == 0U) {
    return 0;
  }
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    return 0;
  }
  read_len = fread(buffer, 1U, capacity - 1U, fp);
  fclose(fp);
  buffer[read_len] = '\0';
  return 1;
}

static void normalize_text_newlines(char *text) {
  size_t read_index = 0U;
  size_t write_index = 0U;
  if (text == NULL) {
    return;
  }
  while (text[read_index] != '\0') {
    if (text[read_index] != '\r') {
      text[write_index++] = text[read_index];
    }
    ++read_index;
  }
  text[write_index] = '\0';
}
