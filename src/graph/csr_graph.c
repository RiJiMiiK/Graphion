/* SPDX-License-Identifier: MIT */

#include "graph/csr_graph.h"

#include <stddef.h>
#include <stdint.h>

static int is_valid_node(const graphion_csr_graph *graph, uint32_t node) {
  return graph != NULL && (size_t)node < graph->node_count;
}

int graphion_csr_graph_init(graphion_csr_graph *graph,
                            size_t node_count,
                            size_t edge_count,
                            const uint32_t *offsets,
                            const uint32_t *neighbors) {
  size_t i;
  if (graph == NULL || offsets == NULL || neighbors == NULL) {
    return -1;
  }
  if (node_count == 0U) {
    return -2;
  }
  if ((size_t)offsets[0] != 0U) {
    return -3;
  }
  for (i = 0; i < node_count; ++i) {
    if (offsets[i] > offsets[i + 1U]) {
      return -4;
    }
  }
  if ((size_t)offsets[node_count] != edge_count) {
    return -5;
  }
  for (i = 0; i < edge_count; ++i) {
    if ((size_t)neighbors[i] >= node_count) {
      return -6;
    }
  }

  graph->node_count = node_count;
  graph->edge_count = edge_count;
  graph->offsets = offsets;
  graph->neighbors = neighbors;
  return 0;
}

size_t graphion_csr_graph_neighbor_count(const graphion_csr_graph *graph, uint32_t node) {
  if (!is_valid_node(graph, node)) {
    return 0U;
  }
  return (size_t)(graph->offsets[node + 1U] - graph->offsets[node]);
}

const uint32_t *graphion_csr_graph_neighbors(const graphion_csr_graph *graph, uint32_t node) {
  if (!is_valid_node(graph, node)) {
    return NULL;
  }
  return &graph->neighbors[graph->offsets[node]];
}

int graphion_bfs_levels(const graphion_csr_graph *graph,
                        uint32_t source,
                        int32_t *levels,
                        uint32_t *queue,
                        size_t queue_capacity) {
  size_t i;
  size_t q_head = 0U;
  size_t q_tail = 0U;

  if (graph == NULL || levels == NULL || queue == NULL) {
    return -1;
  }
  if (!is_valid_node(graph, source)) {
    return -2;
  }
  if (queue_capacity < graph->node_count) {
    return -3;
  }

  for (i = 0; i < graph->node_count; ++i) {
    levels[i] = -1;
  }

  levels[source] = 0;
  queue[q_tail++] = source;

  while (q_head < q_tail) {
    const uint32_t u = queue[q_head++];
    const int32_t next_level = levels[u] + 1;
    const size_t begin = (size_t)graph->offsets[u];
    const size_t end = (size_t)graph->offsets[u + 1U];
    size_t j;

    for (j = begin; j < end; ++j) {
      const uint32_t v = graph->neighbors[j];
      if (levels[v] == -1) {
        levels[v] = next_level;
        queue[q_tail++] = v;
      }
    }
  }

  return 0;
}
