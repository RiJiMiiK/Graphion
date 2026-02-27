/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

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
  if (vm.regs[0] != 42) {
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
  if (rc != -2) {
    return 2;
  }
  return 0;
}
