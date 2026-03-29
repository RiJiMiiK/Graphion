/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now_seconds(void) {
#if defined(TIME_UTC)
  struct timespec ts;
  (void)timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
#else
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

int main(int argc, char **argv) {
  const char *source =
      "count = 42\n"
      "ratio = 3.5\n"
      "name = \"graphion\"\n"
      "ready = true\n"
      "copy = count\n"
      "print(7)\n"
      "print(\"raw\")\n"
      "print(false)\n"
      "print(count)\n"
      "print(ratio)\n"
      "print(name)\n"
      "print(ready)\n"
      "print(copy)\n";
  graphion_runtime_program program;
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  graphion_output_sink sink;
  long iterations = 100000;
  long i;
  double start;
  double end;
  double seconds;
  double ns_per_iteration;
  double mops;
  uint64_t checksum = 0U;
  int rc;

  if (argc > 1) {
    iterations = strtol(argv[1], NULL, 10);
    if (iterations <= 0) {
      fprintf(stderr, "iterations must be > 0\n");
      return 2;
    }
  }

  graphion_output_sink_from_counter(&sink, &checksum);

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    graphion_runtime_program_init(&program);
    graphion_runtime_scope_init(&scope);
    rc = graphion_prepare_source(source, &program, &diagnostic);
    if (rc != GINT_OK) {
      fprintf(stderr, "prepare failed rc=%d line=%u col=%u msg=%s\n",
              rc,
              diagnostic.line,
              diagnostic.column,
              diagnostic.message != NULL ? diagnostic.message : "(null)");
      return 3;
    }
    rc = graphion_execute_prepared_program_with_sink(&program, &scope, &diagnostic, &sink);
    if (rc != GINT_OK) {
      fprintf(stderr, "execute failed rc=%d line=%u col=%u msg=%s\n",
              rc,
              diagnostic.line,
              diagnostic.column,
              diagnostic.message != NULL ? diagnostic.message : "(null)");
      return 4;
    }
    graphion_runtime_program_dispose(&program);
    graphion_runtime_scope_dispose(&scope);
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  ns_per_iteration = (seconds * 1000000000.0) / (double)iterations;
  mops = ((double)iterations * 13.0) / seconds / 1000000.0;

  printf("{\"benchmark\":\"gion_scalar_values_print\",\"iterations\":%ld,"
         "\"source_ops_per_iteration\":13,\"print_ops_per_iteration\":8,"
         "\"seconds\":%.6f,\"ns_per_iteration\":%.3f,\"mops\":%.3f,\"checksum\":%llu}\n",
         iterations, seconds, ns_per_iteration, mops, (unsigned long long)checksum);
  return 0;
}
