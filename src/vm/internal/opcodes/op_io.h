/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_OP_IO_H
#define GRAPHION_VM_OP_IO_H

#include "vm/vm.h"

int op_print_const(graphion_vm *vm, const graphion_insn *in);
int op_print_global(graphion_vm *vm, const graphion_insn *in);
int op_print_reg(graphion_vm *vm, const graphion_insn *in);
int op_print_const_part(graphion_vm *vm, const graphion_insn *in);
int op_print_global_part(graphion_vm *vm, const graphion_insn *in);
int op_print_reg_part(graphion_vm *vm, const graphion_insn *in);
int op_print_newline(graphion_vm *vm, const graphion_insn *in);

#endif

