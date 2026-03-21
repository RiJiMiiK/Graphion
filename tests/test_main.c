/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int (*test_fn)(void);

typedef struct {
  const char *name;
  test_fn fn;
} test_case;

int test_vm_addition_program(void);
int test_vm_invalid_register_fails(void);
int test_vm_bfs_levels_opcode(void);
int test_vm_hypergraph_opcodes(void);
int test_vm_superinstruction_add_pair_semantics(void);
int test_vm_superinstruction_movimm_add_semantics(void);
int test_vm_deterministic_mode_toggle(void);
int test_vm_deterministic_mode_unknown_opcode(void);
int test_vm_deterministic_mode_graph_semantics(void);
int test_vm_add_wraparound_semantics(void);
int test_vm_frontier_primitives(void);
int test_vm_frontier_errors(void);
int test_vm_neighbor_iteration_primitives(void);
int test_vm_neighbor_iteration_errors(void);
int test_vm_weighted_graph_opcodes(void);
int test_vm_weighted_graph_opcode_errors(void);
int test_vm_hyperedge_traversal_primitives(void);
int test_vm_hyperedge_traversal_errors(void);
int test_vm_snapshot_format(void);
int test_vm_fastpath_shape_cache_load_flags(void);
int test_vm_fastpath_shape_cache_same_pointer_content_change(void);
int test_vm_dispatch_variant_edge_semantics(void);
int test_arena_alignment_and_reset(void);
int test_arena_invalid_alignment_fails(void);
int test_parser_decode_valid_program(void);
int test_parser_rejects_truncated_input(void);
int test_frontend_parse_and_ir_lowering(void);
int test_frontend_rejects_invalid_source(void);
int test_frontend_source_to_vm_execution(void);
int test_frontend_reference_graph_execution_examples(void);
int test_gion_source_path_detection(void);
int test_interpreter_dynamic_assignments(void);
int test_interpreter_rejects_declared_type_syntax(void);
int test_interpreter_print_and_function_return(void);
int test_gion_entry_flow_execution(void);
int test_isa_decode_golden_fixtures(void);
int test_isa_execute_golden_fixtures(void);
int test_graph_init_and_neighbors(void);
int test_graph_bfs_levels(void);
int test_graph_optional_edge_data(void);
int test_graph_frontier_mode_heuristics(void);
int test_hypergraph_init_and_queries(void);

static int should_run_test(const char *name, int argc, char **argv) {
  int i;
  if (argc <= 1) {
    return 1;
  }
  for (i = 1; i < argc; ++i) {
    if (strcmp(name, argv[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static int unknown_requested_tests(const test_case *tests, size_t count, int argc, char **argv) {
  int i;
  for (i = 1; i < argc; ++i) {
    size_t j;
    int found = 0;
    for (j = 0; j < count; ++j) {
      if (strcmp(argv[i], tests[j].name) == 0) {
        found = 1;
        break;
      }
    }
    if (!found) {
      fprintf(stderr, "[FAIL] unknown test '%s'\n", argv[i]);
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  const test_case tests[] = {
      {"vm_addition_program", test_vm_addition_program},
      {"vm_invalid_register_fails", test_vm_invalid_register_fails},
      {"vm_bfs_levels_opcode", test_vm_bfs_levels_opcode},
      {"vm_hypergraph_opcodes", test_vm_hypergraph_opcodes},
      {"vm_superinstruction_add_pair_semantics", test_vm_superinstruction_add_pair_semantics},
      {"vm_superinstruction_movimm_add_semantics", test_vm_superinstruction_movimm_add_semantics},
      {"vm_deterministic_mode_toggle", test_vm_deterministic_mode_toggle},
      {"vm_deterministic_mode_unknown_opcode", test_vm_deterministic_mode_unknown_opcode},
      {"vm_deterministic_mode_graph_semantics", test_vm_deterministic_mode_graph_semantics},
      {"vm_add_wraparound_semantics", test_vm_add_wraparound_semantics},
      {"vm_frontier_primitives", test_vm_frontier_primitives},
      {"vm_frontier_errors", test_vm_frontier_errors},
      {"vm_neighbor_iteration_primitives", test_vm_neighbor_iteration_primitives},
      {"vm_neighbor_iteration_errors", test_vm_neighbor_iteration_errors},
      {"vm_weighted_graph_opcodes", test_vm_weighted_graph_opcodes},
      {"vm_weighted_graph_opcode_errors", test_vm_weighted_graph_opcode_errors},
      {"vm_hyperedge_traversal_primitives", test_vm_hyperedge_traversal_primitives},
      {"vm_hyperedge_traversal_errors", test_vm_hyperedge_traversal_errors},
      {"vm_snapshot_format", test_vm_snapshot_format},
      {"vm_fastpath_shape_cache_load_flags", test_vm_fastpath_shape_cache_load_flags},
      {"vm_fastpath_shape_cache_same_pointer_content_change",
       test_vm_fastpath_shape_cache_same_pointer_content_change},
      {"vm_dispatch_variant_edge_semantics", test_vm_dispatch_variant_edge_semantics},
      {"arena_alignment_and_reset", test_arena_alignment_and_reset},
      {"arena_invalid_alignment_fails", test_arena_invalid_alignment_fails},
      {"parser_decode_valid_program", test_parser_decode_valid_program},
      {"parser_rejects_truncated_input", test_parser_rejects_truncated_input},
      {"frontend_parse_and_ir_lowering", test_frontend_parse_and_ir_lowering},
      {"frontend_rejects_invalid_source", test_frontend_rejects_invalid_source},
      {"frontend_source_to_vm_execution", test_frontend_source_to_vm_execution},
      {"frontend_reference_graph_execution_examples", test_frontend_reference_graph_execution_examples},
      {"gion_source_path_detection", test_gion_source_path_detection},
      {"interpreter_dynamic_assignments", test_interpreter_dynamic_assignments},
      {"interpreter_rejects_declared_type_syntax", test_interpreter_rejects_declared_type_syntax},
      {"interpreter_print_and_function_return", test_interpreter_print_and_function_return},
      {"gion_entry_flow_execution", test_gion_entry_flow_execution},
      {"isa_decode_golden_fixtures", test_isa_decode_golden_fixtures},
      {"isa_execute_golden_fixtures", test_isa_execute_golden_fixtures},
      {"graph_init_and_neighbors", test_graph_init_and_neighbors},
      {"graph_bfs_levels", test_graph_bfs_levels},
      {"graph_optional_edge_data", test_graph_optional_edge_data},
      {"graph_frontier_mode_heuristics", test_graph_frontier_mode_heuristics},
      {"hypergraph_init_and_queries", test_hypergraph_init_and_queries},
  };
  const size_t count = sizeof(tests) / sizeof(tests[0]);
  size_t i;
  size_t executed = 0;

  if (unknown_requested_tests(tests, count, argc, argv) != 0) {
    return EXIT_FAILURE;
  }

  for (i = 0; i < count; ++i) {
    if (!should_run_test(tests[i].name, argc, argv)) {
      continue;
    }
    const int rc = tests[i].fn();
    if (rc != 0) {
      fprintf(stderr, "[FAIL] %s (rc=%d)\n", tests[i].name, rc);
      return EXIT_FAILURE;
    }
    fprintf(stdout, "[OK] %s\n", tests[i].name);
    executed++;
  }

  fprintf(stdout, "All tests passed (%zu)\n", executed);
  return EXIT_SUCCESS;
}
