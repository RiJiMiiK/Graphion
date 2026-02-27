#ifndef GRAPHION_PARSER_BYTECODE_H
#define GRAPHION_PARSER_BYTECODE_H

#include <stddef.h>
#include <stdint.h>

#include "vm/vm.h"

typedef enum {
  GBC_OK = 0,
  GBC_ERR_INVALID_ARG = -1,
  GBC_ERR_TRUNCATED = -2,
  GBC_ERR_CAPACITY = -3
} graphion_bytecode_result;

int graphion_decode_bytecode(const uint8_t *bytes,
                             size_t byte_len,
                             graphion_insn *out_program,
                             size_t out_capacity,
                             size_t *out_count);

#endif
