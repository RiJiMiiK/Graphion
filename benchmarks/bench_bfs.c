/* SPDX-License-Identifier: MIT */

#include "graph/csr_graph.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double elapsed_seconds(clock_t start, clock_t end) {
  return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(int argc, char **argv) {
  const uint32_t offsets[] = {0, 2, 4, 6, 9, 12, 14, 17, 19};
  const uint32_t neighbors[] = {
      1, 2, 3, 4, 4, 5, 0, 6, 7, 1, 5, 7, 6, 7, 0, 2, 3, 1, 4,
  };
  graphion_csr_graph graph;
  int32_t levels[8];
  uint32_t queue[8];
  long iterations = 200000;
  long i;
  clock_t start;
  clock_t end;
  double seconds;
  double mteps;
  int rc;

  if (argc > 1) {
    iterations = strtol(argv[1], NULL, 10);
    if (iterations <= 0) {
      fprintf(stderr, "iterations must be > 0\n");
      return 2;
    }
  }

  rc = graphion_csr_graph_init(&graph, 8U, 19U, offsets, neighbors);
  if (rc != 0) {
    fprintf(stderr, "graph init failed rc=%d\n", rc);
    return 3;
  }

  start = clock();
  for (i = 0; i < iterations; ++i) {
    rc = graphion_bfs_levels(&graph, 0U, levels, queue, 8U);
    if (rc != 0) {
      fprintf(stderr, "bfs failed rc=%d\n", rc);
      return 4;
    }
  }
  end = clock();

  seconds = elapsed_seconds(start, end);
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mteps = ((double)(iterations * (long)graph.edge_count) / seconds) / 1000000.0;

  printf("{\"benchmark\":\"bfs_levels\",\"iterations\":%ld,\"edges_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mteps\":%.3f}\n",
         iterations, graph.edge_count, seconds, mteps);
  return 0;
}
