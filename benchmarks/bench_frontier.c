/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BENCH_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

static void bench_reset_regs(graphion_vm *vm) {
  size_t i;
  for (i = 0U; i < (sizeof(vm->regs) / sizeof(vm->regs[0])); ++i) {
    vm->regs[i].kind = GVM_VALUE_INT;
    vm->regs[i].as.int_value = 0;
  }
}

enum {
  FRONTIER_INPUT_LEN = 64,
  FRONTIER_CAPACITY = 64,
  FRONTIER_ITEMS_PER_ITERATION = 128
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

int main(int argc, char **argv) {
  graphion_vm vm;
  uint32_t frontier_a[FRONTIER_CAPACITY];
  uint32_t frontier_b[FRONTIER_CAPACITY];
  const graphion_insn program[] = {
      {GVM_OP_FRONTIER_FILTER_LT_IMM, 0U, 0U, 32},
      {GVM_OP_FRONTIER_SWAP, 1U, 0U, 0},
      {GVM_OP_FRONTIER_MAP_ADD_IMM, 2U, 0U, 3},
      {GVM_OP_FRONTIER_SWAP, 3U, 0U, 0},
      {GVM_OP_FRONTIER_REDUCE_SUM, 4U, 0U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  const size_t instruction_count = sizeof(program) / sizeof(program[0]);
  long iterations = 300000;
  long i;
  double start;
  double end;
  double seconds;
  double mips;
  double ns_per_instruction;
  double ns_per_frontier_item;
  int rc;
  uint64_t checksum = 0U;

  if (argc > 1) {
    iterations = strtol(argv[1], NULL, 10);
    if (iterations <= 0) {
      fprintf(stderr, "iterations must be > 0\n");
      return 2;
    }
  }

  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, program, instruction_count);
  if (rc != 0) {
    fprintf(stderr, "load failed rc=%d\n", rc);
    return 3;
  }

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    size_t j;
    for (j = 0U; j < FRONTIER_INPUT_LEN; ++j) {
      frontier_a[j] = (uint32_t)j;
      frontier_b[j] = 0U;
    }
    bench_reset_regs(&vm);
    vm.pc = 0U;
    vm.halted = false;
    graphion_vm_bind_frontier(&vm, frontier_a, FRONTIER_INPUT_LEN, frontier_b, FRONTIER_CAPACITY);
    rc = graphion_vm_run(&vm);
    if (rc != 0) {
      fprintf(stderr, "run failed rc=%d\n", rc);
      return 4;
    }
    checksum += (uint64_t)BENCH_REG_I(vm, 4);
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mips = ((double)(iterations * (long)instruction_count) / seconds) / 1000000.0;
  ns_per_instruction = (seconds * 1000000000.0) / ((double)iterations * (double)instruction_count);
  ns_per_frontier_item = (seconds * 1000000000.0) / ((double)iterations * (double)FRONTIER_ITEMS_PER_ITERATION);

  printf("{\"benchmark\":\"frontier_primitives\",\"iterations\":%ld,\"instructions_per_iteration\":%zu,"
         "\"frontier_items_per_iteration\":%d,\"seconds\":%.6f,\"mips\":%.3f,"
         "\"ns_per_instruction\":%.3f,\"ns_per_frontier_item\":%.3f,\"checksum\":%llu}\n",
         iterations,
         instruction_count,
         FRONTIER_ITEMS_PER_ITERATION,
         seconds,
         mips,
         ns_per_instruction,
         ns_per_frontier_item,
         (unsigned long long)checksum);
  return 0;
}
