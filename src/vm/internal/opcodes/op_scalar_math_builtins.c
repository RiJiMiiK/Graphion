/* SPDX-License-Identifier: MIT */

#include "vm/internal/opcodes/op_scalar.h"

#include <float.h>
#include <math.h>

#include "vm/internal/core/value.h"

int op_sqrt(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f < 0.0) {
    return GVM_ERR_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], sqrt(value_f));
  return GVM_OK;
}

int op_cbrt_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], cbrt(value_f));
  return GVM_OK;
}

int op_sin_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], sin(value_f));
  return GVM_OK;
}

int op_csc_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], 1.0 / sin(value_f));
  return GVM_OK;
}

int op_sec_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], 1.0 / cos(value_f));
  return GVM_OK;
}

int op_cot_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], 1.0 / tan(value_f));
  return GVM_OK;
}

int op_acsc_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f > -1.0 && value_f < 1.0) {
    return GVM_ERR_ACSC_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], asin(1.0 / value_f));
  return GVM_OK;
}

int op_asec_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f > -1.0 && value_f < 1.0) {
    return GVM_ERR_ASEC_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], acos(1.0 / value_f));
  return GVM_OK;
}

int op_acot_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], atan2(1.0, value_f));
  return GVM_OK;
}

int op_sech_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], 1.0 / cosh(value_f));
  return GVM_OK;
}

int op_csch_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], 1.0 / sinh(value_f));
  return GVM_OK;
}

int op_coth_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], 1.0 / tanh(value_f));
  return GVM_OK;
}

int op_sinh_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], sinh(value_f));
  return GVM_OK;
}

int op_asinh_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], asinh(value_f));
  return GVM_OK;
}

int op_acosh_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f < 1.0) {
    return GVM_ERR_ACOSH_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], acosh(value_f));
  return GVM_OK;
}

int op_cosh_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], cosh(value_f));
  return GVM_OK;
}

int op_tanh_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], tanh(value_f));
  return GVM_OK;
}

int op_atanh_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f <= -1.0 || value_f >= 1.0) {
    return GVM_ERR_ATANH_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], atanh(value_f));
  return GVM_OK;
}

int op_cos_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], cos(value_f));
  return GVM_OK;
}

int op_tan_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], tan(value_f));
  return GVM_OK;
}

int op_asin_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f < -1.0 || value_f > 1.0) {
    return GVM_ERR_ASIN_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], asin(value_f));
  return GVM_OK;
}

int op_acos_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f < -1.0 || value_f > 1.0) {
    return GVM_ERR_ACOS_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], acos(value_f));
  return GVM_OK;
}

int op_atan_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], atan(value_f));
  return GVM_OK;
}

int op_atan2_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t y_i;
  int64_t x_i;
  double y_f;
  double x_f;
  int y_is_float;
  int x_is_float;

  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &y_i, &y_f, &y_is_float) ||
      !vm_value_get_numeric(&vm->regs[in->b], &x_i, &x_f, &x_is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], atan2(y_f, x_f));
  return GVM_OK;
}

int op_hypot_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], hypot(x_f, y_f));
  return GVM_OK;
}

int op_copysign_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], copysign(x_f, y_f));
  return GVM_OK;
}

int op_degrees_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], value_f * (180.0 / 3.14159265358979323846));
  return GVM_OK;
}

int op_radians_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], value_f * (3.14159265358979323846 / 180.0));
  return GVM_OK;
}

int op_isnan_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_bool(&vm->regs[in->a], is_float != 0 && isnan(value_f));
  return GVM_OK;
}

int op_isinf_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_bool(&vm->regs[in->a], is_float != 0 && (value_f > DBL_MAX || value_f < -DBL_MAX));
  return GVM_OK;
}

int op_isfinite_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;
  int is_finite;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  is_finite = is_float == 0 || (value_f == value_f && value_f <= DBL_MAX && value_f >= -DBL_MAX);
  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_bool(&vm->regs[in->a], is_finite);
  return GVM_OK;
}

int op_exp(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], exp(value_f));
  return GVM_OK;
}

int op_exp2_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], exp2(value_f));
  return GVM_OK;
}

int op_expm1_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], expm1(value_f));
  return GVM_OK;
}

int op_log1p_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f <= -1.0) {
    return GVM_ERR_LOG1P_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], log1p(value_f));
  return GVM_OK;
}

int op_erf_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], erf(value_f));
  return GVM_OK;
}

int op_erfc_builtin(graphion_vm *vm, const graphion_insn *in) {
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
  vm_value_set_float(&vm->regs[in->a], erfc(value_f));
  return GVM_OK;
}

int op_gamma_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f <= 0.0 && value_f == trunc(value_f)) {
    return GVM_ERR_GAMMA_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], tgamma(value_f));
  return GVM_OK;
}

int op_lgamma_builtin(graphion_vm *vm, const graphion_insn *in) {
  int64_t value_i;
  double value_f;
  int is_float;

  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_get_numeric(&vm->regs[in->a], &value_i, &value_f, &is_float)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value_f <= 0.0 && value_f == trunc(value_f)) {
    return GVM_ERR_LGAMMA_DOMAIN;
  }

  vm_free_owned_reg_string(vm, in->a);
  vm_value_set_float(&vm->regs[in->a], lgamma(value_f));
  return GVM_OK;
}
