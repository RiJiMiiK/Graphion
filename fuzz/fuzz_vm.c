#include "vm/vm.h"
#include "parser/bytecode.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  graphion_vm vm;
  graphion_insn local_program[64];
  size_t count = 0U;

  if (size < 7U) {
    return 0;
  }

  graphion_vm_init(&vm);
  if (graphion_decode_bytecode(data, size, local_program, 64U, &count) == GBC_OK &&
      graphion_vm_load(&vm, local_program, count) == 0) {
    (void)graphion_vm_run(&vm);
  }

  return 0;
}
