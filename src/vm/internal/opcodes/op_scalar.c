/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_scalar.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vm/internal/core/value.h"

static int op_numeric_binary(graphion_vm *vm, const graphion_insn *in, uint8_t opcode) {
  int64_t lhs_i;
  int64_t rhs_i;
  double lhs_f;
  double rhs_f;
  int lhs_is_float;
  int rhs_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (opcode == GVM_OP_ADD && vm->regs[in->a].kind == GVM_VALUE_STRING && vm->regs[in->b].kind == GVM_VALUE_STRING) {
    size_t lhs_len = strlen(vm->regs[in->a].as.string_value != NULL ? vm->regs[in->a].as.string_value : "");
    size_t rhs_len = strlen(vm->regs[in->b].as.string_value != NULL ? vm->regs[in->b].as.string_value : "");
    char *buffer = (char *)malloc(lhs_len + rhs_len + 1U);
    int rc;
    if (buffer == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    memcpy(buffer, vm->regs[in->a].as.string_value, lhs_len);
    memcpy(buffer + lhs_len, vm->regs[in->b].as.string_value, rhs_len + 1U);
    rc = vm_reg_set_string_copy(vm, in->a, buffer);
    free(buffer);
    return rc;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &lhs_i, &lhs_f, &lhs_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &rhs_i, &rhs_f, &rhs_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  if (opcode == GVM_OP_DIV) {
    if ((rhs_is_float && rhs_f == 0.0) || (!rhs_is_float && rhs_i == 0)) {
      return GVM_ERR_DIVIDE_BY_ZERO;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm_value_set_float(&vm->regs[in->a], lhs_f / rhs_f);
    return GVM_OK;
  }
  if (opcode == GVM_OP_FLOOR_DIV) {
    if ((rhs_is_float && rhs_f == 0.0) || (!rhs_is_float && rhs_i == 0)) {
      return GVM_ERR_DIVIDE_BY_ZERO;
    }
    vm_free_owned_reg_string(vm, in->a);
    if (!lhs_is_float && !rhs_is_float) {
      int64_t q = lhs_i / rhs_i;
      int64_t r = lhs_i % rhs_i;
      if (r != 0 && ((lhs_i < 0) != (rhs_i < 0))) {
        q -= 1;
      }
      vm_value_set_int(&vm->regs[in->a], q);
    } else {
      vm_value_set_float(&vm->regs[in->a], floor(lhs_f / rhs_f));
    }
    return GVM_OK;
  }
  if (opcode == GVM_OP_MOD) {
    if ((rhs_is_float && rhs_f == 0.0) || (!rhs_is_float && rhs_i == 0)) {
      return GVM_ERR_DIVIDE_BY_ZERO;
    }
    vm_free_owned_reg_string(vm, in->a);
    if (!lhs_is_float && !rhs_is_float) {
      vm_value_set_int(&vm->regs[in->a], lhs_i % rhs_i);
    } else {
      vm_value_set_float(&vm->regs[in->a], fmod(lhs_f, rhs_f));
    }
    return GVM_OK;
  }
  if (opcode == GVM_OP_POW) {
    vm_free_owned_reg_string(vm, in->a);
    vm_value_set_float(&vm->regs[in->a], pow(lhs_f, rhs_f));
    return GVM_OK;
  }

  if (!lhs_is_float && !rhs_is_float) {
    switch (opcode) {
      case GVM_OP_ADD:
        vm_free_owned_reg_string(vm, in->a);
        vm_value_set_int(&vm->regs[in->a], wrap_add_i64(lhs_i, rhs_i));
        return GVM_OK;
      case GVM_OP_SUB:
        vm_free_owned_reg_string(vm, in->a);
        vm_value_set_int(&vm->regs[in->a], wrap_sub_i64(lhs_i, rhs_i));
        return GVM_OK;
      case GVM_OP_MUL:
        vm_free_owned_reg_string(vm, in->a);
        vm_value_set_int(&vm->regs[in->a], wrap_mul_i64(lhs_i, rhs_i));
        return GVM_OK;
      default:
        return GVM_ERR_UNKNOWN_OPCODE;
    }
  }

  switch (opcode) {
    case GVM_OP_ADD:
      vm_free_owned_reg_string(vm, in->a);
      vm_value_set_float(&vm->regs[in->a], lhs_f + rhs_f);
      return GVM_OK;
    case GVM_OP_SUB:
      vm_free_owned_reg_string(vm, in->a);
      vm_value_set_float(&vm->regs[in->a], lhs_f - rhs_f);
      return GVM_OK;
    case GVM_OP_MUL:
      vm_free_owned_reg_string(vm, in->a);
      vm_value_set_float(&vm->regs[in->a], lhs_f * rhs_f);
      return GVM_OK;
    default:
      return GVM_ERR_UNKNOWN_OPCODE;
  }
}

static int op_eq(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  int compatible = 0;
  int result = 0;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (vm_values_deep_equal(lhs, rhs, &compatible, &result) != GVM_OK || !compatible) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], result);
  return GVM_OK;
}

static int op_ne(graphion_vm *vm, const graphion_insn *in) {
  int rc = op_eq(vm, in);
  if (rc != GVM_OK) {
    return rc;
  }
  vm->regs[in->a].as.bool_value = vm->regs[in->a].as.bool_value == 0 ? 1 : 0;
  return GVM_OK;
}

static int op_lt(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  int64_t lhs_i;
  int64_t rhs_i;
  double lhs_f;
  double rhs_f;
  int lhs_is_float;
  int rhs_is_float;
  int result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (!vm_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
      !vm_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  result = lhs_f < rhs_f ? 1 : 0;
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], result);
  return GVM_OK;
}

static int op_le(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  int64_t lhs_i;
  int64_t rhs_i;
  double lhs_f;
  double rhs_f;
  int lhs_is_float;
  int rhs_is_float;
  int result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (!vm_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
      !vm_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  result = lhs_f <= rhs_f ? 1 : 0;
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], result);
  return GVM_OK;
}

static int op_gt(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  int64_t lhs_i;
  int64_t rhs_i;
  double lhs_f;
  double rhs_f;
  int lhs_is_float;
  int rhs_is_float;
  int result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (!vm_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
      !vm_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  result = lhs_f > rhs_f ? 1 : 0;
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], result);
  return GVM_OK;
}

static int op_ge(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  int64_t lhs_i;
  int64_t rhs_i;
  double lhs_f;
  double rhs_f;
  int lhs_is_float;
  int rhs_is_float;
  int result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (!vm_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
      !vm_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  result = lhs_f >= rhs_f ? 1 : 0;
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], result);
  return GVM_OK;
}

int vm_value_get_boolean(const graphion_vm_value *value, int *out) {
  if (value == NULL || out == NULL) {
    return 0;
  }
  if (value->kind == GVM_VALUE_BOOL) {
    *out = value->as.bool_value != 0 ? 1 : 0;
    return 1;
  }
  if (value->kind == GVM_VALUE_INT && (value->as.int_value == 0 || value->as.int_value == 1)) {
    *out = (int)value->as.int_value;
    return 1;
  }
  return 0;
}

static int op_and(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  int lhs_bool;
  int rhs_bool;
  int result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (!vm_value_get_boolean(lhs, &lhs_bool) || !vm_value_get_boolean(rhs, &rhs_bool)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  result = (lhs_bool != 0 && rhs_bool != 0) ? 1 : 0;
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], result);
  return GVM_OK;
}

static int op_or(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  int lhs_bool;
  int rhs_bool;
  int result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (!vm_value_get_boolean(lhs, &lhs_bool) || !vm_value_get_boolean(rhs, &rhs_bool)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  result = (lhs_bool != 0 || rhs_bool != 0) ? 1 : 0;
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], result);
  return GVM_OK;
}

static int op_not(graphion_vm *vm, const graphion_insn *in) {
  int bool_value;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_boolean(&vm->regs[in->a], &bool_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], bool_value == 0 ? 1 : 0);
  return GVM_OK;
}

static int op_nand(graphion_vm *vm, const graphion_insn *in) {
  int rc = op_and(vm, in);
  if (rc != GVM_OK) {
    return rc;
  }
  vm->regs[in->a].as.bool_value = vm->regs[in->a].as.bool_value == 0 ? 1 : 0;
  return GVM_OK;
}

static int op_nor(graphion_vm *vm, const graphion_insn *in) {
  int rc = op_or(vm, in);
  if (rc != GVM_OK) {
    return rc;
  }
  vm->regs[in->a].as.bool_value = vm->regs[in->a].as.bool_value == 0 ? 1 : 0;
  return GVM_OK;
}

static int op_bit_and(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  uint8_t lhs_width;
  uint8_t rhs_width;
  uint64_t result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (lhs->kind != GVM_VALUE_BITS || rhs->kind != GVM_VALUE_BITS) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  lhs_width = vm_value_get_bits_width(lhs);
  rhs_width = vm_value_get_bits_width(rhs);
  if (lhs_width == 0U || rhs_width == 0U || lhs_width != rhs_width) {
    return GVM_ERR_BITS_WIDTH_MISMATCH;
  }

  result = vm_value_get_bits_payload(lhs) & vm_value_get_bits_payload(rhs);
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bits(&vm->regs[in->a], result, lhs_width);
  return GVM_OK;
}

static int op_bit_or(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  uint8_t lhs_width;
  uint8_t rhs_width;
  uint64_t result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (lhs->kind != GVM_VALUE_BITS || rhs->kind != GVM_VALUE_BITS) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  lhs_width = vm_value_get_bits_width(lhs);
  rhs_width = vm_value_get_bits_width(rhs);
  if (lhs_width == 0U || rhs_width == 0U || lhs_width != rhs_width) {
    return GVM_ERR_BITS_WIDTH_MISMATCH;
  }

  result = vm_value_get_bits_payload(lhs) | vm_value_get_bits_payload(rhs);
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bits(&vm->regs[in->a], result, lhs_width);
  return GVM_OK;
}

static int op_bit_xor(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  uint8_t lhs_width;
  uint8_t rhs_width;
  uint64_t result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (lhs->kind != GVM_VALUE_BITS || rhs->kind != GVM_VALUE_BITS) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  lhs_width = vm_value_get_bits_width(lhs);
  rhs_width = vm_value_get_bits_width(rhs);
  if (lhs_width == 0U || rhs_width == 0U || lhs_width != rhs_width) {
    return GVM_ERR_BITS_WIDTH_MISMATCH;
  }

  result = vm_value_get_bits_payload(lhs) ^ vm_value_get_bits_payload(rhs);
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bits(&vm->regs[in->a], result, lhs_width);
  return GVM_OK;
}

static int op_bit_not(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *value;
  uint8_t width;
  uint64_t payload;
  uint64_t mask;
  uint64_t result;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }

  value = &vm->regs[in->a];
  if (value->kind != GVM_VALUE_BITS) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  width = vm_value_get_bits_width(value);
  if (width == 0U) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  payload = vm_value_get_bits_payload(value);
  mask = width >= 64U ? UINT64_MAX : ((1ULL << width) - 1ULL);
  result = (~payload) & mask;
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bits(&vm->regs[in->a], result, width);
  return GVM_OK;
}

