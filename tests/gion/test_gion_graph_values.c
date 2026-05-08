/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"

int test_gion_empty_graph_declaration(void) {
  const char *source =
      "graph Nom_du_graph;\n"
      "print(Nom_du_graph)\n";
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *graph_value;
  const graphion_csr_graph *graph;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_empty_graph_declaration.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  graph_value = graphion_runtime_scope_find(&scope, "Nom_du_graph");
  if (graph_value == NULL || graph_value->kind != GVM_VALUE_GRAPH_REF) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  graph = (const graphion_csr_graph *)graph_value->as.ref_value;
  if (graph == NULL || graph->node_count != 0U || graph->edge_count != 0U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "graph()\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_node_block_declaration(void) {
  const char *source =
      "graph G:\n"
      "    alpha\n"
      "    2\n"
      "    \"named beta\"\n"
      "print(G)\n";
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *graph_value;
  const graphion_csr_graph *graph;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_node_block_declaration.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  graph_value = graphion_runtime_scope_find(&scope, "G");
  if (graph_value == NULL || graph_value->kind != GVM_VALUE_GRAPH_REF) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  graph = (const graphion_csr_graph *)graph_value->as.ref_value;
  if (graph == NULL || graph->node_count != 3U || graph->edge_count != 0U || graph->offsets == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (graph->offsets[0] != 0U || graph->offsets[1] != 0U || graph->offsets[2] != 0U ||
      graph->offsets[3] != 0U) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=3)\n") != 0) {
    return finish_scope_test(&scope, 7);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_declaration_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"graph;\n", GINT_ERR_PARSE, "expected graph name"},
      {"graph graph;\n", GINT_ERR_RESERVED_NAME, "reserved name cannot be assigned"},
      {"graph G\n", GINT_ERR_PARSE, "expected ';' or ':' after graph declaration"},
      {"graph G; extra\n", GINT_ERR_PARSE, "unexpected trailing tokens after graph declaration"},
      {"graph G:\n", GINT_ERR_PARSE, "expected indented graph node block"},
      {"graph G:\n    -1\n", GINT_ERR_PARSE, "graph node id must be non-negative"},
      {"graph G:\n    1\n    1\n", GINT_ERR_PARSE, "duplicate graph node id"},
      {"graph G:\n    ?\n", GINT_ERR_PARSE, "expected graph node name or id"},
      {"graph G:\n    alpha extra\n", GINT_ERR_PARSE, "unexpected trailing tokens after graph node"},
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
      return (int)(100 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(200 + i);
    }
  }
  return 0;
}
