/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_vm_helpers.h"
#include "vm/internal/opcodes/op_scalar.h"

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

