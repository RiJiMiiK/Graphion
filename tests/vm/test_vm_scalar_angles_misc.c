/* SPDX-License-Identifier: MIT */

#include <float.h>
#include <math.h>
#include <string.h>

#include "test_vm_helpers.h"
#include "vm/internal/opcodes/op_scalar.h"

int test_vm_cos_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_COS, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_COS, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 3.14159265358979323846);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  vm.const_pool = const_pool;
  vm.const_count = 2;
  vm.globals = globals;
  vm.global_count = 2;
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT) {
    return 3;
  }
  if (globals[0].as.float_value < 0.999999999 || globals[0].as.float_value > 1.000000001) {
    return 4;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value > -0.999999999 ||
      globals[1].as.float_value < -1.000000001) {
    return 5;
  }
  return 0;
}

int test_vm_tan_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_TAN, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_TAN, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 0.7853981633974483);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  vm.const_pool = const_pool;
  vm.const_count = 2;
  vm.globals = globals;
  vm.global_count = 2;
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < -0.000000001 ||
      globals[0].as.float_value > 0.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.999999999 ||
      globals[1].as.float_value > 1.000000001) {
    return 4;
  }
  return 0;
}

int test_vm_asin_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ASIN, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ASIN, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ASIN, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 1.0);
  test_set_value_float(&const_pool[2], 0.5);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

  graphion_vm_init(&vm);
  vm.const_pool = const_pool;
  vm.const_count = 3;
  vm.globals = globals;
  vm.global_count = 3;
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < -0.000000001 ||
      globals[0].as.float_value > 0.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 1.570796326 ||
      globals[1].as.float_value > 1.570796328) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 0.523598775 ||
      globals[2].as.float_value > 0.523598777) {
    return 5;
  }
    return 0;
}

int test_vm_acos_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 1.0;
    insn.op = GVM_OP_ACOS;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_acos_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.000000001 ||
        vm.regs[0].as.float_value > 0.000000001) {
        return 18120;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    rc = op_acos_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.570796326 ||
        vm.regs[0].as.float_value > 1.570796328) {
        return 18121;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.5;
    rc = op_acos_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.047197550 ||
        vm.regs[0].as.float_value > 1.047197552) {
        return 18122;
    }

      return 0;
  }

int test_vm_atan_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    insn.op = GVM_OP_ATAN;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_atan_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.000000001 ||
        vm.regs[0].as.float_value > 0.000000001) {
        return 18123;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 1.0;
    rc = op_atan_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.785398163 ||
        vm.regs[0].as.float_value > 0.785398164) {
        return 18124;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -1.0;
    rc = op_atan_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.785398164 ||
        vm.regs[0].as.float_value > -0.785398163) {
        return 18125;
    }

    return 0;
}

int test_vm_atan2_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 1.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = 1.0;
    insn.op = GVM_OP_ATAN2;
    insn.a = 0;
    insn.b = 1;
    insn.imm = 0;
    rc = op_atan2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.785398163 ||
        vm.regs[0].as.float_value > 0.785398164) {
        return 18126;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 1.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = -1.0;
    rc = op_atan2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 2.356194489 ||
        vm.regs[0].as.float_value > 2.356194491) {
        return 18127;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -1.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = -1.0;
    rc = op_atan2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -2.356194491 ||
        vm.regs[0].as.float_value > -2.356194489) {
        return 18128;
    }

    return 0;
}

int test_vm_hypot_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 3.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = 4.0;
    insn.op = GVM_OP_HYPOT;
    insn.a = 0;
    insn.b = 1;
    insn.imm = 0;
    rc = op_hypot_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 4.999999999 ||
        vm.regs[0].as.float_value > 5.000000001) {
        return 18129;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 5.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = 12.0;
    rc = op_hypot_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 12.999999999 ||
        vm.regs[0].as.float_value > 13.000000001) {
        return 18130;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -3.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = 4.0;
    rc = op_hypot_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 4.999999999 ||
        vm.regs[0].as.float_value > 5.000000001) {
        return 18131;
    }

    return 0;
}

int test_vm_copysign_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 3;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = -2;
    insn.op = GVM_OP_COPYSIGN;
    insn.a = 0;
    insn.b = 1;
    insn.imm = 0;
    rc = op_copysign_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != -3.0) {
        return 18132;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -3.5;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 2;
    rc = op_copysign_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 3.5) {
        return 18133;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 3.5;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = -2.0;
    rc = op_copysign_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != -3.5) {
        return 18134;
    }

    return 0;
}

int test_vm_fma_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 2;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 3;
    vm.regs[2].kind = GVM_VALUE_INT;
    vm.regs[2].as.int_value = 4;
    insn.op = GVM_OP_FMA;
    insn.a = 0;
    insn.b = 1;
    insn.imm = 2;
    rc = op_fma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 10.0) {
        return 18135;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.5;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 8;
    vm.regs[2].kind = GVM_VALUE_FLOAT;
    vm.regs[2].as.float_value = -1.0;
    rc = op_fma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 3.0) {
        return 18136;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -2.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = 4.0;
    vm.regs[2].kind = GVM_VALUE_FLOAT;
    vm.regs[2].as.float_value = 1.5;
    rc = op_fma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != -6.5) {
        return 18137;
    }

    return 0;
}

int test_vm_fdim_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 7;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 3;
    insn.op = GVM_OP_FDIM;
    insn.a = 0;
    insn.b = 1;
    insn.imm = 0;
    rc = op_fdim_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 4.0) {
        return 18138;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 3;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 7;
    rc = op_fdim_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18139;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 5.0;
    vm.regs[1].kind = GVM_VALUE_FLOAT;
    vm.regs[1].as.float_value = 5.0;
    rc = op_fdim_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18140;
    }

    return 0;
}

int test_vm_remainder_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 7;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 4;
    insn.op = GVM_OP_REMAINDER;
    insn.a = 0;
    insn.b = 1;
    insn.imm = 0;
    rc = op_remainder_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != -1.0) {
        return 18141;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 5.5;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 2;
    rc = op_remainder_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != -0.5) {
        return 18142;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = -7;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 4;
    rc = op_remainder_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 1.0) {
        return 18143;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    vm.regs[1].kind = GVM_VALUE_INT;
    vm.regs[1].as.int_value = 0;
    rc = op_remainder_builtin(&vm, &insn);
    if (rc != GVM_ERR_REMAINDER_DOMAIN) {
        return 18144;
    }

    return 0;
}

int test_vm_degrees_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    insn.op = GVM_OP_DEGREES;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_degrees_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.000000001 ||
        vm.regs[0].as.float_value > 0.000000001) {
        return 18132;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 1.5707963267948966;
    rc = op_degrees_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 89.999999999 ||
        vm.regs[0].as.float_value > 90.000000001) {
        return 18133;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -0.7853981633974483;
    rc = op_degrees_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -45.000000001 ||
        vm.regs[0].as.float_value > -44.999999999) {
        return 18134;
    }

    return 0;
}

int test_vm_radians_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    insn.op = GVM_OP_RADIANS;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_radians_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.000000001 ||
        vm.regs[0].as.float_value > 0.000000001) {
        return 18135;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 180.0;
    rc = op_radians_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 3.141592652589793 ||
        vm.regs[0].as.float_value > 3.141592654589793) {
        return 18136;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -45.0;
    rc = op_radians_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.7853981643974483 ||
        vm.regs[0].as.float_value > -0.7853981623974483) {
        return 18137;
    }

    return 0;
}

