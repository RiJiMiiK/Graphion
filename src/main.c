/* SPDX-License-Identifier: MIT */

#include <stdio.h>

#include "runtime/entry.h"

int main(int argc, char **argv) {
  graphion_runtime_scope scope;
  int rc;

  if (argc != 2) {
    fprintf(stderr, "usage: graphion <program.gion>\n");
    return 1;
  }
  rc = graphion_run_gion_path(argv[1], &scope);
  if (rc == GENTRY_ERR_EXTENSION) {
    fprintf(stderr, "error: source file must use the .gion extension\n");
    return 2;
  }
  if (rc != GENTRY_OK) {
    fprintf(stderr, "error: failed to execute '%s' (rc=%d)\n", argv[1], rc);
    return 3;
  }
  return 0;
}
