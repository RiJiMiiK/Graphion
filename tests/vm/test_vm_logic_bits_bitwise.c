/* SPDX-License-Identifier: MIT */

#include "test_vm_helpers.h"

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
  graphion_vm_value const_pool[2] = {0};
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
  if (rc != GVM_ERR_BITS_WIDTH_MISMATCH) {
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
  graphion_vm_value const_pool[2] = {0};
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
  if (rc != GVM_ERR_BITS_WIDTH_MISMATCH) {
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
  graphion_vm_value const_pool[2] = {0};
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
  if (rc != GVM_ERR_BITS_WIDTH_MISMATCH) {
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
  graphion_vm_value const_pool[2] = {0};
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
  if (rc != GVM_ERR_NEGATIVE_SHIFT) {
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
  graphion_vm_value const_pool[2] = {0};
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
  if (rc != GVM_ERR_NEGATIVE_SHIFT) {
    return finish_vm_test(&vm, 15);
  }
  return finish_vm_test(&vm, 0);
}
