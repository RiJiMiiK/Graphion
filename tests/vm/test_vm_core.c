/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_vm_helpers.h"

int test_vm_addition_program(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 7},
      {GVM_OP_MOV_IMM, 1, 0, 35},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 2;
  }
  if (!vm.halted) {
    return 3;
  }
  if (TEST_REG_I(vm, 0) != 42) {
    return 4;
  }
  return 0;
}

int test_vm_invalid_register_fails(void) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 17, 0, 7},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return 1;
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_MOV_IMM_REG) {
    return 2;
  }
  return 0;
}

int test_vm_typed_register_defaults(void) {
  graphion_vm vm;
  size_t i;

  graphion_vm_init(&vm);
  for (i = 0U; i < 16U; ++i) {
    if (vm.regs[i].kind != GVM_VALUE_INT || vm.regs[i].as.int_value != 0) {
      return 1;
    }
  }
  return 0;
}

int test_vm_value_movement_and_globals(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_STORE_CONST_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_GLOBAL, 1, 0, 0},
      {GVM_OP_MOV, 2, 1, 0},
      {GVM_OP_STORE_CONST_GLOBAL, 0, 1, 1},
      {GVM_OP_COPY_GLOBAL, 0, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 3.5);
  test_set_value_string(&const_pool[1], "graphion");
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return finish_vm_test(&vm, 2);
  }
  if (!vm.halted) {
    return finish_vm_test(&vm, 3);
  }
  if (globals[0].kind != GVM_VALUE_STRING || strcmp(globals[0].as.string_value, "graphion") != 0) {
    return finish_vm_test(&vm, 4);
  }
  if (vm.regs[1].kind != GVM_VALUE_FLOAT || vm.regs[1].as.float_value != 3.5) {
    return finish_vm_test(&vm, 5);
  }
  if (vm.regs[2].kind != GVM_VALUE_FLOAT || vm.regs[2].as.float_value != 3.5) {
    return finish_vm_test(&vm, 6);
  }
  if (globals[1].kind != GVM_VALUE_STRING || strcmp(globals[1].as.string_value, "graphion") != 0) {
    return finish_vm_test(&vm, 7);
  }
  return finish_vm_test(&vm, 0);
}

int test_vm_typed_value_errors(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[1];
  const graphion_insn bad_const_program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 1},
  };
  const graphion_insn bad_global_program[] = {
      {GVM_OP_STORE_GLOBAL, 0, 0, 2},
  };
  const graphion_insn bad_store_const_program[] = {
      {GVM_OP_STORE_CONST_GLOBAL, 0, 2, 0},
  };
  int rc;

  test_set_value_float(&const_pool[0], 1.25);
  test_set_value_string(&const_pool[1], "bad");
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  rc = graphion_vm_load(&vm, bad_const_program, sizeof(bad_const_program) / sizeof(bad_const_program[0]));
  if (rc != 0) {
    return finish_vm_test(&vm, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_CONST_INDEX) {
    return finish_vm_test(&vm, 2);
  }

  graphion_vm_dispose(&vm);
  graphion_vm_init(&vm);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(&vm, bad_global_program, sizeof(bad_global_program) / sizeof(bad_global_program[0]));
  if (rc != 0) {
    return finish_vm_test(&vm, 3);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_GLOBAL_INDEX) {
    return finish_vm_test(&vm, 4);
  }

  graphion_vm_dispose(&vm);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 1U);
  graphion_vm_bind_globals(&vm, globals, 1U);
  rc = graphion_vm_load(
      &vm, bad_store_const_program, sizeof(bad_store_const_program) / sizeof(bad_store_const_program[0]));
  if (rc != 0) {
    return finish_vm_test(&vm, 5);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_INVALID_GLOBAL_INDEX) {
    return finish_vm_test(&vm, 6);
  }

  graphion_vm_dispose(&vm);
  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, (const graphion_insn[]){{GVM_OP_LOAD_CONST, 1, 0, 1}, {GVM_OP_ADD, 0, 1, 0}}, 2U);
  test_set_reg_i(&vm, 0U, 7);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  if (rc != 0) {
    return finish_vm_test(&vm, 7);
  }
  rc = graphion_vm_run(&vm);
  if (rc != GVM_ERR_TYPE_MISMATCH) {
    return finish_vm_test(&vm, 8);
  }

  return finish_vm_test(&vm, 0);
}

