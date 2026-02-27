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
} graphion_csr_graph;

int graphion_csr_graph_init(graphion_csr_graph *graph,
                            size_t node_count,
                            size_t edge_count,
                            const uint32_t *offsets,
                            const uint32_t *neighbors);

size_t graphion_csr_graph_neighbor_count(const graphion_csr_graph *graph, uint32_t node);
const uint32_t *graphion_csr_graph_neighbors(const graphion_csr_graph *graph, uint32_t node);

int graphion_bfs_levels(const graphion_csr_graph *graph,
                        uint32_t source,
                        int32_t *levels,
                        uint32_t *queue,
                        size_t queue_capacity);

#endif
