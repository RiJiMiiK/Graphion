/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <stdlib.h>

#include "runtime/entry.h"

int main(int argc, char **argv) {
  graphion_runtime_scope *scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  scope = (graphion_runtime_scope *)calloc(1U, sizeof(*scope));
  if (scope == NULL) {
    fprintf(stderr, "error: runtime scope allocation failed\n");
    return 5;
  }

  if (argc == 1) {
    graphion_runtime_scope_init(scope);
    rc = graphion_interpret_source("warmup = 1\n", scope, &diagnostic);
    if (rc != GINT_OK) {
      fprintf(stderr, "error: interpreter warmup failed (rc=%d)\n", rc);
      graphion_runtime_scope_dispose(scope);
      free(scope);
      return 4;
    }
    graphion_runtime_scope_dispose(scope);
    free(scope);
    return 0;
  }
  if (argc != 2) {
    fprintf(stderr, "usage: graphion <program.gion>\n");
    free(scope);
    return 1;
  }
  rc = graphion_run_gion_path(argv[1], scope);
  if (rc == GENTRY_ERR_EXTENSION) {
    fprintf(stderr, "error: source file must use the .gion extension\n");
    free(scope);
    return 2;
  }
  if (rc != GENTRY_OK) {
    fprintf(stderr, "error: failed to execute '%s' (rc=%d)\n", argv[1], rc);
    graphion_runtime_scope_dispose(scope);
    free(scope);
    return 3;
  }
  graphion_runtime_scope_dispose(scope);
  free(scope);
  return 0;
}
