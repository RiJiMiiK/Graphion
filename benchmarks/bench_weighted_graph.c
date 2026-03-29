/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BENCH_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

static void bench_reset_regs(graphion_vm *vm) {
  size_t i;
  for (i = 0U; i < (sizeof(vm->regs) / sizeof(vm->regs[0])); ++i) {
    vm->regs[i].kind = GVM_VALUE_INT;
    vm->regs[i].as.int_value = 0;
  }
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
  const size_t inner_repeats = 4U;
  enum {
    NODE_COUNT = 8,
    EDGES_PER_NODE = 1024,
    EDGE_COUNT = NODE_COUNT * EDGES_PER_NODE
  };
  uint32_t offsets[NODE_COUNT + 1U];
  uint32_t neighbors[EDGE_COUNT];
  int64_t weights[EDGE_COUNT];
  uint32_t edge_attrs[EDGE_COUNT];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0U, 0U, 0},
      {GVM_OP_NEIGHBOR_WEIGHT_SUM, 0U, 1U, 0},
      {GVM_OP_MOV_IMM, 2U, 0U, 2},
      {GVM_OP_NEIGHBOR_ATTR_SUM, 2U, 3U, 0},
      {GVM_OP_MOV_IMM, 4U, 0U, 4},
      {GVM_OP_NEIGHBOR_WEIGHT_SUM, 4U, 5U, 0},
      {GVM_OP_MOV_IMM, 6U, 0U, 6},
      {GVM_OP_NEIGHBOR_ATTR_SUM, 6U, 7U, 0},
      {GVM_OP_MOV_IMM, 8U, 0U, 1},
      {GVM_OP_NEIGHBOR_WEIGHT_SUM, 8U, 9U, 0},
      {GVM_OP_MOV_IMM, 10U, 0U, 3},
      {GVM_OP_NEIGHBOR_ATTR_SUM, 10U, 11U, 0},
      {GVM_OP_MOV_IMM, 12U, 0U, 5},
      {GVM_OP_NEIGHBOR_WEIGHT_SUM, 12U, 13U, 0},
      {GVM_OP_MOV_IMM, 14U, 0U, 7},
      {GVM_OP_NEIGHBOR_ATTR_SUM, 14U, 15U, 0},
      {GVM_OP_HALT, 0U, 0U, 0},
  };
  const size_t instruction_count = sizeof(program) / sizeof(program[0]);
  graphion_csr_graph graph;
  graphion_vm vm;
  long iterations = 300000;
  double start;
  double end;
  double seconds;
  double mteps;
  double ns_per_instruction;
  double ns_per_edge_data;
  uint64_t checksum = 0U;
  const size_t edge_data_items_per_iteration = 8192U;
  int rc;

  if (argc > 1) {
    iterations = strtol(argv[1], NULL, 10);
    if (iterations <= 0) {
      fprintf(stderr, "iterations must be > 0\n");
      return 2;
    }
  }

  for (size_t node = 0U; node <= NODE_COUNT; ++node) {
    offsets[node] = (uint32_t)(node * EDGES_PER_NODE);
  }
  for (size_t node = 0U; node < NODE_COUNT; ++node) {
    for (size_t edge = 0U; edge < EDGES_PER_NODE; ++edge) {
      const size_t idx = (node * EDGES_PER_NODE) + edge;
      neighbors[idx] = (uint32_t)((node + edge + 1U) % NODE_COUNT);
      weights[idx] = (int64_t)(idx + 1U);
      edge_attrs[idx] = (uint32_t)(100U + idx);
    }
  }

  rc = graphion_csr_graph_init_with_edge_data(&graph, NODE_COUNT, EDGE_COUNT, offsets, neighbors,
                                              weights, edge_attrs);
  if (rc != 0) {
    fprintf(stderr, "graph init failed rc=%d\n", rc);
    return 3;
  }

  graphion_vm_init(&vm);
  graphion_vm_bind_csr(&vm, &graph, NULL, NULL, 0U);
  rc = graphion_vm_load(&vm, program, instruction_count);
  if (rc != 0) {
    fprintf(stderr, "load failed rc=%d\n", rc);
    return 4;
  }

  start = now_seconds();
  for (long i = 0; i < iterations; ++i) {
    for (size_t repeat = 0; repeat < inner_repeats; ++repeat) {
      bench_reset_regs(&vm);
      vm.pc = 0U;
      vm.halted = false;
      rc = graphion_vm_run(&vm);
      if (rc != 0) {
        fprintf(stderr, "run failed rc=%d\n", rc);
        return 5;
      }
      checksum += (uint64_t)(BENCH_REG_I(vm, 1) + BENCH_REG_I(vm, 3) + BENCH_REG_I(vm, 5) +
                             BENCH_REG_I(vm, 7) + BENCH_REG_I(vm, 9) + BENCH_REG_I(vm, 11) +
                             BENCH_REG_I(vm, 13) + BENCH_REG_I(vm, 15));
    }
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mteps =
      (((double)iterations) * ((double)(edge_data_items_per_iteration * inner_repeats)) / seconds) / 1000000.0;
  ns_per_instruction =
      (seconds * 1000000000.0) / ((double)iterations * (double)(instruction_count * inner_repeats));
  ns_per_edge_data =
      (seconds * 1000000000.0) / ((double)iterations * (double)(edge_data_items_per_iteration * inner_repeats));

  printf("{\"benchmark\":\"weighted_neighbor_sums\",\"iterations\":%ld,"
         "\"instructions_per_iteration\":%zu,\"edge_data_items_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mteps\":%.3f,\"ns_per_instruction\":%.3f,"
         "\"ns_per_edge_data\":%.3f,\"checksum\":%llu}\n",
         iterations, instruction_count * inner_repeats, edge_data_items_per_iteration * inner_repeats, seconds, mteps,
         ns_per_instruction, ns_per_edge_data, (unsigned long long)checksum);
  return 0;
}
