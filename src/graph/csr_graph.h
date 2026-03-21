/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_GRAPH_CSR_GRAPH_H
#define GRAPHION_GRAPH_CSR_GRAPH_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t node_count;
  size_t edge_count;
  const uint32_t *offsets;
  const uint32_t *neighbors;
  const int64_t *weights;
  const uint32_t *edge_attrs;
} graphion_csr_graph;

typedef enum {
  GRAPHION_FRONTIER_MODE_SPARSE = 0,
  GRAPHION_FRONTIER_MODE_DENSE = 1
} graphion_frontier_mode;

int graphion_csr_graph_init(graphion_csr_graph *graph,
                            size_t node_count,
                            size_t edge_count,
                            const uint32_t *offsets,
                            const uint32_t *neighbors);
int graphion_csr_graph_init_with_edge_data(graphion_csr_graph *graph,
                                           size_t node_count,
                                           size_t edge_count,
                                           const uint32_t *offsets,
                                           const uint32_t *neighbors,
                                           const int64_t *weights,
                                           const uint32_t *edge_attrs);

size_t graphion_csr_graph_neighbor_count(const graphion_csr_graph *graph, uint32_t node);
const uint32_t *graphion_csr_graph_neighbors(const graphion_csr_graph *graph, uint32_t node);
const int64_t *graphion_csr_graph_weights(const graphion_csr_graph *graph, uint32_t node);
const uint32_t *graphion_csr_graph_edge_attrs(const graphion_csr_graph *graph, uint32_t node);
int graphion_csr_graph_has_weights(const graphion_csr_graph *graph);
int graphion_csr_graph_has_edge_attrs(const graphion_csr_graph *graph);
graphion_frontier_mode graphion_csr_graph_recommend_frontier_mode(const graphion_csr_graph *graph,
                                                                  size_t frontier_len,
                                                                  size_t frontier_neighbor_work);

int graphion_bfs_levels(const graphion_csr_graph *graph,
                        uint32_t source,
                        int32_t *levels,
                        uint32_t *queue,
                        size_t queue_capacity);

#endif
