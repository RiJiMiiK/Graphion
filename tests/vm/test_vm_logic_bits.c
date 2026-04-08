/* SPDX-License-Identifier: MIT */

#include "test_vm_helpers.h"

int test_vm_and_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[10];
  graphion_vm_value globals[10];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_AND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_AND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_AND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 1, 0, 7},
      {GVM_OP_AND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 8},
      {GVM_OP_LOAD_CONST, 1, 0, 9},
      {GVM_OP_AND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_bool(&const_pool[1], 1);
  test_set_value_bool(&const_pool[2], 1);
  test_set_value_bool(&const_pool[3], 0);
  test_set_value_int(&const_pool[4], 1);
  test_set_value_bool(&const_pool[5], 1);
  test_set_value_bool(&const_pool[6], 0);
  test_set_value_int(&const_pool[7], 1);
  test_set_value_int(&const_pool[8], 1);
  test_set_value_int(&const_pool[9], 0);
  for (i = 0U; i < 10U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 10U);
  graphion_vm_bind_globals(&vm, globals, 10U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 1) {
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
  if (globals[4].kind != GVM_VALUE_BOOL || globals[4].as.bool_value != 0) {
    graphion_vm_dispose(&vm);
    return 7;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_and_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_AND, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 2);
  test_set_value_bool(&const_pool[1], 1);

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
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
  graphion_vm_init(&vm);
  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_bool(&const_pool[1], 1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 12;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 13;
  }

  graphion_vm_dispose(&vm);
  graphion_vm_init(&vm);
  test_set_value_string(&const_pool[0], "x");
  test_set_value_bool(&const_pool[1], 1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 14;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 15;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_bit_and_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_AND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bits(&const_pool[0], 12U, 4U);
  test_set_value_bits(&const_pool[1], 10U, 4U);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 2);
  }
  if (globals[0].kind != GVM_VALUE_BITS || globals[0].reserved[0] != 4U || (uint64_t)globals[0].as.int_value != 8U) {
    return finish_vm_test(&vm, 3);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_and_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_AND, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_bits(&const_pool[1], 2U, 4U);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 10);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 11);
  }
  graphion_vm_dispose(&vm);

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_int(&const_pool[1], 1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 12);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 13);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_or_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_OR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bits(&const_pool[0], 12U, 4U);
  test_set_value_bits(&const_pool[1], 10U, 4U);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 2);
  }
  if (globals[0].kind != GVM_VALUE_BITS || globals[0].reserved[0] != 4U || (uint64_t)globals[0].as.int_value != 14U) {
    return finish_vm_test(&vm, 3);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_or_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_OR, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_bits(&const_pool[1], 2U, 4U);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 10);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 11);
  }
  graphion_vm_dispose(&vm);

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_int(&const_pool[1], 1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 12);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 13);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_xor_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_XOR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bits(&const_pool[0], 12U, 4U);
  test_set_value_bits(&const_pool[1], 10U, 4U);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 2);
  }
  if (globals[0].kind != GVM_VALUE_BITS || globals[0].reserved[0] != 4U || (uint64_t)globals[0].as.int_value != 6U) {
    return finish_vm_test(&vm, 3);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_xor_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_XOR, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_bits(&const_pool[1], 2U, 4U);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 10);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 11);
  }
  graphion_vm_dispose(&vm);

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_int(&const_pool[1], 1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 12);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 13);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_not_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_BIT_NOT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 1},
      {GVM_OP_BIT_NOT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bits(&const_pool[0], 2U, 4U);
  test_set_value_bits(&const_pool[1], 2U, 2U);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 2);
  }
  if (globals[0].kind != GVM_VALUE_BITS || globals[0].reserved[0] != 4U || (uint64_t)globals[0].as.int_value != 13U) {
    return finish_vm_test(&vm, 3);
  }
  if (globals[1].kind != GVM_VALUE_BITS || globals[1].reserved[0] != 2U || (uint64_t)globals[1].as.int_value != 1U) {
    return finish_vm_test(&vm, 4);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_not_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_BIT_NOT, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  test_set_value_int(&const_pool[0], 1);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 10);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 11);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_shl_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_SHL, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_BIT_SHL, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bits(&const_pool[0], 3U, 4U);
  test_set_value_int(&const_pool[1], 1);
  test_set_value_bits(&const_pool[2], 15U, 4U);
  test_set_value_int(&const_pool[3], 1);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 2);
  }
  if (globals[0].kind != GVM_VALUE_BITS || globals[0].reserved[0] != 4U || (uint64_t)globals[0].as.int_value != 6U) {
    return finish_vm_test(&vm, 3);
  }
  if (globals[1].kind != GVM_VALUE_BITS || globals[1].reserved[0] != 4U || (uint64_t)globals[1].as.int_value != 14U) {
    return finish_vm_test(&vm, 4);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_shl_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_SHL, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_bits(&const_pool[1], 2U, 4U);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 10);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 11);
  }
  graphion_vm_dispose(&vm);

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_float(&const_pool[1], 1.0);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 12);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 13);
  }
  graphion_vm_dispose(&vm);

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_int(&const_pool[1], -1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 14);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 15);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_shr_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_SHR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_BIT_SHR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_bits(&const_pool[0], 10U, 4U);
  test_set_value_int(&const_pool[1], 1);
  test_set_value_bits(&const_pool[2], 10U, 4U);
  test_set_value_int(&const_pool[3], 4);
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 2);
  }
  if (globals[0].kind != GVM_VALUE_BITS || globals[0].reserved[0] != 4U || (uint64_t)globals[0].as.int_value != 5U) {
    return finish_vm_test(&vm, 3);
  }
  if (globals[1].kind != GVM_VALUE_BITS || globals[1].reserved[0] != 4U || (uint64_t)globals[1].as.int_value != 0U) {
    return finish_vm_test(&vm, 4);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_bit_shr_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_BIT_SHR, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_bits(&const_pool[1], 2U, 4U);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 10);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 11);
  }
  graphion_vm_dispose(&vm);

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_float(&const_pool[1], 1.0);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 12);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 13);
  }
  graphion_vm_dispose(&vm);

  graphion_vm_init(&vm);
  test_set_value_bits(&const_pool[0], 2U, 2U);
  test_set_value_int(&const_pool[1], -1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    return finish_vm_test(&vm, 14);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 15);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_or_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[10];
  graphion_vm_value globals[10];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_OR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_OR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 5},
      {GVM_OP_OR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 6},
      {GVM_OP_LOAD_CONST, 1, 0, 7},
      {GVM_OP_OR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 8},
      {GVM_OP_LOAD_CONST, 1, 0, 9},
      {GVM_OP_OR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_bool(&const_pool[1], 1);
  test_set_value_bool(&const_pool[2], 1);
  test_set_value_bool(&const_pool[3], 0);
  test_set_value_int(&const_pool[4], 1);
  test_set_value_bool(&const_pool[5], 0);
  test_set_value_bool(&const_pool[6], 0);
  test_set_value_int(&const_pool[7], 1);
  test_set_value_int(&const_pool[8], 0);
  test_set_value_int(&const_pool[9], 0);
  for (i = 0U; i < 10U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 10U);
  graphion_vm_bind_globals(&vm, globals, 10U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
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
  if (globals[2].kind != GVM_VALUE_BOOL || globals[2].as.bool_value != 1) {
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
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_or_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_OR, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 2);
  test_set_value_bool(&const_pool[1], 1);

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
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
  graphion_vm_init(&vm);
  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_bool(&const_pool[1], 1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 12;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 13;
  }

  graphion_vm_dispose(&vm);
  graphion_vm_init(&vm);
  test_set_value_string(&const_pool[0], "x");
  test_set_value_bool(&const_pool[1], 1);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 14;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 15;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_not_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[4];
  char *global_string_owners[4] = {NULL, NULL, NULL, NULL};
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_NOT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 1},
      {GVM_OP_NOT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_NOT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 3},
      {GVM_OP_NOT, 0, 0, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_bool(&const_pool[1], 0);
  test_set_value_int(&const_pool[2], 1);
  test_set_value_int(&const_pool[3], 0);
  for (i = 0U; i < 4U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  graphion_vm_bind_globals(&vm, globals, 4U);
  vm.global_string_owners = global_string_owners;
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 0) {
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
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_not_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[1];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_NOT, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 2);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
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

  test_set_value_float(&const_pool[0], 1.0);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 12;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 13;
  }
  graphion_vm_dispose(&vm);

  test_set_value_string(&const_pool[0], "x");
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 14;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 15;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_nand_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[5];
  graphion_vm_value globals[5];
  char *global_string_owners[5] = {NULL, NULL, NULL, NULL, NULL};
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NAND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_NAND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NAND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_NAND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 2},
      {GVM_OP_NAND, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_bool(&const_pool[0], 1);
  test_set_value_bool(&const_pool[1], 1);
  test_set_value_bool(&const_pool[2], 1);
  test_set_value_bool(&const_pool[3], 0);
  test_set_value_int(&const_pool[4], 1);
  for (i = 0U; i < 5U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 5U);
  graphion_vm_bind_globals(&vm, globals, 5U);
  vm.global_string_owners = global_string_owners;
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 0) {
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
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_nand_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NAND, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 2);
  test_set_value_bool(&const_pool[1], 1);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
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

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_bool(&const_pool[1], 1);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 12;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 13;
  }
  graphion_vm_dispose(&vm);

  test_set_value_string(&const_pool[0], "x");
  test_set_value_bool(&const_pool[1], 1);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 14;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 15;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_nor_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[5];
  graphion_vm_value globals[5];
  char *global_string_owners[5] = {NULL, NULL, NULL, NULL, NULL};
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NOR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_NOR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_LOAD_CONST, 0, 0, 4},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NOR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 1, 0, 3},
      {GVM_OP_NOR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 3},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_LOAD_CONST, 1, 0, 2},
      {GVM_OP_NOR, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_bool(&const_pool[0], 0);
  test_set_value_bool(&const_pool[1], 0);
  test_set_value_bool(&const_pool[2], 1);
  test_set_value_bool(&const_pool[3], 0);
  test_set_value_int(&const_pool[4], 1);
  for (i = 0U; i < 5U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 5U);
  graphion_vm_bind_globals(&vm, globals, 5U);
  vm.global_string_owners = global_string_owners;
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 2;
  }
  if (globals[0].kind != GVM_VALUE_BOOL || globals[0].as.bool_value != 1) {
    graphion_vm_dispose(&vm);
    return 3;
  }
  if (globals[1].kind != GVM_VALUE_BOOL || globals[1].as.bool_value != 0) {
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
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_nor_incompatible_types_fail(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_NOR, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_int(&const_pool[0], 2);
  test_set_value_bool(&const_pool[1], 0);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
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

  test_set_value_float(&const_pool[0], 1.0);
  test_set_value_bool(&const_pool[1], 0);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 12;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 13;
  }
  graphion_vm_dispose(&vm);

  test_set_value_string(&const_pool[0], "x");
  test_set_value_bool(&const_pool[1], 0);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != GVM_OK) {
    graphion_vm_dispose(&vm);
    return 14;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    graphion_vm_dispose(&vm);
    return 15;
  }
  graphion_vm_dispose(&vm);
  return 0;
}

