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

int test_gion_empty_hypergraph_declaration(void) {
  const char *source =
      "hypergraph Nom_du_hypergraph;\n"
      "print(Nom_du_hypergraph)\n";
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *hypergraph_value;
  const graphion_hypergraph *hypergraph;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_empty_hypergraph_declaration.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  hypergraph_value = graphion_runtime_scope_find(&scope, "Nom_du_hypergraph");
  if (hypergraph_value == NULL || hypergraph_value->kind != GVM_VALUE_HYPERGRAPH_REF) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  hypergraph = (const graphion_hypergraph *)hypergraph_value->as.ref_value;
  if (hypergraph == NULL || hypergraph->node_count != 0U || hypergraph->hyperedge_count != 0U ||
      hypergraph->incidence_count != 0U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "hypergraph()\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_vertex_block_declaration(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "hypergraph H:\n"
      "    alice\n"
      "    2\n"
      "    bob\n"
      "print(H)\n";
  char path[512];
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *hypergraph_value;
  const graphion_hypergraph *hypergraph;
  const graphion_hypergraph_value *value;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_vertex_block_declaration.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  hypergraph_value = graphion_runtime_scope_find(&scope, "H");
  if (hypergraph_value == NULL || hypergraph_value->kind != GVM_VALUE_HYPERGRAPH_REF) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  hypergraph = (const graphion_hypergraph *)hypergraph_value->as.ref_value;
  value = (const graphion_hypergraph_value *)hypergraph_value->as.ref_value;
  if (hypergraph == NULL || hypergraph->node_count != 3U || hypergraph->hyperedge_count != 0U ||
      hypergraph->incidence_count != 0U || hypergraph->node_offsets == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (value == NULL || value->vertex_count != 3U || value->vertices == NULL ||
      value->vertices[0].id != 0U || strcmp(value->vertices[0].name, "Alice") != 0 ||
      value->vertices[1].id != 1U || strcmp(value->vertices[1].name, "Bob") != 0 ||
      value->vertices[2].id != 2U || value->vertices[2].name != NULL) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  remove(path);
  if (strcmp(output, "hypergraph(vertices=3)\n") != 0) {
    return finish_scope_test(&scope, 7);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_vertex_attributes(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "hypergraph H:\n"
      "    defaults vertex {\"label\": \"unknown\", \"score\": 0}\n"
      "    alice {\"label\": \"start\", \"score\": 1}\n"
      "    2 {\"score\": 2}\n"
      "    bob\n"
      "print(H)\n";
  char path[512];
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *hypergraph_value;
  const graphion_hypergraph_value *value;
  size_t attr_len = 0U;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_vertex_attributes.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  hypergraph_value = graphion_runtime_scope_find(&scope, "H");
  if (hypergraph_value == NULL || hypergraph_value->kind != GVM_VALUE_HYPERGRAPH_REF) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  value = (const graphion_hypergraph_value *)hypergraph_value->as.ref_value;
  if (value == NULL || value->vertex_attrs == NULL || value->vertex_attr_count != 3U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!vm_value_dict_length(&value->vertex_attrs[0], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&value->vertex_attrs[1], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&value->vertex_attrs[2], &attr_len) || attr_len != 2U) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  remove(path);
  if (strcmp(output, "hypergraph(vertices=3, vertex_attrs=2)\n") != 0) {
    return finish_scope_test(&scope, 7);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_hyperedge_declaration(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "hypergraph H:\n"
      "    defaults hyperedge {\"kind\": \"group\", \"color\": \"blue\"}\n"
      "    alice\n"
      "    [alice, bob, 2] {\"kind\": \"team\"}\n"
      "    [2, \"Carol\"]\n"
      "print(H)\n"
      "print(hyperedge_vertices(H, 0))\n"
      "print(hyperedge_vertices(H, 1))\n"
      "print(hyperedge_attrs(H, 0)[\"kind\"])\n"
      "print(hyperedge_attrs(H, 1)[\"color\"])\n"
      "print(vertex_count(H))\n"
      "print(hyperedge_count(H))\n"
      "print(vertex_attr_count(H))\n"
      "print(hyperedge_attr_count(H))\n";
  char path[512];
  char output[256];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *hypergraph_value;
  const graphion_hypergraph *hypergraph;
  const graphion_hypergraph_value *value;
  size_t attr_len = 0U;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_hyperedge_declaration.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  hypergraph_value = graphion_runtime_scope_find(&scope, "H");
  if (hypergraph_value == NULL || hypergraph_value->kind != GVM_VALUE_HYPERGRAPH_REF) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  hypergraph = (const graphion_hypergraph *)hypergraph_value->as.ref_value;
  value = (const graphion_hypergraph_value *)hypergraph_value->as.ref_value;
  if (hypergraph == NULL || hypergraph->node_count != 4U || hypergraph->hyperedge_count != 2U ||
      hypergraph->incidence_count != 5U || hypergraph->hyperedge_offsets == NULL ||
      hypergraph->hyperedge_nodes == NULL || hypergraph->node_offsets == NULL ||
      hypergraph->node_hyperedges == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (value == NULL || value->vertex_count != 4U || value->vertices == NULL ||
      value->vertices[0].id != 0U || strcmp(value->vertices[0].name, "Alice") != 0 ||
      value->vertices[1].id != 1U || strcmp(value->vertices[1].name, "Bob") != 0 ||
      value->vertices[2].id != 2U || value->vertices[2].name != NULL ||
      value->vertices[3].id != 3U || strcmp(value->vertices[3].name, "Carol") != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (hypergraph->hyperedge_offsets[0] != 0U || hypergraph->hyperedge_offsets[1] != 3U ||
      hypergraph->hyperedge_offsets[2] != 5U || hypergraph->hyperedge_nodes[0] != 0U ||
      hypergraph->hyperedge_nodes[1] != 1U || hypergraph->hyperedge_nodes[2] != 2U ||
      hypergraph->hyperedge_nodes[3] != 2U || hypergraph->hyperedge_nodes[4] != 3U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (value->hyperedge_attrs == NULL || value->hyperedge_attr_count != 2U ||
      !vm_value_dict_length(&value->hyperedge_attrs[0], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&value->hyperedge_attrs[1], &attr_len) || attr_len != 2U) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  remove(path);
  if (strcmp(output,
             "hypergraph(vertices=4, hyperedges=2, hyperedge_attrs=2)\n"
             "[0, 1, 2]\n"
             "[2, 3]\n"
             "team\n"
             "blue\n"
             "4\n"
             "2\n"
             "0\n"
             "2\n") != 0) {
    return finish_scope_test(&scope, 9);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_vertex_and_hyperedge_attributes(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "hypergraph H:\n"
      "    defaults vertex {\"label\": \"unknown\", \"score\": 0}\n"
      "    defaults hyperedge {\"kind\": \"group\", \"color\": \"blue\"}\n"
      "    alice\n"
      "    2 {\"label\": \"explicit id\", \"score\": 2}\n"
      "    bob {\"score\": 1}\n"
      "    [alice, bob, 2] {\"kind\": \"team\"}\n"
      "    [2, \"Carol\"]\n"
      "print(H)\n"
      "print(vertex_count(H))\n"
      "print(hyperedge_count(H))\n"
      "print(vertex_attr_count(H))\n"
      "print(hyperedge_attr_count(H))\n"
      "print(vertex_attrs(H, alice)[\"label\"])\n"
      "print(vertex_attrs(H, 2)[\"score\"])\n"
      "print(hyperedge_attrs(H, 0)[\"kind\"])\n";
  char path[512];
  char output[256];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *hypergraph_value;
  const graphion_hypergraph_value *value;
  size_t attr_len = 0U;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_vertex_and_hyperedge_attributes.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  hypergraph_value = graphion_runtime_scope_find(&scope, "H");
  if (hypergraph_value == NULL || hypergraph_value->kind != GVM_VALUE_HYPERGRAPH_REF) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  value = (const graphion_hypergraph_value *)hypergraph_value->as.ref_value;
  if (value == NULL || value->vertex_attrs == NULL || value->vertex_attr_count != 4U ||
      value->hyperedge_attrs == NULL || value->hyperedge_attr_count != 2U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!vm_value_dict_length(&value->vertex_attrs[0], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&value->vertex_attrs[1], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&value->vertex_attrs[2], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&value->vertex_attrs[3], &attr_len) || attr_len != 2U) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!vm_value_dict_length(&value->hyperedge_attrs[0], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&value->hyperedge_attrs[1], &attr_len) || attr_len != 2U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  remove(path);
  if (strcmp(output,
             "hypergraph(vertices=4, hyperedges=2, vertex_attrs=2, hyperedge_attrs=2)\n"
             "4\n"
             "2\n"
             "2\n"
             "2\n"
             "unknown\n"
             "2\n"
             "team\n") != 0) {
    return finish_scope_test(&scope, 8);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_membership_query_builtins(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "hypergraph H:\n"
      "    alice\n"
      "    [alice, bob, 2]\n"
      "    [2, \"Carol\"]\n"
      "print(has_vertex(H, alice))\n"
      "print(has_vertex(H, \"Carol\"))\n"
      "print(has_vertex(H, 99))\n"
      "print(has_hyperedge(H, 0))\n"
      "print(has_hyperedge(H, 2))\n"
      "print(incident_hyperedges(H, 2))\n"
      "print(incident_hyperedges(H, bob))\n"
      "print(hyperedge_vertices(H, 1))\n";
  const char *expected =
      "true\n"
      "true\n"
      "false\n"
      "true\n"
      "false\n"
      "[0, 1]\n"
      "[0]\n"
      "[2, 3]\n";
  char path[512];
  char output[256];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_membership_query_builtins.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_listing_query_builtins(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "hypergraph H:\n"
      "    alice\n"
      "    [alice, bob, 2]\n"
      "    [2, \"Carol\"]\n"
      "print(vertex_ids(H))\n"
      "print(vertices(H))\n"
      "print(hyperedges(H))\n"
      "print(hyperedges(H)[1][\"vertices\"])\n";
  const char *expected =
      "[0, 1, 2, 3]\n"
      "[{\"id\": 0, \"name\": \"Alice\"}, {\"id\": 1, \"name\": \"Bob\"}, {\"id\": 2}, "
      "{\"id\": 3, \"name\": \"Carol\"}]\n"
      "[{\"id\": 0, \"vertices\": [0, 1, 2]}, {\"id\": 1, \"vertices\": [2, 3]}]\n"
      "[2, 3]\n";
  char path[512];
  char output[768];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_listing_query_builtins.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_structure_mutation_statements(void) {
  const char *source =
      "hypergraph H:\n"
      "    defaults vertex {\"label\": \"unknown\", \"score\": 0}\n"
      "    defaults hyperedge {\"kind\": \"group\", \"color\": \"blue\"}\n"
      "    \"Alice\"\n"
      "    2 {\"label\": \"explicit\", \"score\": 2}\n"
      "    [\"Alice\", 2]\n"
      "add_vertex(H, \"Bob\")\n"
      "add_hyperedge(H, [\"Bob\", \"Carol\", 5])\n"
      "print(H)\n"
      "print(vertex_ids(H))\n"
      "print(hyperedge_vertices(H, 1))\n"
      "print(hyperedge_attrs(H, 1)[\"kind\"])\n"
      "print(vertex_attrs(H, \"Carol\")[\"label\"])\n"
      "print(vertex_attrs(H, 5)[\"score\"])\n"
      "print(has_vertex(H, 5))\n"
      "print(incident_hyperedges(H, \"Bob\"))\n";
  const char *expected =
      "hypergraph(vertices=5, hyperedges=2, vertex_attrs=2, hyperedge_attrs=2)\n"
      "[0, 2, 1, 3, 5]\n"
      "[1, 3, 5]\n"
      "group\n"
      "unknown\n"
      "0\n"
      "true\n"
      "[1]\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_structure_mutation_statements.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_attribute_mutation_statements(void) {
  const char *source =
      "hypergraph H:\n"
      "    defaults vertex {\"label\": \"unknown\", \"score\": 0}\n"
      "    defaults hyperedge {\"kind\": \"group\", \"color\": \"blue\"}\n"
      "    \"Alice\"\n"
      "    [\"Alice\", \"Bob\"]\n"
      "add_vertex(H, \"Carol\")\n"
      "add_hyperedge(H, [\"Bob\", \"Carol\"])\n"
      "set_vertex_attrs(H, \"Alice\", {\"score\": 10})\n"
      "set_hyperedge_attrs(H, 0, {\"color\": \"red\"})\n"
      "set_vertex_attrs(H, \"Carol\", {\"label\": \"end\"})\n"
      "set_hyperedge_attrs(H, 1, {\"kind\": \"pair\"})\n"
      "print(vertex_attrs(H, \"Alice\"))\n"
      "print(hyperedge_attrs(H, 0))\n"
      "print(vertex_attrs(H, \"Carol\"))\n"
      "print(hyperedge_attrs(H, 1))\n";
  const char *expected =
      "{\"label\": \"unknown\", \"score\": 10}\n"
      "{\"kind\": \"group\", \"color\": \"red\"}\n"
      "{\"label\": \"end\", \"score\": 0}\n"
      "{\"kind\": \"pair\", \"color\": \"blue\"}\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_attribute_mutation_statements.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_removal_mutation_statements(void) {
  const char *source =
      "hypergraph H:\n"
      "    defaults vertex {\"label\": \"unknown\"}\n"
      "    defaults hyperedge {\"kind\": \"group\"}\n"
      "    [\"Alice\", \"Bob\"]\n"
      "    [\"Bob\", \"Carol\"]\n"
      "    [\"Solo\"]\n"
      "remove_vertex(H, \"Bob\")\n"
      "remove_hyperedge(H, 0)\n"
      "add_hyperedge(H, [\"Alice\", 9])\n"
      "remove_vertex(H, \"Solo\")\n"
      "print(H)\n"
      "print(vertex_ids(H))\n"
      "print(hyperedge_count(H))\n"
      "print(has_vertex(H, \"Bob\"))\n"
      "print(has_hyperedge(H, 0))\n"
      "print(has_hyperedge(H, 1))\n"
      "print(has_hyperedge(H, 2))\n"
      "print(has_hyperedge(H, 3))\n"
      "print(hyperedges(H))\n"
      "print(hyperedge_vertices(H, 1))\n"
      "print(incident_hyperedges(H, \"Alice\"))\n"
      "print(vertex_attrs(H, 9))\n"
      "print(hyperedge_attrs(H, 3))\n";
  const char *expected =
      "hypergraph(vertices=3, hyperedges=2, vertex_attrs=1, hyperedge_attrs=1)\n"
      "[0, 2, 9]\n"
      "2\n"
      "false\n"
      "false\n"
      "true\n"
      "false\n"
      "true\n"
      "[{\"id\": 1, \"vertices\": [2]}, {\"id\": 3, \"vertices\": [0, 9]}]\n"
      "[2]\n"
      "[3]\n"
      "{\"label\": \"unknown\"}\n"
      "{\"kind\": \"group\"}\n";
  char path[512];
  char output[768];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_hypergraph_removal_mutation_statements.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_hypergraph_error_coverage(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } error_cases[] = {
      {"hypergraph H:\n"
       "    defaults vertex {\"score\": 0}\n"
       "    \"Alice\"\n"
       "set_vertex_attrs(H, \"Alice\", {\"label\": \"start\"})\n",
       GINT_ERR_RUN,
       "incompatible operand types"},
      {"hypergraph H:\n"
       "    defaults hyperedge {\"kind\": \"group\"}\n"
       "    [\"Alice\", \"Bob\"]\n"
       "set_hyperedge_attrs(H, 0, {\"color\": \"blue\"})\n",
       GINT_ERR_RUN,
       "incompatible operand types"},
      {"hypergraph H:\n"
       "    \"Alice\"\n"
       "set_vertex_attrs(H, \"Bob\", {\"label\": \"missing\"})\n",
       GINT_ERR_RUN,
       "invalid node id"},
      {"hypergraph H:\n"
       "    [\"Alice\", \"Bob\"]\n"
       "set_hyperedge_attrs(H, 9, {\"kind\": \"missing\"})\n",
       GINT_ERR_RUN,
       "invalid hyperedge id"},
      {"hypergraph H:\n"
       "    \"Alice\"\n"
       "remove_vertex(H, \"Bob\")\n",
       GINT_ERR_RUN,
       "invalid node id"},
      {"hypergraph H:\n"
       "    [\"Alice\", \"Bob\"]\n"
       "remove_hyperedge(H, 0)\n"
       "remove_hyperedge(H, 0)\n",
       GINT_ERR_RUN,
       "invalid hyperedge id"},
      {"hypergraph H:\n"
       "    []\n",
       GINT_ERR_PARSE,
       "hyperedge must contain at least one vertex"},
      {"hypergraph H:\n"
       "    [\"Alice\", true]\n",
       GINT_ERR_PARSE,
       "graph node variable must be int or string"},
      {"hypergraph H:\n"
       "    defaults vertex {\"label\": \"unknown\", \"score\": 0}\n"
       "    \"Alice\"\n"
       "add_vertex(H, \"Bob\")\n"
       "set_vertex_attrs(H, \"Bob\", {\"unknown\": 1})\n",
       GINT_ERR_RUN,
       "incompatible operand types"},
      {"hypergraph H:\n"
       "    defaults hyperedge {\"kind\": \"group\", \"color\": \"blue\"}\n"
       "    [\"Alice\", \"Bob\"]\n"
       "add_hyperedge(H, [\"Bob\", \"Carol\"])\n"
       "set_hyperedge_attrs(H, 1, {\"unknown\": 1})\n",
       GINT_ERR_RUN,
       "incompatible operand types"},
  };
  const char *vertex_defaults_source =
      "hypergraph H:\n"
      "    defaults vertex {\"label\": \"unknown\", \"score\": 0}\n"
      "    \"Alice\"\n"
      "add_vertex(H, \"Bob\")\n"
      "set_vertex_attrs(H, \"Bob\", {\"score\": 10})\n"
      "print(vertex_attrs(H, \"Bob\")[\"label\"])\n"
      "print(vertex_attrs(H, \"Bob\")[\"score\"])\n";
  const char *hyperedge_defaults_source =
      "hypergraph H:\n"
      "    defaults hyperedge {\"kind\": \"group\", \"color\": \"blue\"}\n"
      "    [\"Alice\", \"Bob\"]\n"
      "add_hyperedge(H, [\"Bob\", \"Carol\"])\n"
      "set_hyperedge_attrs(H, 1, {\"color\": \"red\"})\n"
      "print(hyperedge_attrs(H, 1)[\"kind\"])\n"
      "print(hyperedge_attrs(H, 1)[\"color\"])\n";
  char path[512];
  char output[256];
  graphion_runtime_scope defaults_scope;
  graphion_runtime_diagnostic defaults_diagnostic;
  size_t i;

  for (i = 0U; i < sizeof(error_cases) / sizeof(error_cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(error_cases[i].source, &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != error_cases[i].expected_rc) {
      return (int)(100 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, error_cases[i].message) != 0) {
      return (int)(200 + i);
    }
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("add_vertex(Missing, 1)\n", &scope,
                                   &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != GINT_ERR_UNKNOWN_VARIABLE) {
      return 250;
    }
    if (diagnostic.message == NULL ||
        strcmp(diagnostic.message, "unknown hypergraph variable 'Missing'") !=
            0) {
      return 251;
    }
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("set_vertex_attrs(Missing, 1, {})\n",
                                   &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != GINT_ERR_UNKNOWN_VARIABLE) {
      return 252;
    }
    if (diagnostic.message == NULL ||
        strcmp(diagnostic.message, "unknown hypergraph variable 'Missing'") !=
            0) {
      return 253;
    }
  }

  graphion_runtime_scope_init(&defaults_scope);
  if (test_capture_gion_output(vertex_defaults_source,
                               "gion_hypergraph_error_vertex_defaults.txt",
                               &defaults_scope,
                               &defaults_diagnostic,
                               path,
                               sizeof(path),
                               output,
                               sizeof(output)) == 0) {
    graphion_runtime_scope_dispose(&defaults_scope);
    return 300;
  }
  graphion_runtime_scope_dispose(&defaults_scope);
  if (strcmp(output, "unknown\n10\n") != 0) {
    return 301;
  }
  graphion_runtime_scope_init(&defaults_scope);
  if (test_capture_gion_output(hyperedge_defaults_source,
                               "gion_hypergraph_error_hyperedge_defaults.txt",
                               &defaults_scope,
                               &defaults_diagnostic,
                               path,
                               sizeof(path),
                               output,
                               sizeof(output)) == 0) {
    graphion_runtime_scope_dispose(&defaults_scope);
    return 302;
  }
  graphion_runtime_scope_dispose(&defaults_scope);
  if (strcmp(output, "group\nred\n") != 0) {
    return 303;
  }
  return 0;
}

int test_gion_graph_node_block_declaration(void) {
  const char *source =
      "alpha = \"alpha\"\n"
      "beta = \"named beta\"\n"
      "graph G:\n"
      "    alpha\n"
      "    2\n"
      "    beta\n"
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
      "    \"alpha\"\n"
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
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "graph G:\n"
      "    alice {\"label\": \"start\", \"score\": 1}\n"
      "    2 {\"label\": \"middle\", \"score\": [true, 3]}\n"
      "    bob {\"score\": 3, \"label\": \"end\"}\n"
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
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "graph G:\n"
      "    defaults node {\"label\": \"unknown\", \"score\": 0}\n"
      "    alice {\"label\": \"start\"}\n"
      "    2 {\"score\": 2}\n"
      "    bob\n"
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
  const graphion_graph_value *graph_data;
  const graphion_csr_graph *adjacency;
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
  graph_data = (const graphion_graph_value *)graph_value->as.ref_value;
  adjacency = graph_data != NULL ? &graph_data->csr : NULL;
  if (graph_data == NULL || graph_data->edge_count != 2U || graph_data->edges == NULL ||
      adjacency == NULL || adjacency->node_count != 4U || adjacency->edge_count != 4U ||
      adjacency->offsets == NULL || adjacency->neighbors == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!graph_data->edges[0].bidirectional || !graph_data->edges[1].bidirectional ||
      graph_data->edges[0].directed || graph_data->edges[1].directed) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (adjacency->offsets[0] != 0U || adjacency->offsets[1] != 0U || adjacency->offsets[2] != 1U ||
      adjacency->offsets[3] != 3U || adjacency->offsets[4] != 4U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (adjacency->neighbors[0] != 2U || adjacency->neighbors[1] != 1U ||
      adjacency->neighbors[2] != 3U || adjacency->neighbors[3] != 2U) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=3, edges=2)\n") != 0) {
    return finish_scope_test(&scope, 9);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_edge_attributes(void) {
  const char *source =
      "w = 15\n"
      "attrs = {\"kind\": \"path\", \"weight\": 2.5}\n"
      "graph G:\n"
      "    defaults edge {\"kind\": \"normal\", \"weight\": 1}\n"
      "    1-2 w\n"
      "    2 - 3 attrs\n"
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
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_edge_attributes.txt");
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
  graph_data = (const graphion_graph_value *)graph_value->as.ref_value;
  if (graph_data == NULL || graph_data->edge_count != 2U || graph_data->edges == NULL ||
      graph_data->edge_attrs == NULL || graph_data->edge_attr_count != 2U) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (graph_data->edge_attrs[0].kind != GVM_VALUE_DICT || graph_data->edge_attrs[1].kind != GVM_VALUE_DICT) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!vm_value_dict_length(&graph_data->edge_attrs[0], &attr_len) || attr_len != 2U ||
      !vm_value_dict_length(&graph_data->edge_attrs[1], &attr_len) || attr_len != 2U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=3, edges=2, edge_attrs=2)\n") != 0) {
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
  const graphion_graph_value *graph_data;
  const graphion_csr_graph *adjacency;
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
  graph_data = (const graphion_graph_value *)graph_value->as.ref_value;
  adjacency = graph_data != NULL ? &graph_data->csr : NULL;
  if (graph_data == NULL || graph_data->edge_count != 2U || graph_data->edges == NULL ||
      adjacency == NULL || adjacency->node_count != 5U || adjacency->edge_count != 3U ||
      adjacency->offsets == NULL || adjacency->neighbors == NULL) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!graph_data->edges[0].directed || graph_data->edges[0].bidirectional ||
      !graph_data->edges[1].directed || !graph_data->edges[1].bidirectional) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (adjacency->offsets[0] != 0U || adjacency->offsets[1] != 0U || adjacency->offsets[2] != 1U ||
      adjacency->offsets[3] != 1U || adjacency->offsets[4] != 2U || adjacency->offsets[5] != 3U) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (adjacency->neighbors[0] != 2U || adjacency->neighbors[1] != 4U || adjacency->neighbors[2] != 3U) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  remove(path);
  if (strcmp(output, "graph(nodes=4, edges=2)\n") != 0) {
    return finish_scope_test(&scope, 9);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_inspection_builtins(void) {
  const char *source =
      "graph Empty;\n"
      "graph Nodes:\n"
      "    \"Alice\"\n"
      "    2\n"
      "graph Undirected:\n"
      "    1 - 2\n"
      "    3 - 2\n"
      "graph Weighted:\n"
      "    defaults edge {\"weight\": 1}\n"
      "    1 - 2\n"
      "graph Directed:\n"
      "    1 -> 2\n"
      "    3 <-> 4\n"
      "print(node_count(Empty))\n"
      "print(edge_count(Empty))\n"
      "print(is_directed(Empty))\n"
      "print(is_weighted(Empty))\n"
      "print(orientation(Empty))\n"
      "print(node_count(Nodes))\n"
      "print(edge_count(Nodes))\n"
      "print(is_weighted(Nodes))\n"
      "print(orientation(Nodes))\n"
      "print(node_count(Undirected))\n"
      "print(edge_count(Undirected))\n"
      "print(is_directed(Undirected))\n"
      "print(is_weighted(Undirected))\n"
      "print(orientation(Undirected))\n"
      "print(edge_count(Weighted))\n"
      "print(is_weighted(Weighted))\n"
      "print(node_count(Directed))\n"
      "print(edge_count(Directed))\n"
      "print(is_directed(Directed))\n"
      "print(is_weighted(Directed))\n"
      "print(orientation(Directed))\n";
  const char *expected =
      "0\n"
      "0\n"
      "false\n"
      "false\n"
      "empty\n"
      "2\n"
      "0\n"
      "false\n"
      "empty\n"
      "3\n"
      "2\n"
      "false\n"
      "false\n"
      "undirected\n"
      "1\n"
      "true\n"
      "4\n"
      "2\n"
      "true\n"
      "false\n"
      "directed\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_inspection_builtins.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_attribute_lookup_builtins(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "graph G:\n"
      "    defaults node {\"label\": \"unknown\", \"score\": 0}\n"
      "    alice {\"label\": \"start\"}\n"
      "    2 {\"score\": 2}\n"
      "    bob\n"
      "    defaults edge {\"kind\": \"normal\", \"weight\": 1}\n"
      "    alice - 2 {\"weight\": 3}\n"
      "    2 - bob\n"
      "print(node_attrs(G, alice)[\"label\"])\n"
      "print(node_attrs(G, 2)[\"label\"])\n"
      "print(node_attrs(G, bob)[\"score\"])\n"
      "print(edge_attrs(G, alice, 2)[\"kind\"])\n"
      "print(edge_weight(G, alice, 2))\n"
      "print(edge_weight(G, 2, alice))\n"
      "print(edge_attrs(G, 2, bob)[\"weight\"])\n";
  const char *expected =
      "start\n"
      "unknown\n"
      "0\n"
      "normal\n"
      "3\n"
      "3\n"
      "1\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_attribute_lookup_builtins.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_membership_query_builtins(void) {
  const char *source =
      "alice = \"Alice\"\n"
      "bob = \"Bob\"\n"
      "graph Undirected:\n"
      "    alice - 2\n"
      "    2 - bob\n"
      "graph Directed:\n"
      "    \"A\" -> \"B\"\n"
      "    \"B\" <-> 3\n"
      "print(has_node(Undirected, alice))\n"
      "print(has_node(Undirected, 99))\n"
      "print(has_node(Undirected, \"Missing\"))\n"
      "print(has_edge(Undirected, alice, 2))\n"
      "print(has_edge(Undirected, 2, alice))\n"
      "print(has_edge(Undirected, alice, bob))\n"
      "print(neighbors(Undirected, 2))\n"
      "print(has_edge(Directed, \"A\", \"B\"))\n"
      "print(has_edge(Directed, \"B\", \"A\"))\n"
      "print(has_edge(Directed, 3, \"B\"))\n"
      "print(neighbors(Directed, \"B\"))\n"
      "print(indegree(Directed, \"B\"))\n"
      "print(outdegree(Directed, \"B\"))\n"
      "print(len(indegree(Directed, \"B\")))\n"
      "print(len(outdegree(Directed, \"B\")))\n";
  const char *expected =
      "true\n"
      "false\n"
      "false\n"
      "true\n"
      "true\n"
      "false\n"
      "[0, 1]\n"
      "true\n"
      "false\n"
      "true\n"
      "[0, 3]\n"
      "[0, 3]\n"
      "[3]\n"
      "2\n"
      "1\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_membership_query_builtins.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_listing_query_builtins(void) {
  const char *source =
      "graph G:\n"
      "    \"Alice\"\n"
      "    \"Bob\"\n"
      "    10\n"
      "    \"Alice\" -> \"Bob\"\n"
      "    \"Bob\" <-> 10\n"
      "print(node_ids(G))\n"
      "print(nodes(G))\n"
      "print(edges(G))\n"
      "remove_edge(G, \"Bob\", 10)\n"
      "print(edges(G))\n";
  const char *expected =
      "[0, 1, 10]\n"
      "[{\"id\": 0, \"name\": \"Alice\"}, {\"id\": 1, \"name\": \"Bob\"}, {\"id\": 10}]\n"
      "[{\"from\": 0, \"to\": 1, \"directed\": true, \"bidirectional\": false}, "
      "{\"from\": 1, \"to\": 10, \"directed\": true, \"bidirectional\": true}]\n"
      "[{\"from\": 0, \"to\": 1, \"directed\": true, \"bidirectional\": false}, "
      "{\"from\": 10, \"to\": 1, \"directed\": true, \"bidirectional\": false}]\n";
  char path[512];
  char output[768];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_listing_query_builtins.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_structure_mutation_statements(void) {
  const char *source =
      "graph G;\n"
      "add_node(G, \"Alice\")\n"
      "add_edge(G, \"Alice\", 2)\n"
      "add_edge(G, 2, \"Alice\")\n"
      "add_node(G, 2)\n"
      "add_node(G, \"Bob\")\n"
      "add_edge(G, \"Bob\", \"Alice\")\n"
      "print(node_count(G))\n"
      "print(edge_count(G))\n"
      "print(has_node(G, \"Bob\"))\n"
      "print(has_edge(G, 2, \"Alice\"))\n"
      "print(neighbors(G, \"Alice\"))\n"
      "print(G)\n";
  const char *expected =
      "3\n"
      "2\n"
      "true\n"
      "true\n"
      "[2, 1]\n"
      "graph(nodes=3, edges=2)\n";
  const char *source_non_empty =
      "graph H:\n"
      "    \"A\"\n"
      "add_node(H, \"B\")\n"
      "add_edge(H, \"A\", \"B\")\n"
      "print(node_count(H))\n"
      "print(edge_count(H))\n";
  const char *expected_non_empty =
      "2\n"
      "1\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_structure_mutation_statements.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_structure_mutation_non_empty.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 5);
  }
  rc = graphion_interpret_source_with_output(source_non_empty, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  remove(path);
  if (strcmp(output, expected_non_empty) != 0) {
    return finish_scope_test(&scope, 8);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_attribute_mutation_statements(void) {
  const char *source =
      "graph G;\n"
      "add_edge(G, \"Alice\", 2)\n"
      "set_node_attrs(G, \"Alice\", {\"label\": \"start\", \"score\": 1})\n"
      "set_node_attrs(G, 2, {\"label\": \"end\", \"score\": 2})\n"
      "set_node_attrs(G, \"Alice\", {\"score\": 10})\n"
      "set_edge_attrs(G, \"Alice\", 2, {\"kind\": \"path\", \"weight\": 3})\n"
      "set_edge_weight(G, 2, \"Alice\", 4.5)\n"
      "set_edge_attrs(G, \"Alice\", 2, {\"weight\": 5})\n"
      "set_edge_attrs(G, \"Alice\", 2, {\"kind\": \"shortcut\"})\n"
      "print(node_attrs(G, \"Alice\")[\"label\"])\n"
      "print(node_attrs(G, \"Alice\")[\"score\"])\n"
      "print(node_attrs(G, 2)[\"score\"])\n"
      "print(edge_attrs(G, \"Alice\", 2)[\"kind\"])\n"
      "print(edge_weight(G, \"Alice\", 2))\n"
      "set_edge_attrs(G, \"Alice\", 2, {\"kind\": \"direct\", \"weight\": 7})\n"
      "print(edge_attrs(G, 2, \"Alice\")[\"kind\"])\n"
      "print(edge_weight(G, 2, \"Alice\"))\n";
  const char *expected =
      "start\n"
      "10\n"
      "2\n"
      "shortcut\n"
      "5\n"
      "direct\n"
      "7\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_attribute_mutation_statements.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_removal_mutation_statements(void) {
  const char *source =
      "graph G:\n"
      "    1 <-> 2 {\"weight\": 8, \"kind\": \"both\"}\n"
      "    2 -> 3 {\"weight\": 5, \"kind\": \"tail\"}\n"
      "print(edge_count(G))\n"
      "print(has_edge(G, 1, 2))\n"
      "print(has_edge(G, 2, 1))\n"
      "remove_edge(G, 1, 2)\n"
      "print(edge_count(G))\n"
      "print(has_edge(G, 1, 2))\n"
      "print(has_edge(G, 2, 1))\n"
      "print(edge_weight(G, 2, 1))\n"
      "remove_node(G, 2)\n"
      "print(node_count(G))\n"
      "print(edge_count(G))\n"
      "print(has_node(G, 1))\n"
      "print(has_node(G, 2))\n"
      "print(has_node(G, 3))\n";
  const char *expected =
      "2\n"
      "true\n"
      "true\n"
      "2\n"
      "false\n"
      "true\n"
      "8\n"
      "2\n"
      "0\n"
      "true\n"
      "false\n"
      "true\n";
  char path[512];
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  fp = test_open_temp_output(path, sizeof(path), "gion_graph_removal_mutation_statements.txt");
  if (fp == NULL) {
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  remove(path);
  if (strcmp(output, expected) != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_graph_mutation_error_coverage(void) {
  static const struct {
    const char *source;
    const char *message;
  } error_cases[] = {
      {"graph G:\n"
       "    defaults node {\"score\": 0}\n"
       "    \"Alice\"\n"
       "set_node_attrs(G, \"Alice\", {\"label\": \"start\"})\n",
       "incompatible operand types"},
      {"graph G:\n"
       "    defaults edge {\"weight\": 1}\n"
       "    1 - 2\n"
       "set_edge_attrs(G, 1, 2, {\"kind\": \"path\"})\n",
       "incompatible operand types"},
      {"graph G:\n"
       "    1 - 2\n"
       "set_edge_attrs(G, 1, 2, {\"weight\": \"heavy\"})\n",
       "incompatible operand types"},
      {"graph G:\n"
       "    1 - 2\n"
       "set_edge_weight(G, 1, 2, \"heavy\")\n",
       "incompatible operand types"},
      {"graph G:\n"
       "    1\n"
       "remove_node(G, 2)\n",
       "invalid node id"},
      {"graph G:\n"
       "    1\n"
       "    2\n"
       "remove_edge(G, 1, 2)\n",
       "dict key not found"},
  };
  const char *node_defaults_source =
      "graph G:\n"
      "    defaults node {\"label\": \"unknown\", \"score\": 0}\n"
      "    \"Alice\"\n"
      "add_node(G, \"Bob\")\n"
      "set_node_attrs(G, \"Bob\", {\"score\": 10})\n"
      "print(node_attrs(G, \"Bob\")[\"label\"])\n"
      "print(node_attrs(G, \"Bob\")[\"score\"])\n";
  const char *edge_defaults_source =
      "graph G:\n"
      "    defaults edge {\"kind\": \"normal\", \"weight\": 1}\n"
      "    1 - 2\n"
      "add_edge(G, 2, 3)\n"
      "set_edge_attrs(G, 2, 3, {\"weight\": 7})\n"
      "print(edge_attrs(G, 2, 3)[\"kind\"])\n"
      "print(edge_weight(G, 2, 3))\n";
  char path[512];
  char output[256];
  graphion_runtime_scope defaults_scope;
  graphion_runtime_diagnostic defaults_diagnostic;
  size_t i;

  for (i = 0U; i < sizeof(error_cases) / sizeof(error_cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(error_cases[i].source, &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != GINT_ERR_RUN) {
      return (int)(100 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, error_cases[i].message) != 0) {
      return (int)(200 + i);
    }
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("add_node(Missing, 1)\n", &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != GINT_ERR_UNKNOWN_VARIABLE) {
      return 250;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "unknown graph variable 'Missing'") != 0) {
      return 251;
    }
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("set_node_attrs(Missing, 1, {})\n", &scope, &diagnostic);
    graphion_runtime_scope_dispose(&scope);
    if (rc != GINT_ERR_UNKNOWN_VARIABLE) {
      return 252;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "unknown graph variable 'Missing'") != 0) {
      return 253;
    }
  }

  graphion_runtime_scope_init(&defaults_scope);
  if (test_capture_gion_output(node_defaults_source,
                               "gion_graph_mutation_node_defaults.txt",
                               &defaults_scope,
                               &defaults_diagnostic,
                               path,
                               sizeof(path),
                               output,
                               sizeof(output)) == 0) {
    graphion_runtime_scope_dispose(&defaults_scope);
    return 300;
  }
  graphion_runtime_scope_dispose(&defaults_scope);
  if (strcmp(output, "unknown\n10\n") != 0) {
    return 301;
  }
  graphion_runtime_scope_init(&defaults_scope);
  if (test_capture_gion_output(edge_defaults_source,
                               "gion_graph_mutation_edge_defaults.txt",
                               &defaults_scope,
                               &defaults_diagnostic,
                               path,
                               sizeof(path),
                               output,
                               sizeof(output)) == 0) {
    graphion_runtime_scope_dispose(&defaults_scope);
    return 302;
  }
  graphion_runtime_scope_dispose(&defaults_scope);
  if (strcmp(output, "normal\n7\n") != 0) {
    return 303;
  }
  return 0;
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
  if (report.items[0].line != 2U || report.items[0].column != 5U ||
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
  if (report.items[0].line != 2U || report.items[0].column != 5U ||
      strcmp(report.items[0].message, "graph numeric node ids have gaps; missing id: 0") != 0) {
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
                                        "    \"Alice\"\n"
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
                                        "    \"Alice\"\n"
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
  if (report.items[0].line != 3U || report.items[0].column != 5U ||
      strcmp(report.items[0].message, "graph numeric node ids have gaps; missing id: 2") != 0) {
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
  if (report.items[0].line != 3U || report.items[0].column != 5U) {
    return 16;
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
      {"graph G:\n    alpha\n", GINT_ERR_UNKNOWN_VARIABLE, "unknown graph node variable"},
      {"flag = true\ngraph G:\n    flag\n", GINT_ERR_PARSE, "graph node variable must be int or string"},
      {"graph G:\n    \"alpha\" extra\n", GINT_ERR_PARSE, "unexpected trailing tokens after graph node"},
      {"graph G:\n    \"alpha\" {\"a\": 1,}\n", GINT_ERR_PARSE, "trailing comma is not allowed in dict literal"},
      {"graph G:\n    \"alpha\" [1]\n", GINT_ERR_PARSE, "unexpected trailing tokens after graph node"},
      {"graph G:\n    \"alpha\" {\"a\": 1}\n    \"beta\" {\"b\": 2}\n",
       GINT_ERR_PARSE,
       "graph node attributes must use the same keys"},
      {"graph G:\n    \"alpha\" {\"a\": 1}\n    \"beta\"\n",
       GINT_ERR_PARSE,
       "graph node attributes must use the same keys"},
      {"graph G:\n    defaults node {\"a\": 0}\n    \"alpha\" {\"b\": 1}\n",
       GINT_ERR_PARSE,
       "graph node attributes must use declared default keys"},
      {"graph G:\n    defaults node {\"a\": 0}\n    defaults node {\"a\": 1}\n    \"alpha\"\n",
       GINT_ERR_PARSE,
       "duplicate graph node attribute defaults"},
      {"graph G:\n    defaults edge {\"weight\": \"heavy\"}\n    1 - 2\n",
       GINT_ERR_PARSE,
       "graph edge weight must be int or float"},
      {"graph G:\n    defaults edge {\"weight\": 1}\n    defaults edge {\"weight\": 2}\n    1 - 2\n",
       GINT_ERR_PARSE,
       "duplicate graph edge attribute defaults"},
      {"graph G:\n    defaults edge {\"weight\": 1}\n    1 - 2 {\"kind\": \"path\"}\n",
       GINT_ERR_PARSE,
       "graph edge attributes must use declared default keys"},
      {"graph G:\n    defaults other {\"weight\": 1}\n    1 - 2\n",
       GINT_ERR_PARSE,
       "expected 'node' or 'edge' after defaults"},
      {"graph G:\n    1 - 2 {\"weight\": \"heavy\"}\n", GINT_ERR_PARSE, "graph edge weight must be int or float"},
      {"text = \"heavy\"\ngraph G:\n    1 - 2 text\n",
       GINT_ERR_PARSE,
       "graph edge weight expression must be int, float, or dict"},
      {"graph G:\n    1 - 2 {\"weight\": 1}\n    2 - 3\n",
       GINT_ERR_PARSE,
       "graph edge attributes must use the same keys"},
      {"graph G:\n    1 - 2 {\"weight\": 1}\n    2 - 3 {\"kind\": \"path\"}\n",
       GINT_ERR_PARSE,
       "graph edge attributes must use the same keys"},
      {"graph G:\n    1 - 2 {\"kind\": \"path\",}\n",
       GINT_ERR_PARSE,
       "trailing comma is not allowed in dict literal"},
      {"graph G:\n    1 -\n", GINT_ERR_PARSE, "expected graph node name or id"},
      {"graph G:\n    1 - 2 extra\n", GINT_ERR_UNKNOWN_OPERAND, "unknown operand 'extra'"},
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

int test_gion_graph_declaration_column_diagnostics(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_line;
    unsigned int expected_column;
    const char *message;
  } cases[] = {
      {"graph;\n", GINT_ERR_PARSE, 1U, 6U, "expected graph name"},
      {"graph G\n", GINT_ERR_PARSE, 1U, 8U, "expected ';' or ':' after graph declaration"},
      {"graph G; extra\n", GINT_ERR_PARSE, 1U, 10U, "unexpected trailing tokens after graph declaration"},
      {"graph G:\n", GINT_ERR_PARSE, 1U, 9U, "expected indented graph node block"},
      {"graph G:\n    ?\n", GINT_ERR_PARSE, 2U, 1U, "expected graph node name or id"},
      {"graph G:\n    \"alpha\" extra\n", GINT_ERR_PARSE, 2U, 9U, "unexpected trailing tokens after graph node"},
      {"graph G:\n    defaults other {\"weight\": 1}\n    1 - 2\n",
       GINT_ERR_PARSE,
       2U,
       10U,
       "expected 'node' or 'edge' after defaults"},
      {"graph G:\n    defaults node {\"a\": 0}\n    defaults node {\"a\": 1}\n    \"alpha\"\n",
       GINT_ERR_PARSE,
       3U,
       15U,
       "duplicate graph node attribute defaults"},
      {"graph G:\n    defaults edge {\"weight\": \"heavy\"}\n    1 - 2\n",
       GINT_ERR_PARSE,
       2U,
       15U,
       "graph edge weight must be int or float"},
      {"graph G:\n    1 - 2 {\"weight\": \"heavy\"}\n",
       GINT_ERR_PARSE,
       2U,
       7U,
       "graph edge weight must be int or float"},
      {"text = \"heavy\"\ngraph G:\n    1 - 2 text\n",
       GINT_ERR_PARSE,
       3U,
       7U,
       "graph edge weight expression must be int, float, or dict"},
      {"graph G:\n    1 -\n", GINT_ERR_PARSE, 2U, 4U, "expected graph node name or id"},
      {"hypergraph;\n", GINT_ERR_PARSE, 1U, 11U, "expected hypergraph name"},
      {"hypergraph H\n", GINT_ERR_PARSE, 1U, 13U, "expected ';' after hypergraph declaration"},
      {"hypergraph H; extra\n",
       GINT_ERR_PARSE,
       1U,
       15U,
       "unexpected trailing tokens after hypergraph declaration"},
      {"hypergraph H:\n", GINT_ERR_PARSE, 1U, 14U, "expected indented hypergraph vertex block"},
      {"hypergraph H:\n    ?\n", GINT_ERR_PARSE, 2U, 1U, "expected graph node name or id"},
      {"hypergraph H:\n    \"Alice\" extra\n",
       GINT_ERR_PARSE,
       2U,
       9U,
       "unexpected trailing tokens after hypergraph vertex"},
      {"hypergraph H:\n    [1, 2\n", GINT_ERR_PARSE, 2U, 6U, "expected ']' after hyperedge vertex list"},
      {"hypergraph H:\n    []\n", GINT_ERR_PARSE, 2U, 1U, "hyperedge must contain at least one vertex"},
      {"hypergraph H:\n    defaults other {}\n",
       GINT_ERR_PARSE,
       2U,
       10U,
       "expected 'vertex' or 'hyperedge' after defaults"},
      {"hypergraph H:\n    defaults vertex {\"a\": 0}\n    defaults vertex {\"a\": 1}\n    \"Alice\"\n",
       GINT_ERR_PARSE,
       3U,
       17U,
       "duplicate hypergraph vertex attribute defaults"},
      {"hypergraph H:\n    [1] [\"bad\"]\n",
       GINT_ERR_PARSE,
       2U,
       5U,
       "hypergraph hyperedge attributes must be a dict literal"},
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
    if (diagnostic.line != cases[i].expected_line || diagnostic.column != cases[i].expected_column) {
      return (int)(200 + i);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(300 + i);
    }
  }
  return 0;
}
