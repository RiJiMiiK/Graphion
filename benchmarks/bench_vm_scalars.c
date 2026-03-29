/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

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

static void set_int(graphion_vm_value *value, int64_t number) {
  value->kind = GVM_VALUE_INT;
  value->as.int_value = number;
}

static void set_float(graphion_vm_value *value, double number) {
  value->kind = GVM_VALUE_FLOAT;
  value->as.float_value = number;
}

static void set_bool(graphion_vm_value *value, int flag) {
  value->kind = GVM_VALUE_BOOL;
  value->as.bool_value = flag;
}

static void set_string(graphion_vm_value *value, const char *text) {
  value->kind = GVM_VALUE_STRING;
  value->as.string_value = text;
}

static void set_none(graphion_vm_value *value) {
  value->kind = GVM_VALUE_NONE;
  value->as.int_value = 0;
}

int main(int argc, char **argv) {
  graphion_vm vm;
  graphion_vm_value const_pool[8];
  graphion_vm_value globals[5];
  graphion_output_sink sink;
  const graphion_insn program[] = {
      {GVM_OP_STORE_CONST_GLOBAL, 0, 0, 0},
      {GVM_OP_STORE_CONST_GLOBAL, 0, 1, 1},
      {GVM_OP_STORE_CONST_GLOBAL, 0, 2, 2},
      {GVM_OP_STORE_CONST_GLOBAL, 0, 3, 3},
      {GVM_OP_COPY_GLOBAL, 0, 4, 0},
      {GVM_OP_PRINT_CONST, 0, 0, 4},
      {GVM_OP_PRINT_CONST, 0, 0, 5},
      {GVM_OP_PRINT_CONST, 0, 0, 6},
      {GVM_OP_PRINT_GLOBAL, 0, 0, 0},
      {GVM_OP_PRINT_GLOBAL, 0, 0, 1},
      {GVM_OP_PRINT_GLOBAL, 0, 0, 2},
      {GVM_OP_PRINT_GLOBAL, 0, 0, 3},
      {GVM_OP_PRINT_GLOBAL, 0, 0, 4},
      {GVM_OP_HALT, 0, 0, 0},
  };
  const size_t instruction_count = sizeof(program) / sizeof(program[0]);
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

  set_int(&const_pool[0], 42);
  set_float(&const_pool[1], 3.5);
  set_string(&const_pool[2], "graphion");
  set_bool(&const_pool[3], 1);
  set_int(&const_pool[4], 7);
  set_string(&const_pool[5], "raw");
  set_bool(&const_pool[6], 0);
  set_none(&const_pool[7]);

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 7U);
  graphion_vm_bind_globals(&vm, globals, 5U);
  graphion_output_sink_from_counter(&sink, &checksum);
  graphion_vm_bind_output_sink(&vm, &sink);
  rc = graphion_vm_load(&vm, program, instruction_count);
  if (rc != GVM_OK) {
    fprintf(stderr, "load failed rc=%d\n", rc);
    return 3;
  }

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    size_t global_index;
    for (global_index = 0U; global_index < 5U; ++global_index) {
      set_none(&globals[global_index]);
    }
    vm.pc = 0U;
    vm.halted = false;
    rc = graphion_vm_run(&vm);
    if (rc != GVM_OK) {
      fprintf(stderr, "run failed rc=%d\n", rc);
      return 4;
    }
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  ns_per_iteration = (seconds * 1000000000.0) / (double)iterations;
  mops = ((double)iterations * 13.0) / seconds / 1000000.0;

  printf("{\"benchmark\":\"vm_scalar_values_print\",\"iterations\":%ld,"
         "\"instructions_per_iteration\":%zu,\"typed_value_ops_per_iteration\":13,"
         "\"seconds\":%.6f,\"ns_per_iteration\":%.3f,\"mops\":%.3f,\"checksum\":%llu}\n",
         iterations, instruction_count, seconds, ns_per_iteration, mops, (unsigned long long)checksum);
  return 0;
}
