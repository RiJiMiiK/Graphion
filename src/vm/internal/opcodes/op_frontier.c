/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_frontier.h"

#include <limits.h>

#include "vm/internal/core/frontier.h"
#include "vm/internal/core/value.h"

int op_frontier_clear(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  vm->frontier_output_len = 0U;
  vm_reg_set_int(vm, in->a, 0);
  return GVM_OK;
}

int op_frontier_push(graphion_vm *vm, const graphion_insn *in) {
  int64_t value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value < 0 || (uint64_t)value > UINT32_MAX) {
    return GVM_ERR_INVALID_FRONTIER_VALUE;
  }
  if (vm->frontier_output_len >= vm->frontier_capacity) {
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output[vm->frontier_output_len++] = (uint32_t)value;
  vm_reg_set_int(vm, in->b, (int64_t)vm->frontier_output_len);
  return GVM_OK;
}

int op_frontier_filter_lt_imm(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  size_t out_len = 0U;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    if ((int64_t)vm->frontier_input[i] < (int64_t)in->imm) {
      if (out_len >= vm->frontier_capacity) {
        vm->frontier_output_len = 0U;
        return GVM_ERR_FRONTIER_OVERFLOW;
      }
      vm->frontier_output[out_len++] = vm->frontier_input[i];
    }
  }
  vm->frontier_output_len = out_len;
  vm_reg_set_int(vm, in->a, (int64_t)vm->frontier_output_len);
  return GVM_OK;
}

int op_frontier_map_add_imm(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (vm->frontier_input_len > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    const uint64_t sum = (uint64_t)vm->frontier_input[i] + (uint64_t)(int64_t)in->imm;
    if (sum > UINT32_MAX) {
      return GVM_ERR_FRONTIER_OVERFLOW;
    }
    vm->frontier_output[i] = (uint32_t)sum;
  }
  vm->frontier_output_len = vm->frontier_input_len;
  vm_reg_set_int(vm, in->a, (int64_t)vm->frontier_output_len);
  return GVM_OK;
}

int op_frontier_reduce_sum(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  uint64_t sum = 0U;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    sum += (uint64_t)vm->frontier_input[i];
  }
  vm_reg_set_int(vm, in->a, (int64_t)sum);
  return GVM_OK;
}

int op_frontier_swap(graphion_vm *vm, const graphion_insn *in) {
  uint32_t *tmp;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  tmp = vm->frontier_input;
  vm->frontier_input = vm->frontier_output;
  vm->frontier_input_len = vm->frontier_output_len;
  vm->frontier_output = tmp;
  vm->frontier_output_len = 0U;
  vm_reg_set_int(vm, in->a, (int64_t)vm->frontier_input_len);
  return GVM_OK;
}