static int op_bit_shl(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  uint8_t lhs_width;
  uint64_t payload;
  int64_t shift_i;
  uint64_t mask;
  uint64_t result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (lhs->kind != GVM_VALUE_BITS || rhs->kind != GVM_VALUE_INT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  lhs_width = vm_value_get_bits_width(lhs);
  if (lhs_width == 0U) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  shift_i = rhs->as.int_value;
  if (shift_i < 0) {
    return GVM_ERR_NEGATIVE_SHIFT;
  }

  payload = vm_value_get_bits_payload(lhs);
  mask = lhs_width >= 64U ? UINT64_MAX : ((1ULL << lhs_width) - 1ULL);
  result = (uint64_t)shift_i >= lhs_width ? 0ULL : ((payload << (unsigned int)shift_i) & mask);
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bits(&vm->regs[in->a], result, lhs_width);
  return GVM_OK;
}

static int op_bit_shr(graphion_vm *vm, const graphion_insn *in) {
  const graphion_vm_value *lhs;
  const graphion_vm_value *rhs;
  uint8_t lhs_width;
  uint64_t payload;
  int64_t shift_i;
  uint64_t result;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];
  if (lhs->kind != GVM_VALUE_BITS || rhs->kind != GVM_VALUE_INT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  lhs_width = vm_value_get_bits_width(lhs);
  if (lhs_width == 0U) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  shift_i = rhs->as.int_value;
  if (shift_i < 0) {
    return GVM_ERR_NEGATIVE_SHIFT;
  }

  payload = vm_value_get_bits_payload(lhs);
  result = (uint64_t)shift_i >= lhs_width ? 0ULL : (payload >> (unsigned int)shift_i);
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bits(&vm->regs[in->a], result, lhs_width);
  return GVM_OK;
}

int op_add(graphion_vm *vm, const graphion_insn *in) { return op_numeric_binary(vm, in, GVM_OP_ADD); }
int op_sub(graphion_vm *vm, const graphion_insn *in) { return op_numeric_binary(vm, in, GVM_OP_SUB); }
int op_mul(graphion_vm *vm, const graphion_insn *in) { return op_numeric_binary(vm, in, GVM_OP_MUL); }
int op_div(graphion_vm *vm, const graphion_insn *in) { return op_numeric_binary(vm, in, GVM_OP_DIV); }
int op_mod(graphion_vm *vm, const graphion_insn *in) { return op_numeric_binary(vm, in, GVM_OP_MOD); }
int op_pow(graphion_vm *vm, const graphion_insn *in) { return op_numeric_binary(vm, in, GVM_OP_POW); }
int op_floor_div(graphion_vm *vm, const graphion_insn *in) {
  return op_numeric_binary(vm, in, GVM_OP_FLOOR_DIV);
}

int op_eq_cmp(graphion_vm *vm, const graphion_insn *in) { return op_eq(vm, in); }
int op_ne_cmp(graphion_vm *vm, const graphion_insn *in) { return op_ne(vm, in); }
int op_lt_cmp(graphion_vm *vm, const graphion_insn *in) { return op_lt(vm, in); }
int op_le_cmp(graphion_vm *vm, const graphion_insn *in) { return op_le(vm, in); }
int op_gt_cmp(graphion_vm *vm, const graphion_insn *in) { return op_gt(vm, in); }
int op_ge_cmp(graphion_vm *vm, const graphion_insn *in) { return op_ge(vm, in); }

int op_and_cmp(graphion_vm *vm, const graphion_insn *in) { return op_and(vm, in); }
int op_or_cmp(graphion_vm *vm, const graphion_insn *in) { return op_or(vm, in); }
int op_not_cmp(graphion_vm *vm, const graphion_insn *in) { return op_not(vm, in); }
int op_nand_cmp(graphion_vm *vm, const graphion_insn *in) { return op_nand(vm, in); }
int op_nor_cmp(graphion_vm *vm, const graphion_insn *in) { return op_nor(vm, in); }

int op_bit_and_cmp(graphion_vm *vm, const graphion_insn *in) { return op_bit_and(vm, in); }
int op_bit_or_cmp(graphion_vm *vm, const graphion_insn *in) { return op_bit_or(vm, in); }
int op_bit_xor_cmp(graphion_vm *vm, const graphion_insn *in) { return op_bit_xor(vm, in); }
int op_bit_not_cmp(graphion_vm *vm, const graphion_insn *in) { return op_bit_not(vm, in); }
int op_bit_shl_cmp(graphion_vm *vm, const graphion_insn *in) { return op_bit_shl(vm, in); }
int op_bit_shr_cmp(graphion_vm *vm, const graphion_insn *in) { return op_bit_shr(vm, in); }

int op_abs(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (is_float) {
    vm_value_set_float(&vm->regs[in->a], fabs(value_f));
    return GVM_OK;
  }
  if (value_i == INT64_MIN) {
    vm_value_set_float(&vm->regs[in->a], fabs((double)value_i));
    return GVM_OK;
  }
  vm_value_set_int(&vm->regs[in->a], value_i < 0 ? -value_i : value_i);
  return GVM_OK;
}

int op_min(graphion_vm *vm, const graphion_insn *in) {
  int64_t lhs_i;
  int64_t rhs_i;
  double lhs_f;
  double rhs_f;
  int lhs_is_float;
  int rhs_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &lhs_i, &lhs_f, &lhs_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &rhs_i, &rhs_f, &rhs_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (!lhs_is_float && !rhs_is_float) {
    vm_value_set_int(&vm->regs[in->a], lhs_i < rhs_i ? lhs_i : rhs_i);
    return GVM_OK;
  }
  vm_value_set_float(&vm->regs[in->a], lhs_f < rhs_f ? lhs_f : rhs_f);
  return GVM_OK;
}

int op_max(graphion_vm *vm, const graphion_insn *in) {
  int64_t lhs_i;
  int64_t rhs_i;
  double lhs_f;
  double rhs_f;
  int lhs_is_float;
  int rhs_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &lhs_i, &lhs_f, &lhs_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &rhs_i, &rhs_f, &rhs_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (!lhs_is_float && !rhs_is_float) {
    vm_value_set_int(&vm->regs[in->a], lhs_i > rhs_i ? lhs_i : rhs_i);
    return GVM_OK;
  }
  vm_value_set_float(&vm->regs[in->a], lhs_f > rhs_f ? lhs_f : rhs_f);
  return GVM_OK;
}

int op_clamp(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  int64_t lo_i;
  int64_t hi_i;
  double value_f;
  double lo_f;
  double hi_f;
  int value_is_float;
  int lo_is_float;
  int hi_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &value_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &lo_i, &lo_f, &lo_is_float) ||
      !vm_value_get_numeric(&vm->regs[(uint8_t)in->imm], &hi_i, &hi_f, &hi_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (!value_is_float && !lo_is_float && !hi_is_float) {
    if (value_i < lo_i) {
      vm_value_set_int(&vm->regs[in->a], lo_i);
    } else if (value_i > hi_i) {
      vm_value_set_int(&vm->regs[in->a], hi_i);
    } else {
      vm_value_set_int(&vm->regs[in->a], value_i);
    }
    return GVM_OK;
  }

  if (value_f < lo_f) {
    vm_value_set_float(&vm->regs[in->a], lo_f);
  } else if (value_f > hi_f) {
    vm_value_set_float(&vm->regs[in->a], hi_f);
  } else {
    vm_value_set_float(&vm->regs[in->a], value_f);
  }
  return GVM_OK;
}


int op_fma_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t a_i;
  int64_t b_i;
  int64_t c_i;
  double a_f;
  double b_f;
  double c_f;
  int a_is_float;
  int b_is_float;
  int c_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &a_i, &a_f, &a_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &b_i, &b_f, &b_is_float) ||
      !vm_value_get_numeric(&vm->regs[(uint8_t)in->imm], &c_i, &c_f, &c_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], fma(a_f, b_f, c_f));
  return GVM_OK;
}

int op_fdim_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t x_i;
  int64_t y_i;
  double x_f;
  double y_f;
  int x_is_float;
  int y_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &x_i, &x_f, &x_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &y_i, &y_f, &y_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], fdim(x_f, y_f));
  return GVM_OK;
}

int op_remainder_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t x_i;
  int64_t y_i;
  double x_f;
  double y_f;
  int x_is_float;
  int y_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &x_i, &x_f, &x_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &y_i, &y_f, &y_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (y_f == 0.0) {
    return GVM_ERR_REMAINDER_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], remainder(x_f, y_f));
  return GVM_OK;
}

int op_rint_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], rint(value_f));
  return GVM_OK;
}

int op_ln(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f <= 0.0) {
    return GVM_ERR_LN_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], log(value_f));
  return GVM_OK;
}

int op_log(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  int64_t base_i;
  double value_f;
  double base_f;
  int value_is_float;
  int base_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &value_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &base_i, &base_f, &base_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f <= 0.0 || base_f <= 0.0 || base_f == 1.0) {
    return GVM_ERR_LOG_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], log(value_f) / log(base_f));
  return GVM_OK;
}

int op_floor_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (!is_float) {
    vm_value_set_int(&vm->regs[in->a], value_i);
    return GVM_OK;
  }
  vm_value_set_float(&vm->regs[in->a], floor(value_f));
  return GVM_OK;
}

int op_ceil_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (!is_float) {
    vm_value_set_int(&vm->regs[in->a], value_i);
    return GVM_OK;
  }
  vm_value_set_float(&vm->regs[in->a], ceil(value_f));
  return GVM_OK;
}

int op_round_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (!is_float) {
    vm_value_set_int(&vm->regs[in->a], value_i);
    return GVM_OK;
  }
  vm_value_set_float(&vm->regs[in->a], round(value_f));
  return GVM_OK;
}

int op_trunc_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  double truncated;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (!is_float) {
    vm_value_set_int(&vm->regs[in->a], value_i);
    return GVM_OK;
  }
  truncated = trunc(value_f);
  vm_value_set_float(&vm->regs[in->a], truncated);
  return GVM_OK;
}

int op_fract_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  double fraction;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  fraction = value_f - floor(value_f);
  vm_value_set_float(&vm->regs[in->a], fraction);
  return GVM_OK;
}

int op_sign_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  if (value_f < 0.0) {
    vm_value_set_int(&vm->regs[in->a], -1);
  } else if (value_f > 0.0) {
    vm_value_set_int(&vm->regs[in->a], 1);
  } else {
    vm_value_set_int(&vm->regs[in->a], 0);
  }
  return GVM_OK;
}

