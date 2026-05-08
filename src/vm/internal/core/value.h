/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_VALUE_H
#define GRAPHION_VM_VALUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "vm/vm.h"

int is_valid_reg(uint8_t reg);

int64_t wrap_add_i64(int64_t lhs, int64_t rhs);
int64_t wrap_sub_i64(int64_t lhs, int64_t rhs);
int64_t wrap_mul_i64(int64_t lhs, int64_t rhs);

void vm_value_set_int(graphion_vm_value *value, int64_t int_value);
void vm_value_set_float(graphion_vm_value *value, double float_value);
void vm_value_set_bool(graphion_vm_value *value, int bool_value);
void vm_value_set_bits(graphion_vm_value *value, uint64_t bits_value, uint8_t width);
void vm_value_copy(graphion_vm_value *dst, const graphion_vm_value *src);
int vm_value_clone(graphion_vm_value *dst, const graphion_vm_value *src);
void vm_value_dispose_owned(graphion_vm_value *value);

int vm_value_get_int(const graphion_vm_value *value, int64_t *out_value);
int vm_value_get_numeric(const graphion_vm_value *value,
                         int64_t *out_int,
                         double *out_float,
                         int *out_is_float);
int vm_values_deep_equal(const graphion_vm_value *lhs,
                         const graphion_vm_value *rhs,
                         int *compatible_out,
                         int *equal_out);
int vm_value_list_length(const graphion_vm_value *value, size_t *len_out);
int vm_value_dict_length(const graphion_vm_value *value, size_t *len_out);
int vm_value_tuple_length(const graphion_vm_value *value, size_t *len_out);
int vm_value_set_length(const graphion_vm_value *value, size_t *len_out);
uint8_t vm_value_get_bits_width(const graphion_vm_value *value);
uint64_t vm_value_get_bits_payload(const graphion_vm_value *value);
size_t vm_write_bits_text(char *buffer,
                          size_t buffer_size,
                          const graphion_vm_value *value,
                          int include_newline);

void vm_free_owned_reg_string(graphion_vm *vm, uint8_t reg);
void vm_release_all_reg_strings(graphion_vm *vm);
void vm_release_global_value(graphion_vm *vm, size_t index);
int vm_reg_set_string_copy(graphion_vm *vm, uint8_t reg, const char *text);
int vm_global_set_string_copy(graphion_vm *vm, size_t index, const char *text);
int vm_reg_set_empty_list(graphion_vm *vm, uint8_t reg);
int vm_list_append_reg(graphion_vm *vm, uint8_t list_reg, uint8_t value_reg);
int vm_list_get_element(graphion_vm *vm, uint8_t list_reg, uint8_t index_reg);
int vm_reg_set_empty_tuple(graphion_vm *vm, uint8_t reg);
int vm_tuple_append_reg(graphion_vm *vm, uint8_t tuple_reg, uint8_t value_reg);
int vm_reg_set_empty_set(graphion_vm *vm, uint8_t reg);
int vm_set_add_reg(graphion_vm *vm, uint8_t set_reg, uint8_t value_reg);
int vm_set_contains_reg(graphion_vm *vm, uint8_t set_reg, uint8_t value_reg);
int vm_reg_set_empty_dict(graphion_vm *vm, uint8_t reg);
int vm_dict_set_reg(graphion_vm *vm, uint8_t dict_reg, const char *key, uint8_t value_reg);
int vm_dict_set_element(graphion_vm *vm, uint8_t dict_reg, uint8_t key_reg, uint8_t value_reg);
int vm_dict_get_element(graphion_vm *vm, uint8_t dict_reg, uint8_t key_reg);

int vm_write_bytes(FILE *output, const char *bytes, size_t len);
int vm_write_bytes_sink(const graphion_output_sink *sink, const char *bytes, size_t len);
int vm_file_output_write(void *ctx, const char *bytes, size_t len);
int vm_count_output_write(void *ctx, const char *bytes, size_t len);
int vm_sink_is_counter(const graphion_output_sink *sink);

size_t vm_write_i64_text(char *buffer, int64_t value);
int vm_value_text_len(const graphion_vm_value *value, size_t *len_out);
int vm_write_value_sink(const graphion_output_sink *output, const graphion_vm_value *value);
int vm_write_value_sink_inline(const graphion_output_sink *output, const graphion_vm_value *value);

int vm_reg_get_int(const graphion_vm *vm, uint8_t reg, int64_t *out_value);
void vm_reg_set_int(graphion_vm *vm, uint8_t reg, int64_t value);
int vm_copy_regs_to_raw_i64(const graphion_vm *vm, int64_t raw_regs[16]);
void vm_copy_raw_i64_to_regs(graphion_vm *vm, const int64_t raw_regs[16]);

#endif
