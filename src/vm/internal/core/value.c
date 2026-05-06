/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t count;
  size_t capacity;
  graphion_vm_value *items;
} graphion_vm_list;

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

static graphion_vm_list *vm_list_create(void) {
  graphion_vm_list *list = (graphion_vm_list *)calloc(1U, sizeof(*list));
  return list;
}

static void vm_value_clear(graphion_vm_value *value) {
  if (value == NULL) {
    return;
  }
  memset(value, 0, sizeof(*value));
  value->kind = GVM_VALUE_NONE;
}

void vm_value_set_int(graphion_vm_value *value, int64_t int_value) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
  value->kind = GVM_VALUE_INT;
  value->as.int_value = int_value;
}

void vm_value_set_float(graphion_vm_value *value, double float_value) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
  value->kind = GVM_VALUE_FLOAT;
  value->as.float_value = float_value;
}

void vm_value_set_bool(graphion_vm_value *value, int bool_value) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
  value->kind = GVM_VALUE_BOOL;
  value->as.bool_value = bool_value != 0 ? 1 : 0;
}

void vm_value_set_bits(graphion_vm_value *value, uint64_t bits_value, uint8_t width) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
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

static int vm_list_append_value(graphion_vm_list *list, const graphion_vm_value *src);

void vm_value_dispose_owned(graphion_vm_value *value) {
  size_t i;
  graphion_vm_list *list;

  if (value == NULL) {
    return;
  }
  if (value->kind == GVM_VALUE_STRING && value->as.string_value != NULL) {
    free((void *)value->as.string_value);
  } else if (value->kind == GVM_VALUE_LIST) {
    list = (graphion_vm_list *)value->as.ref_value;
    if (list != NULL) {
      for (i = 0U; i < list->count; ++i) {
        vm_value_dispose_owned(&list->items[i]);
      }
      free(list->items);
      free(list);
    }
  }
  vm_value_clear(value);
}

