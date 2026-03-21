/* SPDX-License-Identifier: MIT */

#include <stdio.h>

#include "runtime/entry.h"
#include "vm/vm.h"

int main(int argc, char **argv) {
  graphion_vm vm;
  int rc;

  if (argc != 2) {
    fprintf(stderr, "usage: graphion <program.gion>\n");
    return 1;
  }
  rc = graphion_run_gion_path(argv[1], &vm);
  if (rc == GENTRY_ERR_EXTENSION) {
    fprintf(stderr, "error: source file must use the .gion extension\n");
    return 2;
  }
  if (rc != GENTRY_OK) {
    fprintf(stderr, "error: failed to execute '%s' (rc=%d)\n", argv[1], rc);
    return 3;
  }
  if (!vm.halted) {
    fprintf(stderr, "error: program '%s' did not halt cleanly\n", argv[1]);
    return 4;
  }
  if (vm.regs[0] != 0) {
    printf("%lld\n", (long long)vm.regs[0]);
  }
  return 0;
}
