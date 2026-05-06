/* SPDX-License-Identifier: MIT */

#include <float.h>
#include <math.h>
#include <string.h>

#include "test_vm_helpers.h"
#include "vm/internal/opcodes/op_scalar.h"

int test_vm_numeric_arithmetic_opcodes(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 10},
      {GVM_OP_MOV_IMM, 1, 0, 4},
      {GVM_OP_SUB, 0, 1, 0},
      {GVM_OP_MOV_IMM, 2, 0, 3},
      {GVM_OP_MUL, 0, 2, 0},
      {GVM_OP_LOAD_CONST, 3, 0, 0},
      {GVM_OP_ADD, 0, 3, 0},
      {GVM_OP_LOAD_CONST, 4, 0, 1},
      {GVM_OP_DIV, 0, 4, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.5);
  test_set_value_int(&const_pool[1], 2);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 9.25) {
    return 3;
  }
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 9.25) {
    return 4;
  }

  return 0;
}

int test_vm_numeric_arithmetic_precedence_shapes(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_SUB, 0, 1, 0},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_MUL, 0, 2, 0},
      {GVM_OP_LOAD_CONST, 3, 0, 3},
      {GVM_OP_DIV, 0, 3, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 10.5);
  test_set_value_int(&const_pool[1], 2);
  test_set_value_float(&const_pool[2], 1.5);
  test_set_value_int(&const_pool[3], 2);

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 6.375) {
    return 3;
  }
  return 0;
}

int test_vm_divide_by_zero_fails(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 0},
      {GVM_OP_DIV, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc = run_vm_program(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_ERR_DIVIDE_BY_ZERO) {
    return 1;
  }
  return 0;
}

int test_vm_modulo_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 10},
      {GVM_OP_MOV_IMM, 1, 0, 4},
      {GVM_OP_MOD, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 2, 0, 0},
      {GVM_OP_LOAD_CONST, 3, 0, 1},
      {GVM_OP_MOD, 2, 3, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 7.5);
  test_set_value_float(&const_pool[1], 2.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 2) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 1.5) {
    return 4;
  }
  return 0;
}

int test_vm_power_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 2},
      {GVM_OP_MOV_IMM, 1, 0, 3},
      {GVM_OP_POW, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 2, 0, 0},
      {GVM_OP_LOAD_CONST, 3, 0, 1},
      {GVM_OP_POW, 2, 3, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 9.0);
  test_set_value_float(&const_pool[1], 0.5);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
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
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 3.0) {
    return 4;
  }
  return 0;
}

int test_vm_floor_div_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 2},
      {GVM_OP_FLOOR_DIV, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_MOV_IMM, 2, 0, -7},
      {GVM_OP_MOV_IMM, 3, 0, 2},
      {GVM_OP_FLOOR_DIV, 2, 3, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 1},
      {GVM_OP_LOAD_CONST, 4, 0, 0},
      {GVM_OP_LOAD_CONST, 5, 0, 1},
      {GVM_OP_FLOOR_DIV, 4, 5, 0},
      {GVM_OP_STORE_GLOBAL, 4, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 7.5);
  test_set_value_float(&const_pool[1], 2.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[1].kind = GVM_VALUE_NONE;
  globals[2].kind = GVM_VALUE_NONE;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 3U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 3) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_INT || globals[1].as.int_value != -4) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value != 3.0) {
    return 5;
  }
  return 0;
}

int test_vm_abs_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, -42},
      {GVM_OP_ABS, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_ABS, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], -3.5);
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
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 42) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 3.5) {
    return 4;
  }
  return 0;
}

int test_vm_min_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 3},
      {GVM_OP_MIN, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 2, 0, 0},
      {GVM_OP_LOAD_CONST, 3, 0, 1},
      {GVM_OP_MIN, 2, 3, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 3.5);
  test_set_value_int(&const_pool[1], 2);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 3) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 2.0) {
    return 4;
  }
  return 0;
}

int test_vm_max_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 3},
      {GVM_OP_MAX, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 2, 0, 0},
      {GVM_OP_LOAD_CONST, 3, 0, 1},
      {GVM_OP_MAX, 2, 3, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 3.5);
  test_set_value_int(&const_pool[1], 2);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 7) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 3.5) {
    return 4;
  }
  return 0;
}

int test_vm_clamp_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[6];
  graphion_vm_value globals[4];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, -2},
      {GVM_OP_MOV_IMM, 1, 0, 0},
      {GVM_OP_MOV_IMM, 2, 0, 10},
      {GVM_OP_CLAMP, 0, 1, 2},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_MOV_IMM, 3, 0, 5},
      {GVM_OP_MOV_IMM, 4, 0, 0},
      {GVM_OP_MOV_IMM, 5, 0, 10},
      {GVM_OP_CLAMP, 3, 4, 5},
      {GVM_OP_STORE_GLOBAL, 3, 0, 1},
      {GVM_OP_LOAD_CONST, 6, 0, 0},
      {GVM_OP_LOAD_CONST, 7, 0, 1},
      {GVM_OP_LOAD_CONST, 8, 0, 2},
      {GVM_OP_CLAMP, 6, 7, 8},
      {GVM_OP_STORE_GLOBAL, 6, 0, 2},
      {GVM_OP_LOAD_CONST, 9, 0, 3},
      {GVM_OP_LOAD_CONST, 10, 0, 4},
      {GVM_OP_LOAD_CONST, 11, 0, 5},
      {GVM_OP_CLAMP, 9, 10, 11},
      {GVM_OP_STORE_GLOBAL, 9, 0, 3},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_float(&const_pool[0], 12.5);
  test_set_value_float(&const_pool[1], 0.0);
  test_set_value_float(&const_pool[2], 10.0);
  test_set_value_float(&const_pool[3], 4.5);
  test_set_value_float(&const_pool[4], 0.0);
  test_set_value_float(&const_pool[5], 10.0);
  for (i = 0U; i < 4U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 6U);
  graphion_vm_bind_globals(&vm, globals, 4U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_INT || globals[1].as.int_value != 5) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value != 10.0) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_FLOAT || globals[3].as.float_value != 4.5) {
    return 6;
  }
  return 0;
}

int test_vm_len_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[1];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LEN, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_string(&const_pool[0], "graphion");
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_INT || globals[0].as.int_value != 8) {
    return 3;
  }
  return 0;
}

int test_vm_factorial_opcode(void) {
  graphion_vm vm;
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 0},
      {GVM_OP_FACTORIAL, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_MOV_IMM, 1, 0, 5},
      {GVM_OP_FACTORIAL, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_MOV_IMM, 2, 0, 3},
      {GVM_OP_FACTORIAL, 2, 0, 0},
      {GVM_OP_FACTORIAL, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  for (i = 0U; i < 3U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_globals(&vm, globals, 3U);
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
  if (globals[1].kind != GVM_VALUE_INT || globals[1].as.int_value != 120) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_INT || globals[2].as.int_value != 720) {
    return 5;
  }
  return 0;
}
