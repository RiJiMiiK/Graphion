/* SPDX-License-Identifier: MIT */

#include <stdio.h>
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
int test_arena_alignment_and_reset(void);
int test_arena_invalid_alignment_fails(void);
int test_parser_decode_valid_program(void);
int test_parser_rejects_truncated_input(void);
int test_frontend_parse_and_ir_lowering(void);
int test_frontend_rejects_invalid_source(void);
int test_frontend_source_to_vm_execution(void);
int test_graph_init_and_neighbors(void);
int test_graph_bfs_levels(void);
int test_hypergraph_init_and_queries(void);

int main(void) {
  const test_case tests[] = {
      {"vm_addition_program", test_vm_addition_program},
      {"vm_invalid_register_fails", test_vm_invalid_register_fails},
      {"vm_bfs_levels_opcode", test_vm_bfs_levels_opcode},
      {"vm_hypergraph_opcodes", test_vm_hypergraph_opcodes},
      {"arena_alignment_and_reset", test_arena_alignment_and_reset},
      {"arena_invalid_alignment_fails", test_arena_invalid_alignment_fails},
      {"parser_decode_valid_program", test_parser_decode_valid_program},
      {"parser_rejects_truncated_input", test_parser_rejects_truncated_input},
      {"frontend_parse_and_ir_lowering", test_frontend_parse_and_ir_lowering},
      {"frontend_rejects_invalid_source", test_frontend_rejects_invalid_source},
      {"frontend_source_to_vm_execution", test_frontend_source_to_vm_execution},
      {"graph_init_and_neighbors", test_graph_init_and_neighbors},
      {"graph_bfs_levels", test_graph_bfs_levels},
      {"hypergraph_init_and_queries", test_hypergraph_init_and_queries},
  };
  const size_t count = sizeof(tests) / sizeof(tests[0]);
  size_t i;

  for (i = 0; i < count; ++i) {
    const int rc = tests[i].fn();
    if (rc != 0) {
      fprintf(stderr, "[FAIL] %s (rc=%d)\n", tests[i].name, rc);
      return EXIT_FAILURE;
    }
    fprintf(stdout, "[OK] %s\n", tests[i].name);
  }

  fprintf(stdout, "All tests passed (%zu)\n", count);
  return EXIT_SUCCESS;
}
