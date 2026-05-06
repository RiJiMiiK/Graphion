/* SPDX-License-Identifier: MIT */

#include <float.h>
#include <math.h>
#include <string.h>

#include "test_vm_helpers.h"
#include "vm/internal/opcodes/op_scalar.h"

int test_vm_isnan_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = NAN;
    insn.op = GVM_OP_ISNAN;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_isnan_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 1) {
        return 18138;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 1.0;
    rc = op_isnan_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
        return 18139;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 7;
    rc = op_isnan_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
        return 18140;
    }

    return 0;
}

int test_vm_isinf_builtin_opcode(void) {
  graphion_vm vm;
  graphion_insn insn;
  int rc;

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = INFINITY;
  insn.op = GVM_OP_ISINF;
  insn.a = 0;
  insn.b = 0;
  insn.imm = 0;
  rc = op_isinf_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 1) {
    return 18141;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = 1.0;
  rc = op_isinf_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
    return 18142;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_INT;
  vm.regs[0].as.int_value = 7;
  rc = op_isinf_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
    return 18143;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = DBL_MAX;
  rc = op_isinf_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
    return 18144;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = -DBL_MAX;
  rc = op_isinf_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
    return 18145;
  }

  return 0;
}

int test_vm_isfinite_builtin_opcode(void) {
  graphion_vm vm;
  graphion_insn insn;
  int rc;

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = INFINITY;
  insn.op = GVM_OP_ISFINITE;
  insn.a = 0;
  insn.b = 0;
  insn.imm = 0;
  rc = op_isfinite_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
    return 18146;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = NAN;
  rc = op_isfinite_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 0) {
    return 18147;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = 1.0;
  rc = op_isfinite_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 1) {
    return 18148;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_INT;
  vm.regs[0].as.int_value = 7;
  rc = op_isfinite_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 1) {
    return 18149;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = DBL_MAX;
  rc = op_isfinite_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 1) {
    return 18150;
  }

  memset(&vm, 0, sizeof(vm));
  vm.regs[0].kind = GVM_VALUE_FLOAT;
  vm.regs[0].as.float_value = -DBL_MAX;
  rc = op_isfinite_builtin(&vm, &insn);
  if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_BOOL || vm.regs[0].as.bool_value != 1) {
    return 18151;
  }

  memset(&vm, 0, sizeof(vm));
  insn.a = 16;
  rc = op_isfinite_builtin(&vm, &insn);
  if (rc != GVM_ERR_INVALID_REG) {
    return 18152;
  }

  return 0;
}

int test_vm_fract_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 7;
    insn.op = GVM_OP_FRACT;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_fract_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18148;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 7.25;
    rc = op_fract_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.249999999 ||
        vm.regs[0].as.float_value > 0.250000001) {
        return 18149;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -3.75;
    rc = op_fract_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.249999999 ||
        vm.regs[0].as.float_value > 0.250000001) {
        return 18150;
    }

    return 0;
}
