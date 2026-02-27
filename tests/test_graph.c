/* SPDX-License-Identifier: MIT */

#include "graph/csr_graph.h"

#include <stddef.h>
#include <stdint.h>

int test_graph_init_and_neighbors(void) {
  const uint32_t offsets[] = {0, 2, 3, 5, 6};
  const uint32_t neighbors[] = {1, 2, 3, 0, 3, 1};
  graphion_csr_graph g;
  const int rc = graphion_csr_graph_init(&g, 4U, 6U, offsets, neighbors);
  if (rc != 0) {
    return 1;
  }
  if (graphion_csr_graph_neighbor_count(&g, 0U) != 2U) {
    return 2;
  }
  if (graphion_csr_graph_neighbors(&g, 1U)[0] != 3U) {
    return 3;
  }
  return 0;
}

int test_graph_bfs_levels(void) {
  const uint32_t offsets[] = {0, 2, 3, 5, 6};
  const uint32_t neighbors[] = {1, 2, 3, 0, 3, 1};
  graphion_csr_graph g;
  int32_t levels[4];
  uint32_t queue[4];
  int rc;

  rc = graphion_csr_graph_init(&g, 4U, 6U, offsets, neighbors);
  if (rc != 0) {
    return 1;
  }
  rc = graphion_bfs_levels(&g, 0U, levels, queue, 4U);
  if (rc != 0) {
    return 2;
  }
  if (levels[0] != 0 || levels[1] != 1 || levels[2] != 1 || levels[3] != 2) {
    return 3;
  }
  return 0;
}
