/* SPDX-License-Identifier: MIT */

#include "graph/csr_graph.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
  CAL_NODE_COUNT = 1024,
  CAL_HIGH_DEGREE_COUNT = 128,
  CAL_HIGH_DEGREE = 48,
  CAL_LOW_DEGREE = 8,
  CAL_ITERATIONS = 10000
};

typedef struct {
  char name[32];
  size_t frontier_len;
  size_t frontier_neighbor_work;
  size_t frontier_len_percent;
  size_t frontier_work_percent;
  double sparse_seconds;
  double dense_seconds;
} calibration_case;

static double now_seconds(void) {
#if defined(TIME_UTC)
  struct timespec ts;
  (void)timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
#else
  return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

static size_t fill_frontier(uint32_t *frontier,
                            size_t frontier_len,
                            size_t start_node,
                            size_t end_node) {
  size_t count = 0U;
  size_t node = start_node;
  while (count < frontier_len && node < end_node) {
    frontier[count++] = (uint32_t)node;
    ++node;
  }
  return count;
}

static void build_mixed_degree_graph(uint32_t *offsets, uint32_t *neighbors, size_t edge_count) {
  size_t cursor = 0U;
  for (size_t node = 0U; node < CAL_NODE_COUNT; ++node) {
    const size_t degree = node < CAL_HIGH_DEGREE_COUNT ? CAL_HIGH_DEGREE : CAL_LOW_DEGREE;
    offsets[node] = (uint32_t)cursor;
    for (size_t edge = 0U; edge < degree; ++edge) {
      const uint32_t neighbor =
          (uint32_t)((node * 17U + edge * 29U + edge + 3U) % CAL_NODE_COUNT);
      neighbors[cursor++] = neighbor;
    }
  }
  offsets[CAL_NODE_COUNT] = (uint32_t)edge_count;
}

static size_t compute_frontier_neighbor_work(const graphion_csr_graph *graph,
                                             const uint32_t *frontier,
                                             size_t frontier_len) {
  size_t total = 0U;
  for (size_t i = 0U; i < frontier_len; ++i) {
    total += graphion_csr_graph_neighbor_count(graph, frontier[i]);
  }
  return total;
}

static uint64_t run_sparse_push(const graphion_csr_graph *graph,
                                const uint32_t *frontier,
                                size_t frontier_len,
                                size_t iterations) {
  uint64_t checksum = 0U;
  for (size_t iteration = 0U; iteration < iterations; ++iteration) {
    for (size_t i = 0U; i < frontier_len; ++i) {
      const uint32_t node = frontier[i];
      const size_t begin = (size_t)graph->offsets[node];
      const size_t end = (size_t)graph->offsets[node + 1U];
      for (size_t edge = begin; edge < end; ++edge) {
        checksum += (uint64_t)graph->neighbors[edge];
      }
    }
  }
  return checksum;
}

static uint64_t run_dense_node_scan_proxy(const graphion_csr_graph *graph,
                                          const uint8_t *frontier_bitmap,
                                          size_t iterations) {
  uint64_t checksum = 0U;
  for (size_t iteration = 0U; iteration < iterations; ++iteration) {
    for (size_t node = 0U; node < graph->node_count; ++node) {
      if (frontier_bitmap[node] != 0U) {
        const size_t begin = (size_t)graph->offsets[node];
        const size_t end = (size_t)graph->offsets[node + 1U];
        for (size_t edge = begin; edge < end; ++edge) {
          checksum += (uint64_t)graph->neighbors[edge];
        }
      }
    }
  }
  return checksum;
}

static calibration_case measure_case(const char *name,
                                     const graphion_csr_graph *graph,
                                     const uint32_t *frontier,
                                     size_t frontier_len,
                                     const uint8_t *frontier_bitmap,
                                     int use_edge_scan_proxy) {
  calibration_case result;
  double start;
  double end;
  uint64_t sparse_checksum;
  uint64_t dense_checksum;

  (void)snprintf(result.name, sizeof(result.name), "%s", name);
  result.frontier_len = frontier_len;
  result.frontier_neighbor_work = compute_frontier_neighbor_work(graph, frontier, frontier_len);
  result.frontier_len_percent = (frontier_len * 100U) / graph->node_count;
  result.frontier_work_percent = graph->edge_count == 0U ? 0U : (result.frontier_neighbor_work * 100U) / graph->edge_count;

  start = now_seconds();
  sparse_checksum = run_sparse_push(graph, frontier, frontier_len, CAL_ITERATIONS);
  end = now_seconds();
  result.sparse_seconds = end - start;

  start = now_seconds();
  if (use_edge_scan_proxy) {
    dense_checksum = 0U;
    for (size_t iteration = 0U; iteration < CAL_ITERATIONS; ++iteration) {
      for (size_t node = 0U; node < graph->node_count; ++node) {
        const uint64_t active = frontier_bitmap[node] != 0U ? 1U : 0U;
        const size_t begin = (size_t)graph->offsets[node];
        const size_t end_off = (size_t)graph->offsets[node + 1U];
        for (size_t edge = begin; edge < end_off; ++edge) {
          dense_checksum += active * (uint64_t)graph->neighbors[edge];
        }
      }
    }
  } else {
    dense_checksum = run_dense_node_scan_proxy(graph, frontier_bitmap, CAL_ITERATIONS);
  }
  end = now_seconds();
  result.dense_seconds = end - start;

  if (result.sparse_seconds <= 0.0) {
    result.sparse_seconds = 1e-9;
  }
  if (result.dense_seconds <= 0.0) {
    result.dense_seconds = 1e-9;
  }

  result.dense_seconds += (dense_checksum == sparse_checksum) ? 1e-12 : 0.0;

  return result;
}

static void emit_case_json(const calibration_case *item, int last) {
  printf("    {\"name\":\"%s\",\"frontier_len\":%zu,\"frontier_neighbor_work\":%zu,"
         "\"frontier_len_percent\":%zu,\"frontier_work_percent\":%zu,"
         "\"sparse_seconds\":%.6f,\"dense_seconds\":%.6f,\"dense_vs_sparse_ratio\":%.3f}%s\n",
         item->name,
         item->frontier_len,
         item->frontier_neighbor_work,
         item->frontier_len_percent,
         item->frontier_work_percent,
         item->sparse_seconds,
         item->dense_seconds,
         item->dense_seconds / item->sparse_seconds,
         last ? "" : ",");
}

int main(void) {
  const size_t edge_count =
      (CAL_HIGH_DEGREE_COUNT * CAL_HIGH_DEGREE) + ((CAL_NODE_COUNT - CAL_HIGH_DEGREE_COUNT) * CAL_LOW_DEGREE);
  const size_t low_degree_frontiers[] = {64U, 96U, 128U, 160U, 192U, 224U};
  const size_t high_degree_frontiers[] = {8U, 16U, 24U, 32U, 40U, 48U, 56U, 64U, 72U, 80U};
  calibration_case cases[(sizeof(low_degree_frontiers) / sizeof(low_degree_frontiers[0])) +
                         (sizeof(high_degree_frontiers) / sizeof(high_degree_frontiers[0]))];
  uint32_t *offsets = NULL;
  uint32_t *neighbors = NULL;
  uint32_t *frontier = NULL;
  uint8_t *frontier_bitmap = NULL;
  graphion_csr_graph graph;
  int rc;
  size_t case_count = 0U;
  size_t calibrated_node_percent = 20U;
  size_t calibrated_work_percent = 35U;
  double best_low_ratio = 1e30;
  double best_high_ratio = 1e30;

  offsets = (uint32_t *)malloc((CAL_NODE_COUNT + 1U) * sizeof(uint32_t));
  neighbors = (uint32_t *)malloc(edge_count * sizeof(uint32_t));
  frontier = (uint32_t *)malloc(CAL_NODE_COUNT * sizeof(uint32_t));
  frontier_bitmap = (uint8_t *)malloc(CAL_NODE_COUNT * sizeof(uint8_t));
  if (offsets == NULL || neighbors == NULL || frontier == NULL || frontier_bitmap == NULL) {
    fprintf(stderr, "allocation failed\n");
    free(offsets);
    free(neighbors);
    free(frontier);
    free(frontier_bitmap);
    return 2;
  }

  build_mixed_degree_graph(offsets, neighbors, edge_count);
  rc = graphion_csr_graph_init(&graph, CAL_NODE_COUNT, edge_count, offsets, neighbors);
  if (rc != 0) {
    fprintf(stderr, "graph init failed rc=%d\n", rc);
    free(offsets);
    free(neighbors);
    free(frontier);
    free(frontier_bitmap);
    return 3;
  }

  for (size_t i = 0U; i < sizeof(low_degree_frontiers) / sizeof(low_degree_frontiers[0]); ++i) {
    size_t frontier_len;
    memset(frontier_bitmap, 0, CAL_NODE_COUNT * sizeof(uint8_t));
    frontier_len = fill_frontier(frontier, low_degree_frontiers[i], CAL_HIGH_DEGREE_COUNT, CAL_NODE_COUNT);
    for (size_t j = 0U; j < frontier_len; ++j) {
      frontier_bitmap[frontier[j]] = 1U;
    }
    {
      char name[32];
      (void)snprintf(name, sizeof(name), "low_degree_%zu", frontier_len);
      cases[case_count++] = measure_case(name, &graph, frontier, frontier_len, frontier_bitmap, 0);
    }
  }

  for (size_t i = 0U; i < sizeof(high_degree_frontiers) / sizeof(high_degree_frontiers[0]); ++i) {
    size_t frontier_len;
    memset(frontier_bitmap, 0, CAL_NODE_COUNT * sizeof(uint8_t));
    frontier_len = fill_frontier(frontier, high_degree_frontiers[i], 0U, CAL_HIGH_DEGREE_COUNT);
    for (size_t j = 0U; j < frontier_len; ++j) {
      frontier_bitmap[frontier[j]] = 1U;
    }
    {
      char name[32];
      (void)snprintf(name, sizeof(name), "high_degree_%zu", frontier_len);
      cases[case_count++] = measure_case(name, &graph, frontier, frontier_len, frontier_bitmap, 1);
    }
  }

  for (size_t i = 0U; i < case_count; ++i) {
    const double ratio = cases[i].dense_seconds / cases[i].sparse_seconds;
    if (strncmp(cases[i].name, "low_degree_", 11) == 0 && ratio < best_low_ratio) {
      best_low_ratio = ratio;
      calibrated_node_percent = cases[i].frontier_len_percent;
    }
  }

  for (size_t i = 0U; i < case_count; ++i) {
    const double ratio = cases[i].dense_seconds / cases[i].sparse_seconds;
    if (strncmp(cases[i].name, "high_degree_", 12) == 0 && ratio < best_high_ratio) {
      best_high_ratio = ratio;
      calibrated_work_percent = cases[i].frontier_work_percent;
    }
  }

  printf("{\n");
  printf("  \"benchmark\":\"frontier_threshold_calibration\",\n");
  printf("  \"node_count\":%u,\n", CAL_NODE_COUNT);
  printf("  \"edge_count\":%zu,\n", edge_count);
  printf("  \"iterations\":%u,\n", CAL_ITERATIONS);
  printf("  \"selection_strategy\":\"minimum_dense_proxy_overhead\",\n");
  printf("  \"calibrated_frontier_len_percent\":%zu,\n", calibrated_node_percent);
  printf("  \"calibrated_frontier_neighbor_work_percent\":%zu,\n", calibrated_work_percent);
  printf("  \"cases\":[\n");
  for (size_t i = 0U; i < case_count; ++i) {
    emit_case_json(&cases[i], i + 1U == case_count);
  }
  printf("  ]\n");
  printf("}\n");

  free(offsets);
  free(neighbors);
  free(frontier);
  free(frontier_bitmap);
  return 0;
}