int op_len(graphion_vm *vm, const graphion_insn *in) {
  const char *text;
  size_t len;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind == GVM_VALUE_LIST) {
    if (!vm_value_list_length(&vm->regs[in->a], &len)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm_value_set_int(&vm->regs[in->a], (int64_t)len);
    return GVM_OK;
  }
  if (vm->regs[in->a].kind == GVM_VALUE_DICT) {
    if (!vm_value_dict_length(&vm->regs[in->a], &len)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm_value_set_int(&vm->regs[in->a], (int64_t)len);
    return GVM_OK;
  }
  if (vm->regs[in->a].kind == GVM_VALUE_TUPLE) {
    if (!vm_value_tuple_length(&vm->regs[in->a], &len)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm_value_set_int(&vm->regs[in->a], (int64_t)len);
    return GVM_OK;
  }
  if (vm->regs[in->a].kind == GVM_VALUE_SET) {
    if (!vm_value_set_length(&vm->regs[in->a], &len)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm_value_set_int(&vm->regs[in->a], (int64_t)len);
    return GVM_OK;
  }
  if (vm->regs[in->a].kind == GVM_VALUE_STRUCT) {
    if (!vm_value_struct_field_count(&vm->regs[in->a], &len)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    vm_free_owned_reg_string(vm, in->a);
    vm_value_set_int(&vm->regs[in->a], (int64_t)len);
    return GVM_OK;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_STRING) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  text = vm->regs[in->a].as.string_value != NULL ? vm->regs[in->a].as.string_value : "";
  len = strlen(text);
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_int(&vm->regs[in->a], (int64_t)len);
  return GVM_OK;
}

static int graph_reg_value(const graphion_vm *vm, const graphion_insn *in, const graphion_graph_value **graph_out) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  *graph_out = (const graphion_graph_value *)vm->regs[in->a].as.ref_value;
  return GVM_OK;
}

static int hypergraph_reg_value(const graphion_vm *vm,
                                const graphion_insn *in,
                                const graphion_hypergraph_value **hypergraph_out) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_HYPERGRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  *hypergraph_out = (const graphion_hypergraph_value *)vm->regs[in->a].as.ref_value;
  return GVM_OK;
}

static int hypergraph_id_from_value(const graphion_vm_value *value, size_t count, size_t *id_out) {
  int64_t id;

  if (value == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (value->kind != GVM_VALUE_INT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  id = value->as.int_value;
  if (id < 0 || (uint64_t)id >= (uint64_t)count) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  *id_out = (size_t)id;
  return GVM_OK;
}

static int hypergraph_hyperedge_id_is_active(const graphion_hypergraph *hypergraph, size_t id) {
  if (hypergraph == NULL || id >= hypergraph->hyperedge_count || hypergraph->hyperedge_offsets == NULL) {
    return 0;
  }
  return hypergraph->hyperedge_offsets[id] < hypergraph->hyperedge_offsets[id + 1U];
}

static int hypergraph_active_hyperedge_id_from_value(const graphion_vm_value *value,
                                                     const graphion_hypergraph *hypergraph,
                                                     size_t *id_out) {
  size_t id;
  int rc;

  if (hypergraph == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  rc = hypergraph_id_from_value(value, hypergraph->hyperedge_count, &id);
  if (rc != GVM_OK) {
    return rc;
  }
  if (!hypergraph_hyperedge_id_is_active(hypergraph, id)) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  *id_out = id;
  return GVM_OK;
}

static size_t hypergraph_active_hyperedge_count(const graphion_hypergraph *hypergraph) {
  size_t count = 0U;
  size_t i;

  if (hypergraph == NULL || hypergraph->hyperedge_offsets == NULL) {
    return 0U;
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    if (hypergraph_hyperedge_id_is_active(hypergraph, i)) {
      count += 1U;
    }
  }
  return count;
}

static int hypergraph_vertex_id_from_value(const graphion_hypergraph_value *hypergraph,
                                           const graphion_vm_value *value,
                                           uint32_t *id_out) {
  size_t i;

  if (hypergraph == NULL || value == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (value->kind == GVM_VALUE_INT) {
    const int64_t id = value->as.int_value;
    if (id < 0 || id > UINT32_MAX) {
      return GVM_ERR_INVALID_NODE_ID;
    }
    for (i = 0U; i < hypergraph->vertex_count; ++i) {
      if (hypergraph->vertices[i].id == (uint32_t)id) {
        *id_out = (uint32_t)id;
        return GVM_OK;
      }
    }
    return GVM_ERR_INVALID_NODE_ID;
  }
  if (value->kind == GVM_VALUE_STRING) {
    const char *name = value->as.string_value != NULL ? value->as.string_value : "";
    for (i = 0U; i < hypergraph->vertex_count; ++i) {
      if (hypergraph->vertices[i].name != NULL && strcmp(hypergraph->vertices[i].name, name) == 0) {
        *id_out = hypergraph->vertices[i].id;
        return GVM_OK;
      }
    }
    return GVM_ERR_INVALID_NODE_ID;
  }
  return GVM_ERR_TYPE_MISMATCH;
}

static size_t hypergraph_visible_vertex_count(const graphion_vm_value *value,
                                              const graphion_hypergraph_value *hypergraph) {
  size_t count;

  count = (size_t)value->reserved[1] | ((size_t)value->reserved[2] << 8U);
  if (count != 0U) {
    return count;
  }
  return hypergraph != NULL ? hypergraph->hypergraph.node_count : 0U;
}

static size_t hypergraph_visible_attr_key_count(const graphion_vm_value *attrs, size_t attr_count) {
  size_t i;
  size_t len = 0U;

  if (attrs == NULL) {
    return 0U;
  }
  for (i = 0U; i < attr_count; ++i) {
    if (attrs[i].kind == GVM_VALUE_DICT && vm_value_dict_length(&attrs[i], &len) && len > 0U) {
      return len;
    }
  }
  return 0U;
}

static size_t graph_visible_node_count(const graphion_vm_value *value, const graphion_graph_value *graph) {
  size_t count;

  count = (size_t)value->reserved[1] | ((size_t)value->reserved[2] << 8U);
  if (count != 0U) {
    return count;
  }
  return graph != NULL ? graph->csr.node_count : 0U;
}

static int graph_has_directed_edges(const graphion_graph_value *graph) {
  size_t i;

  if (graph == NULL) {
    return 0;
  }
  for (i = 0U; i < graph->edge_count; ++i) {
    if (graph->edges[i].directed) {
      return 1;
    }
  }
  return 0;
}

static int graph_has_weighted_edges(const graphion_graph_value *graph) {
  size_t i;

  if (graph == NULL || graph->edge_attrs == NULL) {
    return 0;
  }
  for (i = 0U; i < graph->edge_attr_count; ++i) {
    uint8_t kind = GVM_VALUE_NONE;
    int found = 0;
    if (vm_value_dict_key_kind(&graph->edge_attrs[i], "weight", &kind, &found) && found) {
      return 1;
    }
  }
  return 0;
}

static int graph_node_id_from_value(const graphion_graph_value *graph,
                                    const graphion_vm_value *value,
                                    uint32_t *id_out) {
  size_t i;

  if (graph == NULL || value == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (value->kind == GVM_VALUE_INT) {
    const int64_t id = value->as.int_value;
    if (id < 0 || id > UINT32_MAX) {
      return GVM_ERR_INVALID_NODE_ID;
    }
    for (i = 0U; i < graph->node_count; ++i) {
      if (graph->nodes[i].id == (uint32_t)id) {
        *id_out = (uint32_t)id;
        return GVM_OK;
      }
    }
    return GVM_ERR_INVALID_NODE_ID;
  }
  if (value->kind == GVM_VALUE_STRING) {
    const char *name = value->as.string_value != NULL ? value->as.string_value : "";
    for (i = 0U; i < graph->node_count; ++i) {
      if (graph->nodes[i].name != NULL && strcmp(graph->nodes[i].name, name) == 0) {
        *id_out = graph->nodes[i].id;
        return GVM_OK;
      }
    }
    return GVM_ERR_INVALID_NODE_ID;
  }
  return GVM_ERR_TYPE_MISMATCH;
}

static char *graph_strdup_text(const char *text) {
  size_t len;
  char *copy;

  if (text == NULL) {
    return NULL;
  }
  len = strlen(text);
  copy = (char *)malloc(len + 1U);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, text, len + 1U);
  return copy;
}

static int graph_node_id_exists(const graphion_graph_value *graph, uint32_t id) {
  size_t i;

  if (graph == NULL) {
    return 0;
  }
  for (i = 0U; i < graph->node_count; ++i) {
    if (graph->nodes[i].id == id) {
      return 1;
    }
  }
  return 0;
}

static int graph_next_free_node_id(const graphion_graph_value *graph, uint32_t *id_out) {
  uint32_t id;

  if (graph == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  for (id = 0U; id < UINT32_MAX; ++id) {
    if (!graph_node_id_exists(graph, id)) {
      *id_out = id;
      return GVM_OK;
    }
  }
  return GVM_ERR_INVALID_ARG;
}

static int graph_rebuild_adjacency(graphion_graph_value *graph) {
  graphion_csr_graph *csr;
  uint32_t *offsets = NULL;
  uint32_t *neighbors = NULL;
  uint32_t *cursor = NULL;
  size_t node_count = 0U;
  size_t adjacency_count = 0U;
  size_t i;

  if (graph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  csr = &graph->csr;
  for (i = 0U; i < graph->node_count; ++i) {
    const size_t candidate = (size_t)graph->nodes[i].id + 1U;
    if (candidate > node_count) {
      node_count = candidate;
    }
  }
  for (i = 0U; i < graph->edge_count; ++i) {
    adjacency_count += graph->edges[i].bidirectional ? 2U : 1U;
  }
  if (node_count > 0U) {
    offsets = (uint32_t *)calloc(node_count + 1U, sizeof(*offsets));
    if (offsets == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
  }
  if (adjacency_count > 0U) {
    neighbors = (uint32_t *)calloc(adjacency_count, sizeof(*neighbors));
    cursor = (uint32_t *)calloc(node_count, sizeof(*cursor));
    if (neighbors == NULL || cursor == NULL) {
      free(offsets);
      free(neighbors);
      free(cursor);
      return GVM_ERR_INVALID_ARG;
    }
    for (i = 0U; i < graph->edge_count; ++i) {
      offsets[graph->edges[i].from + 1U] += 1U;
      if (graph->edges[i].bidirectional) {
        offsets[graph->edges[i].to + 1U] += 1U;
      }
    }
    for (i = 1U; i <= node_count; ++i) {
      offsets[i] += offsets[i - 1U];
    }
    memcpy(cursor, offsets, node_count * sizeof(*cursor));
    for (i = 0U; i < graph->edge_count; ++i) {
      const uint32_t from = graph->edges[i].from;
      const uint32_t to = graph->edges[i].to;
      neighbors[cursor[from]++] = to;
      if (graph->edges[i].bidirectional) {
        neighbors[cursor[to]++] = from;
      }
    }
    free(cursor);
  }
  free((void *)csr->offsets);
  free((void *)csr->neighbors);
  csr->node_count = node_count;
  csr->edge_count = adjacency_count;
  csr->offsets = offsets;
  csr->neighbors = neighbors;
  csr->weights = NULL;
  csr->edge_attrs = NULL;
  return GVM_OK;
}

static int graph_add_node_id(graphion_graph_value *graph, uint32_t id, const char *name) {
  graphion_graph_node_value *nodes;
  graphion_vm_value *attrs;

  if (graph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (graph_node_id_exists(graph, id)) {
    return GVM_OK;
  }
  nodes = (graphion_graph_node_value *)realloc(graph->nodes, (graph->node_count + 1U) * sizeof(*nodes));
  if (nodes == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  graph->nodes = nodes;
  graph->nodes[graph->node_count].id = id;
  graph->nodes[graph->node_count].name = NULL;
  if (name != NULL) {
    graph->nodes[graph->node_count].name = graph_strdup_text(name);
    if (graph->nodes[graph->node_count].name == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
  }
  graph->node_count += 1U;
  if ((size_t)id >= graph->node_attr_count) {
    size_t i;
    attrs = (graphion_vm_value *)realloc(graph->node_attrs, ((size_t)id + 1U) * sizeof(*attrs));
    if (attrs == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    graph->node_attrs = attrs;
    for (i = graph->node_attr_count; i <= (size_t)id; ++i) {
      memset(&graph->node_attrs[i], 0, sizeof(graph->node_attrs[i]));
      graph->node_attrs[i].kind = GVM_VALUE_NONE;
    }
    graph->node_attr_count = (size_t)id + 1U;
  }
  return graph_rebuild_adjacency(graph);
}

static int graph_node_id_from_value_or_add(graphion_graph_value *graph,
                                           const graphion_vm_value *value,
                                           uint32_t *id_out) {
  uint32_t id;
  int rc;

  if (graph == NULL || value == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  rc = graph_node_id_from_value(graph, value, id_out);
  if (rc == GVM_OK) {
    return GVM_OK;
  }
  if (rc != GVM_ERR_INVALID_NODE_ID) {
    return rc;
  }
  if (value->kind == GVM_VALUE_INT) {
    if (value->as.int_value < 0 || value->as.int_value > UINT32_MAX) {
      return GVM_ERR_INVALID_NODE_ID;
    }
    id = (uint32_t)value->as.int_value;
    rc = graph_add_node_id(graph, id, NULL);
    if (rc != GVM_OK) {
      return rc;
    }
    *id_out = id;
    return GVM_OK;
  }
  if (value->kind == GVM_VALUE_STRING) {
    const char *name = value->as.string_value != NULL ? value->as.string_value : "";
    rc = graph_next_free_node_id(graph, &id);
    if (rc != GVM_OK) {
      return rc;
    }
    rc = graph_add_node_id(graph, id, name);
    if (rc != GVM_OK) {
      return rc;
    }
    *id_out = id;
    return GVM_OK;
  }
  return GVM_ERR_TYPE_MISMATCH;
}

static int graph_edge_index_from_ids(const graphion_graph_value *graph,
                                     uint32_t from,
                                     uint32_t to,
                                     size_t *index_out) {
  size_t i;

  if (graph == NULL || index_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  for (i = 0U; i < graph->edge_count; ++i) {
    const graphion_graph_edge_value *edge = &graph->edges[i];
    if (edge->from == from && edge->to == to) {
      *index_out = i;
      return GVM_OK;
    }
    if (edge->bidirectional && edge->from == to && edge->to == from) {
      *index_out = i;
      return GVM_OK;
    }
  }
  return GVM_ERR_MISSING_KEY;
}

static int graph_clone_result_into_target(graphion_vm *vm, uint8_t target, const graphion_vm_value *value) {
  graphion_vm_value cloned;
  int rc;

  memset(&cloned, 0, sizeof(cloned));
  cloned.kind = GVM_VALUE_NONE;
  rc = vm_value_clone(&cloned, value);
  if (rc != GVM_OK) {
    return rc;
  }
  vm_value_dispose_owned(&vm->regs[target]);
  vm->regs[target] = cloned;
  return GVM_OK;
}

static int graph_make_empty_dict_value(graphion_vm_value *out) {
  graphion_vm temp;
  int rc;

  if (out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  graphion_vm_init(&temp);
  rc = vm_reg_set_empty_dict(&temp, 0U);
  if (rc == GVM_OK) {
    *out = temp.regs[0];
    temp.regs[0].kind = GVM_VALUE_NONE;
  }
  graphion_vm_dispose(&temp);
  return rc;
}

static void graph_update_visible_counts(graphion_vm_value *value, const graphion_graph_value *graph) {
  if (value == NULL || graph == NULL) {
    return;
  }
  value->reserved[1] = (uint8_t)(graph->node_count & 0xFFU);
  value->reserved[2] = (uint8_t)((graph->node_count >> 8U) & 0xFFU);
  value->reserved[3] = (uint8_t)(graph->edge_count & 0xFFU);
  value->reserved[4] = (uint8_t)((graph->edge_count >> 8U) & 0xFFU);
}

static int graph_value_has_weight_if_present_valid(const graphion_vm_value *attrs) {
  uint8_t kind = GVM_VALUE_NONE;
  int found = 0;

  if (attrs == NULL || attrs->kind != GVM_VALUE_DICT) {
    return 0;
  }
  if (!vm_value_dict_key_kind(attrs, "weight", &kind, &found)) {
    return 0;
  }
  return !found || kind == GVM_VALUE_INT || kind == GVM_VALUE_FLOAT;
}

static const graphion_vm_value *graph_first_node_attrs(const graphion_graph_value *graph) {
  size_t i;

  if (graph == NULL || graph->node_attrs == NULL) {
    return NULL;
  }
  for (i = 0U; i < graph->node_attr_count; ++i) {
    if (graph->node_attrs[i].kind == GVM_VALUE_DICT) {
      return &graph->node_attrs[i];
    }
  }
  return NULL;
}

static const graphion_vm_value *graph_first_edge_attrs(const graphion_graph_value *graph) {
  size_t i;

  if (graph == NULL || graph->edge_attrs == NULL) {
    return NULL;
  }
  for (i = 0U; i < graph->edge_attr_count; ++i) {
    if (graph->edge_attrs[i].kind == GVM_VALUE_DICT) {
      return &graph->edge_attrs[i];
    }
  }
  return NULL;
}

static int graph_replace_attr_value(graphion_vm_value *slot, const graphion_vm_value *src) {
  graphion_vm_value cloned;
  int rc;

  if (slot == NULL || src == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  memset(&cloned, 0, sizeof(cloned));
  cloned.kind = GVM_VALUE_NONE;
  rc = vm_value_clone(&cloned, src);
  if (rc != GVM_OK) {
    return rc;
  }
  vm_value_dispose_owned(slot);
  *slot = cloned;
  return GVM_OK;
}

static int graph_apply_node_defaults(graphion_graph_value *graph,
                                     uint32_t node_id,
                                     const graphion_vm_value *defaults) {
  if (graph == NULL || defaults == NULL || defaults->kind == GVM_VALUE_NONE) {
    return GVM_OK;
  }
  if (defaults->kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if ((size_t)node_id >= graph->node_attr_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  if (graph->node_attrs[node_id].kind == GVM_VALUE_NONE) {
    return graph_replace_attr_value(&graph->node_attrs[node_id], defaults);
  }
  return GVM_OK;
}

static void hypergraph_update_visible_counts(graphion_vm_value *value, const graphion_hypergraph_value *hypergraph) {
  if (value == NULL || hypergraph == NULL) {
    return;
  }
  value->reserved[1] = (uint8_t)(hypergraph->vertex_count & 0xFFU);
  value->reserved[2] = (uint8_t)((hypergraph->vertex_count >> 8U) & 0xFFU);
}

static int hypergraph_vertex_id_exists(const graphion_hypergraph_value *hypergraph, uint32_t id) {
  size_t i;

  if (hypergraph == NULL) {
    return 0;
  }
  for (i = 0U; i < hypergraph->vertex_count; ++i) {
    if (hypergraph->vertices[i].id == id) {
      return 1;
    }
  }
  return 0;
}

static int hypergraph_next_free_vertex_id(const graphion_hypergraph_value *hypergraph, uint32_t *id_out) {
  uint32_t id;

  if (hypergraph == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  for (id = 0U; id < UINT32_MAX; ++id) {
    if (!hypergraph_vertex_id_exists(hypergraph, id)) {
      *id_out = id;
      return GVM_OK;
    }
  }
  return GVM_ERR_INVALID_ARG;
}

static int hypergraph_rebuild_incidence(graphion_hypergraph_value *value) {
  graphion_hypergraph *hypergraph;
  uint32_t *node_offsets = NULL;
  uint32_t *node_hyperedges = NULL;
  uint32_t *cursor = NULL;
  size_t dense_vertex_count = 0U;
  size_t i;

  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph = &value->hypergraph;
  for (i = 0U; i < value->vertex_count; ++i) {
    const size_t candidate = (size_t)value->vertices[i].id + 1U;
    if (candidate > dense_vertex_count) {
      dense_vertex_count = candidate;
    }
  }
  if (dense_vertex_count > 0U) {
    node_offsets = (uint32_t *)calloc(dense_vertex_count + 1U, sizeof(*node_offsets));
    if (node_offsets == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
  }
  if (hypergraph->incidence_count > 0U) {
    if (hypergraph->hyperedge_count == 0U || hypergraph->hyperedge_offsets == NULL ||
        hypergraph->hyperedge_nodes == NULL || node_offsets == NULL) {
      free(node_offsets);
      return GVM_ERR_INVALID_ARG;
    }
    node_hyperedges = (uint32_t *)calloc(hypergraph->incidence_count, sizeof(*node_hyperedges));
    cursor = (uint32_t *)calloc(dense_vertex_count, sizeof(*cursor));
    if (node_hyperedges == NULL || cursor == NULL) {
      free(node_offsets);
      free(node_hyperedges);
      free(cursor);
      return GVM_ERR_INVALID_ARG;
    }
    for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
      size_t j;
      const size_t start = hypergraph->hyperedge_offsets[i];
      const size_t end = hypergraph->hyperedge_offsets[i + 1U];
      for (j = start; j < end; ++j) {
        const uint32_t vertex_id = hypergraph->hyperedge_nodes[j];
        if ((size_t)vertex_id >= dense_vertex_count) {
          free(node_offsets);
          free(node_hyperedges);
          free(cursor);
          return GVM_ERR_INVALID_ARG;
        }
        node_offsets[(size_t)vertex_id + 1U] += 1U;
      }
    }
    for (i = 1U; i <= dense_vertex_count; ++i) {
      node_offsets[i] += node_offsets[i - 1U];
    }
    memcpy(cursor, node_offsets, dense_vertex_count * sizeof(*cursor));
    for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
      size_t j;
      const size_t start = hypergraph->hyperedge_offsets[i];
      const size_t end = hypergraph->hyperedge_offsets[i + 1U];
      for (j = start; j < end; ++j) {
        const uint32_t vertex_id = hypergraph->hyperedge_nodes[j];
        node_hyperedges[cursor[vertex_id]++] = (uint32_t)i;
      }
    }
    free(cursor);
  }
  free((void *)hypergraph->node_offsets);
  free((void *)hypergraph->node_hyperedges);
  hypergraph->node_count = dense_vertex_count;
  hypergraph->node_offsets = node_offsets;
  hypergraph->node_hyperedges = node_hyperedges;
  return GVM_OK;
}

static int hypergraph_add_vertex_id(graphion_hypergraph_value *hypergraph, uint32_t id, const char *name) {
  graphion_graph_node_value *vertices;
  graphion_vm_value *attrs;

  if (hypergraph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (hypergraph_vertex_id_exists(hypergraph, id)) {
    return GVM_OK;
  }
  vertices = (graphion_graph_node_value *)realloc(hypergraph->vertices,
                                                  (hypergraph->vertex_count + 1U) * sizeof(*vertices));
  if (vertices == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph->vertices = vertices;
  hypergraph->vertices[hypergraph->vertex_count].id = id;
  hypergraph->vertices[hypergraph->vertex_count].name = NULL;
  if (name != NULL) {
    hypergraph->vertices[hypergraph->vertex_count].name = graph_strdup_text(name);
    if (hypergraph->vertices[hypergraph->vertex_count].name == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
  }
  hypergraph->vertex_count += 1U;
  if ((size_t)id >= hypergraph->vertex_attr_count) {
    size_t i;
    attrs = (graphion_vm_value *)realloc(hypergraph->vertex_attrs, ((size_t)id + 1U) * sizeof(*attrs));
    if (attrs == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    hypergraph->vertex_attrs = attrs;
    for (i = hypergraph->vertex_attr_count; i <= (size_t)id; ++i) {
      memset(&hypergraph->vertex_attrs[i], 0, sizeof(hypergraph->vertex_attrs[i]));
      hypergraph->vertex_attrs[i].kind = GVM_VALUE_NONE;
    }
    hypergraph->vertex_attr_count = (size_t)id + 1U;
  }
  return hypergraph_rebuild_incidence(hypergraph);
}

static int hypergraph_vertex_id_from_value_or_add(graphion_hypergraph_value *hypergraph,
                                                  const graphion_vm_value *value,
                                                  uint32_t *id_out) {
  uint32_t id;
  uint32_t converted_id = 0U;
  int rc;

  if (hypergraph == NULL || value == NULL || id_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  rc = hypergraph_vertex_id_from_value(hypergraph, value, &converted_id);
  if (rc == GVM_OK) {
    *id_out = converted_id;
    return GVM_OK;
  }
  if (rc != GVM_ERR_INVALID_NODE_ID) {
    return rc;
  }
  if (value->kind == GVM_VALUE_INT) {
    if (value->as.int_value < 0 || value->as.int_value > UINT32_MAX) {
      return GVM_ERR_INVALID_NODE_ID;
    }
    id = (uint32_t)value->as.int_value;
    rc = hypergraph_add_vertex_id(hypergraph, id, NULL);
    if (rc != GVM_OK) {
      return rc;
    }
    *id_out = id;
    return GVM_OK;
  }
  if (value->kind == GVM_VALUE_STRING) {
    const char *name = value->as.string_value != NULL ? value->as.string_value : "";
    rc = hypergraph_next_free_vertex_id(hypergraph, &id);
    if (rc != GVM_OK) {
      return rc;
    }
    rc = hypergraph_add_vertex_id(hypergraph, id, name);
    if (rc != GVM_OK) {
      return rc;
    }
    *id_out = id;
    return GVM_OK;
  }
  return GVM_ERR_TYPE_MISMATCH;
}

static const graphion_vm_value *hypergraph_first_vertex_attrs(const graphion_hypergraph_value *hypergraph) {
  size_t i;

  if (hypergraph == NULL || hypergraph->vertex_attrs == NULL) {
    return NULL;
  }
  for (i = 0U; i < hypergraph->vertex_attr_count; ++i) {
    if (hypergraph->vertex_attrs[i].kind == GVM_VALUE_DICT) {
      return &hypergraph->vertex_attrs[i];
    }
  }
  return NULL;
}

static const graphion_vm_value *hypergraph_first_hyperedge_attrs(const graphion_hypergraph_value *hypergraph) {
  size_t i;

  if (hypergraph == NULL || hypergraph->hyperedge_attrs == NULL) {
    return NULL;
  }
  for (i = 0U; i < hypergraph->hyperedge_attr_count; ++i) {
    if (hypergraph->hyperedge_attrs[i].kind == GVM_VALUE_DICT) {
      return &hypergraph->hyperedge_attrs[i];
    }
  }
  return NULL;
}

static int hypergraph_apply_vertex_defaults(graphion_hypergraph_value *hypergraph,
                                            uint32_t vertex_id,
                                            const graphion_vm_value *defaults) {
  if (hypergraph == NULL || defaults == NULL || defaults->kind == GVM_VALUE_NONE) {
    return GVM_OK;
  }
  if (defaults->kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if ((size_t)vertex_id >= hypergraph->vertex_attr_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  if (hypergraph->vertex_attrs[vertex_id].kind == GVM_VALUE_NONE) {
    return graph_replace_attr_value(&hypergraph->vertex_attrs[vertex_id], defaults);
  }
  return GVM_OK;
}

static int hypergraph_ensure_vertex_attr_capacity(graphion_hypergraph_value *hypergraph, uint32_t vertex_id) {
  graphion_vm_value *attrs;
  size_t i;

  if (hypergraph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if ((size_t)vertex_id < hypergraph->vertex_attr_count) {
    return GVM_OK;
  }
  attrs = (graphion_vm_value *)realloc(hypergraph->vertex_attrs, ((size_t)vertex_id + 1U) * sizeof(*attrs));
  if (attrs == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph->vertex_attrs = attrs;
  for (i = hypergraph->vertex_attr_count; i <= (size_t)vertex_id; ++i) {
    memset(&hypergraph->vertex_attrs[i], 0, sizeof(hypergraph->vertex_attrs[i]));
    hypergraph->vertex_attrs[i].kind = GVM_VALUE_NONE;
  }
  hypergraph->vertex_attr_count = (size_t)vertex_id + 1U;
  return GVM_OK;
}

static int hypergraph_ensure_hyperedge_attr_capacity(graphion_hypergraph_value *hypergraph) {
  graphion_vm_value *attrs;
  size_t i;

  if (hypergraph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (hypergraph->hyperedge_attr_count >= hypergraph->hypergraph.hyperedge_count) {
    return GVM_OK;
  }
  attrs = (graphion_vm_value *)realloc(hypergraph->hyperedge_attrs,
                                       hypergraph->hypergraph.hyperedge_count * sizeof(*attrs));
  if (attrs == NULL && hypergraph->hypergraph.hyperedge_count > 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph->hyperedge_attrs = attrs;
  for (i = hypergraph->hyperedge_attr_count; i < hypergraph->hypergraph.hyperedge_count; ++i) {
    memset(&hypergraph->hyperedge_attrs[i], 0, sizeof(hypergraph->hyperedge_attrs[i]));
    hypergraph->hyperedge_attrs[i].kind = GVM_VALUE_NONE;
  }
  hypergraph->hyperedge_attr_count = hypergraph->hypergraph.hyperedge_count;
  return GVM_OK;
}

static int hypergraph_append_hyperedge(graphion_hypergraph_value *value,
                                       const uint32_t *vertices,
                                       size_t vertex_count,
                                       const graphion_vm_value *defaults) {
  graphion_hypergraph *hypergraph;
  uint32_t *offsets;
  uint32_t *nodes;
  size_t old_hyperedge_count;
  size_t old_incidence_count;
  size_t edge_index;
  int rc;

  if (value == NULL || vertices == NULL || vertex_count == 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph = &value->hypergraph;
  old_hyperedge_count = hypergraph->hyperedge_count;
  old_incidence_count = hypergraph->incidence_count;
  offsets = (uint32_t *)calloc(old_hyperedge_count + 2U, sizeof(*offsets));
  nodes = (uint32_t *)calloc(old_incidence_count + vertex_count, sizeof(*nodes));
  if (offsets == NULL || nodes == NULL) {
    free(offsets);
    free(nodes);
    return GVM_ERR_INVALID_ARG;
  }
  if (hypergraph->hyperedge_offsets != NULL && old_hyperedge_count > 0U) {
    memcpy(offsets, hypergraph->hyperedge_offsets, (old_hyperedge_count + 1U) * sizeof(*offsets));
  } else {
    offsets[0] = 0U;
  }
  if (hypergraph->hyperedge_nodes != NULL && old_incidence_count > 0U) {
    memcpy(nodes, hypergraph->hyperedge_nodes, old_incidence_count * sizeof(*nodes));
  }
  memcpy(&nodes[old_incidence_count], vertices, vertex_count * sizeof(*nodes));
  offsets[old_hyperedge_count + 1U] = (uint32_t)(old_incidence_count + vertex_count);
  free((void *)hypergraph->hyperedge_offsets);
  free((void *)hypergraph->hyperedge_nodes);
  hypergraph->hyperedge_offsets = offsets;
  hypergraph->hyperedge_nodes = nodes;
  hypergraph->hyperedge_count = old_hyperedge_count + 1U;
  hypergraph->incidence_count = old_incidence_count + vertex_count;
  edge_index = old_hyperedge_count;
  rc = hypergraph_rebuild_incidence(value);
  if (rc == GVM_OK && defaults != NULL && defaults->kind != GVM_VALUE_NONE) {
    rc = hypergraph_ensure_hyperedge_attr_capacity(value);
    if (rc == GVM_OK) {
      rc = graph_replace_attr_value(&value->hyperedge_attrs[edge_index], defaults);
    }
  }
  return rc;
}

static int hypergraph_rewrite_hyperedges(graphion_hypergraph_value *value,
                                         const uint32_t *nodes,
                                         const uint32_t *sizes,
                                         size_t hyperedge_count,
                                         size_t incidence_count) {
  graphion_hypergraph *hypergraph;
  uint32_t *offsets = NULL;
  uint32_t *new_nodes = NULL;
  size_t i;
  size_t write_index = 0U;

  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph = &value->hypergraph;
  if (hyperedge_count > 0U) {
    offsets = (uint32_t *)calloc(hyperedge_count + 1U, sizeof(*offsets));
    if (offsets == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
  }
  if (incidence_count > 0U) {
    new_nodes = (uint32_t *)calloc(incidence_count, sizeof(*new_nodes));
    if (new_nodes == NULL) {
      free(offsets);
      return GVM_ERR_INVALID_ARG;
    }
  }
  for (i = 0U; i < hyperedge_count; ++i) {
    offsets[i] = (uint32_t)write_index;
    if (sizes != NULL && sizes[i] > 0U) {
      memcpy(&new_nodes[write_index], &nodes[write_index], (size_t)sizes[i] * sizeof(*new_nodes));
      write_index += (size_t)sizes[i];
    }
  }
  if (offsets != NULL) {
    offsets[hyperedge_count] = (uint32_t)write_index;
  }
  free((void *)hypergraph->hyperedge_offsets);
  free((void *)hypergraph->hyperedge_nodes);
  hypergraph->hyperedge_offsets = offsets;
  hypergraph->hyperedge_nodes = new_nodes;
  hypergraph->hyperedge_count = hyperedge_count;
  hypergraph->incidence_count = incidence_count;
  return hypergraph_rebuild_incidence(value);
}

static int hypergraph_make_hyperedge_empty(graphion_hypergraph_value *value, size_t hyperedge_id) {
  graphion_hypergraph *hypergraph;
  uint32_t *nodes = NULL;
  uint32_t *sizes = NULL;
  size_t i;
  size_t write_index = 0U;
  size_t incidence_count = 0U;
  int rc;

  if (value == NULL || hyperedge_id >= value->hypergraph.hyperedge_count) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph = &value->hypergraph;
  sizes = (uint32_t *)calloc(hypergraph->hyperedge_count, sizeof(*sizes));
  if (sizes == NULL && hypergraph->hyperedge_count > 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    const size_t start = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i] : 0U;
    const size_t end = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i + 1U] : 0U;
    sizes[i] = i == hyperedge_id ? 0U : (uint32_t)(end - start);
    incidence_count += sizes[i];
  }
  if (incidence_count > 0U) {
    nodes = (uint32_t *)calloc(incidence_count, sizeof(*nodes));
    if (nodes == NULL) {
      free(sizes);
      return GVM_ERR_INVALID_ARG;
    }
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    size_t j;
    const size_t start = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i] : 0U;
    const size_t end = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i + 1U] : 0U;
    if (i == hyperedge_id) {
      continue;
    }
    for (j = start; j < end; ++j) {
      nodes[write_index++] = hypergraph->hyperedge_nodes[j];
    }
  }
  rc = hypergraph_rewrite_hyperedges(value, nodes, sizes, hypergraph->hyperedge_count, incidence_count);
  free(nodes);
  free(sizes);
  return rc;
}

static int hypergraph_remove_vertex_id(graphion_hypergraph_value *value, uint32_t vertex_id) {
  graphion_hypergraph *hypergraph;
  uint32_t *nodes = NULL;
  uint32_t *sizes = NULL;
  size_t vertex_index = SIZE_MAX;
  size_t incidence_count = 0U;
  size_t write_index = 0U;
  size_t i;
  int rc;

  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph = &value->hypergraph;
  for (i = 0U; i < value->vertex_count; ++i) {
    if (value->vertices[i].id == vertex_id) {
      vertex_index = i;
      break;
    }
  }
  if (vertex_index == SIZE_MAX) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  sizes = (uint32_t *)calloc(hypergraph->hyperedge_count, sizeof(*sizes));
  if (sizes == NULL && hypergraph->hyperedge_count > 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    size_t j;
    const size_t start = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i] : 0U;
    const size_t end = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i + 1U] : 0U;
    for (j = start; j < end; ++j) {
      if (hypergraph->hyperedge_nodes[j] != vertex_id) {
        sizes[i] += 1U;
        incidence_count += 1U;
      }
    }
  }
  if (incidence_count > 0U) {
    nodes = (uint32_t *)calloc(incidence_count, sizeof(*nodes));
    if (nodes == NULL) {
      free(sizes);
      return GVM_ERR_INVALID_ARG;
    }
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    size_t j;
    const size_t start = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i] : 0U;
    const size_t end = hypergraph->hyperedge_offsets != NULL ? hypergraph->hyperedge_offsets[i + 1U] : 0U;
    for (j = start; j < end; ++j) {
      if (hypergraph->hyperedge_nodes[j] != vertex_id) {
        nodes[write_index++] = hypergraph->hyperedge_nodes[j];
      }
    }
    if (i < value->hyperedge_attr_count && sizes[i] == 0U) {
      vm_value_dispose_owned(&value->hyperedge_attrs[i]);
      value->hyperedge_attrs[i].kind = GVM_VALUE_NONE;
    }
  }
  free((void *)value->vertices[vertex_index].name);
  if (vertex_index + 1U < value->vertex_count) {
    memmove(&value->vertices[vertex_index],
            &value->vertices[vertex_index + 1U],
            (value->vertex_count - vertex_index - 1U) * sizeof(*value->vertices));
  }
  value->vertex_count -= 1U;
  if (value->vertex_count == 0U) {
    free(value->vertices);
    value->vertices = NULL;
  } else {
    graphion_graph_node_value *vertices =
        (graphion_graph_node_value *)realloc(value->vertices, value->vertex_count * sizeof(*vertices));
    if (vertices != NULL) {
      value->vertices = vertices;
    }
  }
  if ((size_t)vertex_id < value->vertex_attr_count) {
    vm_value_dispose_owned(&value->vertex_attrs[vertex_id]);
    value->vertex_attrs[vertex_id].kind = GVM_VALUE_NONE;
  }
  rc = hypergraph_rewrite_hyperedges(value, nodes, sizes, hypergraph->hyperedge_count, incidence_count);
  free(nodes);
  free(sizes);
  return rc;
}

static int graph_ensure_edge_attr_capacity(graphion_graph_value *graph) {
  graphion_vm_value *attrs;
  size_t i;

  if (graph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (graph->edge_attr_count >= graph->edge_count) {
    return GVM_OK;
  }
  attrs = (graphion_vm_value *)realloc(graph->edge_attrs, graph->edge_count * sizeof(*attrs));
  if (attrs == NULL && graph->edge_count > 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  graph->edge_attrs = attrs;
  for (i = graph->edge_attr_count; i < graph->edge_count; ++i) {
    memset(&graph->edge_attrs[i], 0, sizeof(graph->edge_attrs[i]));
    graph->edge_attrs[i].kind = GVM_VALUE_NONE;
  }
  graph->edge_attr_count = graph->edge_count;
  return GVM_OK;
}

static int graph_delete_edge_at(graphion_graph_value *graph, size_t edge_index) {
  if (graph == NULL || edge_index >= graph->edge_count) {
    return GVM_ERR_INVALID_ARG;
  }
  if (graph->edge_attrs != NULL && edge_index < graph->edge_attr_count) {
    vm_value_dispose_owned(&graph->edge_attrs[edge_index]);
    if (edge_index + 1U < graph->edge_attr_count) {
      memmove(&graph->edge_attrs[edge_index],
              &graph->edge_attrs[edge_index + 1U],
              (graph->edge_attr_count - edge_index - 1U) * sizeof(*graph->edge_attrs));
    }
    graph->edge_attr_count -= 1U;
    if (graph->edge_attr_count < graph->edge_count) {
      memset(&graph->edge_attrs[graph->edge_attr_count], 0, sizeof(*graph->edge_attrs));
      graph->edge_attrs[graph->edge_attr_count].kind = GVM_VALUE_NONE;
    }
  }
  if (edge_index + 1U < graph->edge_count) {
    memmove(&graph->edges[edge_index],
            &graph->edges[edge_index + 1U],
            (graph->edge_count - edge_index - 1U) * sizeof(*graph->edges));
  }
  graph->edge_count -= 1U;
  return graph_rebuild_adjacency(graph);
}

static int graph_remove_edge_direction(graphion_graph_value *graph, uint32_t from, uint32_t to) {
  size_t i;

  if (graph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  for (i = 0U; i < graph->edge_count; ++i) {
    graphion_graph_edge_value *edge = &graph->edges[i];
    if (edge->from == from && edge->to == to) {
      if (edge->directed && edge->bidirectional) {
        edge->from = to;
        edge->to = from;
        edge->bidirectional = 0U;
        return graph_rebuild_adjacency(graph);
      }
      return graph_delete_edge_at(graph, i);
    }
    if (edge->bidirectional && edge->from == to && edge->to == from) {
      if (edge->directed) {
        edge->bidirectional = 0U;
        return graph_rebuild_adjacency(graph);
      }
      return graph_delete_edge_at(graph, i);
    }
  }
  return GVM_ERR_MISSING_KEY;
}

static int graph_remove_node_id(graphion_graph_value *graph, uint32_t node_id) {
  size_t node_index;
  size_t write_index;
  size_t i;
  int found = 0;

  if (graph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  node_index = 0U;
  for (i = 0U; i < graph->node_count; ++i) {
    if (graph->nodes[i].id == node_id) {
      node_index = i;
      found = 1;
      break;
    }
  }
  if (!found) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  free((void *)graph->nodes[node_index].name);
  if (node_index + 1U < graph->node_count) {
    memmove(&graph->nodes[node_index],
            &graph->nodes[node_index + 1U],
            (graph->node_count - node_index - 1U) * sizeof(*graph->nodes));
  }
  graph->node_count -= 1U;
  if (graph->node_attrs != NULL && (size_t)node_id < graph->node_attr_count) {
    vm_value_dispose_owned(&graph->node_attrs[node_id]);
  }
  write_index = 0U;
  for (i = 0U; i < graph->edge_count; ++i) {
    if (graph->edges[i].from == node_id || graph->edges[i].to == node_id) {
      if (graph->edge_attrs != NULL && i < graph->edge_attr_count) {
        vm_value_dispose_owned(&graph->edge_attrs[i]);
      }
      continue;
    }
    if (write_index != i) {
      graph->edges[write_index] = graph->edges[i];
      if (graph->edge_attrs != NULL && i < graph->edge_attr_count) {
        graph->edge_attrs[write_index] = graph->edge_attrs[i];
        memset(&graph->edge_attrs[i], 0, sizeof(*graph->edge_attrs));
        graph->edge_attrs[i].kind = GVM_VALUE_NONE;
      }
    }
    write_index++;
  }
  graph->edge_count = write_index;
  if (graph->edge_attrs != NULL) {
    graph->edge_attr_count = write_index;
  }
  return graph_rebuild_adjacency(graph);
}

int op_graph_node_count(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  size_t count;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  count = graph_visible_node_count(&vm->regs[in->a], graph);
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_int(&vm->regs[in->a], (int64_t)count);
  return GVM_OK;
}

int op_graph_edge_count(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  size_t count;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  count = graph->edge_count;
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_int(&vm->regs[in->a], (int64_t)count);
  return GVM_OK;
}

int op_graph_is_directed(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  int is_directed;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  is_directed = graph_has_directed_edges(graph);
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_bool(&vm->regs[in->a], is_directed);
  return GVM_OK;
}

int op_graph_is_weighted(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  int is_weighted;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  is_weighted = graph_has_weighted_edges(graph);
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_bool(&vm->regs[in->a], is_weighted);
  return GVM_OK;
}

int op_graph_orientation(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  const char *orientation;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  if (graph->edge_count == 0U) {
    orientation = "empty";
  } else if (graph_has_directed_edges(graph)) {
    orientation = "directed";
  } else {
    orientation = "undirected";
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  return vm_reg_set_string_copy(vm, in->a, orientation);
}

int op_graph_node_attrs(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  uint32_t node_id;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &node_id);
  if (rc != GVM_OK) {
    return rc;
  }
  if (graph->node_attrs == NULL || (size_t)node_id >= graph->node_attr_count ||
      graph->node_attrs[node_id].kind == GVM_VALUE_NONE) {
    vm_value_dispose_owned(&vm->regs[in->a]);
    return vm_reg_set_empty_dict(vm, in->a);
  }
  return graph_clone_result_into_target(vm, in->a, &graph->node_attrs[node_id]);
}

int op_graph_edge_attrs(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  uint32_t from;
  uint32_t to;
  size_t edge_index;
  int rc;

  if (!is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &from);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[(uint8_t)in->imm], &to);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_edge_index_from_ids(graph, from, to, &edge_index);
  if (rc != GVM_OK) {
    return rc;
  }
  if (graph->edge_attrs == NULL || edge_index >= graph->edge_attr_count ||
      graph->edge_attrs[edge_index].kind == GVM_VALUE_NONE) {
    vm_value_dispose_owned(&vm->regs[in->a]);
    return vm_reg_set_empty_dict(vm, in->a);
  }
  return graph_clone_result_into_target(vm, in->a, &graph->edge_attrs[edge_index]);
}

int op_graph_edge_weight(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  graphion_vm_value weight;
  uint32_t from;
  uint32_t to;
  size_t edge_index;
  int rc;

  if (!is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &from);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[(uint8_t)in->imm], &to);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_edge_index_from_ids(graph, from, to, &edge_index);
  if (rc != GVM_OK) {
    return rc;
  }
  if (graph->edge_attrs == NULL || edge_index >= graph->edge_attr_count ||
      graph->edge_attrs[edge_index].kind == GVM_VALUE_NONE) {
    return GVM_ERR_MISSING_KEY;
  }
  memset(&weight, 0, sizeof(weight));
  weight.kind = GVM_VALUE_NONE;
  rc = vm_value_dict_get_clone(&graph->edge_attrs[edge_index], "weight", &weight);
  if (rc != GVM_OK) {
    return rc;
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = weight;
  return GVM_OK;
}

int op_graph_has_node(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  uint32_t node_id;
  int exists;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &node_id);
  if (rc != GVM_OK && rc != GVM_ERR_INVALID_NODE_ID) {
    return rc;
  }
  exists = rc == GVM_OK ? 1 : 0;
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_bool(&vm->regs[in->a], exists);
  return GVM_OK;
}

int op_graph_has_edge(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  uint32_t from;
  uint32_t to;
  size_t edge_index;
  int exists;
  int rc;

  if (!is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &from);
  if (rc != GVM_OK && rc != GVM_ERR_INVALID_NODE_ID) {
    return rc;
  }
  if (rc == GVM_ERR_INVALID_NODE_ID) {
    vm_value_dispose_owned(&vm->regs[in->a]);
    vm_value_set_bool(&vm->regs[in->a], 0);
    return GVM_OK;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[(uint8_t)in->imm], &to);
  if (rc != GVM_OK && rc != GVM_ERR_INVALID_NODE_ID) {
    return rc;
  }
  if (rc == GVM_ERR_INVALID_NODE_ID) {
    vm_value_dispose_owned(&vm->regs[in->a]);
    vm_value_set_bool(&vm->regs[in->a], 0);
    return GVM_OK;
  }
  rc = graph_edge_index_from_ids(graph, from, to, &edge_index);
  if (rc != GVM_OK && rc != GVM_ERR_MISSING_KEY) {
    return rc;
  }
  exists = rc == GVM_OK ? 1 : 0;
  (void)edge_index;
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_bool(&vm->regs[in->a], exists);
  return GVM_OK;
}

int op_graph_neighbors(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  uint32_t *ids = NULL;
  uint32_t node_id;
  size_t count;
  size_t i;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &node_id);
  if (rc != GVM_OK) {
    return rc;
  }
  count = 0U;
  if (graph->edge_count > 0U) {
    ids = (uint32_t *)calloc(graph->edge_count, sizeof(*ids));
    if (ids == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    for (i = 0U; i < graph->edge_count; ++i) {
      uint32_t candidate = 0U;
      size_t j;
      int present = 0;

      if (graph->edges[i].from == node_id) {
        candidate = graph->edges[i].to;
      } else if (graph->edges[i].to == node_id) {
        candidate = graph->edges[i].from;
      } else {
        continue;
      }
      for (j = 0U; j < count; ++j) {
        if (ids[j] == candidate) {
          present = 1;
          break;
        }
      }
      if (!present) {
        ids[count++] = candidate;
      }
    }
  }

  vm_value_dispose_owned(&vm->regs[in->a]);
  rc = vm_reg_set_empty_list(vm, in->a);
  if (rc != GVM_OK) {
    free(ids);
    return rc;
  }
  for (i = 0U; i < count; ++i) {
    rc = vm_list_append_int(vm, in->a, (int64_t)ids[i]);
    if (rc != GVM_OK) {
      free(ids);
      return rc;
    }
  }
  free(ids);
  return GVM_OK;
}

static int graph_degree_builtin(graphion_vm *vm, const graphion_insn *in, int incoming) {
  const graphion_graph_value *graph;
  uint32_t *ids = NULL;
  uint32_t node_id;
  size_t count;
  size_t i;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &node_id);
  if (rc != GVM_OK) {
    return rc;
  }
  count = 0U;
  if (graph->edge_count > 0U) {
    ids = (uint32_t *)calloc(graph->edge_count, sizeof(*ids));
    if (ids == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    for (i = 0U; i < graph->edge_count; ++i) {
      const graphion_graph_edge_value *edge = &graph->edges[i];
      uint32_t candidate = 0U;
      size_t j;
      int include = 0;
      int present = 0;

      if (edge->directed && !edge->bidirectional) {
        if (incoming && edge->to == node_id) {
          candidate = edge->from;
          include = 1;
        } else if (!incoming && edge->from == node_id) {
          candidate = edge->to;
          include = 1;
        }
      } else if (edge->from == node_id) {
        candidate = edge->to;
        include = 1;
      } else if (edge->to == node_id) {
        candidate = edge->from;
        include = 1;
      }
      if (!include) {
        continue;
      }
      for (j = 0U; j < count; ++j) {
        if (ids[j] == candidate) {
          present = 1;
          break;
        }
      }
      if (!present) {
        ids[count++] = candidate;
      }
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  rc = vm_reg_set_empty_list(vm, in->a);
  if (rc != GVM_OK) {
    free(ids);
    return rc;
  }
  for (i = 0U; i < count; ++i) {
    rc = vm_list_append_int(vm, in->a, (int64_t)ids[i]);
    if (rc != GVM_OK) {
      free(ids);
      return rc;
    }
  }
  free(ids);
  return GVM_OK;
}

int op_graph_indegree(graphion_vm *vm, const graphion_insn *in) {
  return graph_degree_builtin(vm, in, 1);
}

int op_graph_outdegree(graphion_vm *vm, const graphion_insn *in) {
  return graph_degree_builtin(vm, in, 0);
}

static int graph_dict_set_int(graphion_vm_value *dict, const char *key, int64_t value) {
  graphion_vm_value item;

  memset(&item, 0, sizeof(item));
  item.kind = GVM_VALUE_NONE;
  vm_value_set_int(&item, value);
  return vm_value_dict_set_clone(dict, key, &item);
}

static int graph_dict_set_bool(graphion_vm_value *dict, const char *key, int value) {
  graphion_vm_value item;

  memset(&item, 0, sizeof(item));
  item.kind = GVM_VALUE_NONE;
  vm_value_set_bool(&item, value);
  return vm_value_dict_set_clone(dict, key, &item);
}

static int graph_list_append_int_value(graphion_vm_value *list, int64_t value) {
  graphion_vm_value item;

  memset(&item, 0, sizeof(item));
  item.kind = GVM_VALUE_NONE;
  vm_value_set_int(&item, value);
  return vm_value_list_append_clone(list, &item);
}

static int graph_dict_set_string(graphion_vm_value *dict, const char *key, const char *value) {
  graphion_vm temp;
  int rc;

  graphion_vm_init(&temp);
  rc = vm_reg_set_string_copy(&temp, 0U, value != NULL ? value : "");
  if (rc == GVM_OK) {
    rc = vm_value_dict_set_clone(dict, key, &temp.regs[0]);
  }
  graphion_vm_dispose(&temp);
  return rc;
}

int op_graph_node_ids(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  graphion_vm_value list;
  size_t i;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = 0U; i < graph->node_count; ++i) {
    rc = graph_list_append_int_value(&list, (int64_t)graph->nodes[i].id);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_graph_nodes(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  graphion_vm_value list;
  size_t i;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = 0U; i < graph->node_count; ++i) {
    graphion_vm_value node;
    memset(&node, 0, sizeof(node));
    node.kind = GVM_VALUE_NONE;
    rc = vm_value_set_empty_dict_value(&node);
    if (rc == GVM_OK) {
      rc = graph_dict_set_int(&node, "id", (int64_t)graph->nodes[i].id);
    }
    if (rc == GVM_OK && graph->nodes[i].name != NULL) {
      rc = graph_dict_set_string(&node, "name", graph->nodes[i].name);
    }
    if (rc == GVM_OK) {
      rc = vm_value_list_append_clone(&list, &node);
    }
    vm_value_dispose_owned(&node);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_graph_edges(graphion_vm *vm, const graphion_insn *in) {
  const graphion_graph_value *graph;
  graphion_vm_value list;
  size_t i;
  int rc;

  rc = graph_reg_value(vm, in, &graph);
  if (rc != GVM_OK) {
    return rc;
  }
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = 0U; i < graph->edge_count; ++i) {
    graphion_vm_value edge;
    memset(&edge, 0, sizeof(edge));
    edge.kind = GVM_VALUE_NONE;
    rc = vm_value_set_empty_dict_value(&edge);
    if (rc == GVM_OK) {
      rc = graph_dict_set_int(&edge, "from", (int64_t)graph->edges[i].from);
    }
    if (rc == GVM_OK) {
      rc = graph_dict_set_int(&edge, "to", (int64_t)graph->edges[i].to);
    }
    if (rc == GVM_OK) {
      rc = graph_dict_set_bool(&edge, "directed", graph->edges[i].directed ? 1 : 0);
    }
    if (rc == GVM_OK) {
      rc = graph_dict_set_bool(&edge, "bidirectional", graph->edges[i].bidirectional ? 1 : 0);
    }
    if (rc == GVM_OK) {
      rc = vm_value_list_append_clone(&list, &edge);
    }
    vm_value_dispose_owned(&edge);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_graph_add_node(graphion_vm *vm, const graphion_insn *in) {
  graphion_graph_value *graph;
  const graphion_vm_value *schema;
  graphion_vm_value defaults;
  uint32_t node_id;
  int rc;

  memset(&defaults, 0, sizeof(defaults));
  defaults.kind = GVM_VALUE_NONE;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graph = (graphion_graph_value *)vm->regs[in->a].as.ref_value;
  schema = graph_first_node_attrs(graph);
  if (schema != NULL) {
    rc = vm_value_clone(&defaults, schema);
    if (rc != GVM_OK) {
      return rc;
    }
  }
  rc = graph_node_id_from_value_or_add(graph, &vm->regs[in->b], &node_id);
  if (rc == GVM_OK) {
    rc = graph_apply_node_defaults(graph, node_id, &defaults);
  }
  vm_value_dispose_owned(&defaults);
  if (rc == GVM_OK) {
    graph_update_visible_counts(&vm->regs[in->a], graph);
  }
  return rc;
}

int op_graph_add_edge(graphion_vm *vm, const graphion_insn *in) {
  graphion_graph_edge_value *edges;
  graphion_graph_value *graph;
  const graphion_vm_value *node_schema;
  const graphion_vm_value *edge_schema;
  graphion_vm_value node_defaults;
  graphion_vm_value edge_defaults;
  uint32_t from;
  uint32_t to;
  size_t edge_index;
  int rc;

  memset(&node_defaults, 0, sizeof(node_defaults));
  node_defaults.kind = GVM_VALUE_NONE;
  memset(&edge_defaults, 0, sizeof(edge_defaults));
  edge_defaults.kind = GVM_VALUE_NONE;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graph = (graphion_graph_value *)vm->regs[in->a].as.ref_value;
  if (graph_has_directed_edges(graph)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  node_schema = graph_first_node_attrs(graph);
  if (node_schema != NULL) {
    rc = vm_value_clone(&node_defaults, node_schema);
    if (rc != GVM_OK) {
      return rc;
    }
  }
  edge_schema = graph_first_edge_attrs(graph);
  if (edge_schema != NULL) {
    rc = vm_value_clone(&edge_defaults, edge_schema);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&node_defaults);
      return rc;
    }
  }
  rc = graph_node_id_from_value_or_add(graph, &vm->regs[in->b], &from);
  if (rc != GVM_OK) {
    vm_value_dispose_owned(&node_defaults);
    vm_value_dispose_owned(&edge_defaults);
    return rc;
  }
  rc = graph_apply_node_defaults(graph, from, &node_defaults);
  if (rc != GVM_OK) {
    vm_value_dispose_owned(&node_defaults);
    vm_value_dispose_owned(&edge_defaults);
    return rc;
  }
  rc = graph_node_id_from_value_or_add(graph, &vm->regs[(uint8_t)in->imm], &to);
  if (rc != GVM_OK) {
    vm_value_dispose_owned(&node_defaults);
    vm_value_dispose_owned(&edge_defaults);
    return rc;
  }
  rc = graph_apply_node_defaults(graph, to, &node_defaults);
  if (rc != GVM_OK) {
    vm_value_dispose_owned(&node_defaults);
    vm_value_dispose_owned(&edge_defaults);
    return rc;
  }
  vm_value_dispose_owned(&node_defaults);
  rc = graph_edge_index_from_ids(graph, from, to, &edge_index);
  if (rc == GVM_OK) {
    vm_value_dispose_owned(&edge_defaults);
    return GVM_OK;
  }
  if (rc != GVM_ERR_MISSING_KEY) {
    vm_value_dispose_owned(&edge_defaults);
    return rc;
  }
  edges = (graphion_graph_edge_value *)realloc(graph->edges, (graph->edge_count + 1U) * sizeof(*edges));
  if (edges == NULL) {
    vm_value_dispose_owned(&edge_defaults);
    return GVM_ERR_INVALID_ARG;
  }
  graph->edges = edges;
  graph->edges[graph->edge_count].from = from;
  graph->edges[graph->edge_count].to = to;
  graph->edges[graph->edge_count].directed = 0U;
  graph->edges[graph->edge_count].bidirectional = 1U;
  graph->edges[graph->edge_count].reserved[0] = 0U;
  graph->edges[graph->edge_count].reserved[1] = 0U;
  edge_index = graph->edge_count;
  graph->edge_count += 1U;
  rc = graph_rebuild_adjacency(graph);
  if (rc == GVM_OK && edge_defaults.kind != GVM_VALUE_NONE) {
    rc = graph_ensure_edge_attr_capacity(graph);
    if (rc == GVM_OK) {
      rc = graph_replace_attr_value(&graph->edge_attrs[edge_index], &edge_defaults);
    }
  }
  vm_value_dispose_owned(&edge_defaults);
  if (rc == GVM_OK) {
    graph_update_visible_counts(&vm->regs[in->a], graph);
  }
  return rc;
}

int op_graph_set_node_attrs(graphion_vm *vm, const graphion_insn *in) {
  graphion_graph_value *graph;
  const graphion_vm_value *schema;
  graphion_vm_value *slot;
  uint32_t node_id;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (vm->regs[(uint8_t)in->imm].kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graph = (graphion_graph_value *)vm->regs[in->a].as.ref_value;
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &node_id);
  if (rc != GVM_OK) {
    return rc;
  }
  schema = graph_first_node_attrs(graph);
  if ((size_t)node_id >= graph->node_attr_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  slot = &graph->node_attrs[node_id];
  if (slot->kind == GVM_VALUE_DICT) {
    if (schema != NULL && !vm_value_dict_keys_subset(&vm->regs[(uint8_t)in->imm], schema)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    return vm_value_dict_patch_existing(slot, &vm->regs[(uint8_t)in->imm]);
  }
  if (slot->kind != GVM_VALUE_NONE) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (schema != NULL && !vm_value_dict_keys_equal(schema, &vm->regs[(uint8_t)in->imm])) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  return graph_replace_attr_value(slot, &vm->regs[(uint8_t)in->imm]);
}

int op_graph_set_edge_attrs(graphion_vm *vm, const graphion_insn *in) {
  graphion_graph_value *graph;
  const graphion_vm_value *schema;
  graphion_vm_value *slot;
  uint32_t from;
  uint32_t to;
  size_t edge_index;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm) ||
      !is_valid_reg(3U)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL ||
      vm->regs[3].kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (!graph_value_has_weight_if_present_valid(&vm->regs[3])) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graph = (graphion_graph_value *)vm->regs[in->a].as.ref_value;
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &from);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[(uint8_t)in->imm], &to);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_edge_index_from_ids(graph, from, to, &edge_index);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_ensure_edge_attr_capacity(graph);
  if (rc != GVM_OK) {
    return rc;
  }
  schema = graph_first_edge_attrs(graph);
  slot = &graph->edge_attrs[edge_index];
  if (slot->kind == GVM_VALUE_DICT) {
    if (schema != NULL && !vm_value_dict_keys_subset(&vm->regs[3], schema)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    return vm_value_dict_patch_existing(slot, &vm->regs[3]);
  }
  if (slot->kind != GVM_VALUE_NONE) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (schema != NULL && !vm_value_dict_keys_equal(schema, &vm->regs[3])) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  return graph_replace_attr_value(slot, &vm->regs[3]);
}

int op_graph_set_edge_weight(graphion_vm *vm, const graphion_insn *in) {
  graphion_graph_value *graph;
  const graphion_vm_value *schema;
  uint32_t from;
  uint32_t to;
  size_t edge_index;
  uint8_t kind = GVM_VALUE_NONE;
  int found = 0;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm) ||
      !is_valid_reg(3U)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL ||
      (vm->regs[3].kind != GVM_VALUE_INT && vm->regs[3].kind != GVM_VALUE_FLOAT)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graph = (graphion_graph_value *)vm->regs[in->a].as.ref_value;
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &from);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[(uint8_t)in->imm], &to);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_edge_index_from_ids(graph, from, to, &edge_index);
  if (rc != GVM_OK) {
    return rc;
  }
  schema = graph_first_edge_attrs(graph);
  if (schema != NULL && !vm_value_dict_key_kind(schema, "weight", &kind, &found)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (schema != NULL && !found) {
    return GVM_ERR_MISSING_KEY;
  }
  rc = graph_ensure_edge_attr_capacity(graph);
  if (rc != GVM_OK) {
    return rc;
  }
  if (schema != NULL && graph->edge_attrs[edge_index].kind != GVM_VALUE_DICT) {
    return GVM_ERR_MISSING_KEY;
  }
  if (graph->edge_attrs[edge_index].kind == GVM_VALUE_NONE) {
    rc = graph_make_empty_dict_value(&graph->edge_attrs[edge_index]);
    if (rc != GVM_OK) {
      return rc;
    }
  }
  return vm_value_dict_set_clone(&graph->edge_attrs[edge_index], "weight", &vm->regs[3]);
}

int op_graph_remove_node(graphion_vm *vm, const graphion_insn *in) {
  graphion_graph_value *graph;
  uint32_t node_id;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graph = (graphion_graph_value *)vm->regs[in->a].as.ref_value;
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &node_id);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_remove_node_id(graph, node_id);
  if (rc == GVM_OK) {
    graph_update_visible_counts(&vm->regs[in->a], graph);
  }
  return rc;
}

int op_graph_remove_edge(graphion_vm *vm, const graphion_insn *in) {
  graphion_graph_value *graph;
  uint32_t from;
  uint32_t to;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_GRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graph = (graphion_graph_value *)vm->regs[in->a].as.ref_value;
  rc = graph_node_id_from_value(graph, &vm->regs[in->b], &from);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_node_id_from_value(graph, &vm->regs[(uint8_t)in->imm], &to);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = graph_remove_edge_direction(graph, from, to);
  if (rc == GVM_OK) {
    graph_update_visible_counts(&vm->regs[in->a], graph);
  }
  return rc;
}

int op_hypergraph_hyperedge_vertices(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  const graphion_hypergraph *hypergraph;
  graphion_vm_value list;
  size_t hyperedge_id;
  size_t start;
  size_t end;
  size_t i;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  hypergraph = &value->hypergraph;
  rc = hypergraph_active_hyperedge_id_from_value(&vm->regs[in->b], hypergraph, &hyperedge_id);
  if (rc != GVM_OK) {
    return rc;
  }
  if (hypergraph->hyperedge_offsets == NULL || hypergraph->hyperedge_nodes == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  start = hypergraph->hyperedge_offsets[hyperedge_id];
  end = hypergraph->hyperedge_offsets[hyperedge_id + 1U];
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = start; i < end; ++i) {
    rc = graph_list_append_int_value(&list, (int64_t)hypergraph->hyperedge_nodes[i]);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_hypergraph_hyperedge_attrs(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  size_t hyperedge_id;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_active_hyperedge_id_from_value(&vm->regs[in->b], &value->hypergraph, &hyperedge_id);
  if (rc != GVM_OK) {
    return rc;
  }
  if (value->hyperedge_attrs == NULL || hyperedge_id >= value->hyperedge_attr_count ||
      value->hyperedge_attrs[hyperedge_id].kind == GVM_VALUE_NONE) {
    vm_value_dispose_owned(&vm->regs[in->a]);
    return vm_reg_set_empty_dict(vm, in->a);
  }
  return graph_clone_result_into_target(vm, in->a, &value->hyperedge_attrs[hyperedge_id]);
}

int op_hypergraph_vertex_count(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  size_t count;
  int rc;

  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  count = hypergraph_visible_vertex_count(&vm->regs[in->a], value);
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_int(&vm->regs[in->a], (int64_t)count);
  return GVM_OK;
}

int op_hypergraph_hyperedge_count(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  size_t count;
  int rc;

  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  count = hypergraph_active_hyperedge_count(&value->hypergraph);
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_int(&vm->regs[in->a], (int64_t)count);
  return GVM_OK;
}

int op_hypergraph_vertex_attr_count(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  size_t count;
  int rc;

  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  count = hypergraph_visible_attr_key_count(value->vertex_attrs, value->vertex_attr_count);
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_int(&vm->regs[in->a], (int64_t)count);
  return GVM_OK;
}

int op_hypergraph_hyperedge_attr_count(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  size_t count;
  int rc;

  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  count = hypergraph_visible_attr_key_count(value->hyperedge_attrs, value->hyperedge_attr_count);
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_int(&vm->regs[in->a], (int64_t)count);
  return GVM_OK;
}

int op_hypergraph_vertex_attrs(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  uint32_t vertex_id;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_vertex_id_from_value(value, &vm->regs[in->b], &vertex_id);
  if (rc != GVM_OK) {
    return rc;
  }
  if (value->vertex_attrs == NULL || (size_t)vertex_id >= value->vertex_attr_count ||
      value->vertex_attrs[vertex_id].kind == GVM_VALUE_NONE) {
    vm_value_dispose_owned(&vm->regs[in->a]);
    return vm_reg_set_empty_dict(vm, in->a);
  }
  return graph_clone_result_into_target(vm, in->a, &value->vertex_attrs[vertex_id]);
}

int op_hypergraph_has_vertex(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  uint32_t vertex_id;
  int exists;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_vertex_id_from_value(value, &vm->regs[in->b], &vertex_id);
  if (rc != GVM_OK && rc != GVM_ERR_INVALID_NODE_ID) {
    return rc;
  }
  exists = rc == GVM_OK ? 1 : 0;
  (void)vertex_id;
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_bool(&vm->regs[in->a], exists);
  return GVM_OK;
}

int op_hypergraph_has_hyperedge(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  size_t hyperedge_id;
  int exists;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_id_from_value(&vm->regs[in->b], value->hypergraph.hyperedge_count, &hyperedge_id);
  if (rc != GVM_OK && rc != GVM_ERR_INVALID_HYPEREDGE_ID) {
    return rc;
  }
  exists = rc == GVM_OK && hypergraph_hyperedge_id_is_active(&value->hypergraph, hyperedge_id) ? 1 : 0;
  (void)hyperedge_id;
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm_value_set_bool(&vm->regs[in->a], exists);
  return GVM_OK;
}

int op_hypergraph_incident_hyperedges(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  const graphion_hypergraph *hypergraph;
  graphion_vm_value list;
  uint32_t vertex_id;
  size_t start = 0U;
  size_t end = 0U;
  size_t i;
  int rc;

  if (!is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_vertex_id_from_value(value, &vm->regs[in->b], &vertex_id);
  if (rc != GVM_OK) {
    return rc;
  }
  hypergraph = &value->hypergraph;
  if (hypergraph->node_offsets != NULL && (size_t)vertex_id < hypergraph->node_count) {
    start = hypergraph->node_offsets[vertex_id];
    end = hypergraph->node_offsets[(size_t)vertex_id + 1U];
  }
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = start; i < end; ++i) {
    rc = graph_list_append_int_value(&list, (int64_t)hypergraph->node_hyperedges[i]);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_hypergraph_vertex_ids(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  graphion_vm_value list;
  size_t i;
  int rc;

  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = 0U; i < value->vertex_count; ++i) {
    rc = graph_list_append_int_value(&list, (int64_t)value->vertices[i].id);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_hypergraph_vertices(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  graphion_vm_value list;
  size_t i;
  int rc;

  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = 0U; i < value->vertex_count; ++i) {
    graphion_vm_value vertex;
    memset(&vertex, 0, sizeof(vertex));
    vertex.kind = GVM_VALUE_NONE;
    rc = vm_value_set_empty_dict_value(&vertex);
    if (rc == GVM_OK) {
      rc = graph_dict_set_int(&vertex, "id", (int64_t)value->vertices[i].id);
    }
    if (rc == GVM_OK && value->vertices[i].name != NULL) {
      rc = graph_dict_set_string(&vertex, "name", value->vertices[i].name);
    }
    if (rc == GVM_OK) {
      rc = vm_value_list_append_clone(&list, &vertex);
    }
    vm_value_dispose_owned(&vertex);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_hypergraph_hyperedges(graphion_vm *vm, const graphion_insn *in) {
  const graphion_hypergraph_value *value;
  const graphion_hypergraph *hypergraph;
  graphion_vm_value list;
  size_t i;
  int rc;

  rc = hypergraph_reg_value(vm, in, &value);
  if (rc != GVM_OK) {
    return rc;
  }
  hypergraph = &value->hypergraph;
  if (hypergraph->hyperedge_count > 0U &&
      (hypergraph->hyperedge_offsets == NULL || hypergraph->hyperedge_nodes == NULL)) {
    return GVM_ERR_INVALID_ARG;
  }
  memset(&list, 0, sizeof(list));
  list.kind = GVM_VALUE_NONE;
  rc = vm_value_set_empty_list_value(&list);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    graphion_vm_value edge;
    graphion_vm_value vertices;
    size_t j;
    const size_t start = hypergraph->hyperedge_offsets[i];
    const size_t end = hypergraph->hyperedge_offsets[i + 1U];

    if (start == end) {
      continue;
    }
    memset(&edge, 0, sizeof(edge));
    memset(&vertices, 0, sizeof(vertices));
    edge.kind = GVM_VALUE_NONE;
    vertices.kind = GVM_VALUE_NONE;
    rc = vm_value_set_empty_dict_value(&edge);
    if (rc == GVM_OK) {
      rc = graph_dict_set_int(&edge, "id", (int64_t)i);
    }
    if (rc == GVM_OK) {
      rc = vm_value_set_empty_list_value(&vertices);
    }
    for (j = start; rc == GVM_OK && j < end; ++j) {
      graphion_vm_value vertex_id;
      memset(&vertex_id, 0, sizeof(vertex_id));
      vertex_id.kind = GVM_VALUE_NONE;
      vm_value_set_int(&vertex_id, (int64_t)hypergraph->hyperedge_nodes[j]);
      rc = vm_value_list_append_clone(&vertices, &vertex_id);
    }
    if (rc == GVM_OK) {
      rc = vm_value_dict_set_clone(&edge, "vertices", &vertices);
    }
    if (rc == GVM_OK) {
      rc = vm_value_list_append_clone(&list, &edge);
    }
    vm_value_dispose_owned(&vertices);
    vm_value_dispose_owned(&edge);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&list);
      return rc;
    }
  }
  vm_value_dispose_owned(&vm->regs[in->a]);
  vm->regs[in->a] = list;
  return GVM_OK;
}

int op_hypergraph_add_vertex(graphion_vm *vm, const graphion_insn *in) {
  graphion_hypergraph_value *hypergraph;
  const graphion_vm_value *schema;
  graphion_vm_value defaults;
  uint32_t vertex_id;
  int rc;

  memset(&defaults, 0, sizeof(defaults));
  defaults.kind = GVM_VALUE_NONE;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_HYPERGRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  hypergraph = (graphion_hypergraph_value *)vm->regs[in->a].as.ref_value;
  schema = hypergraph_first_vertex_attrs(hypergraph);
  if (schema != NULL) {
    rc = vm_value_clone(&defaults, schema);
    if (rc != GVM_OK) {
      return rc;
    }
  }
  rc = hypergraph_vertex_id_from_value_or_add(hypergraph, &vm->regs[in->b], &vertex_id);
  if (rc == GVM_OK) {
    rc = hypergraph_apply_vertex_defaults(hypergraph, vertex_id, &defaults);
  }
  vm_value_dispose_owned(&defaults);
  if (rc == GVM_OK) {
    hypergraph_update_visible_counts(&vm->regs[in->a], hypergraph);
  }
  return rc;
}

int op_hypergraph_add_hyperedge(graphion_vm *vm, const graphion_insn *in) {
  graphion_hypergraph_value *hypergraph;
  const graphion_vm_value *vertex_schema;
  const graphion_vm_value *hyperedge_schema;
  graphion_vm_value vertex_defaults;
  graphion_vm_value hyperedge_defaults;
  uint32_t *vertices = NULL;
  size_t vertex_count = 0U;
  size_t i;
  int rc;

  memset(&vertex_defaults, 0, sizeof(vertex_defaults));
  vertex_defaults.kind = GVM_VALUE_NONE;
  memset(&hyperedge_defaults, 0, sizeof(hyperedge_defaults));
  hyperedge_defaults.kind = GVM_VALUE_NONE;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_HYPERGRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (!vm_value_list_length(&vm->regs[in->b], &vertex_count)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (vertex_count == 0U || vertex_count > SIZE_MAX / sizeof(*vertices)) {
    return GVM_ERR_INVALID_ARG;
  }
  vertices = (uint32_t *)calloc(vertex_count, sizeof(*vertices));
  if (vertices == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph = (graphion_hypergraph_value *)vm->regs[in->a].as.ref_value;
  vertex_schema = hypergraph_first_vertex_attrs(hypergraph);
  if (vertex_schema != NULL) {
    rc = vm_value_clone(&vertex_defaults, vertex_schema);
    if (rc != GVM_OK) {
      free(vertices);
      return rc;
    }
  }
  hyperedge_schema = hypergraph_first_hyperedge_attrs(hypergraph);
  if (hyperedge_schema != NULL) {
    rc = vm_value_clone(&hyperedge_defaults, hyperedge_schema);
    if (rc != GVM_OK) {
      free(vertices);
      vm_value_dispose_owned(&vertex_defaults);
      return rc;
    }
  }
  for (i = 0U; i < vertex_count; ++i) {
    graphion_vm_value vertex_value;
    uint32_t vertex_id = 0U;

    memset(&vertex_value, 0, sizeof(vertex_value));
    vertex_value.kind = GVM_VALUE_NONE;
    rc = vm_value_list_clone_item(&vm->regs[in->b], i, &vertex_value);
    if (rc != GVM_OK) {
      free(vertices);
      vm_value_dispose_owned(&vertex_defaults);
      vm_value_dispose_owned(&hyperedge_defaults);
      return rc;
    }
    rc = hypergraph_vertex_id_from_value_or_add(hypergraph, &vertex_value, &vertex_id);
    vm_value_dispose_owned(&vertex_value);
    if (rc != GVM_OK) {
      free(vertices);
      vm_value_dispose_owned(&vertex_defaults);
      vm_value_dispose_owned(&hyperedge_defaults);
      return rc;
    }
    rc = hypergraph_apply_vertex_defaults(hypergraph, vertex_id, &vertex_defaults);
    if (rc != GVM_OK) {
      free(vertices);
      vm_value_dispose_owned(&vertex_defaults);
      vm_value_dispose_owned(&hyperedge_defaults);
      return rc;
    }
    vertices[i] = vertex_id;
  }
  vm_value_dispose_owned(&vertex_defaults);
  rc = hypergraph_append_hyperedge(hypergraph, vertices, vertex_count, &hyperedge_defaults);
  free(vertices);
  vm_value_dispose_owned(&hyperedge_defaults);
  if (rc == GVM_OK) {
    hypergraph_update_visible_counts(&vm->regs[in->a], hypergraph);
  }
  return rc;
}

int op_hypergraph_set_vertex_attrs(graphion_vm *vm, const graphion_insn *in) {
  graphion_hypergraph_value *hypergraph;
  const graphion_vm_value *schema;
  graphion_vm_value *slot;
  uint32_t vertex_id;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_HYPERGRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (vm->regs[(uint8_t)in->imm].kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  hypergraph = (graphion_hypergraph_value *)vm->regs[in->a].as.ref_value;
  rc = hypergraph_vertex_id_from_value(hypergraph, &vm->regs[in->b], &vertex_id);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_ensure_vertex_attr_capacity(hypergraph, vertex_id);
  if (rc != GVM_OK) {
    return rc;
  }
  schema = hypergraph_first_vertex_attrs(hypergraph);
  slot = &hypergraph->vertex_attrs[vertex_id];
  if (slot->kind == GVM_VALUE_DICT) {
    if (schema != NULL && !vm_value_dict_keys_subset(&vm->regs[(uint8_t)in->imm], schema)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    return vm_value_dict_patch_existing(slot, &vm->regs[(uint8_t)in->imm]);
  }
  if (slot->kind != GVM_VALUE_NONE) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (schema != NULL && !vm_value_dict_keys_equal(schema, &vm->regs[(uint8_t)in->imm])) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  return graph_replace_attr_value(slot, &vm->regs[(uint8_t)in->imm]);
}

int op_hypergraph_set_hyperedge_attrs(graphion_vm *vm, const graphion_insn *in) {
  graphion_hypergraph_value *hypergraph;
  const graphion_vm_value *schema;
  graphion_vm_value *slot;
  size_t hyperedge_id;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b) || in->imm < 0 || !is_valid_reg((uint8_t)in->imm)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_HYPERGRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (vm->regs[(uint8_t)in->imm].kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  hypergraph = (graphion_hypergraph_value *)vm->regs[in->a].as.ref_value;
  rc = hypergraph_id_from_value(&vm->regs[in->b], hypergraph->hypergraph.hyperedge_count, &hyperedge_id);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_ensure_hyperedge_attr_capacity(hypergraph);
  if (rc != GVM_OK) {
    return rc;
  }
  schema = hypergraph_first_hyperedge_attrs(hypergraph);
  slot = &hypergraph->hyperedge_attrs[hyperedge_id];
  if (slot->kind == GVM_VALUE_DICT) {
    if (schema != NULL && !vm_value_dict_keys_subset(&vm->regs[(uint8_t)in->imm], schema)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    return vm_value_dict_patch_existing(slot, &vm->regs[(uint8_t)in->imm]);
  }
  if (slot->kind != GVM_VALUE_NONE) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (schema != NULL && !vm_value_dict_keys_equal(schema, &vm->regs[(uint8_t)in->imm])) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  return graph_replace_attr_value(slot, &vm->regs[(uint8_t)in->imm]);
}

int op_hypergraph_remove_vertex(graphion_vm *vm, const graphion_insn *in) {
  graphion_hypergraph_value *hypergraph;
  uint32_t vertex_id;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_HYPERGRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  hypergraph = (graphion_hypergraph_value *)vm->regs[in->a].as.ref_value;
  rc = hypergraph_vertex_id_from_value(hypergraph, &vm->regs[in->b], &vertex_id);
  if (rc != GVM_OK) {
    return rc;
  }
  rc = hypergraph_remove_vertex_id(hypergraph, vertex_id);
  if (rc == GVM_OK) {
    hypergraph_update_visible_counts(&vm->regs[in->a], hypergraph);
  }
  return rc;
}

int op_hypergraph_remove_hyperedge(graphion_vm *vm, const graphion_insn *in) {
  graphion_hypergraph_value *hypergraph;
  size_t hyperedge_id;
  int rc;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[in->a].kind != GVM_VALUE_HYPERGRAPH_REF || vm->regs[in->a].as.ref_value == NULL) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  hypergraph = (graphion_hypergraph_value *)vm->regs[in->a].as.ref_value;
  rc = hypergraph_active_hyperedge_id_from_value(&vm->regs[in->b], &hypergraph->hypergraph, &hyperedge_id);
  if (rc != GVM_OK) {
    return rc;
  }
  if (hyperedge_id < hypergraph->hyperedge_attr_count) {
    vm_value_dispose_owned(&hypergraph->hyperedge_attrs[hyperedge_id]);
    hypergraph->hyperedge_attrs[hyperedge_id].kind = GVM_VALUE_NONE;
  }
  return hypergraph_make_hyperedge_empty(hypergraph, hyperedge_id);
}

int op_factorial(graphion_vm *vm, const graphion_insn *in) {
  int64_t value;
  int64_t result;
  int64_t i;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_int(&vm->regs[in->a], &value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value < 0) {
    return GVM_ERR_FACTORIAL_DOMAIN;
  }

  result = 1;
  for (i = 2; i <= value; ++i) {
    result = wrap_mul_i64(result, i);
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_int(&vm->regs[in->a], result);
  return GVM_OK;
}
