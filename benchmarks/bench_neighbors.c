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
  const uint32_t base_frontier[] = {0, 3, 6};
  const size_t base_frontier_len = sizeof(base_frontier) / sizeof(base_frontier[0]);
  const size_t frontier_repeats = 32U;
  uint32_t frontier[3U * 32U];
  const size_t frontier_len = sizeof(frontier) / sizeof(frontier[0]);
  graphion_csr_graph graph;
  long iterations = 10000000;
  long i;
  double start;
  double end;
  double seconds;
  double ns_per_neighbor;
  double mteps;
  int rc;
  uint64_t checksum = 0U;
  size_t frontier_neighbor_work = 0U;
  graphion_frontier_mode mode;
  const char *mode_label;

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

  for (size_t repeat = 0; repeat < frontier_repeats; ++repeat) {
    for (size_t j = 0; j < base_frontier_len; ++j) {
      frontier[(repeat * base_frontier_len) + j] = base_frontier[j];
    }
  }

  for (size_t j = 0; j < frontier_len; ++j) {
    frontier_neighbor_work += graphion_csr_graph_neighbor_count(&graph, frontier[j]);
  }
  mode = graphion_csr_graph_recommend_frontier_mode(&graph, frontier_len, frontier_neighbor_work);
  mode_label = mode == GRAPHION_FRONTIER_MODE_DENSE ? "dense" : "sparse";

  start = now_seconds();
  for (i = 0; i < iterations; ++i) {
    for (size_t j = 0; j < frontier_len; ++j) {
      const uint32_t node = frontier[j];
      const size_t begin = (size_t)graph.offsets[node];
      const size_t end_off = (size_t)graph.offsets[node + 1U];
      for (size_t k = begin; k < end_off; ++k) {
        checksum += (uint64_t)graph.neighbors[k];
      }
    }
  }
  end = now_seconds();

  seconds = end - start;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  mteps = ((double)iterations * (double)frontier_neighbor_work / seconds) / 1000000.0;
  ns_per_neighbor =
      (seconds * 1000000000.0) / ((double)iterations * (double)frontier_neighbor_work);

  printf("{\"benchmark\":\"neighbor_iteration\",\"iterations\":%ld,"
         "\"frontier_len\":%zu,\"neighbors_per_iteration\":%zu,"
         "\"frontier_neighbor_work\":%zu,\"recommended_frontier_mode\":\"%s\","
         "\"seconds\":%.6f,\"mteps\":%.3f,\"ns_per_neighbor\":%.3f,"
         "\"checksum\":%llu}\n",
         iterations, frontier_len, frontier_neighbor_work, frontier_neighbor_work, mode_label,
         seconds, mteps, ns_per_neighbor,
         (unsigned long long)checksum);
  return 0;
}
