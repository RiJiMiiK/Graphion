#include "vm/vm.h"

#include <stddef.h>

static int is_valid_reg(uint8_t reg) { return reg < 16U ? 1 : 0; }

void graphion_vm_init(graphion_vm *vm) {
  size_t i;
  if (vm == NULL) {
    return;
  }
  for (i = 0; i < 16U; ++i) {
    vm->regs[i] = 0;
  }
  vm->program = NULL;
  vm->program_len = 0U;
  vm->pc = 0U;
  vm->halted = false;
}

int graphion_vm_run(graphion_vm *vm) {
  if (vm == NULL || vm->program == NULL) {
    return -1;
  }

  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc];
    vm->pc++;

    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        break;
      case GVM_OP_MOV_IMM:
        if (!is_valid_reg(in.a)) {
          return -2;
        }
        vm->regs[in.a] = (int64_t)in.imm;
        break;
      case GVM_OP_ADD:
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return -3;
        }
        vm->regs[in.a] += vm->regs[in.b];
        break;
      default:
        return -4;
    }
  }

  return 0;
}