int test_vm_jump_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[4];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_JUMP_IF_FALSE, 0, 0, 4},
      {GVM_OP_MOV_IMM, 1, 0, 99},
      {GVM_OP_JUMP, 0, 0, 5},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_STORE_GLOBAL, 1, 0, 0},
      {GVM_OP_LOAD_CONST, 0, 0, 2},
      {GVM_OP_JUMP_IF_TRUE, 0, 0, 10},
      {GVM_OP_MOV_IMM, 2, 0, 77},
      {GVM_OP_JUMP, 0, 0, 11},
      {GVM_OP_LOAD_CONST, 2, 0, 3},
      {GVM_OP_STORE_GLOBAL, 2, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;
  size_t i;

  test_set_value_bool(&const_pool[0], 0);
  test_set_value_bool(&const_pool[1], 1);
  test_set_value_int(&const_pool[2], 1);
  test_set_value_bool(&const_pool[3], 1);
  for (i = 0U; i < 4U; ++i) {
    globals[i].kind = GVM_VALUE_NONE;
    globals[i].as.int_value = 0;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  graphion_vm_bind_globals(&vm, globals, 4U);
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
  graphion_vm_dispose(&vm);
  return 0;
}

int test_vm_string_addition_opcode(void) {
  graphion_vm vm;
  graphion_vm_value const_pool[3];
  graphion_vm_value globals[2];
  char *global_string_owners[2] = {NULL, NULL};
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_GLOBAL, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 2},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_STORE_GLOBAL, 0, 0, 1},
      {GVM_OP_HALT, 0, 0, 0},
  };
  int rc;

  test_set_value_string(&const_pool[0], "debut");
  test_set_value_string(&const_pool[1], "fin");
  test_set_value_string(&const_pool[2], "!");
  globals[0].kind = GVM_VALUE_NONE;
  globals[1].kind = GVM_VALUE_NONE;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 3U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  graphion_vm_bind_global_string_owners(&vm, global_string_owners, 2U);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    return finish_vm_test_with_owned_globals(&vm, global_string_owners, 2U, 1);
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return finish_vm_test_with_owned_globals(&vm, global_string_owners, 2U, 2);
  }
  if (globals[0].kind != GVM_VALUE_STRING || strcmp(globals[0].as.string_value, "debutfin") != 0) {
    return finish_vm_test_with_owned_globals(&vm, global_string_owners, 2U, 3);
  }
  if (globals[1].kind != GVM_VALUE_STRING || strcmp(globals[1].as.string_value, "debutfin!") != 0) {
    return finish_vm_test_with_owned_globals(&vm, global_string_owners, 2U, 4);
  }
  return finish_vm_test_with_owned_globals(&vm, global_string_owners, 2U, 0);
}

int test_vm_print_scalar_opcodes(void) {
  char path[512];
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_STORE_CONST_GLOBAL, 0, 0, 0},
      {GVM_OP_PRINT_CONST, 0, 0, 1},
      {GVM_OP_PRINT_GLOBAL, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  char output[64];
  FILE *fp = NULL;
  size_t read_len;
  int rc;

  test_set_value_float(&const_pool[0], 3.5);
  test_set_value_string(&const_pool[1], "graphion");
  globals[0].kind = GVM_VALUE_NONE;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  fp = test_open_temp_output_vm(path, sizeof(path), "vm_print_scalar_output.txt");
  if (fp == NULL) {
    return 1;
  }
  graphion_vm_bind_output(&vm, fp);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    fclose(fp);
    remove(path);
    return 2;
  }
  rc = graphion_vm_run(&vm);
  fclose(fp);
  if (rc != 0) {
    remove(path);
    return 3;
  }
  fp = NULL;
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    remove(path);
    return 4;
  }
  read_len = fread(output, 1U, sizeof(output) - 1U, fp);
  fclose(fp);
  remove(path);
  output[read_len] = '\0';
  if (strcmp(output, "graphion\n3.5\n") != 0) {
    return 5;
  }
  return 0;
}

int test_vm_print_reg_opcode(void) {
  char path[512];
  graphion_vm vm;
  graphion_vm_value const_pool[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},
      {GVM_OP_LOAD_CONST, 1, 0, 1},
      {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_PRINT_REG, 0, 0, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  char output[32];
  FILE *fp = NULL;
  size_t read_len;
  int rc;

  const_pool[0].kind = GVM_VALUE_INT;
  const_pool[0].as.int_value = 1;
  const_pool[1].kind = GVM_VALUE_INT;
  const_pool[1].as.int_value = 2;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 2U);
  fp = test_open_temp_output_vm(path, sizeof(path), "vm_print_reg_output.txt");
  if (fp == NULL) {
    return 1;
  }
  graphion_vm_bind_output(&vm, fp);
  rc = graphion_vm_load(&vm, program, sizeof(program) / sizeof(program[0]));
  if (rc != 0) {
    fclose(fp);
    remove(path);
    return 2;
  }
  rc = graphion_vm_run(&vm);
  fclose(fp);
  if (rc != 0) {
    remove(path);
    return 3;
  }
  fp = NULL;
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    remove(path);
    return 4;
  }
  read_len = fread(output, 1U, sizeof(output) - 1U, fp);
  fclose(fp);
  remove(path);
  output[read_len] = '\0';
  if (strcmp(output, "3\n") != 0) {
    return 5;
  }
  return 0;
}
