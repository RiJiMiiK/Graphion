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
  long iterations = 10000000;
  long i;
  uint64_t checksum = 0U;
  size_t memberships_per_iteration;
  double start;
  double end;
  double seconds;
  double mteps;
  double ns_per_membership;
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

  memberships_per_iteration = graph.incidence_count + graph.incidence_count;

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    for (uint32_t node = 0; node < (uint32_t)graph.node_count; ++node) {
      size_t count = graphion_hypergraph_incident_count(&graph, node);
      const uint32_t *incident = graphion_hypergraph_incident(&graph, node);
      for (size_t j = 0; j < count; ++j) {
        checksum += (uint64_t)incident[j];
      }
    }
    for (uint32_t hyperedge = 0; hyperedge < (uint32_t)graph.hyperedge_count; ++hyperedge) {
      size_t count = graphion_hypergraph_hyperedge_size(&graph, hyperedge);
      const uint32_t *nodes = graphion_hypergraph_hyperedge_nodes(&graph, hyperedge);
      for (size_t j = 0; j < count; ++j) {
        checksum += (uint64_t)nodes[j];
      }
    }
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mteps = ((double)(iterations * (long)memberships_per_iteration) / seconds) / 1000000.0;
  ns_per_membership =
      (seconds * 1000000000.0) / ((double)iterations * (double)memberships_per_iteration);

  printf("{\"benchmark\":\"hypergraph_traversal\",\"iterations\":%ld,"
         "\"memberships_per_iteration\":%zu,\"seconds\":%.6f,"
         "\"mteps\":%.3f,\"ns_per_membership\":%.3f,\"checksum\":%llu}\n",
         iterations, memberships_per_iteration, seconds, mteps, ns_per_membership,
         (unsigned long long)checksum);
  return 0;
}
