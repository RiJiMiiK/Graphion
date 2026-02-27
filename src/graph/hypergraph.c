/* SPDX-License-Identifier: MIT */

#include "graph/hypergraph.h"

#include <stddef.h>
#include <stdint.h>

static int valid_node(const graphion_hypergraph *graph, uint32_t node) {
  return graph != NULL && (size_t)node < graph->node_count;
}

static int valid_hyperedge(const graphion_hypergraph *graph, uint32_t hyperedge) {
  return graph != NULL && (size_t)hyperedge < graph->hyperedge_count;
}

static int offsets_non_decreasing(const uint32_t *offsets, size_t count) {
  size_t i;
  for (i = 0; i < count; ++i) {
    if (offsets[i] > offsets[i + 1U]) {
      return 0;
    }
  }
  return 1;
}

int graphion_hypergraph_init(graphion_hypergraph *graph,
                             size_t node_count,
                             size_t hyperedge_count,
                             size_t incidence_count,
                             const uint32_t *node_offsets,
                             const uint32_t *node_hyperedges,
                             const uint32_t *hyperedge_offsets,
                             const uint32_t *hyperedge_nodes) {
  size_t i;
  if (graph == NULL || node_offsets == NULL || node_hyperedges == NULL || hyperedge_offsets == NULL ||
      hyperedge_nodes == NULL) {
    return -1;
  }
  if (node_count == 0U || hyperedge_count == 0U) {
    return -2;
  }
  if ((size_t)node_offsets[0] != 0U || (size_t)hyperedge_offsets[0] != 0U) {
    return -3;
  }
  if (!offsets_non_decreasing(node_offsets, node_count) ||
      !offsets_non_decreasing(hyperedge_offsets, hyperedge_count)) {
    return -4;
  }
  if ((size_t)node_offsets[node_count] != incidence_count ||
      (size_t)hyperedge_offsets[hyperedge_count] != incidence_count) {
    return -5;
  }
  for (i = 0; i < incidence_count; ++i) {
    if ((size_t)node_hyperedges[i] >= hyperedge_count || (size_t)hyperedge_nodes[i] >= node_count) {
      return -6;
    }
  }

  graph->node_count = node_count;
  graph->hyperedge_count = hyperedge_count;
  graph->incidence_count = incidence_count;
  graph->node_offsets = node_offsets;
  graph->node_hyperedges = node_hyperedges;
  graph->hyperedge_offsets = hyperedge_offsets;
  graph->hyperedge_nodes = hyperedge_nodes;
  return 0;
}

size_t graphion_hypergraph_incident_count(const graphion_hypergraph *graph, uint32_t node) {
  if (!valid_node(graph, node)) {
    return 0U;
  }
  return (size_t)(graph->node_offsets[node + 1U] - graph->node_offsets[node]);
}

const uint32_t *graphion_hypergraph_incident(const graphion_hypergraph *graph, uint32_t node) {
  if (!valid_node(graph, node)) {
    return NULL;
  }
  return &graph->node_hyperedges[graph->node_offsets[node]];
}

size_t graphion_hypergraph_hyperedge_size(const graphion_hypergraph *graph, uint32_t hyperedge) {
  if (!valid_hyperedge(graph, hyperedge)) {
    return 0U;
  }
  return (size_t)(graph->hyperedge_offsets[hyperedge + 1U] - graph->hyperedge_offsets[hyperedge]);
}

const uint32_t *graphion_hypergraph_hyperedge_nodes(const graphion_hypergraph *graph, uint32_t hyperedge) {
  if (!valid_hyperedge(graph, hyperedge)) {
    return NULL;
  }
  return &graph->hyperedge_nodes[graph->hyperedge_offsets[hyperedge]];
}

uint64_t graphion_hypergraph_incident_sum(const graphion_hypergraph *graph, uint32_t node) {
  uint64_t total = 0U;
  size_t count;
  const uint32_t *values;
  size_t i = 0U;
  size_t n4;

  if (!valid_node(graph, node)) {
    return 0U;
  }
  values = &graph->node_hyperedges[graph->node_offsets[node]];
  count = (size_t)(graph->node_offsets[node + 1U] - graph->node_offsets[node]);
  if (count <= 3U) {
    if (count >= 1U) {
      total += (uint64_t)values[0];
    }
    if (count >= 2U) {
      total += (uint64_t)values[1];
    }
    if (count == 3U) {
      total += (uint64_t)values[2];
    }
    return total;
  }

  n4 = count & ~(size_t)3U;
  while (i < n4) {
    total += (uint64_t)values[i] + (uint64_t)values[i + 1U] + (uint64_t)values[i + 2U] +
             (uint64_t)values[i + 3U];
    i += 4U;
  }
  while (i < count) {
    total += (uint64_t)values[i];
    i++;
  }
  return total;
}

uint64_t graphion_hypergraph_hyperedge_node_sum(const graphion_hypergraph *graph, uint32_t hyperedge) {
  uint64_t total = 0U;
  size_t count;
  const uint32_t *values;
  size_t i = 0U;
  size_t n4;

  if (!valid_hyperedge(graph, hyperedge)) {
    return 0U;
  }
  values = &graph->hyperedge_nodes[graph->hyperedge_offsets[hyperedge]];
  count = (size_t)(graph->hyperedge_offsets[hyperedge + 1U] - graph->hyperedge_offsets[hyperedge]);
  if (count <= 3U) {
    if (count >= 1U) {
      total += (uint64_t)values[0];
    }
    if (count >= 2U) {
      total += (uint64_t)values[1];
    }
    if (count == 3U) {
      total += (uint64_t)values[2];
    }
    return total;
  }

  n4 = count & ~(size_t)3U;
  while (i < n4) {
    total += (uint64_t)values[i] + (uint64_t)values[i + 1U] + (uint64_t)values[i + 2U] +
             (uint64_t)values[i + 3U];
    i += 4U;
  }
  while (i < count) {
    total += (uint64_t)values[i];
    i++;
  }
  return total;
}
