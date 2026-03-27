/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
  GION_EXPR_SOURCE_MAX = 4096,
  GION_EXPR_SOURCE_OPS_PER_ITERATION = 7
};

static volatile uint64_t g_gion_expr_bytes_sink = 0U;

typedef struct {
  uint64_t bytes_written;
} graphion_bench_count_sink;

static double now_seconds(void) {
#if defined(TIME_UTC)
  struct timespec ts;
  (void)timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
#else
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

static int load_source_file(const char *path, char *buffer, size_t capacity) {
  FILE *input;
  size_t bytes_read;

  if (path == NULL || buffer == NULL || capacity == 0U) {
    return 0;
  }
#if defined(_MSC_VER)
  if (fopen_s(&input, path, "rb") != 0) {
    input = NULL;
  }
#else
  input = fopen(path, "rb");
#endif
  if (input == NULL) {
    return 0;
  }
  bytes_read = fread(buffer, 1U, capacity - 1U, input);
  fclose(input);
  buffer[bytes_read] = '\0';
  return bytes_read > 0U;
}

static int find_program_global_slot(const graphion_runtime_program *program, const char *name) {
  size_t i;
  if (program == NULL || name == NULL) {
    return -1;
  }
  for (i = 0U; i < program->global_count; ++i) {
    if (strcmp(program->global_names[i], name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

int main(int argc, char **argv) {
  char source[GION_EXPR_SOURCE_MAX];
  const char *path = "benchmarks/workloads/expr_values.gion";
  graphion_runtime_program program;
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  graphion_output_sink output;
  graphion_bench_count_sink count_sink;
  long iterations = 10000000;
  long i;
  double start;
  double end;
  double seconds;
  double mops;
  double ns_per_operation;
  double ns_per_iteration;
  uint64_t checksum = 0U;
  int e_slot = -1;

  if (argc > 1) {
    iterations = strtol(argv[1], NULL, 10);
    if (iterations <= 0) {
      fprintf(stderr, "iterations must be > 0\n");
      return 2;
    }
  }
  if (argc > 2) {
    path = argv[2];
  }
  if (!load_source_file(path, source, sizeof(source))) {
    fprintf(stderr, "failed to load gion workload: %s\n", path);
    return 3;
  }
  memset(&count_sink, 0, sizeof(count_sink));
  graphion_output_sink_from_counter(&output, &count_sink.bytes_written);
  graphion_runtime_program_init(&program);
  graphion_runtime_scope_init(&scope);
  if (graphion_prepare_source(source, &program, &diagnostic) != GINT_OK) {
    fprintf(stderr, "prepare failed at %zu:%zu (%s)\n",
            diagnostic.line,
            diagnostic.column,
            diagnostic.message != NULL ? diagnostic.message : "unknown");
    graphion_runtime_program_dispose(&program);
    return 5;
  }
  e_slot = find_program_global_slot(&program, "e");
  if (e_slot < 0) {
    graphion_runtime_scope_dispose(&scope);
    graphion_runtime_program_dispose(&program);
    fprintf(stderr, "required global not found in prepared workload\n");
    return 6;
  }

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    int rc;
    rc = graphion_execute_prepared_program_with_sink(&program, &scope, &diagnostic, &output);
    if (rc != GINT_OK) {
      fprintf(stderr, "interpret failed rc=%d at %zu:%zu (%s)\n",
              rc,
              diagnostic.line,
              diagnostic.column,
              diagnostic.message != NULL ? diagnostic.message : "unknown");
      graphion_runtime_scope_dispose(&scope);
      graphion_runtime_program_dispose(&program);
      return 7;
    }
    checksum += (uint64_t)scope.vm_globals[(size_t)e_slot].as.int_value;
  }
  graphion_runtime_scope_dispose(&scope);
  end = now_seconds();
  g_gion_expr_bytes_sink = count_sink.bytes_written;
  graphion_runtime_program_dispose(&program);

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mops = ((double)(iterations * GION_EXPR_SOURCE_OPS_PER_ITERATION) / seconds) / 1000000.0;
  ns_per_operation = (seconds * 1000000000.0) / ((double)iterations * (double)GION_EXPR_SOURCE_OPS_PER_ITERATION);
  ns_per_iteration = (seconds * 1000000000.0) / (double)iterations;

  printf("{\"benchmark\":\"gion_expr_source\",\"iterations\":%ld,\"source_ops_per_iteration\":%d,"
         "\"print_ops_per_iteration\":2,\"seconds\":%.6f,\"mops\":%.3f,"
         "\"ns_per_operation\":%.3f,\"ns_per_iteration\":%.3f,\"checksum\":%llu}\n",
         iterations,
         GION_EXPR_SOURCE_OPS_PER_ITERATION,
         seconds,
         mops,
         ns_per_operation,
         ns_per_iteration,
         (unsigned long long)checksum);
  return 0;
}
