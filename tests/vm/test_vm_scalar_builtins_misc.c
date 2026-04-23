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

int test_vm_expm1_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_EXPM1;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_expm1_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.718281828 ||
        vm.regs[0].as.float_value > 1.718281829) {
        return 18151;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    rc = op_expm1_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18152;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 2;
    rc = op_expm1_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 6.389056098 ||
        vm.regs[0].as.float_value > 6.389056100) {
        return 18153;
    }

    return 0;
}

int test_vm_exp2_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_EXP2;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_exp2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.999999999 ||
        vm.regs[0].as.float_value > 2.000000001) {
        return 18160;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    rc = op_exp2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 1.0) {
        return 18161;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 2;
    rc = op_exp2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 3.999999999 ||
        vm.regs[0].as.float_value > 4.000000001) {
        return 18162;
    }

    return 0;
}

int test_vm_log1p_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_LOG1P;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_log1p_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.693147180 ||
        vm.regs[0].as.float_value > 0.693147181) {
        return 18154;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    rc = op_log1p_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18155;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -1.0;
    rc = op_log1p_builtin(&vm, &insn);
    if (rc != GVM_ERR_LOG1P_DOMAIN) {
        return 18156;
    }

    return 0;
}

int test_vm_erf_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    insn.op = GVM_OP_ERF;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_erf_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18157;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    rc = op_erf_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.842700792 ||
        vm.regs[0].as.float_value > 0.842700793) {
        return 18158;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = -1;
    rc = op_erf_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.842700793 ||
        vm.regs[0].as.float_value > -0.842700792) {
        return 18159;
    }

    return 0;
}

int test_vm_erfc_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    insn.op = GVM_OP_ERFC;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_erfc_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 1.0) {
        return 18160;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    rc = op_erfc_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.157299207 ||
        vm.regs[0].as.float_value > 0.157299208) {
        return 18161;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = -1;
    rc = op_erfc_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.842700792 ||
        vm.regs[0].as.float_value > 1.842700793) {
        return 18162;
    }

    return 0;
}

int test_vm_gamma_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_GAMMA;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 1.0) {
        return 18163;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 5;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 24.0) {
        return 18164;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.5;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.772453849 ||
        vm.regs[0].as.float_value > 1.772453851) {
        return 18165;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_ERR_GAMMA_DOMAIN) {
        return 18166;
    }

    return 0;
}

int test_vm_lgamma_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_LGAMMA;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18167;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 5;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 3.178053829 ||
        vm.regs[0].as.float_value > 3.178053831) {
        return 18168;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.5;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.572364941 ||
        vm.regs[0].as.float_value > 0.572364943) {
        return 18169;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_ERR_LGAMMA_DOMAIN) {
        return 18170;
    }

    return 0;
}

int test_vm_exp_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 1},
      {GVM_OP_EXP, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_EXP, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 2.718281828 || globals[0].as.float_value > 2.718281829) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 1.0) {
    return 4;
  }
  return 0;
}

int test_vm_ln_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 1},
      {GVM_OP_LN, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_LN, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 2.71828182845904523536);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 0.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.999999999 ||
      globals[1].as.float_value > 1.000000001) {
    return 4;
  }
  return 0;
}

int test_vm_log_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_LOG, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_LOG, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 8);
  test_set_value_int(&const_pool[1], 2);
  test_set_value_int(&const_pool[2], 100);
  test_set_value_int(&const_pool[3], 10);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 2.999999999 ||
      globals[0].as.float_value > 3.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 1.999999999 ||
      globals[1].as.float_value > 2.000000001) {
    return 4;
  }
    return 0;
}

int test_vm_rint_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 7;
    insn.op = GVM_OP_RINT;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_rint_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 7.0) {
        return 18186;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 7.4;
    rc = op_rint_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 7.0) {
        return 18187;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -3.2;
    rc = op_rint_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != -3.0) {
        return 18188;
    }

    return 0;
}

