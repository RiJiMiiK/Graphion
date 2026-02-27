#include "vm/vm.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  graphion_vm vm;
  size_t count;
  size_t i;
  graphion_insn local_program[64];

  if (size < sizeof(graphion_insn)) {
    return 0;
  }

  count = size / sizeof(graphion_insn);
  if (count > 64U) {
    count = 64U;
  }

  for (i = 0; i < count; ++i) {
    const size_t at = i * sizeof(graphion_insn);
    graphion_insn in;
    const uint8_t *p = data + at;
    in.op = p[0];
    in.a = p[1];
    in.b = p[2];
    in.imm = (int32_t)((uint32_t)p[3] | ((uint32_t)p[4] << 8U) | ((uint32_t)p[5] << 16U) |
                       ((uint32_t)p[6] << 24U));
    local_program[i] = in;
  }

  graphion_vm_init(&vm);
  if (graphion_vm_load(&vm, local_program, count) == 0) {
    (void)graphion_vm_run(&vm);
  }

  return 0;
}
