/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_GRAPH_HYPERGRAPH_H
#define GRAPHION_GRAPH_HYPERGRAPH_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t node_count;
  size_t hyperedge_count;
  size_t incidence_count;
  const uint32_t *node_offsets;
  const uint32_t *node_hyperedges;
  const uint32_t *hyperedge_offsets;
  const uint32_t *hyperedge_nodes;
} graphion_hypergraph;

int graphion_hypergraph_init(graphion_hypergraph *graph,
                             size_t node_count,
                             size_t hyperedge_count,
                             size_t incidence_count,
                             const uint32_t *node_offsets,
                             const uint32_t *node_hyperedges,
                             const uint32_t *hyperedge_offsets,
                             const uint32_t *hyperedge_nodes);

size_t graphion_hypergraph_incident_count(const graphion_hypergraph *graph, uint32_t node);
const uint32_t *graphion_hypergraph_incident(const graphion_hypergraph *graph, uint32_t node);

size_t graphion_hypergraph_hyperedge_size(const graphion_hypergraph *graph, uint32_t hyperedge);
const uint32_t *graphion_hypergraph_hyperedge_nodes(const graphion_hypergraph *graph, uint32_t hyperedge);

#endif
