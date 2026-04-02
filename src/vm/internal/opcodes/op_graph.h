/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_OP_GRAPH_H
#define GRAPHION_VM_OP_GRAPH_H

#include "vm/vm.h"

int op_neighbors_of(graphion_vm *vm, const graphion_insn *in);
int op_neighbors_expand(graphion_vm *vm, const graphion_insn *in);
int op_neighbor_weight_sum(graphion_vm *vm, const graphion_insn *in);
int op_neighbor_attr_sum(graphion_vm *vm, const graphion_insn *in);
int op_bfs_levels(graphion_vm *vm, const graphion_insn *in);
int op_bfs_level_count(graphion_vm *vm, const graphion_insn *in);
int op_bfs_order(graphion_vm *vm, const graphion_insn *in);

#endif

