/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_valid_reg(uint8_t reg) { return reg < 16U ? 1 : 0; }

int64_t wrap_add_i64(int64_t lhs, int64_t rhs) {
  const uint64_t ulhs = (uint64_t)lhs;
  const uint64_t urhs = (uint64_t)rhs;
  return (int64_t)(ulhs + urhs);
}

int64_t wrap_sub_i64(int64_t lhs, int64_t rhs) {
  const uint64_t ulhs = (uint64_t)lhs;
  const uint64_t urhs = (uint64_t)rhs;
  return (int64_t)(ulhs - urhs);
}

int64_t wrap_mul_i64(int64_t lhs, int64_t rhs) {
  const uint64_t ulhs = (uint64_t)lhs;
  const uint64_t urhs = (uint64_t)rhs;
  return (int64_t)(ulhs * urhs);
}

static char *vm_strdup_text(const char *text) {
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

void vm_value_set_int(graphion_vm_value *value, int64_t int_value) {
  if (value == NULL) {
    return;
  }
  value->kind = GVM_VALUE_INT;
  value->as.int_value = int_value;
}

void vm_value_set_float(graphion_vm_value *value, double float_value) {
  if (value == NULL) {
    return;
  }
  if (float_value == 0.0) {
    float_value = 0.0;
  }
  value->kind = GVM_VALUE_FLOAT;
  value->as.float_value = float_value;
}

void vm_value_set_bool(graphion_vm_value *value, int bool_value) {
  if (value == NULL) {
    return;
  }
  value->kind = GVM_VALUE_BOOL;
  value->as.bool_value = bool_value != 0 ? 1 : 0;
}

void vm_value_set_bits(graphion_vm_value *value, uint64_t bits_value, uint8_t width) {
  if (value == NULL) {
    return;
  }
  value->kind = GVM_VALUE_BITS;
  value->reserved[0] = width;
  value->as.int_value = (int64_t)bits_value;
}

void vm_value_copy(graphion_vm_value *dst, const graphion_vm_value *src) {
  if (dst == NULL || src == NULL) {
    return;
  }
  *dst = *src;
}

int vm_value_get_int(const graphion_vm_value *value, int64_t *out_value) {
  if (value == NULL || out_value == NULL || value->kind != GVM_VALUE_INT) {
    return 0;
  }
  *out_value = value->as.int_value;
  return 1;
}

int vm_value_get_numeric(const graphion_vm_value *value,
                         int64_t *out_int,
                         double *out_float,
                         int *out_is_float) {
  if (value == NULL || out_int == NULL || out_float == NULL || out_is_float == NULL) {
    return 0;
  }
  switch (value->kind) {
    case GVM_VALUE_INT:
      *out_int = value->as.int_value;
      *out_float = (double)value->as.int_value;
      *out_is_float = 0;
      return 1;
    case GVM_VALUE_FLOAT:
      *out_int = 0;
      *out_float = value->as.float_value;
      *out_is_float = 1;
      return 1;
    default:
      return 0;
  }
}

uint8_t vm_value_get_bits_width(const graphion_vm_value *value) {
  if (value == NULL || value->kind != GVM_VALUE_BITS) {
    return 0U;
  }
  return value->reserved[0];
}

uint64_t vm_value_get_bits_payload(const graphion_vm_value *value) {
  if (value == NULL || value->kind != GVM_VALUE_BITS) {
    return 0U;
  }
  return (uint64_t)value->as.int_value;
}

size_t vm_write_bits_text(char *buffer, size_t buffer_size, const graphion_vm_value *value, int include_newline) {
  const uint8_t width = vm_value_get_bits_width(value);
  const uint64_t payload = vm_value_get_bits_payload(value);
  size_t i;
  size_t pos = 0U;

  if (buffer == NULL || buffer_size < (size_t)width + (include_newline ? 4U : 3U) || width == 0U) {
    return 0U;
  }
  buffer[pos++] = '0';
  buffer[pos++] = 'b';
  for (i = 0U; i < (size_t)width; ++i) {
    const size_t bit_index = (size_t)width - 1U - i;
    buffer[pos++] = ((payload >> bit_index) & 1U) != 0U ? '1' : '0';
  }
  if (include_newline) {
    buffer[pos++] = '\n';
  }
  return pos;
}

void vm_free_owned_reg_string(graphion_vm *vm, uint8_t reg) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  if (vm->owned_reg_strings[reg] != NULL) {
    free(vm->owned_reg_strings[reg]);
    vm->owned_reg_strings[reg] = NULL;
  }
}

void vm_release_all_reg_strings(graphion_vm *vm) {
  size_t i;
  if (vm == NULL) {
    return;
  }
  for (i = 0U; i < 16U; ++i) {
    vm_free_owned_reg_string(vm, (uint8_t)i);
  }
}

