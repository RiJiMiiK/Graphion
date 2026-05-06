/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_vm_helpers.h"
#include "vm/internal/opcodes/op_scalar.h"

int test_vm_expm1_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_EXPM1;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_expm1_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.718281828 ||
        vm.regs[0].as.float_value > 1.718281829) {
        return 18151;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    rc = op_expm1_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18152;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 2;
    rc = op_expm1_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 6.389056098 ||
        vm.regs[0].as.float_value > 6.389056100) {
        return 18153;
    }

    return 0;
}

int test_vm_exp2_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_EXP2;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_exp2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.999999999 ||
        vm.regs[0].as.float_value > 2.000000001) {
        return 18160;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    rc = op_exp2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 1.0) {
        return 18161;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 2;
    rc = op_exp2_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 3.999999999 ||
        vm.regs[0].as.float_value > 4.000000001) {
        return 18162;
    }

    return 0;
}

int test_vm_log1p_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_LOG1P;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_log1p_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.693147180 ||
        vm.regs[0].as.float_value > 0.693147181) {
        return 18154;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.0;
    rc = op_log1p_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18155;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = -1.0;
    rc = op_log1p_builtin(&vm, &insn);
    if (rc != GVM_ERR_LOG1P_DOMAIN) {
        return 18156;
    }

    return 0;
}

int test_vm_erf_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    insn.op = GVM_OP_ERF;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_erf_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18157;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    rc = op_erf_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.842700792 ||
        vm.regs[0].as.float_value > 0.842700793) {
        return 18158;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = -1;
    rc = op_erf_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < -0.842700793 ||
        vm.regs[0].as.float_value > -0.842700792) {
        return 18159;
    }

    return 0;
}

int test_vm_erfc_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    insn.op = GVM_OP_ERFC;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_erfc_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 1.0) {
        return 18160;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    rc = op_erfc_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.157299207 ||
        vm.regs[0].as.float_value > 0.157299208) {
        return 18161;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = -1;
    rc = op_erfc_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.842700792 ||
        vm.regs[0].as.float_value > 1.842700793) {
        return 18162;
    }

    return 0;
}

int test_vm_gamma_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_GAMMA;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 1.0) {
        return 18163;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 5;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 24.0) {
        return 18164;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.5;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 1.772453849 ||
        vm.regs[0].as.float_value > 1.772453851) {
        return 18165;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    rc = op_gamma_builtin(&vm, &insn);
    if (rc != GVM_ERR_GAMMA_DOMAIN) {
        return 18166;
    }

    return 0;
}

int test_vm_lgamma_builtin_opcode(void) {
    graphion_vm vm;
    graphion_insn insn;
    int rc;

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 1;
    insn.op = GVM_OP_LGAMMA;
    insn.a = 0;
    insn.b = 0;
    insn.imm = 0;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value != 0.0) {
        return 18167;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 5;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 3.178053829 ||
        vm.regs[0].as.float_value > 3.178053831) {
        return 18168;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_FLOAT;
    vm.regs[0].as.float_value = 0.5;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_OK || vm.regs[0].kind != GVM_VALUE_FLOAT || vm.regs[0].as.float_value < 0.572364941 ||
        vm.regs[0].as.float_value > 0.572364943) {
        return 18169;
    }

    memset(&vm, 0, sizeof(vm));
    vm.regs[0].kind = GVM_VALUE_INT;
    vm.regs[0].as.int_value = 0;
    rc = op_lgamma_builtin(&vm, &insn);
    if (rc != GVM_ERR_LGAMMA_DOMAIN) {
        return 18170;
    }

    return 0;
}
