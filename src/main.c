#include <stdio.h>

#include "vm/vm.h"

int main(void) {
  graphion_vm vm;
  graphion_vm_init(&vm);

  printf("Graphion VM scaffold ready. program_len=%zu\n", vm.program_len);
  return 0;
}
