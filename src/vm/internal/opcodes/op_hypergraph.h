/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_OP_HYPERGRAPH_H
#define GRAPHION_VM_OP_HYPERGRAPH_H

#include "vm/vm.h"

int op_incident_of(graphion_vm *vm, const graphion_insn *in);
int op_hyperedge_nodes_of(graphion_vm *vm, const graphion_insn *in);
int op_incident_count(graphion_vm *vm, const graphion_insn *in);
int op_hyperedge_size(graphion_vm *vm, const graphion_insn *in);
int op_incident_sum(graphion_vm *vm, const graphion_insn *in);
int op_hyperedge_node_sum(graphion_vm *vm, const graphion_insn *in);

#endif

