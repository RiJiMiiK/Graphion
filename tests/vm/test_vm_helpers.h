/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"
#include "vm/internal/opcodes/op_scalar.h"
#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define TEST_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

static void test_set_reg_i(graphion_vm *vm, uint8_t reg, int64_t value) {
  vm->regs[reg].kind = GVM_VALUE_INT;
  vm->regs[reg].as.int_value = value;
}

static void test_set_value_int(graphion_vm_value *value, int64_t number) {
  value->kind = GVM_VALUE_INT;
  value->as.int_value = number;
}

static void test_set_value_float(graphion_vm_value *value, double number) {
  value->kind = GVM_VALUE_FLOAT;
  value->as.float_value = number;
}

static void test_set_value_bool(graphion_vm_value *value, int boolean) {
  value->kind = GVM_VALUE_BOOL;
  value->as.bool_value = boolean != 0 ? 1 : 0;
}

static void test_set_value_string(graphion_vm_value *value, const char *text) {
  value->kind = GVM_VALUE_STRING;
  value->as.string_value = text;
}

static void test_set_value_bits(graphion_vm_value *value, uint64_t bits_value, uint8_t width) {
  value->kind = GVM_VALUE_BITS;
  value->reserved[0] = width;
  value->as.int_value = (int64_t)bits_value;
}

static int run_vm_program(graphion_vm *vm, const graphion_insn *program, size_t len) {
  int rc;
  graphion_vm_init(vm);
  rc = graphion_vm_load(vm, program, len);
  if (rc != 0) {
    return rc;
  }
  return graphion_vm_run(vm);
}

static int finish_vm_test(graphion_vm *vm, int code) {
  graphion_vm_dispose(vm);
  return code;
}

static int finish_vm_test_with_owned_globals(graphion_vm *vm, char **owners, size_t owner_count, int code) {
  size_t i;
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
