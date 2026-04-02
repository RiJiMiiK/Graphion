/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_io.h"

#include "vm/internal/core/value.h"

int op_print_const(graphion_vm *vm, const graphion_insn *in) {
  if (vm->const_pool == NULL) {
    return GVM_ERR_CONST_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->const_count) {
    return GVM_ERR_INVALID_CONST_INDEX;
  }
  return vm_write_value_sink(&vm->output, &vm->const_pool[(size_t)in->imm]);
}

int op_print_global(graphion_vm *vm, const graphion_insn *in) {
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  return vm_write_value_sink(&vm->output, &vm->globals[(size_t)in->imm]);
}

int op_print_reg(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  return vm_write_value_sink(&vm->output, &vm->regs[in->a]);
}

int op_print_const_part(graphion_vm *vm, const graphion_insn *in) {
  if (vm->const_pool == NULL) {
    return GVM_ERR_CONST_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->const_count) {
    return GVM_ERR_INVALID_CONST_INDEX;
  }
  return vm_write_value_sink_inline(&vm->output, &vm->const_pool[(size_t)in->imm]);
}

int op_print_global_part(graphion_vm *vm, const graphion_insn *in) {
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  return vm_write_value_sink_inline(&vm->output, &vm->globals[(size_t)in->imm]);
}

int op_print_reg_part(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  return vm_write_value_sink_inline(&vm->output, &vm->regs[in->a]);
}

int op_print_newline(graphion_vm *vm, const graphion_insn *in) {
  (void)in;
  return vm_write_bytes_sink(&vm->output, "\n", 1U);
}

