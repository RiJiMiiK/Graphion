/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double elapsed_seconds(clock_t start, clock_t end) {
  return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(int argc, char **argv) {
  graphion_vm vm;
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 1},   {GVM_OP_MOV_IMM, 1, 0, 2}, {GVM_OP_ADD, 0, 1, 0},
      {GVM_OP_MOV_IMM, 2, 0, 10},  {GVM_OP_ADD, 0, 2, 0},      {GVM_OP_MOV_IMM, 3, 0, 4},
      {GVM_OP_ADD, 0, 3, 0},       {GVM_OP_MOV_IMM, 4, 0, 5},  {GVM_OP_ADD, 0, 4, 0},
      {GVM_OP_MOV_IMM, 5, 0, 20},  {GVM_OP_ADD, 0, 5, 0},      {GVM_OP_MOV_IMM, 6, 0, 1},
      {GVM_OP_ADD, 0, 6, 0},       {GVM_OP_MOV_IMM, 7, 0, 8},  {GVM_OP_ADD, 0, 7, 0},
      {GVM_OP_MOV_IMM, 8, 0, 100}, {GVM_OP_ADD, 0, 8, 0},      {GVM_OP_HALT, 0, 0, 0},
  };
  const size_t instruction_count = sizeof(program) / sizeof(program[0]);
  long iterations = 500000;
  long i;
  clock_t start;
  clock_t end;
  double seconds;
  double mips;
  int rc;

  if (argc > 1) {
    iterations = strtol(argv[1], NULL, 10);
    if (iterations <= 0) {
      fprintf(stderr, "iterations must be > 0\n");
      return 2;
    }
  }

  start = clock();
  for (i = 0; i < iterations; ++i) {
    graphion_vm_init(&vm);
    rc = graphion_vm_load(&vm, program, instruction_count);
    if (rc != 0) {
      fprintf(stderr, "load failed rc=%d\n", rc);
      return 3;
    }
    rc = graphion_vm_run(&vm);
    if (rc != 0) {
      fprintf(stderr, "run failed rc=%d\n", rc);
      return 4;
    }
  }
  end = clock();

  seconds = elapsed_seconds(start, end);
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mips = ((double)(iterations * (long)instruction_count) / seconds) / 1000000.0;

  printf("{\"benchmark\":\"vm_dispatch\",\"iterations\":%ld,\"instructions_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mips\":%.3f}\n",
         iterations, instruction_count, seconds, mips);
  return 0;
}
