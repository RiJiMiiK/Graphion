/* SPDX-License-Identifier: MIT */

#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include "vm/vm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double elapsed_seconds(clock_t start, clock_t end) {
  return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(int argc, char **argv) {
  graphion_vm vm;
  graphion_csr_graph csr;
  graphion_hypergraph hg;
  const uint32_t csr_offsets[] = {0, 2, 3, 5, 6};
  const uint32_t csr_neighbors[] = {1, 2, 3, 0, 3, 1};
  const uint32_t node_offsets[] = {0, 1, 3, 5, 7};
  const uint32_t node_hyperedges[] = {0, 0, 1, 0, 2, 1, 2};
  const uint32_t hyperedge_offsets[] = {0, 3, 5, 7};
  const uint32_t hyperedge_nodes[] = {0, 1, 2, 1, 3, 2, 3};
  int32_t levels[4];
  uint32_t queue[4];
  const graphion_insn program[] = {
      {GVM_OP_MOV_IMM, 0, 0, 0},          {GVM_OP_BFS_LEVELS, 0, 1, 0},
      {GVM_OP_MOV_IMM, 2, 0, 1},          {GVM_OP_INCIDENT_COUNT, 2, 3, 0},
      {GVM_OP_MOV_IMM, 4, 0, 0},          {GVM_OP_HYPEREDGE_SIZE, 4, 5, 0},
      {GVM_OP_ADD, 6, 1, 0},              {GVM_OP_ADD, 6, 3, 0},
      {GVM_OP_ADD, 6, 5, 0},              {GVM_OP_HALT, 0, 0, 0},
  };
  const size_t instruction_count = sizeof(program) / sizeof(program[0]);
  long iterations = 300000;
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

  rc = graphion_csr_graph_init(&csr, 4U, 6U, csr_offsets, csr_neighbors);
  if (rc != 0) {
    return 3;
  }
  rc = graphion_hypergraph_init(&hg, 4U, 3U, 7U, node_offsets, node_hyperedges, hyperedge_offsets,
                                hyperedge_nodes);
  if (rc != 0) {
    return 4;
  }

  start = clock();
  for (i = 0; i < iterations; ++i) {
    graphion_vm_init(&vm);
    graphion_vm_bind_csr(&vm, &csr, levels, queue, 4U);
    graphion_vm_bind_hypergraph(&vm, &hg);
    rc = graphion_vm_load(&vm, program, instruction_count);
    if (rc != 0) {
      return 5;
    }
    rc = graphion_vm_run(&vm);
    if (rc != 0) {
      return 6;
    }
  }
  end = clock();

  seconds = elapsed_seconds(start, end);
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mips = ((double)(iterations * (long)instruction_count) / seconds) / 1000000.0;

  printf("{\"benchmark\":\"vm_graph_ops\",\"iterations\":%ld,\"instructions_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mips\":%.3f}\n",
         iterations, instruction_count, seconds, mips);
  return 0;
}
