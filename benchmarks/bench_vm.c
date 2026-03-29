/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BENCH_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

static void bench_set_value_int(graphion_vm_value *value, int64_t number) {
  value->kind = GVM_VALUE_INT;
  value->as.int_value = number;
}

static void bench_set_value_string(graphion_vm_value *value, const char *text) {
  value->kind = GVM_VALUE_STRING;
  value->as.string_value = text;
}

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
  graphion_vm vm;
  graphion_vm_value const_pool[4];
  graphion_vm_value globals[2];
  const graphion_insn program[] = {
      {GVM_OP_LOAD_CONST, 0, 0, 0},   {GVM_OP_STORE_GLOBAL, 0, 0, 0}, {GVM_OP_LOAD_GLOBAL, 1, 0, 0},
      {GVM_OP_LOAD_CONST, 2, 0, 1},   {GVM_OP_ADD, 1, 2, 0},          {GVM_OP_STORE_GLOBAL, 1, 0, 0},
      {GVM_OP_LOAD_GLOBAL, 3, 0, 0},  {GVM_OP_MOV, 4, 3, 0},          {GVM_OP_LOAD_CONST, 5, 0, 2},
      {GVM_OP_ADD, 4, 5, 0},          {GVM_OP_STORE_GLOBAL, 4, 0, 0}, {GVM_OP_LOAD_CONST, 6, 0, 3},
      {GVM_OP_STORE_GLOBAL, 6, 0, 1}, {GVM_OP_LOAD_GLOBAL, 7, 0, 0},  {GVM_OP_MOV, 8, 7, 0},
      {GVM_OP_HALT, 0, 0, 0},
  };
  const size_t instruction_count = sizeof(program) / sizeof(program[0]);
  long iterations = 5000000;
  long i;
  double start;
  double end;
  double seconds;
  double mips;
  double ns_per_instruction;
  double ns_per_iteration;
  int rc;
  uint64_t checksum = 0U;

  if (argc > 1) {
    iterations = strtol(argv[1], NULL, 10);
    if (iterations <= 0) {
      fprintf(stderr, "iterations must be > 0\n");
      return 2;
    }
  }

  bench_set_value_int(&const_pool[0], 1);
  bench_set_value_int(&const_pool[1], 2);
  bench_set_value_int(&const_pool[2], 10);
  bench_set_value_string(&const_pool[3], "graphion");
  globals[0].kind = GVM_VALUE_INT;
  globals[0].as.int_value = 0;
  globals[1].kind = GVM_VALUE_NONE;
  globals[1].as.int_value = 0;

  graphion_vm_init(&vm);
  graphion_vm_bind_constants(&vm, const_pool, 4U);
  graphion_vm_bind_globals(&vm, globals, 2U);
  rc = graphion_vm_load(&vm, program, instruction_count);
  if (rc != 0) {
    fprintf(stderr, "load failed rc=%d\n", rc);
    return 3;
  }

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    globals[0].kind = GVM_VALUE_INT;
    globals[0].as.int_value = 0;
    globals[1].kind = GVM_VALUE_NONE;
    globals[1].as.int_value = 0;
    vm.pc = 0U;
    vm.halted = false;
    rc = graphion_vm_run(&vm);
    if (rc != 0) {
      fprintf(stderr, "run failed rc=%d\n", rc);
      return 4;
    }
    checksum += (uint64_t)BENCH_REG_I(vm, 8);
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mips = ((double)(iterations * (long)instruction_count) / seconds) / 1000000.0;
  ns_per_instruction = (seconds * 1000000000.0) / ((double)iterations * (double)instruction_count);
  ns_per_iteration = (seconds * 1000000000.0) / (double)iterations;

  printf("{\"benchmark\":\"vm_dispatch\",\"iterations\":%ld,\"instructions_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mips\":%.3f,\"ns_per_instruction\":%.3f,\"ns_per_iteration\":%.3f,"
         "\"typed_value_ops_per_iteration\":%d,\"checksum\":%llu}\n",
         iterations, instruction_count, seconds, mips, ns_per_instruction, ns_per_iteration, 15,
         (unsigned long long)checksum);
  return 0;
}
