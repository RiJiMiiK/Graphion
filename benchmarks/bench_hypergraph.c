/* SPDX-License-Identifier: MIT */

#include "graph/hypergraph.h"

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
  const uint32_t node_offsets[] = {0, 2, 5, 8, 10, 12};
  const uint32_t node_hyperedges[] = {0, 1, 0, 2, 3, 1, 2, 3, 2, 3, 0, 1};
  const uint32_t hyperedge_offsets[] = {0, 3, 6, 9, 12};
  const uint32_t hyperedge_nodes[] = {0, 1, 4, 0, 2, 4, 1, 2, 3, 1, 2, 3};
  graphion_hypergraph graph;
  long iterations = 500000;
  long i;
  uint64_t checksum = 0U;
  double start;
  double end;
  double seconds;
  double mips;
  double ns_per_incidence;
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

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    const uint32_t *p = graph.node_hyperedges;
    const uint32_t *const endp = graph.node_hyperedges + graph.incidence_count;
    while ((endp - p) >= 4) {
      checksum += (uint64_t)p[0] + (uint64_t)p[1] + (uint64_t)p[2] + (uint64_t)p[3];
      p += 4;
    }
    while (p < endp) {
      checksum += (uint64_t)(*p);
      p++;
    }
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mips = ((double)(iterations * (long)graph.incidence_count) / seconds) / 1000000.0;
  ns_per_incidence = (seconds * 1000000000.0) / ((double)iterations * (double)graph.incidence_count);

  printf("{\"benchmark\":\"hypergraph_incidence\",\"iterations\":%ld,\"incidence_per_iteration\":%zu,"
         "\"seconds\":%.6f,\"mips\":%.3f,\"ns_per_incidence\":%.3f,\"checksum\":%llu}\n",
         iterations, graph.incidence_count, seconds, mips, ns_per_incidence, (unsigned long long)checksum);
  return 0;
}
