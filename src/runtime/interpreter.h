/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_H
#define GRAPHION_RUNTIME_INTERPRETER_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

enum {
  GRAPHION_RUNTIME_BINDING_MAX = 128,
  GRAPHION_RUNTIME_NAME_MAX = 64,
  GRAPHION_RUNTIME_STRING_MAX = 256,
  GRAPHION_RUNTIME_GRAPH_EDGE_MAX = 128,
  GRAPHION_RUNTIME_HYPEREDGE_MAX = 64,
  GRAPHION_RUNTIME_HYPEREDGE_NODE_MAX = 32,
  GRAPHION_RUNTIME_ATTRIBUTE_MAX = 8
};

typedef enum {
  GRAPHION_ATTRIBUTE_INT = 1,
  GRAPHION_ATTRIBUTE_FLOAT = 2,
  GRAPHION_ATTRIBUTE_BOOL = 3,
  GRAPHION_ATTRIBUTE_STRING = 4
} graphion_runtime_attribute_kind;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  int kind;
  int64_t int_value;
  double float_value;
  int bool_value;
  char string_value[GRAPHION_RUNTIME_STRING_MAX];
} graphion_runtime_attribute;

typedef struct {
  int64_t source;
  int64_t target;
  int has_weight;
  double weight;
  graphion_runtime_attribute attributes[GRAPHION_RUNTIME_ATTRIBUTE_MAX];
  size_t attribute_count;
} graphion_runtime_graph_edge;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  graphion_runtime_graph_edge edges[GRAPHION_RUNTIME_GRAPH_EDGE_MAX];
  size_t edge_count;
  size_t node_count;
} graphion_runtime_graph_value;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  int64_t nodes[GRAPHION_RUNTIME_HYPEREDGE_NODE_MAX];
  size_t node_count;
  int has_weight;
  double weight;
  graphion_runtime_attribute attributes[GRAPHION_RUNTIME_ATTRIBUTE_MAX];
  size_t attribute_count;
} graphion_runtime_hyperedge;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  graphion_runtime_hyperedge hyperedges[GRAPHION_RUNTIME_HYPEREDGE_MAX];
  size_t hyperedge_count;
  size_t node_count;
} graphion_runtime_hypergraph_value;

typedef struct {
  int64_t id;
  const graphion_runtime_graph_value *graph;
} graphion_runtime_graph_node_value;

typedef struct {
  const graphion_runtime_graph_value *graph;
  const graphion_runtime_graph_edge *edge;
} graphion_runtime_graph_edge_value;

typedef struct {
  int64_t id;
  const graphion_runtime_hypergraph_value *hypergraph;
} graphion_runtime_hypergraph_node_value;

typedef struct {
  const graphion_runtime_hypergraph_value *hypergraph;
  const graphion_runtime_hyperedge *hyperedge;
} graphion_runtime_hyperedge_value;

typedef enum {
  GRAPHION_VALUE_NONE = 0,
  GRAPHION_VALUE_INT = 1,
  GRAPHION_VALUE_FLOAT = 2,
  GRAPHION_VALUE_BOOL = 3,
  GRAPHION_VALUE_STRING = 4,
  GRAPHION_VALUE_GRAPH = 5,
  GRAPHION_VALUE_HYPERGRAPH = 6,
  GRAPHION_VALUE_GRAPH_NODE = 7,
  GRAPHION_VALUE_GRAPH_EDGE = 8,
  GRAPHION_VALUE_HYPERGRAPH_NODE = 9,
  GRAPHION_VALUE_HYPEREDGE = 10
} graphion_runtime_value_kind;

typedef struct {
  int kind;
  int64_t int_value;
  double float_value;
  int bool_value;
  char string_value[GRAPHION_RUNTIME_STRING_MAX];
  graphion_runtime_graph_value *graph_value;
  graphion_runtime_hypergraph_value *hypergraph_value;
  int owns_graph_value;
  int owns_hypergraph_value;
  graphion_runtime_graph_node_value graph_node_value;
  graphion_runtime_graph_edge_value graph_edge_value;
  graphion_runtime_hypergraph_node_value hypergraph_node_value;
  graphion_runtime_hyperedge_value hyperedge_value;
} graphion_runtime_value;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  graphion_runtime_value value;
} graphion_runtime_binding;

typedef struct {
  graphion_runtime_binding bindings[GRAPHION_RUNTIME_BINDING_MAX];
  size_t count;
} graphion_runtime_scope;

typedef struct {
  size_t line;
  size_t column;
  const char *message;
} graphion_runtime_diagnostic;

typedef enum {
  GINT_OK = 0,
  GINT_ERR_INVALID_ARG = -1,
  GINT_ERR_CAPACITY = -2,
  GINT_ERR_PARSE = -3,
  GINT_ERR_UNKNOWN_VARIABLE = -4,
  GINT_ERR_RESERVED_NAME = -5,
  GINT_ERR_CALL = -6,
  GINT_ERR_RETURN = -7
} graphion_interpreter_result;

void graphion_runtime_scope_init(graphion_runtime_scope *scope);
void graphion_runtime_scope_dispose(graphion_runtime_scope *scope);

const graphion_runtime_value *graphion_runtime_scope_find(const graphion_runtime_scope *scope,
                                                          const char *name);

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic);

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output);

#endif
