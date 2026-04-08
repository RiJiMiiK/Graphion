/* SPDX-License-Identifier: MIT */

#include "test_vm_helpers.h"

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
  if (globals[0].kind != GVM_VALUE_FLOAT || globals[0].as.float_value != 3.0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_FLOAT || globals[1].as.float_value != -2.0) {
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

int test_vm_eq_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[12];
  graphion_vm_value globals[12];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 1, 0, 6},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 1, 0, 7},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 0, 0, 8},
      {GVM_OP_LOAD_CONST, 1, 0, 9},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 5},
      {GVM_OP_LOAD_CONST, 0, 0, 10},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 0, 0, 11},
      {GVM_OP_LOAD_CONST, 1, 0, 10},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 7},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_int(&const_pool[0], 42);
  test_set_value_int(&const_pool[1], 42);
  test_set_value_int(&const_pool[2], 42);
  test_set_value_float(&const_pool[3], 42.0);
  test_set_value_bool(&const_pool[4], 1);
  test_set_value_bool(&const_pool[5], 0);
  test_set_value_string(&const_pool[6], "ok");
  test_set_value_string(&const_pool[7], "no");
  test_set_value_int(&const_pool[8], 1);
  test_set_value_bool(&const_pool[9], 1);
  test_set_value_int(&const_pool[10], 0);
  test_set_value_bool(&const_pool[11], 0);
  for (i = 0U; i < 12U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 12U);
  graphion_vm_bind_globals(&vm, globals, 12U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    graphion_vm_dispose(&vm);
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    graphion_vm_dispose(&vm);
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_BOOL || globals[1].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_BOOL || globals[2].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_BOOL || globals[3].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_BOOL || globals[4].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 7;
  }
  if (globals[5].kind != GVM_VALUE_BOOL || globals[5].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 8;
  }
  if (globals[6].kind != GVM_VALUE_BOOL || globals[6].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 9;
  }
  if (globals[7].kind != GVM_VALUE_BOOL || globals[7].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_eq_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 1);
  test_set_value_string(&const_pool[1], "1");
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_eq_int_bool_out_of_range_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_EQ, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 2);
  test_set_value_bool(&const_pool[1], 1);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_ne_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[12];
  graphion_vm_value globals[12];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 1, 0, 6},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 1, 0, 7},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 0, 0, 8},
      {GVM_OP_LOAD_CONST, 1, 0, 9},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 5},
      {GVM_OP_LOAD_CONST, 0, 0, 10},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 0, 0, 11},
      {GVM_OP_LOAD_CONST, 1, 0, 10},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 7},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_int(&const_pool[0], 42);
  test_set_value_int(&const_pool[1], 42);
  test_set_value_int(&const_pool[2], 42);
  test_set_value_float(&const_pool[3], 42.0);
  test_set_value_bool(&const_pool[4], 1);
  test_set_value_bool(&const_pool[5], 0);
  test_set_value_string(&const_pool[6], "ok");
  test_set_value_string(&const_pool[7], "no");
  test_set_value_int(&const_pool[8], 1);
  test_set_value_bool(&const_pool[9], 1);
  test_set_value_int(&const_pool[10], 0);
  test_set_value_bool(&const_pool[11], 0);
  for (i = 0U; i < 12U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 12U);
  graphion_vm_bind_globals(&vm, globals, 12U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    graphion_vm_dispose(&vm);
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    graphion_vm_dispose(&vm);
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_BOOL || globals[1].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_BOOL || globals[2].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_BOOL || globals[3].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_BOOL || globals[4].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 7;
  }
  if (globals[5].kind != GVM_VALUE_BOOL || globals[5].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 8;
  }
  if (globals[6].kind != GVM_VALUE_BOOL || globals[6].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 9;
  }
  if (globals[7].kind != GVM_VALUE_BOOL || globals[7].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_ne_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 1);
  test_set_value_string(&const_pool[1], "1");
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_ne_int_bool_out_of_range_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NE, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 2);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_lt_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[6];
  graphion_vm_value globals[6];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_LT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_LT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_LT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 5},
      {GVM_OP_LOAD_CONST, 1, 0, 4},
      {GVM_OP_LT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_LT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_int(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 2);
  test_set_value_float(&const_pool[2], 1.5);
  test_set_value_float(&const_pool[3], 1.5);
  test_set_value_int(&const_pool[4], 2);
  test_set_value_float(&const_pool[5], 3.0);
  for (i = 0U; i < 6U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 6U);
  graphion_vm_bind_globals(&vm, globals, 6U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 1) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_BOOL || globals[1].as.bool_value != 0) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_BOOL || globals[2].as.bool_value != 1) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_BOOL || globals[3].as.bool_value != 0) {
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_BOOL || globals[4].as.bool_value != 0) {
    return 7;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_lt_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_LT, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 1);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_le_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[6];
  graphion_vm_value globals[6];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_LE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_LE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_LE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 5},
      {GVM_OP_LOAD_CONST, 1, 0, 4},
      {GVM_OP_LE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_LE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_int(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 2);
  test_set_value_float(&const_pool[2], 1.5);
  test_set_value_float(&const_pool[3], 1.5);
  test_set_value_int(&const_pool[4], 2);
  test_set_value_float(&const_pool[5], 3.0);
  for (i = 0U; i < 6U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 6U);
  graphion_vm_bind_globals(&vm, globals, 6U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 1) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_BOOL || globals[1].as.bool_value != 1) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_BOOL || globals[2].as.bool_value != 1) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_BOOL || globals[3].as.bool_value != 0) {
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_BOOL || globals[4].as.bool_value != 0) {
    return 7;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_le_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_LE, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 1);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_gt_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[6];
  graphion_vm_value globals[6];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_GT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_GT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_GT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 5},
      {GVM_OP_LOAD_CONST, 1, 0, 4},
      {GVM_OP_GT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_GT, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_int(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 2);
  test_set_value_float(&const_pool[2], 1.5);
  test_set_value_float(&const_pool[3], 1.5);
  test_set_value_int(&const_pool[4], 2);
  test_set_value_float(&const_pool[5], 3.0);
  for (i = 0U; i < 6U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 6U);
  graphion_vm_bind_globals(&vm, globals, 6U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_BOOL || globals[1].as.bool_value != 0) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_BOOL || globals[2].as.bool_value != 0) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_BOOL || globals[3].as.bool_value != 1) {
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_BOOL || globals[4].as.bool_value != 1) {
    return 7;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_gt_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_GT, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 1);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_ge_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[6];
  graphion_vm_value globals[6];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_GE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_GE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_GE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 5},
      {GVM_OP_LOAD_CONST, 1, 0, 4},
      {GVM_OP_GE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 1, 0, 0},
      {GVM_OP_GE, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_int(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 2);
  test_set_value_float(&const_pool[2], 1.5);
  test_set_value_float(&const_pool[3], 1.5);
  test_set_value_int(&const_pool[4], 2);
  test_set_value_float(&const_pool[5], 3.0);
  for (i = 0U; i < 6U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 6U);
  graphion_vm_bind_globals(&vm, globals, 6U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 0) {
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_BOOL || globals[1].as.bool_value != 1) {
    return 4;
  }
  if (globals[2].kind != GVM_VALUE_BOOL || globals[2].as.bool_value != 0) {
    return 5;
  }
  if (globals[3].kind != GVM_VALUE_BOOL || globals[3].as.bool_value != 1) {
    return 6;
  }
  if (globals[4].kind != GVM_VALUE_BOOL || globals[4].as.bool_value != 1) {
    return 7;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_ge_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_GE, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_int(&const_pool[1], 1);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 10;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 11;
  }
  graphion_vm_dispose(&vm);
  return 0;
}
