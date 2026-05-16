/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime/entry.h"

int main(int argc, char **argv) {
  graphion_runtime_scope *scope;
  graphion_runtime_diagnostic diagnostic;
  graphion_runtime_warning_report warnings;
  const char *path;
  int debug_warnings = 0;
  int rc;

  scope = (graphion_runtime_scope *)calloc(1U, sizeof(*scope));
  if (scope == NULL) {
    fprintf(stderr, "error: runtime scope allocation failed\n");
    return 5;
  }

  if (argc == 1) {
    free(scope);
    return 0;
  }
  if (argc == 2 && strcmp(argv[1], "-d") != 0) {
    path = argv[1];
  } else if (argc == 3 && strcmp(argv[1], "-d") == 0) {
    debug_warnings = 1;
    path = argv[2];
  } else {
    fprintf(stderr, "usage: graphion [-d] <program.gion>\n");
    free(scope);
    return 1;
  }
  if (debug_warnings) {
    rc = graphion_collect_gion_path_warnings(path, &warnings, &diagnostic);
    if (rc == GENTRY_OK) {
      graphion_emit_warning_report(&warnings, stderr);
    }
  }
  rc = graphion_run_gion_path(path, scope, &diagnostic);
  if (rc == GENTRY_ERR_EXTENSION) {
    fprintf(stderr, "error: source file must use the .gion extension\n");
    free(scope);
    return 2;
  }
  if (rc != GENTRY_OK) {
    if (diagnostic.message != NULL) {
      fprintf(stderr,
              "error:%u:%u: %s\n",
              diagnostic.line,
              diagnostic.column,
              diagnostic.message);
    } else {
      fprintf(stderr, "error: failed to execute '%s' (rc=%d)\n", path, rc);
    }
    graphion_runtime_scope_dispose(scope);
    free(scope);
    return 3;
  }
  graphion_runtime_scope_dispose(scope);
  free(scope);
  return 0;
}
