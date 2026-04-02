/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_OP_FRONTIER_H
#define GRAPHION_VM_OP_FRONTIER_H

#include "vm/vm.h"

int op_frontier_clear(graphion_vm *vm, const graphion_insn *in);
int op_frontier_push(graphion_vm *vm, const graphion_insn *in);
int op_frontier_filter_lt_imm(graphion_vm *vm, const graphion_insn *in);
int op_frontier_map_add_imm(graphion_vm *vm, const graphion_insn *in);
int op_frontier_reduce_sum(graphion_vm *vm, const graphion_insn *in);
int op_frontier_swap(graphion_vm *vm, const graphion_insn *in);

#endif

