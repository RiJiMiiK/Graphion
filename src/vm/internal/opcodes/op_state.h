/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_OP_STATE_H
#define GRAPHION_VM_OP_STATE_H

#include "vm/vm.h"

int op_nop(graphion_vm *vm, const graphion_insn *in);
int op_halt(graphion_vm *vm, const graphion_insn *in);
int op_mov_imm(graphion_vm *vm, const graphion_insn *in);
int op_jump(graphion_vm *vm, const graphion_insn *in);
int op_jump_if_true(graphion_vm *vm, const graphion_insn *in);
int op_jump_if_false(graphion_vm *vm, const graphion_insn *in);
int op_mov(graphion_vm *vm, const graphion_insn *in);
int op_load_const(graphion_vm *vm, const graphion_insn *in);
int op_load_global(graphion_vm *vm, const graphion_insn *in);
int op_store_global(graphion_vm *vm, const graphion_insn *in);
int op_store_const_global(graphion_vm *vm, const graphion_insn *in);
int op_copy_global(graphion_vm *vm, const graphion_insn *in);
int op_list_new(graphion_vm *vm, const graphion_insn *in);
int op_list_append(graphion_vm *vm, const graphion_insn *in);
int op_list_get(graphion_vm *vm, const graphion_insn *in);
int op_dict_new(graphion_vm *vm, const graphion_insn *in);
int op_dict_set(graphion_vm *vm, const graphion_insn *in);
int op_dict_get(graphion_vm *vm, const graphion_insn *in);
int op_dict_set_key(graphion_vm *vm, const graphion_insn *in);

#endif
