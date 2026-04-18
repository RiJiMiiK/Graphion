/* SPDX-License-Identifier: MIT */

#include "test_vm_helpers.h"

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
