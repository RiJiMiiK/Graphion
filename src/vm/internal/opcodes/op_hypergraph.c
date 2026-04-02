/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_hypergraph.h"

#include "vm/internal/core/value.h"

int op_incident_of(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  const uint32_t *hyperedges;
  size_t count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (vm->frontier_output == NULL || vm->frontier_capacity == 0U) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->hypergraph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  hyperedges = graphion_hypergraph_incident(vm->hypergraph, node);
  count = graphion_hypergraph_incident_count(vm->hypergraph, node);
  if (count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = count;
  for (i = 0U; i < count; ++i) {
    vm->frontier_output[i] = hyperedges[i];
  }
  return GVM_OK;
}

int op_hyperedge_nodes_of(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  const uint32_t *nodes;
  size_t count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (vm->frontier_output == NULL || vm->frontier_capacity == 0U) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  hyperedge = (uint32_t)reg_value;
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  nodes = graphion_hypergraph_hyperedge_nodes(vm->hypergraph, hyperedge);
  count = graphion_hypergraph_hyperedge_size(vm->hypergraph, hyperedge);
  if (count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = count;
  for (i = 0U; i < count; ++i) {
    vm->frontier_output[i] = nodes[i];
  }
  return GVM_OK;
}

int op_incident_count(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->hypergraph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  vm_reg_set_int(vm, in->b, (int64_t)(vm->hypergraph->node_offsets[node + 1U] - vm->hypergraph->node_offsets[node]));
  return 0;
}

int op_hyperedge_size(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  hyperedge = (uint32_t)reg_value;
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  vm_reg_set_int(vm,
                 in->b,
                 (int64_t)(vm->hypergraph->hyperedge_offsets[hyperedge + 1U] - vm->hypergraph->hyperedge_offsets[hyperedge]));
  return 0;
}

int op_incident_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->hypergraph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  vm_reg_set_int(vm, in->b, (int64_t)graphion_hypergraph_incident_sum(vm->hypergraph, node));
  return 0;
}

int op_hyperedge_node_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  hyperedge = (uint32_t)reg_value;
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  vm_reg_set_int(vm, in->b, (int64_t)graphion_hypergraph_hyperedge_node_sum(vm->hypergraph, hyperedge));
  return 0;
}

