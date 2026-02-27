/* SPDX-License-Identifier: MIT */

#include "graph/csr_graph.h"

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
  double start;
  double end;
  double seconds;
  double mteps;
  double ns_per_edge;
  int rc;
  uint64_t checksum = 0U;

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

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    rc = graphion_bfs_levels(&graph, 0U, levels, queue, 8U);
    if (rc != 0) {
      fprintf(stderr, "bfs failed rc=%d\n", rc);
      return 4;
    }
    checksum += (uint64_t)levels[7];
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mteps = ((double)(iterations * (long)graph.edge_count) / seconds) / 1000000.0;
  ns_per_edge = (seconds * 1000000000.0) / ((double)iterations * (double)graph.edge_count);

  printf("{\"benchmark\":\"bfs_levels\",\"iterations\":%ld,\"edges_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mteps\":%.3f,\"ns_per_edge\":%.3f,\"checksum\":%llu}\n",
         iterations, graph.edge_count, seconds, mteps, ns_per_edge, (unsigned long long)checksum);
  return 0;
}
