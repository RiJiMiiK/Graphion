/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_TESTS_VM_HELPERS_H
#define GRAPHION_TESTS_VM_HELPERS_H

#include "vm/vm.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define TEST_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

void test_set_reg_i(graphion_vm *vm, uint8_t reg, int64_t value);
void test_set_value_int(graphion_vm_value *value, int64_t number);
void test_set_value_float(graphion_vm_value *value, double number);
void test_set_value_bool(graphion_vm_value *value, int boolean);
void test_set_value_string(graphion_vm_value *value, const char *text);
void test_set_value_bits(graphion_vm_value *value, uint64_t bits_value, uint8_t width);
int test_make_temp_path_vm(char *buffer, size_t capacity, const char *label);
FILE *test_open_temp_output_vm(char *path_buffer, size_t capacity, const char *label);
int run_vm_program(graphion_vm *vm, const graphion_insn *program, size_t len);
int finish_vm_test(graphion_vm *vm, int code);
int finish_vm_test_with_owned_globals(graphion_vm *vm, char **owners, size_t owner_count, int code);

#endif
