/* SPDX-License-Identifier: MIT */

#include "runtime/entry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler/ir.h"
#include "parser/frontend.h"

enum {
  GENTRY_SOURCE_MAX = 16 * 1024,
  GENTRY_PROGRAM_MAX = 1024
};

int graphion_source_path_is_gion(const char *path) {
  size_t len;
  if (path == NULL) {
    return 0;
  }
  len = strlen(path);
  return len >= 5U && strcmp(path + len - 5U, ".gion") == 0;
}

static int read_source_file(const char *path, char *buffer, size_t capacity) {
  FILE *fp;
  size_t read_len;
  if (path == NULL || buffer == NULL || capacity == 0U) {
    return GENTRY_ERR_INVALID_ARG;
  }
  fp = NULL;
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    return GENTRY_ERR_IO;
  }
  read_len = fread(buffer, 1U, capacity - 1U, fp);
  if (ferror(fp) != 0) {
    fclose(fp);
    return GENTRY_ERR_IO;
  }
  if (!feof(fp)) {
    fclose(fp);
    return GENTRY_ERR_CAPACITY;
  }
  buffer[read_len] = '\0';
  fclose(fp);
  return GENTRY_OK;
}

int graphion_run_gion_path(const char *path, graphion_vm *vm) {
  char source[GENTRY_SOURCE_MAX];
  graphion_ir_insn ir[GENTRY_PROGRAM_MAX];
  graphion_insn program[GENTRY_PROGRAM_MAX];
  size_t ir_count = 0U;
  size_t program_count = 0U;
  int rc;

  if (path == NULL || vm == NULL) {
    return GENTRY_ERR_INVALID_ARG;
  }
  if (!graphion_source_path_is_gion(path)) {
    return GENTRY_ERR_EXTENSION;
  }
  rc = read_source_file(path, source, sizeof(source));
  if (rc != GENTRY_OK) {
    return rc;
  }
  rc = graphion_parse_source_to_ir(source, ir, GENTRY_PROGRAM_MAX, &ir_count);
  if (rc != GFE_OK) {
    return GENTRY_ERR_PARSE;
  }
  rc = graphion_ir_lower_to_bytecode(ir, ir_count, program, GENTRY_PROGRAM_MAX, &program_count);
  if (rc != GIR_OK) {
    return GENTRY_ERR_LOWER;
  }
  graphion_vm_init(vm);
  rc = graphion_vm_load(vm, program, program_count);
  if (rc != GVM_OK) {
    return GENTRY_ERR_LOAD;
  }
  rc = graphion_vm_run(vm);
  if (rc != GVM_OK) {
    return GENTRY_ERR_RUN;
  }
  return GENTRY_OK;
}