int test_vm_floor_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_FLOOR, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_FLOOR, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_MOV_IMM, 2, 0, 5},
      {GVM_OP_FLOOR, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 7.5);
  test_set_value_float(&const_pool[1], -3.2);
  test_set_value_int(&const_pool[2], 0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[1].kind = GVM_VALUE_NONE;
  globals[2].kind = GVM_VALUE_NONE;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 3U);
  graphion_vm_bind_globals(&vm, globals, 3U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 7.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != -4.0) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_INT || globals[2].as.int_value != 5) {
    return 5;
  }
  return 0;
}

int test_vm_ceil_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_CEIL, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_CEIL, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_MOV_IMM, 2, 0, 5},
      {GVM_OP_CEIL, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 7.5);
  test_set_value_float(&const_pool[1], -3.2);
  test_set_value_int(&const_pool[2], 0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[1].kind = GVM_VALUE_NONE;
  globals[2].kind = GVM_VALUE_NONE;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 3U);
  graphion_vm_bind_globals(&vm, globals, 3U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 8.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != -3.0) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_INT || globals[2].as.int_value != 5) {
    return 5;
  }
  return 0;
}

int test_vm_round_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[5];
  graphion_vm_value globals[5];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ROUND, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ROUND, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ROUND, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_LOAD_CONST, 3, 0, 3},
      {GVM_OP_ROUND, 3, 0, 0},
      {GVM_OP_STORE_GLOBAL, 3, 0, 3},
      {GVM_OP_MOV_IMM, 4, 0, 5},
      {GVM_OP_ROUND, 4, 0, 0},
      {GVM_OP_STORE_GLOBAL, 4, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 7.4);
  test_set_value_float(&const_pool[1], 7.5);
  test_set_value_float(&const_pool[2], -3.2);
  test_set_value_float(&const_pool[3], -3.5);
  test_set_value_int(&const_pool[4], 0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[1].kind = GVM_VALUE_NONE;
  globals[2].kind = GVM_VALUE_NONE;
  globals[3].kind = GVM_VALUE_NONE;
  globals[4].kind = GVM_VALUE_NONE;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 5U);
  graphion_vm_bind_globals(&vm, globals, 5U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 7.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 8.0) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value != -3.0) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_FLOAT || globals[3].as.float_value != -4.0) {
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_INT || globals[4].as.int_value != 5) {
    return 7;
  }
  return 0;
}

int test_vm_trunc_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[5];
  graphion_vm_value globals[5];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_TRUNC, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_TRUNC, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_TRUNC, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_LOAD_CONST, 3, 0, 3},
      {GVM_OP_TRUNC, 3, 0, 0},
      {GVM_OP_STORE_GLOBAL, 3, 0, 3},
      {GVM_OP_MOV_IMM, 4, 0, 5},
      {GVM_OP_TRUNC, 4, 0, 0},
      {GVM_OP_STORE_GLOBAL, 4, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 7.9);
  test_set_value_float(&const_pool[1], -3.9);
  test_set_value_float(&const_pool[2], 7.0);
  test_set_value_float(&const_pool[3], -0.4);
  test_set_value_int(&const_pool[4], 0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[1].kind = GVM_VALUE_NONE;
  globals[2].kind = GVM_VALUE_NONE;
  globals[3].kind = GVM_VALUE_NONE;
  globals[4].kind = GVM_VALUE_NONE;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 5U);
  graphion_vm_bind_globals(&vm, globals, 5U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 7.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != -3.0) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value != 7.0) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_FLOAT || globals[3].as.float_value != 0.0) {
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_INT || globals[4].as.int_value != 5) {
    return 7;
  }
  return 0;
}

int test_vm_sign_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[4];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_SIGN, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_SIGN, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_SIGN, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_MOV_IMM, 3, 0, 5},
      {GVM_OP_SIGN, 3, 0, 0},
      {GVM_OP_STORE_GLOBAL, 3, 0, 3},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_float(&const_pool[0], 7.9);
  test_set_value_float(&const_pool[1], -3.9);
  test_set_value_float(&const_pool[2], 0.0);
  test_set_value_int(&const_pool[3], 0);
  for (i = 0U; i < 4U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  graphion_vm_bind_globals(&vm, globals, 4U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 1) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_INT || globals[1].as.int_value != -1) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_INT || globals[2].as.int_value != 0) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_INT || globals[3].as.int_value != 1) {
    return 6;
  }
  return 0;
}

