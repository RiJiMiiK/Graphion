/* SPDX-License-Identifier: MIT */

#include <stdio.h>

#include "runtime/entry.h"

int main(int argc, char **argv) {
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  if (argc == 1) {
    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("warmup = 1\n", &scope, &diagnostic);
    if (rc != GINT_OK) {
      fprintf(stderr, "error: interpreter warmup failed (rc=%d)\n", rc);
      return 4;
    }
    return 0;
  }
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
