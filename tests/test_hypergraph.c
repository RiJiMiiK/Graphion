/* SPDX-License-Identifier: MIT */

#include "graph/hypergraph.h"

#include <stddef.h>
#include <stdint.h>

int test_hypergraph_init_and_queries(void) {
  /* H0:{0,1,2}, H1:{1,3}, H2:{2,3} */
  const uint32_t node_offsets[] = {0, 1, 3, 5, 7};
  const uint32_t node_hyperedges[] = {0, 0, 1, 0, 2, 1, 2};
  const uint32_t hyperedge_offsets[] = {0, 3, 5, 7};
  const uint32_t hyperedge_nodes[] = {0, 1, 2, 1, 3, 2, 3};
  graphion_hypergraph g;
  int rc;

  rc = graphion_hypergraph_init(&g, 4U, 3U, 7U, node_offsets, node_hyperedges, hyperedge_offsets,
                                hyperedge_nodes);
  if (rc != 0) {
    return 1;
  }
  if (graphion_hypergraph_incident_count(&g, 1U) != 2U) {
    return 2;
  }
  if (graphion_hypergraph_incident(&g, 2U)[1] != 2U) {
    return 3;
  }
  if (graphion_hypergraph_hyperedge_size(&g, 0U) != 3U) {
    return 4;
  }
  if (graphion_hypergraph_hyperedge_nodes(&g, 1U)[1] != 3U) {
    return 5;
  }
  if (graphion_hypergraph_incident_sum(&g, 1U) != 1U) {
    return 6;
  }
  if (graphion_hypergraph_hyperedge_node_sum(&g, 0U) != 3U) {
    return 7;
  }
  return 0;
}
