#include "parser/bytecode.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  graphion_insn program[64];
  size_t count = 0U;
  (void)graphion_decode_bytecode(data, size, program, 64U, &count);
  return 0;
}
