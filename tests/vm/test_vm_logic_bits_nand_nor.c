/* SPDX-License-Identifier: MIT */

#include "test_vm_helpers.h"

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
