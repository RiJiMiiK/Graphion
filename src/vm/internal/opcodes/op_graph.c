/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_graph.h"

#include "vm/internal/core/frontier.h"
#include "vm/internal/core/sum.h"
#include "vm/internal/core/value.h"

static int64_t count_visited_levels(const int32_t *levels, size_t count) {
  size_t i;
  int64_t total = 0;
  for (i = 0; i < count; ++i) {
    if (levels[i] >= 0) {
      total++;
    }
  }
  return total;
}

static int64_t count_bfs_level_count(const int32_t *levels, size_t count) {
  size_t i;
  int32_t max_level = -1;
  for (i = 0U; i < count; ++i) {
    if (levels[i] > max_level) {
      max_level = levels[i];
    }
  }
  return max_level < 0 ? 0 : (int64_t)max_level + 1;
}

int op_neighbors_of(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  const uint32_t *neighbors;
  size_t count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  neighbors = graphion_csr_graph_neighbors(vm->csr_graph, node);
  count = graphion_csr_graph_neighbor_count(vm->csr_graph, node);
  if (count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = count;
  for (i = 0U; i < count; ++i) {
    vm->frontier_output[i] = neighbors[i];
  }
  return GVM_OK;
}

int op_neighbors_expand(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  size_t out_len = 0U;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    size_t count = graphion_csr_graph_neighbor_count(vm->csr_graph, vm->frontier_input[i]);
    const uint32_t *neighbors = graphion_csr_graph_neighbors(vm->csr_graph, vm->frontier_input[i]);
    size_t j;
    if (out_len + count > vm->frontier_capacity) {
      vm->frontier_output_len = 0U;
      return GVM_ERR_FRONTIER_OVERFLOW;
    }
    for (j = 0U; j < count; ++j) {
      vm->frontier_output[out_len++] = neighbors[j];
    }
  }
  vm->frontier_output_len = out_len;
  vm_reg_set_int(vm, in->a, (int64_t)out_len);
  return GVM_OK;
}

int op_neighbor_weight_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  const graphion_csr_graph *graph;
  size_t begin;
  size_t end;
  uint64_t sum;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  graph = vm->csr_graph;
  if (graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!graphion_csr_graph_has_weights(graph)) {
    return GVM_ERR_CSR_WEIGHTS_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= graph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  begin = (size_t)(graph->offsets[node]);
  end = (size_t)(graph->offsets[node + 1U]);
  sum = sum_weight_slice_wrap(graphion_csr_graph_weights(graph, node), end - begin);
  vm_reg_set_int(vm, in->b, (int64_t)sum);
  return GVM_OK;
}

int op_neighbor_attr_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  const graphion_csr_graph *graph;
  size_t begin;
  size_t end;
  uint64_t sum;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  graph = vm->csr_graph;
  if (graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!graphion_csr_graph_has_edge_attrs(graph)) {
    return GVM_ERR_CSR_EDGE_ATTRS_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= graph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  begin = (size_t)(graph->offsets[node]);
  end = (size_t)(graph->offsets[node + 1U]);
  sum = sum_attr_slice_wrap(graphion_csr_graph_edge_attrs(graph, node), end - begin);
  vm_reg_set_int(vm, in->b, (int64_t)sum);
  return GVM_OK;
}

int op_bfs_levels(graphion_vm *vm, const graphion_insn *in) {
  uint32_t source;
  int rc;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  source = (uint32_t)reg_value;
  if ((size_t)source >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
  if (rc != 0) {
    return GVM_ERR_BFS_RUNTIME;
  }
  vm_reg_set_int(vm, in->b, count_visited_levels(vm->bfs_levels, vm->csr_graph->node_count));
  return 0;
}

int op_bfs_level_count(graphion_vm *vm, const graphion_insn *in) {
  uint32_t source;
  int rc;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  source = (uint32_t)reg_value;
  if ((size_t)source >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
  if (rc != 0) {
    return GVM_ERR_BFS_RUNTIME;
  }
  vm_reg_set_int(vm, in->b, count_bfs_level_count(vm->bfs_levels, vm->csr_graph->node_count));
  return 0;
}

int op_bfs_order(graphion_vm *vm, const graphion_insn *in) {
  uint32_t source;
  int rc;
  int64_t visited_count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  source = (uint32_t)reg_value;
  if ((size_t)source >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
  if (rc != 0) {
    return GVM_ERR_BFS_RUNTIME;
  }
  visited_count = count_visited_levels(vm->bfs_levels, vm->csr_graph->node_count);
  if ((size_t)visited_count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = (size_t)visited_count;
  for (i = 0U; i < vm->frontier_output_len; ++i) {
    vm->frontier_output[i] = vm->bfs_queue[i];
  }
  vm_reg_set_int(vm, in->b, visited_count);
  return 0;
}

