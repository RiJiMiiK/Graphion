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

int test_vm_sqrt_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 9},
      {GVM_OP_SQRT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_SQRT, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 2.25);
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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 3.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != 1.5) {
    return 4;
  }
  return 0;
}

int test_vm_cbrt_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 27},
      {GVM_OP_CBRT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_CBRT, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], -8.0);
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
  if (globals[0].kind != GVM_VALUE_FLOAT || fabs(globals[0].as.float_value - 3.0) > 1e-12) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || fabs(globals[1].as.float_value + 2.0) > 1e-12) {
    return 4;
  }
  return 0;
}

int test_vm_sin_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_SIN, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_SIN, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 1.5707963267948966);
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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 0.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.999999999 ||
      globals[1].as.float_value > 1.000000001) {
    return 4;
  }
  return 0;
}

int test_vm_csc_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_CSC, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_CSC, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_CSC, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.5707963267948966);
  test_set_value_float(&const_pool[1], -1.5707963267948966);
  test_set_value_float(&const_pool[2], 0.5235987755982988);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 0.999999999 ||
      globals[0].as.float_value > 1.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value > -0.999999999 ||
      globals[1].as.float_value < -1.000000001) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 1.999999999 ||
      globals[2].as.float_value > 2.000000001) {
    return 5;
  }
  return 0;
}

int test_vm_sec_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_SEC, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_SEC, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_SEC, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 3.14159265358979323846);
  test_set_value_float(&const_pool[2], 1.0471975511965976);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 0.999999999 ||
      globals[0].as.float_value > 1.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value > -0.999999999 ||
      globals[1].as.float_value < -1.000000001) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 1.999999999 ||
      globals[2].as.float_value > 2.000000001) {
    return 5;
  }
  return 0;
}

int test_vm_cot_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_COT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_COT, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_COT, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.7853981633974483);
  test_set_value_float(&const_pool[1], -0.7853981633974483);
  test_set_value_float(&const_pool[2], 0.5235987755982988);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 0.999999999 ||
      globals[0].as.float_value > 1.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value > -0.999999999 ||
      globals[1].as.float_value < -1.000000001) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 1.732050806 ||
      globals[2].as.float_value > 1.732050809) {
    return 5;
  }
  return 0;
}

int test_vm_acsc_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ACSC, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ACSC, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ACSC, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_float(&const_pool[1], 2.0);
  test_set_value_float(&const_pool[2], -2.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 1.570796326 ||
      globals[0].as.float_value > 1.570796328) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.523598775 ||
      globals[1].as.float_value > 0.523598777) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < -0.523598777 ||
      globals[2].as.float_value > -0.523598775) {
    return 5;
  }
  return 0;
}

int test_vm_asec_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ASEC, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ASEC, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ASEC, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_float(&const_pool[1], 2.0);
  test_set_value_float(&const_pool[2], -2.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < -0.000000001 ||
      globals[0].as.float_value > 0.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 1.047197551 ||
      globals[1].as.float_value > 1.047197553) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 2.094395101 ||
      globals[2].as.float_value > 2.094395103) {
    return 5;
  }
  return 0;
}

int test_vm_acot_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ACOT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ACOT, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ACOT, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_float(&const_pool[1], 0.0);
  test_set_value_float(&const_pool[2], -1.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 0.785398163 ||
      globals[0].as.float_value > 0.785398164) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 1.570796326 ||
      globals[1].as.float_value > 1.570796328) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 2.356194489 ||
      globals[2].as.float_value > 2.356194491) {
    return 5;
  }
  return 0;
}

int test_vm_sech_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_SECH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_SECH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_SECH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 1.0);
  test_set_value_float(&const_pool[2], -1.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 0.999999999 ||
      globals[0].as.float_value > 1.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.648054273 ||
      globals[1].as.float_value > 0.648054274) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 0.648054273 ||
      globals[2].as.float_value > 0.648054274) {
    return 5;
  }
  return 0;
}

int test_vm_csch_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_CSCH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_CSCH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_CSCH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_float(&const_pool[1], -1.0);
  test_set_value_float(&const_pool[2], 2.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 0.850918127 ||
      globals[0].as.float_value > 0.850918129) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < -0.850918129 ||
      globals[1].as.float_value > -0.850918127) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 0.275720564 ||
      globals[2].as.float_value > 0.275720566) {
    return 5;
  }
  return 0;
}

int test_vm_coth_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_COTH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_COTH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_COTH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_float(&const_pool[1], -1.0);
  test_set_value_float(&const_pool[2], 2.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < 1.313035284 ||
      globals[0].as.float_value > 1.313035286) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < -1.313035286 ||
      globals[1].as.float_value > -1.313035284) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 1.037314719 ||
      globals[2].as.float_value > 1.037314721) {
    return 5;
  }
  return 0;
}

int test_vm_sinh_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_SINH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_SINH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 1.0);
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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 0.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 1.175201193 ||
      globals[1].as.float_value > 1.175201195) {
    return 4;
  }
  return 0;
}

int test_vm_asinh_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ASINH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ASINH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ASINH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 1.0);
  test_set_value_float(&const_pool[2], -1.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 0.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.881373586 ||
      globals[1].as.float_value > 0.881373588) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < -0.881373588 ||
      globals[2].as.float_value > -0.881373586) {
    return 5;
  }
  return 0;
}

int test_vm_acosh_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ACOSH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ACOSH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ACOSH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_float(&const_pool[1], 2.0);
  test_set_value_float(&const_pool[2], 4.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < -0.000000001 ||
      globals[0].as.float_value > 0.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 1.316957895 ||
      globals[1].as.float_value > 1.316957897) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 2.063437067 ||
      globals[2].as.float_value > 2.063437069) {
    return 5;
  }
  return 0;
}

int test_vm_cosh_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_COSH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_COSH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_COSH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 1.0);
  test_set_value_float(&const_pool[2], -1.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 1.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 1.543080634 ||
      globals[1].as.float_value > 1.543080636) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < 1.543080634 ||
      globals[2].as.float_value > 1.543080636) {
    return 5;
  }
  return 0;
}

int test_vm_tanh_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_TANH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_TANH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_TANH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 1.0);
  test_set_value_float(&const_pool[2], -1.0);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 0.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.761594155 ||
      globals[1].as.float_value > 0.761594157) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < -0.761594157 ||
      globals[2].as.float_value > -0.761594155) {
    return 5;
  }
  return 0;
}

int test_vm_atanh_builtin_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[3];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_ATANH, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ATANH, 1, 0, 0},
      {GVM_OP_STORE_GLOBAL, 1, 0, 1},
      {GVM_OP_LOAD_CONST, 2, 0, 2},
      {GVM_OP_ATANH, 2, 0, 0},
      {GVM_OP_STORE_GLOBAL, 2, 0, 2},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 0.0);
  test_set_value_float(&const_pool[1], 0.5);
  test_set_value_float(&const_pool[2], -0.5);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;
  globals[2].kind = GVM_VALUE_NONE;
  globals[2].as.int_value = 0;

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value < -0.000000001 ||
      globals[0].as.float_value > 0.000000001) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value < 0.549306143 ||
      globals[1].as.float_value > 0.549306145) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_FLOAT || globals[2].as.float_value < -0.549306145 ||
      globals[2].as.float_value > -0.549306143) {
    return 5;
  }
  return 0;
}

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