int vm_value_clone(graphion_vm_value *dst, const graphion_vm_value *src) {
  size_t i;
  graphion_vm_list *src_list;
  graphion_vm_list *dst_list;

  if (dst == NULL || src == NULL) {
    return GVM_ERR_INVALID_ARG;
  }

  vm_value_clear(dst);
  if (src->kind == GVM_VALUE_STRING) {
    char *copy = vm_strdup_text(src->as.string_value != NULL ? src->as.string_value : "");
    if (copy == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    dst->kind = GVM_VALUE_STRING;
    dst->as.string_value = copy;
    return GVM_OK;
  }
  if (src->kind != GVM_VALUE_LIST) {
    *dst = *src;
    return GVM_OK;
  }

  src_list = (graphion_vm_list *)src->as.ref_value;
  dst_list = vm_list_create();
  if (dst_list == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (src_list != NULL && src_list->count > 0U) {
    dst_list->items = (graphion_vm_value *)calloc(src_list->count, sizeof(*dst_list->items));
    if (dst_list->items == NULL) {
      free(dst_list);
      return GVM_ERR_INVALID_ARG;
    }
    dst_list->capacity = src_list->count;
    for (i = 0U; i < src_list->count; ++i) {
      int rc = vm_value_clone(&dst_list->items[i], &src_list->items[i]);
      if (rc != GVM_OK) {
        size_t j;
        for (j = 0U; j < i; ++j) {
          vm_value_dispose_owned(&dst_list->items[j]);
        }
        free(dst_list->items);
        free(dst_list);
        return rc;
      }
    }
    dst_list->count = src_list->count;
  }
  dst->kind = GVM_VALUE_LIST;
  dst->as.ref_value = dst_list;
  return GVM_OK;
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

int vm_value_list_length(const graphion_vm_value *value, size_t *len_out) {
  graphion_vm_list *list;
  if (value == NULL || len_out == NULL || value->kind != GVM_VALUE_LIST) {
    return 0;
  }
  list = (graphion_vm_list *)value->as.ref_value;
  *len_out = list != NULL ? list->count : 0U;
  return 1;
}

int vm_values_deep_equal(const graphion_vm_value *lhs,
                         const graphion_vm_value *rhs,
                         int *compatible_out,
                         int *equal_out) {
  size_t i;
  graphion_vm_list *lhs_list;
  graphion_vm_list *rhs_list;

  if (lhs == NULL || rhs == NULL || compatible_out == NULL || equal_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  *compatible_out = 1;
  *equal_out = 0;

  if ((lhs->kind == GVM_VALUE_INT || lhs->kind == GVM_VALUE_FLOAT) &&
      (rhs->kind == GVM_VALUE_INT || rhs->kind == GVM_VALUE_FLOAT)) {
    int64_t lhs_i = 0;
    int64_t rhs_i = 0;
    double lhs_f = 0.0;
    double rhs_f = 0.0;
    int lhs_is_float = 0;
    int rhs_is_float = 0;
    if (!vm_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
        !vm_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
      *compatible_out = 0;
      return GVM_OK;
    }
    *equal_out = lhs_f == rhs_f;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_INT) {
    if (rhs->as.int_value != 0 && rhs->as.int_value != 1) {
      *compatible_out = 0;
      return GVM_OK;
    }
    *equal_out = rhs->as.int_value == (int64_t)lhs->as.bool_value;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_INT && rhs->kind == GVM_VALUE_BOOL) {
    if (lhs->as.int_value != 0 && lhs->as.int_value != 1) {
      *compatible_out = 0;
      return GVM_OK;
    }
    *equal_out = lhs->as.int_value == (int64_t)rhs->as.bool_value;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_BOOL) {
    *equal_out = lhs->as.bool_value == rhs->as.bool_value;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_BITS && rhs->kind == GVM_VALUE_BITS) {
    *equal_out = vm_value_get_bits_payload(lhs) == vm_value_get_bits_payload(rhs);
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_STRING && rhs->kind == GVM_VALUE_STRING) {
    const char *lhs_text = lhs->as.string_value != NULL ? lhs->as.string_value : "";
    const char *rhs_text = rhs->as.string_value != NULL ? rhs->as.string_value : "";
    *equal_out = strcmp(lhs_text, rhs_text) == 0;
    return GVM_OK;
  }
  if (lhs->kind != GVM_VALUE_LIST || rhs->kind != GVM_VALUE_LIST) {
    *compatible_out = 0;
    return GVM_OK;
  }

  lhs_list = (graphion_vm_list *)lhs->as.ref_value;
  rhs_list = (graphion_vm_list *)rhs->as.ref_value;
  if ((lhs_list != NULL ? lhs_list->count : 0U) != (rhs_list != NULL ? rhs_list->count : 0U)) {
    *equal_out = 0;
    return GVM_OK;
  }
  for (i = 0U; i < (lhs_list != NULL ? lhs_list->count : 0U); ++i) {
    int nested_compatible = 0;
    int nested_equal = 0;
    const int rc = vm_values_deep_equal(&lhs_list->items[i], &rhs_list->items[i], &nested_compatible, &nested_equal);
    if (rc != GVM_OK) {
      return rc;
    }
    if (!nested_compatible) {
      *compatible_out = 0;
      return GVM_OK;
    }
    if (!nested_equal) {
      *equal_out = 0;
      return GVM_OK;
    }
  }
  *equal_out = 1;
  return GVM_OK;
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

static void vm_release_reg_compound(graphion_vm *vm, uint8_t reg) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  if (vm->regs[reg].kind == GVM_VALUE_LIST) {
    vm_value_dispose_owned(&vm->regs[reg]);
  } else {
    vm_value_clear(&vm->regs[reg]);
  }
}

void vm_free_owned_reg_string(graphion_vm *vm, uint8_t reg) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  if (vm->owned_reg_strings[reg] != NULL) {
    free(vm->owned_reg_strings[reg]);
    vm->owned_reg_strings[reg] = NULL;
    vm_value_clear(&vm->regs[reg]);
    return;
  }
  vm_release_reg_compound(vm, reg);
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

void vm_release_global_value(graphion_vm *vm, size_t index) {
  if (vm == NULL || vm->globals == NULL || index >= vm->global_count) {
    return;
  }
  if (vm->global_string_owners != NULL && vm->global_string_owners[index] != NULL) {
    free(vm->global_string_owners[index]);
    vm->global_string_owners[index] = NULL;
    vm_value_clear(&vm->globals[index]);
    return;
  }
  if (vm->globals[index].kind == GVM_VALUE_LIST) {
    vm_value_dispose_owned(&vm->globals[index]);
  } else {
    vm_value_clear(&vm->globals[index]);
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
    vm_release_global_value(vm, index);
    vm->globals[index].kind = GVM_VALUE_STRING;
    vm->globals[index].as.string_value = text;
    return GVM_OK;
  }
  copy = vm_strdup_text(text);
  if (copy == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_release_global_value(vm, index);
  vm->global_string_owners[index] = copy;
  vm->globals[index].kind = GVM_VALUE_STRING;
  vm->globals[index].as.string_value = copy;
  return GVM_OK;
}

int vm_reg_set_empty_list(graphion_vm *vm, uint8_t reg) {
  graphion_vm_list *list;
  if (vm == NULL || !is_valid_reg(reg)) {
    return GVM_ERR_INVALID_ARG;
  }
  list = vm_list_create();
  if (list == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->regs[reg].kind = GVM_VALUE_LIST;
  vm->regs[reg].as.ref_value = list;
  return GVM_OK;
}

static int vm_list_append_value(graphion_vm_list *list, const graphion_vm_value *src) {
  graphion_vm_value cloned;

  if (list == NULL || src == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (list->count == list->capacity) {
    size_t new_capacity = list->capacity == 0U ? 4U : list->capacity * 2U;
    graphion_vm_value *new_items =
        (graphion_vm_value *)realloc(list->items, new_capacity * sizeof(*new_items));
    if (new_items == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    list->items = new_items;
    list->capacity = new_capacity;
  }
  vm_value_clear(&cloned);
  if (vm_value_clone(&cloned, src) != GVM_OK) {
    return GVM_ERR_INVALID_ARG;
  }
  list->items[list->count++] = cloned;
  return GVM_OK;
}

int vm_list_append_reg(graphion_vm *vm, uint8_t list_reg, uint8_t value_reg) {
  graphion_vm_list *list;
  if (vm == NULL || !is_valid_reg(list_reg) || !is_valid_reg(value_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[list_reg].kind != GVM_VALUE_LIST) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  list = (graphion_vm_list *)vm->regs[list_reg].as.ref_value;
  return vm_list_append_value(list, &vm->regs[value_reg]);
}

int vm_list_get_element(graphion_vm *vm, uint8_t list_reg, uint8_t index_reg) {
  const graphion_vm_value *item;
  graphion_vm_list *list;
  int64_t index_value;
  graphion_vm_value cloned;

  if (vm == NULL || !is_valid_reg(list_reg) || !is_valid_reg(index_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[list_reg].kind != GVM_VALUE_LIST) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (!vm_value_get_int(&vm->regs[index_reg], &index_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  list = (graphion_vm_list *)vm->regs[list_reg].as.ref_value;
  if (index_value < 0 || list == NULL || (size_t)index_value >= list->count) {
    return GVM_ERR_INDEX_OUT_OF_RANGE;
  }
  item = &list->items[(size_t)index_value];
  if (item->kind == GVM_VALUE_STRING) {
    return vm_reg_set_string_copy(vm, list_reg, item->as.string_value != NULL ? item->as.string_value : "");
  }
  vm_value_clear(&cloned);
  if (vm_value_clone(&cloned, item) != GVM_OK) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, list_reg);
  vm->regs[list_reg] = cloned;
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

static int vm_write_value_sink_inline_ex(const graphion_output_sink *output,
                                         const graphion_vm_value *value,
                                         int string_as_list_item);

int vm_value_text_len(const graphion_vm_value *value, size_t *len_out) {
  char buffer[64];
  int written;
  size_t len;
  size_t total;
  size_t i;
  graphion_vm_list *list;

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
    case GVM_VALUE_LIST:
      total = 3U;
      list = (graphion_vm_list *)value->as.ref_value;
      if (list == NULL || list->count == 0U) {
        *len_out = total;
        return GVM_OK;
      }
      for (i = 0U; i < list->count; ++i) {
        size_t item_len = 0U;
        int rc;
        if (i > 0U) {
          total += 2U;
        }
        rc = vm_value_text_len(&list->items[i], &item_len);
        if (rc != GVM_OK) {
          return rc;
        }
        if (list->items[i].kind == GVM_VALUE_STRING && item_len > 0U) {
          item_len += 1U;
        }
        total += item_len - 1U;
      }
      *len_out = total;
      return GVM_OK;
    default:
      return GVM_ERR_TYPE_MISMATCH;
  }
}

static int vm_write_list_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  size_t i;
  graphion_vm_list *list;
  int rc;

  if (vm_write_bytes_sink(output, "[", 1U) != GVM_OK) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  list = (graphion_vm_list *)value->as.ref_value;
  if (list != NULL) {
    for (i = 0U; i < list->count; ++i) {
      if (i > 0U && vm_write_bytes_sink(output, ", ", 2U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      rc = vm_write_value_sink_inline_ex(output, &list->items[i], 1);
      if (rc != GVM_OK) {
        return rc;
      }
    }
  }
  return vm_write_bytes_sink(output, "]", 1U);
}

static int vm_write_value_sink_inline_ex(const graphion_output_sink *output,
                                         const graphion_vm_value *value,
                                         int string_as_list_item) {
  char buffer[64];
  int written;
  size_t len;

  if (output == NULL || output->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
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
      if (string_as_list_item && vm_write_bytes_sink(output, "\"", 1U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      if (value->as.string_value != NULL) {
        len = strlen(value->as.string_value);
        if (vm_write_bytes_sink(output, value->as.string_value, len) != GVM_OK) {
          return GVM_ERR_OUTPUT_UNBOUND;
        }
      }
      if (string_as_list_item) {
        return vm_write_bytes_sink(output, "\"", 1U);
      }
      return GVM_OK;
    case GVM_VALUE_BITS:
      len = vm_write_bits_text(buffer, sizeof(buffer), value, 0);
      if (len == 0U) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, buffer, len);
    case GVM_VALUE_LIST:
      return vm_write_list_inline(output, value);
    default:
      return GVM_ERR_TYPE_MISMATCH;
  }
}

int vm_write_value_sink(const graphion_output_sink *output, const graphion_vm_value *value) {
  int rc;
  if (output == NULL || output->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  rc = vm_write_value_sink_inline_ex(output, value, 0);
  if (rc != GVM_OK) {
    return rc;
  }
  return vm_write_bytes_sink(output, "\n", 1U);
}

int vm_write_value_sink_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  return vm_write_value_sink_inline_ex(output, value, 0);
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
