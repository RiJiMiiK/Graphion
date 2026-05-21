/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_struct_declarations_and_instances(void) {
  const char *source =
      "struct Player:\n"
      "    id: int\n"
      "    name: string = \"unknown\"\n"
      "    score: float = 0.5\n"
      "    tags: list = []\n"
      "\n"
      "alice = Player {\"id\": 1, \"name\": \"Alice\"}\n"
      "bob = Player {\"id\": 2}\n"
      "same = bob == Player {\"id\": 2}\n"
      "different = alice != bob\n"
      "print(Player)\n"
      "print(alice)\n"
      "print(bob)\n"
      "print(bob[\"name\"])\n"
      "print(len(bob))\n"
      "print(same)\n"
      "print(different)\n";
  char path[512];
  char output[1024];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *player;
  const graphion_runtime_value *alice;
  const graphion_runtime_value *bob;
  const graphion_runtime_value *same;
  const graphion_runtime_value *different;

  graphion_runtime_scope_init(&scope);
  if (!test_capture_gion_output(source,
                                "gion_struct_declarations_and_instances.txt",
                                &scope,
                                &diagnostic,
                                path,
                                sizeof(path),
                                output,
                                sizeof(output))) {
    return finish_scope_test(&scope, 1);
  }
  test_cleanup_temp_path(path);
  player = graphion_runtime_scope_find(&scope, "Player");
  alice = graphion_runtime_scope_find(&scope, "alice");
  bob = graphion_runtime_scope_find(&scope, "bob");
  same = graphion_runtime_scope_find(&scope, "same");
  different = graphion_runtime_scope_find(&scope, "different");
  if (player == NULL || player->kind != GVM_VALUE_STRUCT_TYPE) {
    return finish_scope_test(&scope, 2);
  }
  if (alice == NULL || alice->kind != GVM_VALUE_STRUCT || bob == NULL || bob->kind != GVM_VALUE_STRUCT) {
    return finish_scope_test(&scope, 3);
  }
  if (same == NULL || same->kind != GVM_VALUE_BOOL || same->as.bool_value != 1) {
    return finish_scope_test(&scope, 4);
  }
  if (different == NULL || different->kind != GVM_VALUE_BOOL || different->as.bool_value != 1) {
    return finish_scope_test(&scope, 5);
  }
  if (strcmp(output,
             "struct Player(fields=4)\n"
             "Player{\"id\": 1, \"name\": \"Alice\", \"score\": 0.5, \"tags\": []}\n"
             "Player{\"id\": 2, \"name\": \"unknown\", \"score\": 0.5, \"tags\": []}\n"
             "unknown\n"
             "4\n"
             "true\n"
             "true\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_struct_runtime_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"struct Player:\n    id: int\n\np = Player {}\n", GINT_ERR_RUN, "missing or unknown struct field"},
      {"struct Player:\n    id: int\n\np = Player {\"id\": \"one\"}\n",
       GINT_ERR_RUN,
       "struct field value has wrong type"},
      {"struct Player:\n    id: int\n\np = Player {\"id\": 1, \"extra\": true}\n",
       GINT_ERR_RUN,
       "missing or unknown struct field"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != cases[i].expected_rc) {
      return (int)(500 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(600 + i);
    }
  }
  return 0;
}

int test_gion_struct_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"struct Player;\n", GINT_ERR_PARSE, "struct declaration requires ':' and an indented field block"},
      {"struct Player:\n", GINT_ERR_PARSE, "expected indented struct field block"},
      {"struct Player:\n    id int\n", GINT_ERR_PARSE, "expected ':' after struct field name"},
      {"struct Player:\n    id: int\n    id: string\n", GINT_ERR_PARSE, "duplicate struct field"},
      {"struct Player:\n    id: int = \"one\"\n", GINT_ERR_PARSE, "struct field default has wrong type"},
      {"struct Player:\n    id: strnig\n", GINT_ERR_PARSE, "unsupported struct field type"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != cases[i].expected_rc) {
      return (int)(700 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(800 + i);
    }
  }
  return 0;
}

int test_gion_struct_column_diagnostics(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"struct Player;\n", GINT_ERR_PARSE, 1U, 14U, "struct declaration requires ':' and an indented field block"},
      {"struct Player:\n", GINT_ERR_PARSE, 1U, 15U, "expected indented struct field block"},
      {"struct Player:\n    id int\n", GINT_ERR_PARSE, 2U, 4U, "expected ':' after struct field name"},
      {"struct Player:\n    id: int\n    id: string\n", GINT_ERR_PARSE, 3U, 1U, "duplicate struct field"},
      {"struct Player:\n    id: int = \"one\"\n", GINT_ERR_PARSE, 2U, 11U, "struct field default has wrong type"},
      {"struct Player:\n    id: strnig\n", GINT_ERR_PARSE, 2U, 5U, "unsupported struct field type"},
      {"struct Player:\n    id: int\n\np = Player\n",
       GINT_ERR_PARSE,
       4U,
       11U,
       "expected struct instance field dictionary"},
      {"p = Missing {}\n", GINT_ERR_UNKNOWN_VARIABLE, 1U, 5U, "unknown struct type"},
      {"struct Player:\n    id: int\n\np = Player {}\n",
       GINT_ERR_RUN,
       4U,
       12U,
       "missing or unknown struct field"},
      {"struct Player:\n    id: int\n\np = Player {\"id\": \"one\"}\n",
       GINT_ERR_RUN,
       4U,
       12U,
       "struct field value has wrong type"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != cases[i].expected_rc) {
      return (int)(900 + i);
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return (int)(1000 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(1100 + i);
    }
  }
  return 0;
}
