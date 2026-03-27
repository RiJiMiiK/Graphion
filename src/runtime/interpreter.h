/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_RUNTIME_INTERPRETER_H
#define GRAPHION_RUNTIME_INTERPRETER_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include "vm/vm.h"

enum {
  GRAPHION_RUNTIME_BINDING_MAX = 128,
  GRAPHION_RUNTIME_NAME_MAX = 64,
  GRAPHION_RUNTIME_STRING_MAX = 256,
  GRAPHION_RUNTIME_GRAPH_EDGE_MAX = 128,
  GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX = 256,
  GRAPHION_RUNTIME_HYPEREDGE_MAX = 64,
  GRAPHION_RUNTIME_HYPEREDGE_NODE_MAX = 32,
  GRAPHION_RUNTIME_HYPERGRAPH_INCIDENCE_MAX =
      GRAPHION_RUNTIME_HYPEREDGE_MAX * GRAPHION_RUNTIME_HYPEREDGE_NODE_MAX,
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
  graphion_csr_graph lowered_graph;
  uint32_t lowered_offsets[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX + 1U];
  uint32_t lowered_neighbors[GRAPHION_RUNTIME_GRAPH_EDGE_MAX];
  int64_t lowered_node_ids[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  size_t lowered_node_count;
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
  graphion_hypergraph lowered_hypergraph;
  uint32_t lowered_node_offsets[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX + 1U];
  uint32_t lowered_node_hyperedges[GRAPHION_RUNTIME_HYPERGRAPH_INCIDENCE_MAX];
  uint32_t lowered_hyperedge_offsets[GRAPHION_RUNTIME_HYPEREDGE_MAX + 1U];
  uint32_t lowered_hyperedge_nodes[GRAPHION_RUNTIME_HYPERGRAPH_INCIDENCE_MAX];
  int64_t lowered_node_ids[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  size_t lowered_node_count;
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

typedef struct {
  int64_t items[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  size_t count;
} graphion_runtime_int_sequence_value;

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
  GRAPHION_VALUE_HYPEREDGE = 10,
  GRAPHION_VALUE_INT_SEQUENCE = 11
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
  graphion_runtime_int_sequence_value int_sequence_value;
} graphion_runtime_value;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  size_t vm_global_index;
  int is_vm_global;
  int value_materialized;
  graphion_runtime_value value;
} graphion_runtime_binding;

typedef struct {
  graphion_runtime_binding bindings[GRAPHION_RUNTIME_BINDING_MAX];
  graphion_vm_value vm_globals[GRAPHION_RUNTIME_BINDING_MAX];
  graphion_vm prepared_vm;
  const void *prepared_program_key;
  graphion_output_sink prepared_output;
  int prepared_output_bound;
  int prepared_vm_ready;
  int vm_globals_enabled;
  int vm_materialized_dirty;
  size_t count;
} graphion_runtime_scope;

typedef struct {
  size_t line;
  size_t column;
  const char *message;
} graphion_runtime_diagnostic;

enum {
  GRAPHION_RUNTIME_LINE_MAX = 512,
  GRAPHION_RUNTIME_SOURCE_LINE_MAX = 256,
  GRAPHION_RUNTIME_FUNCTION_MAX = 32,
  GRAPHION_RUNTIME_PARAM_MAX = 8,
  GRAPHION_RUNTIME_PREPARED_STEP_MAX = GRAPHION_RUNTIME_SOURCE_LINE_MAX,
  GRAPHION_RUNTIME_PREPARED_CONST_MAX = GRAPHION_RUNTIME_SOURCE_LINE_MAX,
  GRAPHION_RUNTIME_PREPARED_VM_INSN_MAX = (GRAPHION_RUNTIME_SOURCE_LINE_MAX * 2) + 1
};

typedef struct {
  size_t line_no;
  size_t indent;
  char text[GRAPHION_RUNTIME_LINE_MAX];
} graphion_runtime_source_line;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  char params[GRAPHION_RUNTIME_PARAM_MAX][GRAPHION_RUNTIME_NAME_MAX];
  size_t param_count;
  size_t body_start;
  size_t body_end;
} graphion_runtime_function;

typedef enum {
  GRAPHION_RUNTIME_STEP_NONE = 0,
  GRAPHION_RUNTIME_STEP_STORE_LITERAL = 1,
  GRAPHION_RUNTIME_STEP_STORE_COPY = 2,
  GRAPHION_RUNTIME_STEP_PRINT_LITERAL = 3,
  GRAPHION_RUNTIME_STEP_PRINT_COPY = 4,
  GRAPHION_RUNTIME_STEP_STORE_INT_ADD = 5,
  GRAPHION_RUNTIME_STEP_PRINT_INT_ADD = 6
} graphion_runtime_step_kind;

typedef enum {
  GRAPHION_RUNTIME_OPERAND_NONE = 0,
  GRAPHION_RUNTIME_OPERAND_INT_LITERAL = 1,
  GRAPHION_RUNTIME_OPERAND_GLOBAL = 2
} graphion_runtime_prepared_operand_kind;

typedef struct {
  int kind;
  size_t slot;
  int64_t int_value;
} graphion_runtime_prepared_operand;

typedef struct {
  int kind;
  int64_t int_value;
  double float_value;
  int bool_value;
  char string_value[GRAPHION_RUNTIME_STRING_MAX];
} graphion_runtime_prepared_literal;

typedef struct {
  int kind;
  size_t target_slot;
  size_t source_slot;
  graphion_runtime_prepared_literal literal;
  graphion_runtime_prepared_operand lhs_operand;
  graphion_runtime_prepared_operand rhs_operand;
} graphion_runtime_prepared_step;

typedef struct {
  graphion_runtime_source_line lines[GRAPHION_RUNTIME_SOURCE_LINE_MAX];
  size_t line_count;
  graphion_runtime_function functions[GRAPHION_RUNTIME_FUNCTION_MAX];
  size_t function_count;
  char global_names[GRAPHION_RUNTIME_BINDING_MAX][GRAPHION_RUNTIME_NAME_MAX];
  size_t global_count;
  graphion_runtime_prepared_step prepared_steps[GRAPHION_RUNTIME_PREPARED_STEP_MAX];
  size_t prepared_step_count;
  int prepared_top_level_only;
  int prepared_overwrites_all_globals;
  graphion_vm_value prepared_const_pool[GRAPHION_RUNTIME_PREPARED_CONST_MAX];
  size_t prepared_const_count;
  graphion_insn prepared_vm_program[GRAPHION_RUNTIME_PREPARED_VM_INSN_MAX];
  size_t prepared_vm_program_len;
  int prepared_vm_enabled;
} graphion_runtime_program;

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

void graphion_runtime_program_init(graphion_runtime_program *program);
void graphion_runtime_program_dispose(graphion_runtime_program *program);

int graphion_prepare_source(const char *source,
                            graphion_runtime_program *program,
                            graphion_runtime_diagnostic *diagnostic);

int graphion_execute_program(const graphion_runtime_program *program,
                             graphion_runtime_scope *scope,
                             graphion_runtime_diagnostic *diagnostic,
                             FILE *output);

int graphion_execute_prepared_program_with_sink(const graphion_runtime_program *program,
                                                graphion_runtime_scope *scope,
                                                graphion_runtime_diagnostic *diagnostic,
                                                const graphion_output_sink *output);

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic);

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output);

#endif
