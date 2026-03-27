/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
  VM_EXPR_SOURCE_MAX = 4096
};

static volatile uint64_t g_vm_expr_bytes_sink = 0U;

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

static int find_global_slot(const graphion_runtime_program *program, const char *name) {
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
  char source[VM_EXPR_SOURCE_MAX];
  const char *path = "benchmarks/workloads/expr_values.gion";
  graphion_runtime_program program;
  graphion_runtime_diagnostic diagnostic;
  graphion_vm vm;
  graphion_vm_value globals[GRAPHION_RUNTIME_BINDING_MAX];
  graphion_output_sink output;
  graphion_bench_count_sink count_sink;
  long iterations = 10000000;
  long i;
  double start;
  double end;
  double seconds;
  double mips;
  double ns_per_instruction;
  double ns_per_iteration;
  uint64_t checksum = 0U;
  size_t instruction_count = 0U;
  int e_slot;

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
  if (graphion_prepare_source(source, &program, &diagnostic) != GINT_OK) {
    fprintf(stderr, "prepare failed at %zu:%zu (%s)\n",
            diagnostic.line,
            diagnostic.column,
            diagnostic.message != NULL ? diagnostic.message : "unknown");
    graphion_runtime_program_dispose(&program);
    return 5;
  }
  if (program.prepared_vm_enabled == 0) {
    graphion_runtime_program_dispose(&program);
    fprintf(stderr, "prepared vm program unavailable for workload\n");
    return 6;
  }

  memset(globals, 0, sizeof(globals));
  memset(&count_sink, 0, sizeof(count_sink));
  graphion_output_sink_from_counter(&output, &count_sink.bytes_written);
  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, program.prepared_const_pool, program.prepared_const_count);
  graphion_vm_bind_globals(&vm, globals, program.global_count);
  graphion_vm_bind_output_sink(&vm, &output);
  if (graphion_vm_load(&vm, program.prepared_vm_program, program.prepared_vm_program_len) != GVM_OK) {
    graphion_runtime_program_dispose(&program);
    fprintf(stderr, "vm load failed for prepared workload\n");
    return 7;
  }
  instruction_count = program.prepared_vm_program_len;

  e_slot = find_global_slot(&program, "e");
  if (e_slot < 0) {
    graphion_runtime_program_dispose(&program);
    fprintf(stderr, "required global not found in prepared workload\n");
    return 8;
  }

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    int rc;
    graphion_vm_reset_execution(&vm);
    if (program.prepared_overwrites_all_globals == 0) {
      size_t j;
      for (j = 0U; j < program.global_count; ++j) {
        globals[j].kind = GVM_VALUE_NONE;
        globals[j].as.int_value = 0;
      }
    }
    rc = graphion_vm_run(&vm);
    if (rc != GVM_OK) {
      graphion_runtime_program_dispose(&program);
      fprintf(stderr, "vm run failed: %d\n", rc);
      return 9;
    }
    checksum += (uint64_t)globals[(size_t)e_slot].as.int_value;
  }
  end = now_seconds();
  g_vm_expr_bytes_sink = count_sink.bytes_written;

  graphion_runtime_program_dispose(&program);
  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mips = (((double)iterations * (double)instruction_count) / seconds) / 1000000.0;
  ns_per_instruction = (seconds * 1000000000.0) / ((double)iterations * (double)instruction_count);
  ns_per_iteration = (seconds * 1000000000.0) / (double)iterations;

  printf("{\"benchmark\":\"vm_expr_dispatch\",\"iterations\":%ld,\"instructions_per_iteration\":%zu,"
         "\"expr_ops_per_iteration\":7,\"print_ops_per_iteration\":2,\"seconds\":%.6f,\"mips\":%.3f,"
         "\"ns_per_instruction\":%.3f,\"ns_per_iteration\":%.3f,\"checksum\":%llu}\n",
         iterations,
         instruction_count,
         seconds,
         mips,
         ns_per_instruction,
         ns_per_iteration,
         (unsigned long long)checksum);
  return 0;
}
