#include "parser/bytecode.h"

#include <stddef.h>
#include <stdint.h>

enum { GBC_INSN_SIZE = 7 };

static int32_t decode_i32_le(const uint8_t *p) {
  const uint32_t u = (uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) |
                     ((uint32_t)p[3] << 24U);
  return (int32_t)u;
}

int graphion_decode_bytecode(const uint8_t *bytes,
                             size_t byte_len,
                             graphion_insn *out_program,
                             size_t out_capacity,
                             size_t *out_count) {
  size_t i;
  size_t count;
  if (bytes == NULL || out_program == NULL || out_count == NULL) {
    return GBC_ERR_INVALID_ARG;
  }
  if (byte_len == 0U) {
    *out_count = 0U;
    return GBC_OK;
  }
  if ((byte_len % GBC_INSN_SIZE) != 0U) {
    return GBC_ERR_TRUNCATED;
  }

  count = byte_len / GBC_INSN_SIZE;
  if (count > out_capacity) {
    return GBC_ERR_CAPACITY;
  }

  for (i = 0; i < count; ++i) {
    const size_t at = i * GBC_INSN_SIZE;
    out_program[i].op = bytes[at + 0U];
    out_program[i].a = bytes[at + 1U];
    out_program[i].b = bytes[at + 2U];
    out_program[i].imm = decode_i32_le(&bytes[at + 3U]);
  }

  *out_count = count;
  return GBC_OK;
}
