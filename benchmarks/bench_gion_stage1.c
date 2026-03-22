/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
  GION_STAGE1_SOURCE_MAX = 4096,
  GION_STAGE1_OPS_PER_ITERATION = 12
};

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

static int find_binding_index(const graphion_runtime_scope *scope, const char *name) {
  size_t i;
  if (scope == NULL || name == NULL) {
    return -1;
  }
  for (i = 0U; i < scope->count; ++i) {
    if (strcmp(scope->bindings[i].name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static uint64_t update_checksum(const graphion_runtime_scope *scope,
                                int count_index,
                                int ratio_index,
                                int ready_index,
                                int name_index,
                                uint64_t checksum) {
  const graphion_runtime_binding *count_binding;
  const graphion_runtime_binding *ratio_binding;
  const graphion_runtime_binding *ready_binding;
  const graphion_runtime_binding *name_binding;
  const graphion_vm_value *count_value;
  const graphion_vm_value *ratio_value;
  const graphion_vm_value *ready_value;
  const graphion_vm_value *name_value;

  if (scope == NULL || count_index < 0 || ratio_index < 0 || ready_index < 0 || name_index < 0) {
    return checksum;
  }
  count_binding = &scope->bindings[(size_t)count_index];
  ratio_binding = &scope->bindings[(size_t)ratio_index];
  ready_binding = &scope->bindings[(size_t)ready_index];
  name_binding = &scope->bindings[(size_t)name_index];
  count_value = &scope->vm_globals[count_binding->vm_global_index];
  ratio_value = &scope->vm_globals[ratio_binding->vm_global_index];
  ready_value = &scope->vm_globals[ready_binding->vm_global_index];
  name_value = &scope->vm_globals[name_binding->vm_global_index];
  checksum += (uint64_t)count_value->as.int_value;
  checksum += (uint64_t)(ratio_value->as.float_value * 1000.0);
  checksum += (uint64_t)ready_value->as.bool_value;
  checksum += (uint64_t)strlen(name_value->as.string_value);
  return checksum;
}

int main(int argc, char **argv) {
  char source[GION_STAGE1_SOURCE_MAX];
  const char *path = "benchmarks/workloads/stage1_values.gion";
  graphion_runtime_program program;
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  long iterations = 20000;
  long i;
  double start;
  double end;
  double seconds;
  double mops;
  double ns_per_operation;
  uint64_t checksum = 0U;
  int count_index = -1;
  int ratio_index = -1;
  int ready_index = -1;
  int name_index = -1;

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
  graphion_runtime_program_init(&program);
  graphion_runtime_scope_init(&scope);
  if (graphion_prepare_source(source, &program, &diagnostic) != GINT_OK) {
    fprintf(stderr, "prepare failed at %zu:%zu (%s)\n",
            diagnostic.line,
            diagnostic.column,
            diagnostic.message != NULL ? diagnostic.message : "unknown");
    graphion_runtime_program_dispose(&program);
    return 4;
  }

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    int rc;
    rc = graphion_execute_program(&program, &scope, &diagnostic, stdout);
    if (rc != GINT_OK) {
      fprintf(stderr, "interpret failed rc=%d at %zu:%zu (%s)\n",
              rc,
              diagnostic.line,
              diagnostic.column,
              diagnostic.message != NULL ? diagnostic.message : "unknown");
      graphion_runtime_scope_dispose(&scope);
      graphion_runtime_program_dispose(&program);
      return 4;
    }
    if (count_index < 0) {
      count_index = find_binding_index(&scope, "copy_count");
      ratio_index = find_binding_index(&scope, "copy_ratio");
      ready_index = find_binding_index(&scope, "copy_ready");
      name_index = find_binding_index(&scope, "copy_name");
    }
    checksum = update_checksum(&scope, count_index, ratio_index, ready_index, name_index, checksum);
  }
  graphion_runtime_scope_dispose(&scope);
  end = now_seconds();
  graphion_runtime_program_dispose(&program);

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mops = ((double)(iterations * GION_STAGE1_OPS_PER_ITERATION) / seconds) / 1000000.0;
  ns_per_operation = (seconds * 1000000000.0) / ((double)iterations * (double)GION_STAGE1_OPS_PER_ITERATION);

  printf("{\"benchmark\":\"gion_stage1\",\"iterations\":%ld,\"source_ops_per_iteration\":%d,"
         "\"seconds\":%.6f,\"mops\":%.3f,\"ns_per_operation\":%.3f,\"checksum\":%llu}\n",
         iterations,
         GION_STAGE1_OPS_PER_ITERATION,
         seconds,
         mops,
         ns_per_operation,
         (unsigned long long)checksum);
  return 0;
}
