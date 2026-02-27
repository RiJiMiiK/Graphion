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
int test_arena_alignment_and_reset(void);
int test_arena_invalid_alignment_fails(void);
int test_parser_decode_valid_program(void);
int test_parser_rejects_truncated_input(void);
int test_graph_init_and_neighbors(void);
int test_graph_bfs_levels(void);

int main(void) {
  const test_case tests[] = {
      {"vm_addition_program", test_vm_addition_program},
      {"vm_invalid_register_fails", test_vm_invalid_register_fails},
      {"arena_alignment_and_reset", test_arena_alignment_and_reset},
      {"arena_invalid_alignment_fails", test_arena_invalid_alignment_fails},
      {"parser_decode_valid_program", test_parser_decode_valid_program},
      {"parser_rejects_truncated_input", test_parser_rejects_truncated_input},
      {"graph_init_and_neighbors", test_graph_init_and_neighbors},
      {"graph_bfs_levels", test_graph_bfs_levels},
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
