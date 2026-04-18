/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_TESTS_CORE_TEST_CORE_CASES_H
#define GRAPHION_TESTS_CORE_TEST_CORE_CASES_H

#define GRAPHION_CORE_TEST_CASES(X)                    \
  X(arena_alignment_and_reset)                         \
  X(arena_invalid_alignment_fails)                     \
  X(isa_decode_golden_fixtures)                        \
  X(isa_execute_golden_fixtures)                       \
  X(graph_init_and_neighbors)                          \
  X(graph_bfs_levels)                                  \
  X(graph_optional_edge_data)                          \
  X(graph_frontier_mode_heuristics)                    \
  X(hypergraph_init_and_queries)

#define GRAPHION_DECLARE_TEST(name) int test_##name(void);
GRAPHION_CORE_TEST_CASES(GRAPHION_DECLARE_TEST)
#undef GRAPHION_DECLARE_TEST

#endif
