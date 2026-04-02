/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_scalar.h"

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
  int result = 0;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }

  lhs = &vm->regs[in->a];
  rhs = &vm->regs[in->b];

  if ((lhs->kind == GVM_VALUE_INT || lhs->kind == GVM_VALUE_FLOAT) &&
      (rhs->kind == GVM_VALUE_INT || rhs->kind == GVM_VALUE_FLOAT)) {
    int64_t lhs_i;
    int64_t rhs_i;
    double lhs_f;
    double rhs_f;
    int lhs_is_float;
    int rhs_is_float;
    if (!vm_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
        !vm_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    result = lhs_f == rhs_f;
  } else if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_INT) {
    if (rhs->as.int_value != 0 && rhs->as.int_value != 1) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    result = rhs->as.int_value == (int64_t)lhs->as.bool_value;
  } else if (lhs->kind == GVM_VALUE_INT && rhs->kind == GVM_VALUE_BOOL) {
    if (lhs->as.int_value != 0 && lhs->as.int_value != 1) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    result = lhs->as.int_value == (int64_t)rhs->as.bool_value;
  } else if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_BOOL) {
    result = lhs->as.bool_value == rhs->as.bool_value;
  } else if (lhs->kind == GVM_VALUE_BITS && rhs->kind == GVM_VALUE_BITS) {
    result = (uint64_t)lhs->as.int_value == (uint64_t)rhs->as.int_value;
  } else if (lhs->kind == GVM_VALUE_STRING && rhs->kind == GVM_VALUE_STRING) {
    const char *lhs_text = lhs->as.string_value != NULL ? lhs->as.string_value : "";
    const char *rhs_text = rhs->as.string_value != NULL ? rhs->as.string_value : "";
    result = strcmp(lhs_text, rhs_text) == 0;
  } else {
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
    return GVM_ERR_TYPE_MISMATCH;
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
    return GVM_ERR_TYPE_MISMATCH;
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
    return GVM_ERR_TYPE_MISMATCH;
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
    return GVM_ERR_TYPE_MISMATCH;
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
    return GVM_ERR_TYPE_MISMATCH;
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

