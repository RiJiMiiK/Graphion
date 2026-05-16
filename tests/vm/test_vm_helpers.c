/* SPDX-License-Identifier: MIT */

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "test_vm_helpers.h"

#include "vm/internal/core/value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

int test_make_temp_path_vm(char *buffer, size_t capacity, const char *label) {
  if (buffer == NULL || capacity == 0U) {
    return 0;
  }

#if defined(_WIN32)
  {
    char temp_dir[MAX_PATH];
    char temp_file[MAX_PATH];
    DWORD dir_len = GetTempPathA((DWORD)sizeof(temp_dir), temp_dir);
    int written;

    if (dir_len == 0U || dir_len >= sizeof(temp_dir)) {
      return 0;
    }
    if (GetTempFileNameA(temp_dir, "gio", 0U, temp_file) == 0U) {
      return 0;
    }
    remove(temp_file);
    written =
        snprintf(buffer, capacity, "%s%s%s", temp_file, label != NULL ? "_" : "", label != NULL ? label : "");
    return written > 0 && (size_t)written < capacity;
  }
#else
  {
    const char *tmp_dir = getenv("TMPDIR");
    char temp_template[256];
    int fd;
    int written;

    if (tmp_dir == NULL || *tmp_dir == '\0') {
      tmp_dir = "/tmp";
    }
    written = snprintf(temp_template, sizeof(temp_template), "%s/graphion_test_XXXXXX", tmp_dir);
    if (written <= 0 || (size_t)written >= sizeof(temp_template)) {
      return 0;
    }
    fd = mkstemp(temp_template);
    if (fd < 0) {
      return 0;
    }
    close(fd);
    remove(temp_template);
    written = snprintf(buffer, capacity, "%s%s%s", temp_template, label != NULL ? "_" : "", label != NULL ? label : "");
    return written > 0 && (size_t)written < capacity;
  }
#endif
}

FILE *test_open_temp_output_vm(char *path_buffer, size_t capacity, const char *label) {
  FILE *fp = NULL;

  if (!test_make_temp_path_vm(path_buffer, capacity, label)) {
    return NULL;
  }
#if defined(_MSC_VER)
  if (fopen_s(&fp, path_buffer, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path_buffer, "wb");
#endif
  return fp;
}

void test_set_reg_i(graphion_vm *vm, uint8_t reg, int64_t value) {
  vm->regs[reg].kind = GVM_VALUE_INT;
  vm->regs[reg].as.int_value = value;
}

void test_set_value_int(graphion_vm_value *value, int64_t number) {
  value->kind = GVM_VALUE_INT;
  value->as.int_value = number;
}

void test_set_value_float(graphion_vm_value *value, double number) {
  value->kind = GVM_VALUE_FLOAT;
  value->as.float_value = number;
}

void test_set_value_bool(graphion_vm_value *value, int boolean) {
  value->kind = GVM_VALUE_BOOL;
  value->as.bool_value = boolean != 0 ? 1 : 0;
}

void test_set_value_string(graphion_vm_value *value, const char *text) {
  value->kind = GVM_VALUE_STRING;
  value->as.string_value = text;
}

void test_set_value_bits(graphion_vm_value *value, uint64_t bits_value, uint8_t width) {
  value->kind = GVM_VALUE_BITS;
  value->reserved[0] = width;
  value->as.int_value = (int64_t)bits_value;
}

int run_vm_program(graphion_vm *vm, const graphion_insn *program, size_t len) {
  int rc;

  graphion_vm_init(vm);
  rc = graphion_vm_load(vm, program, len);
  if (rc != 0) {
    return rc;
  }
  return graphion_vm_run(vm);
}

int finish_vm_test(graphion_vm *vm, int code) {
  size_t i;

  if (vm != NULL && vm->globals != NULL) {
    for (i = 0U; i < vm->global_count; ++i) {
      vm_release_global_value(vm, i);
    }
  }
  graphion_vm_dispose(vm);
  return code;
}

int finish_vm_test_with_owned_globals(graphion_vm *vm, char **owners, size_t owner_count, int code) {
  size_t i;

  if (vm != NULL && vm->globals != NULL) {
    for (i = 0U; i < vm->global_count; ++i) {
      vm_release_global_value(vm, i);
    }
  }
  graphion_vm_dispose(vm);
  if (owners != NULL) {
    for (i = 0U; i < owner_count; ++i) {
      if (owners[i] != NULL) {
        free(owners[i]);
        owners[i] = NULL;
      }
    }
  }
  return code;
}