int vm_reg_set_string_copy(graphion_vm *vm, uint8_t reg, const char *text) {
  char *copy;
  if (vm == NULL || !is_valid_reg(reg) || text == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  copy = vm_strdup_text(text);
  if (copy == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->owned_reg_strings[reg] = copy;
  vm->regs[reg].kind = GVM_VALUE_STRING;
  vm->regs[reg].as.string_value = copy;
  return GVM_OK;
}

int vm_global_set_string_copy(graphion_vm *vm, size_t index, const char *text) {
  char *copy;
  if (vm == NULL || text == NULL || index >= vm->global_count) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm->global_string_owners == NULL) {
    vm->globals[index].kind = GVM_VALUE_STRING;
    vm->globals[index].as.string_value = text;
    return GVM_OK;
  }
  copy = vm_strdup_text(text);
  if (copy == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm->global_string_owners[index] != NULL) {
    free(vm->global_string_owners[index]);
  }
  vm->global_string_owners[index] = copy;
  vm->globals[index].kind = GVM_VALUE_STRING;
  vm->globals[index].as.string_value = copy;
  return GVM_OK;
}

int vm_write_bytes(FILE *output, const char *bytes, size_t len) {
  if (output == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (bytes == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (len == 0U) {
    return GVM_OK;
  }
  return fwrite(bytes, 1U, len, output) == len ? GVM_OK : GVM_ERR_OUTPUT_UNBOUND;
}

int vm_write_bytes_sink(const graphion_output_sink *sink, const char *bytes, size_t len) {
  if (sink == NULL || sink->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  return sink->write(sink->ctx, bytes, len);
}

int vm_file_output_write(void *ctx, const char *bytes, size_t len) {
  return vm_write_bytes((FILE *)ctx, bytes, len);
}

int vm_count_output_write(void *ctx, const char *bytes, size_t len) {
  uint64_t *byte_count = (uint64_t *)ctx;
  (void)bytes;
  if (byte_count == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  *byte_count += (uint64_t)len;
  return GVM_OK;
}

int vm_sink_is_counter(const graphion_output_sink *sink) {
  return sink != NULL && sink->write == vm_count_output_write;
}

size_t vm_write_i64_text(char *buffer, int64_t value) {
  uint64_t magnitude;
  size_t digits = 0U;
  size_t i = 0U;

  if (value < 0) {
    buffer[i++] = '-';
    magnitude = (uint64_t)(-(value + 1)) + 1U;
  } else {
    magnitude = (uint64_t)value;
  }

  do {
    buffer[i + digits] = (char)('0' + (magnitude % 10U));
    magnitude /= 10U;
    digits += 1U;
  } while (magnitude != 0U);

  {
    size_t start = i;
    size_t end = i + digits - 1U;
    while (start < end) {
      char tmp = buffer[start];
      buffer[start] = buffer[end];
      buffer[end] = tmp;
      start += 1U;
      end -= 1U;
    }
  }
  return i + digits;
}

int vm_value_text_len(const graphion_vm_value *value, size_t *len_out) {
  char buffer[64];
  int written;
  size_t len;
  if (value == NULL || len_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  switch (value->kind) {
    case GVM_VALUE_NONE:
      *len_out = 5U;
      return GVM_OK;
    case GVM_VALUE_INT:
      len = vm_write_i64_text(buffer, value->as.int_value);
      *len_out = len + 1U;
      return GVM_OK;
    case GVM_VALUE_FLOAT:
      written = snprintf(buffer, sizeof(buffer), "%g\n", value->as.float_value);
      if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      *len_out = (size_t)written;
      return GVM_OK;
    case GVM_VALUE_BOOL:
      *len_out = value->as.bool_value != 0 ? 5U : 6U;
      return GVM_OK;
    case GVM_VALUE_STRING:
      *len_out = value->as.string_value != NULL ? strlen(value->as.string_value) + 1U : 1U;
      return GVM_OK;
    case GVM_VALUE_BITS:
      *len_out = (size_t)vm_value_get_bits_width(value) + 3U;
      return GVM_OK;
    default:
      return GVM_ERR_TYPE_MISMATCH;
  }
}

int vm_write_value_sink(const graphion_output_sink *output, const graphion_vm_value *value) {
  char buffer[64];
  int written;
  size_t len;
  if (output == NULL || output->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm_sink_is_counter(output)) {
    uint64_t *byte_count = (uint64_t *)output->ctx;
    const int rc = vm_value_text_len(value, &len);
    if (rc != GVM_OK) {
      return rc;
    }
    if (byte_count == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    *byte_count += (uint64_t)len;
    return GVM_OK;
  }
  switch (value->kind) {
    case GVM_VALUE_NONE:
      return vm_write_bytes_sink(output, "none\n", 5U);
    case GVM_VALUE_INT:
      len = vm_write_i64_text(buffer, value->as.int_value);
      buffer[len++] = '\n';
      return vm_write_bytes_sink(output, buffer, len);
    case GVM_VALUE_FLOAT:
      written = snprintf(buffer, sizeof(buffer), "%g\n", value->as.float_value);
      if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, buffer, (size_t)written);
    case GVM_VALUE_BOOL:
      return value->as.bool_value != 0 ? vm_write_bytes_sink(output, "true\n", 5U)
                                       : vm_write_bytes_sink(output, "false\n", 6U);
    case GVM_VALUE_STRING:
      if (value->as.string_value == NULL) {
        return vm_write_bytes_sink(output, "\n", 1U);
      }
      len = strlen(value->as.string_value);
      if (vm_write_bytes_sink(output, value->as.string_value, len) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, "\n", 1U);
    case GVM_VALUE_BITS:
      len = vm_write_bits_text(buffer, sizeof(buffer), value, 1);
      if (len == 0U) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, buffer, len);
    default:
      return GVM_ERR_TYPE_MISMATCH;
  }
}

int vm_write_value_sink_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  char buffer[64];
  int written;
  size_t len;
  if (output == NULL || output->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm_sink_is_counter(output)) {
    if (value->kind == GVM_VALUE_STRING) {
      len = value->as.string_value != NULL ? strlen(value->as.string_value) : 0U;
      *((uint64_t *)output->ctx) += (uint64_t)len;
      return GVM_OK;
    }
    switch (value->kind) {
      case GVM_VALUE_NONE:
        *((uint64_t *)output->ctx) += 4U;
        return GVM_OK;
      case GVM_VALUE_INT:
        len = vm_write_i64_text(buffer, value->as.int_value);
        *((uint64_t *)output->ctx) += (uint64_t)len;
        return GVM_OK;
      case GVM_VALUE_FLOAT:
        written = snprintf(buffer, sizeof(buffer), "%g", value->as.float_value);
        if (written < 0 || (size_t)written >= sizeof(buffer)) {
          return GVM_ERR_OUTPUT_UNBOUND;
        }
        *((uint64_t *)output->ctx) += (uint64_t)written;
        return GVM_OK;
      case GVM_VALUE_BOOL:
        *((uint64_t *)output->ctx) += value->as.bool_value != 0 ? 4U : 5U;
        return GVM_OK;
      case GVM_VALUE_BITS:
        *((uint64_t *)output->ctx) += (uint64_t)vm_value_get_bits_width(value) + 2U;
        return GVM_OK;
      default:
        return GVM_ERR_TYPE_MISMATCH;
    }
  }
  switch (value->kind) {
    case GVM_VALUE_NONE:
      return vm_write_bytes_sink(output, "none", 4U);
    case GVM_VALUE_INT:
      len = vm_write_i64_text(buffer, value->as.int_value);
      return vm_write_bytes_sink(output, buffer, len);
    case GVM_VALUE_FLOAT:
      written = snprintf(buffer, sizeof(buffer), "%g", value->as.float_value);
      if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, buffer, (size_t)written);
    case GVM_VALUE_BOOL:
      return value->as.bool_value != 0 ? vm_write_bytes_sink(output, "true", 4U)
                                       : vm_write_bytes_sink(output, "false", 5U);
    case GVM_VALUE_STRING:
      if (value->as.string_value == NULL) {
        return GVM_OK;
      }
      len = strlen(value->as.string_value);
      return vm_write_bytes_sink(output, value->as.string_value, len);
    case GVM_VALUE_BITS:
      len = vm_write_bits_text(buffer, sizeof(buffer), value, 0);
      if (len == 0U) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, buffer, len);
    default:
      return GVM_ERR_TYPE_MISMATCH;
  }
}

int vm_reg_get_int(const graphion_vm *vm, uint8_t reg, int64_t *out_value) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return 0;
  }
  return vm_value_get_int(&vm->regs[reg], out_value);
}

void vm_reg_set_int(graphion_vm *vm, uint8_t reg, int64_t value) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  vm_value_set_int(&vm->regs[reg], value);
}

int vm_copy_regs_to_raw_i64(const graphion_vm *vm, int64_t raw_regs[16]) {
  size_t i;
  if (vm == NULL || raw_regs == NULL) {
    return 0;
  }
  for (i = 0U; i < 16U; ++i) {
    if (!vm_value_get_int(&vm->regs[i], &raw_regs[i])) {
      return 0;
    }
  }
  return 1;
}

void vm_copy_raw_i64_to_regs(graphion_vm *vm, const int64_t raw_regs[16]) {
  size_t i;
  if (vm == NULL || raw_regs == NULL) {
    return;
  }
  for (i = 0U; i < 16U; ++i) {
    vm_reg_set_int(vm, (uint8_t)i, raw_regs[i]);
  }
}
