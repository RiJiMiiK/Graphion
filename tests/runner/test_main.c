/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int (*test_fn)(void);

typedef struct {
  const char *name;
  test_fn fn;
} test_case;

typedef enum {
  TEST_SUITE_VM = 0,
  TEST_SUITE_PARSER,
  TEST_SUITE_GION,
  TEST_SUITE_CORE,
  TEST_SUITE_GRAPH,
  TEST_SUITE_COUNT
} test_suite;

#include "../vm/test_vm_cases.h"
#include "../gion/test_parser_cases.h"
#include "../core/test_core_cases.h"

static int test_belongs_to_suite(const char *name, test_suite suite) {
  switch (suite) {
    case TEST_SUITE_VM:
      return strncmp(name, "vm_", 3) == 0;
    case TEST_SUITE_PARSER:
      return strncmp(name, "parser_", 7) == 0 || strncmp(name, "frontend_", 9) == 0;
    case TEST_SUITE_GION:
      return strncmp(name, "gion_", 5) == 0;
    case TEST_SUITE_CORE:
      return strncmp(name, "arena_", 6) == 0 || strncmp(name, "isa_", 4) == 0;
    case TEST_SUITE_GRAPH:
      return strncmp(name, "graph_", 6) == 0 || strncmp(name, "hypergraph_", 11) == 0;
    case TEST_SUITE_COUNT:
      break;
  }
  return 0;
}

static int parse_suite_name(const char *name, test_suite *suite) {
  if (name == NULL || suite == NULL) {
    return 0;
  }
  if (strcmp(name, "vm") == 0) {
    *suite = TEST_SUITE_VM;
    return 1;
  }
  if (strcmp(name, "parser") == 0) {
    *suite = TEST_SUITE_PARSER;
    return 1;
  }
  if (strcmp(name, "gion") == 0) {
    *suite = TEST_SUITE_GION;
    return 1;
  }
  if (strcmp(name, "core") == 0) {
    *suite = TEST_SUITE_CORE;
    return 1;
  }
  if (strcmp(name, "graph") == 0) {
    *suite = TEST_SUITE_GRAPH;
    return 1;
  }
  return 0;
}

static int should_run_test(const char *name, int argc, char **argv, const int *suite_enabled) {
  int i;
  int has_requested_tests = 0;
  int has_enabled_suite = 0;

  for (i = 0; i < TEST_SUITE_COUNT; ++i) {
    if (suite_enabled[i]) {
      has_enabled_suite = 1;
      if (test_belongs_to_suite(name, (test_suite)i)) {
        return 1;
      }
    }
  }

  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--suite") == 0) {
      ++i;
      continue;
    }
    has_requested_tests = 1;
    if (strcmp(name, argv[i]) == 0) {
      return 1;
    }
  }
  if (!has_enabled_suite && !has_requested_tests) {
    return 1;
  }
  return 0;
}

static int unknown_requested_tests(const test_case *tests, size_t count, int argc, char **argv, int *suite_enabled) {
  int i;
  if (suite_enabled == NULL) {
    return 1;
  }
  for (i = 0; i < TEST_SUITE_COUNT; ++i) {
    suite_enabled[i] = 0;
  }
  for (i = 1; i < argc; ++i) {
    size_t j;
    int found = 0;
    test_suite suite;

    if (strcmp(argv[i], "--suite") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "[FAIL] missing suite name after '--suite'\n");
        return 1;
      }
      if (!parse_suite_name(argv[i + 1], &suite)) {
        fprintf(stderr, "[FAIL] unknown suite '%s'\n", argv[i + 1]);
        return 1;
      }
      suite_enabled[suite] = 1;
      ++i;
      continue;
    }
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
#define GRAPHION_MAKE_TEST_CASE(name) {#name, test_##name},
  const test_case tests[] = {
      GRAPHION_VM_TEST_CASES(GRAPHION_MAKE_TEST_CASE)
      GRAPHION_PARSER_TEST_CASES(GRAPHION_MAKE_TEST_CASE)
      GRAPHION_CORE_TEST_CASES(GRAPHION_MAKE_TEST_CASE)
  };
#undef GRAPHION_MAKE_TEST_CASE
  const size_t count = sizeof(tests) / sizeof(tests[0]);
  int suite_enabled[TEST_SUITE_COUNT];
  size_t i;
  size_t executed = 0;

  if (unknown_requested_tests(tests, count, argc, argv, suite_enabled) != 0) {
    return EXIT_FAILURE;
  }

  for (i = 0; i < count; ++i) {
    if (!should_run_test(tests[i].name, argc, argv, suite_enabled)) {
      continue;
    }
    {
      const int rc = tests[i].fn();
      if (rc != 0) {
        fprintf(stderr, "[FAIL] %s (rc=%d)\n", tests[i].name, rc);
        return EXIT_FAILURE;
      }
    }
    fprintf(stdout, "[OK] %s\n", tests[i].name);
    executed++;
  }

  fprintf(stdout, "All tests passed (%zu)\n", executed);
  return EXIT_SUCCESS;
}
