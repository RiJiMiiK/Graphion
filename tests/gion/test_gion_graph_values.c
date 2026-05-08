/* SPDX-License-Identifier: MIT */

#include <string.h>

#include "test_parser_helpers.h"
#include "vm/internal/core/value.h"

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

int test_gion_graph_explicit_ids_have_priority(void) {
  const char *source =
      "graph G:\n"
      "    alpha\n"
      "    0\n"
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
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_explicit_ids_have_priority.txt");
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
  if (graph == NULL || graph->node_count != 2U || graph->edge_count != 0U || graph->offsets == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=2)\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_node_attributes(void) {
  const char *source =
      "graph G:\n"
      "    Alice {\"label\": \"start\", \"score\": 1}\n"
      "    2 {\"label\": \"middle\", \"score\": [true, 3]}\n"
      "    \"Bob\" {\"score\": 3, \"label\": \"end\"}\n"
      "print(G)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *graph_value;
  const graphion_graph_value *graph_data;
  size_t attr_len = 0U;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_node_attributes.txt");
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
  if (graph_value == NULL || graph_value->kind != GVM_VALUE_GRAPH_REF || graph_value->reserved[5] != 2U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  graph_data = (const graphion_graph_value *)graph_value->as.ref_value;
  if (graph_data == NULL || graph_data->csr.node_count != 3U || graph_data->node_attrs == NULL ||
      graph_data->node_attr_count != 3U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (graph_data->node_attrs[0].kind != GVM_VALUE_DICT || graph_data->node_attrs[1].kind != GVM_VALUE_DICT ||
      graph_data->node_attrs[2].kind != GVM_VALUE_DICT) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!vm_value_dict_length(&graph_data->node_attrs[0], &attr_len) || attr_len != 2U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=3, node_attrs=2)\n") != 0) {
    return finish_scope_test(&scope, 8);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_node_attribute_defaults(void) {
  const char *source =
      "graph G:\n"
      "    defaults node {\"label\": \"unknown\", \"score\": 0}\n"
      "    Alice {\"label\": \"start\"}\n"
      "    2 {\"score\": 2}\n"
      "    \"Bob\"\n"
      "print(G)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *graph_value;
  const graphion_graph_value *graph_data;
  size_t attr_len = 0U;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_node_attribute_defaults.txt");
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
  if (graph_value == NULL || graph_value->kind != GVM_VALUE_GRAPH_REF || graph_value->reserved[5] != 2U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  graph_data = (const graphion_graph_value *)graph_value->as.ref_value;
  if (graph_data == NULL || graph_data->node_attrs == NULL || graph_data->node_attr_count != 3U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!vm_value_dict_length(&graph_data->node_attrs[0], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&graph_data->node_attrs[1], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&graph_data->node_attrs[2], &attr_len) || attr_len != 2U) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=3, node_attrs=2)\n") != 0) {
    return finish_scope_test(&scope, 7);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_undirected_edge_declaration(void) {
  const char *source =
      "graph G:\n"
      "    1 - 2\n"
      "    3 - 2\n"
      "print(G)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *graph_value;
  const graphion_csr_graph *graph;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_undirected_edge_declaration.txt");
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
  if (graph == NULL || graph->node_count != 4U || graph->edge_count != 4U ||
      graph->offsets == NULL || graph->neighbors == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (graph->offsets[0] != 0U || graph->offsets[1] != 0U || graph->offsets[2] != 1U ||
      graph->offsets[3] != 3U || graph->offsets[4] != 4U) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (graph->neighbors[0] != 2U || graph->neighbors[1] != 1U ||
      graph->neighbors[2] != 3U || graph->neighbors[3] != 2U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=3, edges=2)\n") != 0) {
    return finish_scope_test(&scope, 8);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_directed_edge_declaration(void) {
  const char *source =
      "graph G:\n"
      "    1 -> 2\n"
      "    3 <-> 4\n"
      "print(G)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *graph_value;
  const graphion_csr_graph *graph;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_directed_edge_declaration.txt");
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
  if (graph_value == NULL || graph_value->kind != GVM_VALUE_GRAPH_REF || graph_value->reserved[0] != 1U) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  graph = (const graphion_csr_graph *)graph_value->as.ref_value;
  if (graph == NULL || graph->node_count != 5U || graph->edge_count != 3U ||
      graph->offsets == NULL || graph->neighbors == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (graph->offsets[0] != 0U || graph->offsets[1] != 0U || graph->offsets[2] != 1U ||
      graph->offsets[3] != 1U || graph->offsets[4] != 2U || graph->offsets[5] != 3U) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (graph->neighbors[0] != 2U || graph->neighbors[1] != 4U || graph->neighbors[2] != 3U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=4, edges=2)\n") != 0) {
    return finish_scope_test(&scope, 8);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_numeric_id_gap_warnings(void) {
  graphion_runtime_warning_report report;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  rc = graphion_collect_source_warnings("graph G:\n    1 - 3\n", &report, &diagnostic);
  if (rc != GINT_OK) {
    return 1;
  }
  if (report.count != 1U) {
    return 2;
  }
  if (report.items[0].line != 1U ||
      strcmp(report.items[0].message, "graph numeric node ids have gaps; missing ids: 0, 2") != 0) {
    return 3;
  }

  rc = graphion_collect_source_warnings("graph G:\n    1 - 2\n    3 - 2\n", &report, &diagnostic);
  if (rc != GINT_OK) {
    return 4;
  }
  if (report.count != 1U) {
    return 5;
  }
  if (strcmp(report.items[0].message, "graph numeric node ids have gaps; missing id: 0") != 0) {
    return 6;
  }

  rc = graphion_collect_source_warnings("graph G:\n    0 - 1\n    2 - 1\n", &report, &diagnostic);
  if (rc != GINT_OK) {
    return 7;
  }
  if (report.count != 0U) {
    return 8;
  }

  rc = graphion_collect_source_warnings("graph G:\n"
                                        "    Alice\n"
                                        "    2\n"
                                        "    \"Bob\"\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 9;
  }
  if (report.count != 0U) {
    return 10;
  }

  rc = graphion_collect_source_warnings("graph G:\n"
                                        "    Alice\n"
                                        "    3\n"
                                        "    \"Bob\"\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 11;
  }
  if (report.count != 1U) {
    return 12;
  }
  if (strcmp(report.items[0].message, "graph numeric node ids have gaps; missing id: 2") != 0) {
    return 13;
  }

  rc = graphion_collect_source_warnings("# graphion: warnings=off\n"
                                        "graph G:\n"
                                        "    1 - 3\n",
                                        &report,
                                        &diagnostic);
  if (rc != GINT_OK) {
    return 14;
  }
  if (!report.enabled || report.count != 1U) {
    return 15;
  }
  return 0;
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
      {"graph G:\n    alpha {\"a\": 1,}\n", GINT_ERR_PARSE, "trailing comma is not allowed in dict literal"},
      {"graph G:\n    alpha [1]\n", GINT_ERR_PARSE, "unexpected trailing tokens after graph node"},
      {"graph G:\n    alpha {\"a\": 1}\n    beta {\"b\": 2}\n",
       GINT_ERR_PARSE,
       "graph node attributes must use the same keys"},
      {"graph G:\n    alpha {\"a\": 1}\n    beta\n",
       GINT_ERR_PARSE,
       "graph node attributes must use the same keys"},
      {"graph G:\n    defaults node {\"a\": 0}\n    alpha {\"b\": 1}\n",
       GINT_ERR_PARSE,
       "graph node attributes must use declared default keys"},
      {"graph G:\n    defaults node {\"a\": 0}\n    defaults node {\"a\": 1}\n    alpha\n",
       GINT_ERR_PARSE,
       "duplicate graph node attribute defaults"},
      {"graph G:\n    defaults edge {\"weight\": 1}\n    alpha\n", GINT_ERR_PARSE, "expected 'node' after defaults"},
      {"graph G:\n    1 -\n", GINT_ERR_PARSE, "expected graph node name or id"},
      {"graph G:\n    1 - 2 extra\n", GINT_ERR_PARSE, "unexpected trailing tokens after graph edge"},
      {"graph G:\n    1 -> 2\n    2 - 3\n", GINT_ERR_PARSE, "directed graph cannot use undirected '-' edges"},
      {"graph G:\n    1 - 2\n    2 -> 3\n", GINT_ERR_PARSE, "directed graph cannot use undirected '-' edges"},
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
