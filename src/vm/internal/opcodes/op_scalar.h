/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_OP_SCALAR_H
#define GRAPHION_VM_OP_SCALAR_H

#include "vm/vm.h"

int vm_value_get_boolean(const graphion_vm_value *value, int *out);

int op_add(graphion_vm *vm, const graphion_insn *in);
int op_sub(graphion_vm *vm, const graphion_insn *in);
int op_mul(graphion_vm *vm, const graphion_insn *in);
int op_div(graphion_vm *vm, const graphion_insn *in);
int op_mod(graphion_vm *vm, const graphion_insn *in);
int op_pow(graphion_vm *vm, const graphion_insn *in);
int op_floor_div(graphion_vm *vm, const graphion_insn *in);

int op_eq_cmp(graphion_vm *vm, const graphion_insn *in);
int op_ne_cmp(graphion_vm *vm, const graphion_insn *in);
int op_lt_cmp(graphion_vm *vm, const graphion_insn *in);
int op_le_cmp(graphion_vm *vm, const graphion_insn *in);
int op_gt_cmp(graphion_vm *vm, const graphion_insn *in);
int op_ge_cmp(graphion_vm *vm, const graphion_insn *in);

int op_and_cmp(graphion_vm *vm, const graphion_insn *in);
int op_or_cmp(graphion_vm *vm, const graphion_insn *in);
int op_not_cmp(graphion_vm *vm, const graphion_insn *in);
int op_nand_cmp(graphion_vm *vm, const graphion_insn *in);
int op_nor_cmp(graphion_vm *vm, const graphion_insn *in);

int op_bit_and_cmp(graphion_vm *vm, const graphion_insn *in);
int op_bit_or_cmp(graphion_vm *vm, const graphion_insn *in);
int op_bit_xor_cmp(graphion_vm *vm, const graphion_insn *in);
int op_bit_not_cmp(graphion_vm *vm, const graphion_insn *in);
int op_bit_shl_cmp(graphion_vm *vm, const graphion_insn *in);
int op_bit_shr_cmp(graphion_vm *vm, const graphion_insn *in);

int op_abs(graphion_vm *vm, const graphion_insn *in);
int op_min(graphion_vm *vm, const graphion_insn *in);
int op_max(graphion_vm *vm, const graphion_insn *in);
int op_clamp(graphion_vm *vm, const graphion_insn *in);
int op_sqrt(graphion_vm *vm, const graphion_insn *in);
int op_exp(graphion_vm *vm, const graphion_insn *in);
int op_ln(graphion_vm *vm, const graphion_insn *in);
int op_log(graphion_vm *vm, const graphion_insn *in);
int op_floor_builtin(graphion_vm *vm, const graphion_insn *in);
int op_len(graphion_vm *vm, const graphion_insn *in);
int op_factorial(graphion_vm *vm, const graphion_insn *in);

#endif
