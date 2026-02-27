/* SPDX-License-Identifier: MIT */

#include "graph/hypergraph.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double elapsed_seconds(clock_t start, clock_t end) {
  return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(int argc, char **argv) {
  const uint32_t node_offsets[] = {0, 2, 5, 8, 10, 12};
  const uint32_t node_hyperedges[] = {0, 1, 0, 2, 3, 1, 2, 3, 2, 3, 0, 1};
  const uint32_t hyperedge_offsets[] = {0, 3, 6, 9, 12};
  const uint32_t hyperedge_nodes[] = {0, 1, 4, 0, 2, 4, 1, 2, 3, 1, 2, 3};
  graphion_hypergraph graph;
  long iterations = 500000;
  long i;
  uint64_t checksum = 0U;
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

  rc = graphion_hypergraph_init(&graph, 5U, 4U, 12U, node_offsets, node_hyperedges, hyperedge_offsets,
                                hyperedge_nodes);
  if (rc != 0) {
    fprintf(stderr, "hypergraph init failed rc=%d\n", rc);
    return 3;
  }

  start = clock();
  for (i = 0; i < iterations; ++i) {
    size_t n;
    for (n = 0U; n < graph.node_count; ++n) {
      const size_t c = graphion_hypergraph_incident_count(&graph, (uint32_t)n);
      const uint32_t *hs = graphion_hypergraph_incident(&graph, (uint32_t)n);
      size_t j;
      for (j = 0U; j < c; ++j) {
        checksum += hs[j];
      }
    }
  }
  end = clock();

  seconds = elapsed_seconds(start, end);
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mips = ((double)(iterations * (long)graph.incidence_count) / seconds) / 1000000.0;

  printf("{\"benchmark\":\"hypergraph_incidence\",\"iterations\":%ld,\"incidence_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mips\":%.3f,\"checksum\":%llu}\n",
         iterations, graph.incidence_count, seconds, mips, (unsigned long long)checksum);
  return 0;
}
