/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum {
  GINT_LINE_MAX = GRAPHION_RUNTIME_LINE_MAX,
  GINT_SOURCE_LINE_MAX = GRAPHION_RUNTIME_SOURCE_LINE_MAX,
  GINT_FUNCTION_MAX = GRAPHION_RUNTIME_FUNCTION_MAX,
  GINT_PARAM_MAX = GRAPHION_RUNTIME_PARAM_MAX,
  GINT_ARG_MAX = 8
};

static void set_diagnostic(graphion_runtime_diagnostic *diagnostic,
                           size_t line,
                           size_t column,
                           const char *message);
static void trim_in_place(char *s);
static int is_valid_identifier(const char *name);
static int is_reserved_name(const char *name);
static int parse_string_literal(const char *token, graphion_runtime_value *value);
static int parse_bool_literal(const char *token, graphion_runtime_value *value);
static int parse_int_literal(const char *token, graphion_runtime_value *value);
static int parse_float_literal(const char *token, graphion_runtime_value *value);
static int prepare_simple_top_level_steps(graphion_runtime_program *program,
                                          graphion_runtime_diagnostic *diagnostic);
static int execute_prepared_top_level_program(const graphion_runtime_program *program,
                                              graphion_runtime_scope *scope);
static int compile_prepared_top_level_vm_program(graphion_runtime_program *program,
                                                 graphion_runtime_diagnostic *diagnostic);

static int parse_graph_header(const char *text,
                              char *name_out,
                              graphion_runtime_diagnostic *diagnostic,
                              size_t line_no) {
  size_t name_len;
  const char *cursor;
  if (strncmp(text, "graph ", 6U) != 0) {
    return 0;
  }
  cursor = text + 6U;
  name_len = strlen(cursor);
  if (name_len == 0U || cursor[name_len - 1U] != ':') {
    set_diagnostic(diagnostic, line_no, 1U, "invalid graph declaration");
    return GINT_ERR_PARSE;
  }
  if (name_len >= GRAPHION_RUNTIME_NAME_MAX) {
    set_diagnostic(diagnostic, line_no, 1U, "graph name too long");
    return GINT_ERR_PARSE;
  }
  memcpy(name_out, cursor, name_len - 1U);
  name_out[name_len - 1U] = '\0';
  trim_in_place(name_out);
  if (!is_valid_identifier(name_out)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid graph name");
    return GINT_ERR_PARSE;
  }
  if (is_reserved_name(name_out)) {
    set_diagnostic(diagnostic, line_no, 1U, "reserved name cannot be used as a graph");
    return GINT_ERR_RESERVED_NAME;
  }
  return GINT_OK;
}

static int parse_hypergraph_header(const char *text,
                                   char *name_out,
                                   graphion_runtime_diagnostic *diagnostic,
                                   size_t line_no) {
  size_t name_len;
  const char *cursor;
  if (strncmp(text, "hypergraph ", 11U) != 0) {
    return 0;
  }
  cursor = text + 11U;
  name_len = strlen(cursor);
  if (name_len == 0U || cursor[name_len - 1U] != ':') {
    set_diagnostic(diagnostic, line_no, 1U, "invalid hypergraph declaration");
    return GINT_ERR_PARSE;
  }
  if (name_len >= GRAPHION_RUNTIME_NAME_MAX) {
    set_diagnostic(diagnostic, line_no, 1U, "hypergraph name too long");
    return GINT_ERR_PARSE;
  }
  memcpy(name_out, cursor, name_len - 1U);
  name_out[name_len - 1U] = '\0';
  trim_in_place(name_out);
  if (!is_valid_identifier(name_out)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid hypergraph name");
    return GINT_ERR_PARSE;
  }
  if (is_reserved_name(name_out)) {
    set_diagnostic(diagnostic, line_no, 1U, "reserved name cannot be used as a hypergraph");
    return GINT_ERR_RESERVED_NAME;
  }
  return GINT_OK;
}

static int parse_graph_edge(const char *text,
                            graphion_runtime_graph_edge *edge,
                            graphion_runtime_diagnostic *diagnostic,
                            size_t line_no) {
  const char *arrow;
  const char *attrs;
  char lhs[GRAPHION_RUNTIME_NAME_MAX];
  char rhs[GRAPHION_RUNTIME_NAME_MAX];
  size_t lhs_len;
  size_t rhs_len;
  graphion_runtime_value source_value;
  graphion_runtime_value target_value;

  if (text == NULL || edge == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  arrow = strstr(text, "->");
  if (arrow == NULL) {
    set_diagnostic(diagnostic, line_no, 1U, "expected graph edge using a -> b syntax");
    return GINT_ERR_PARSE;
  }
  attrs = strchr(arrow + 2U, '[');
  lhs_len = (size_t)(arrow - text);
  rhs_len = attrs != NULL ? (size_t)(attrs - (arrow + 2U)) : strlen(arrow + 2U);
  if (lhs_len == 0U || lhs_len >= sizeof(lhs) || rhs_len == 0U || rhs_len >= sizeof(rhs)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid graph edge");
    return GINT_ERR_PARSE;
  }
  memset(edge, 0, sizeof(*edge));
  memcpy(lhs, text, lhs_len);
  lhs[lhs_len] = '\0';
  memcpy(rhs, arrow + 2U, rhs_len);
  rhs[rhs_len] = '\0';
  trim_in_place(lhs);
  trim_in_place(rhs);
  if (!parse_int_literal(lhs, &source_value) || !parse_int_literal(rhs, &target_value)) {
    set_diagnostic(diagnostic, line_no, 1U, "graph node ids must be integers");
    return GINT_ERR_PARSE;
  }
  edge->source = source_value.int_value;
  edge->target = target_value.int_value;
  if (attrs != NULL) {
    const char *close_bracket = strrchr(attrs, ']');
    char attrs_buf[GINT_LINE_MAX];
    size_t attrs_len;
    int in_string = 0;
    if (close_bracket == NULL || close_bracket < attrs) {
      set_diagnostic(diagnostic, line_no, 1U, "invalid graph attribute list");
      return GINT_ERR_PARSE;
    }
    attrs_len = (size_t)(close_bracket - attrs - 1);
    if (attrs_len >= sizeof(attrs_buf)) {
      set_diagnostic(diagnostic, line_no, 1U, "graph attribute list too long");
      return GINT_ERR_PARSE;
    }
    memcpy(attrs_buf, attrs + 1U, attrs_len);
    attrs_buf[attrs_len] = '\0';
    trim_in_place(attrs_buf);
    if (attrs_buf[0] != '\0') {
      size_t start = 0U;
      size_t i = 0U;
      while (1) {
        const char current = attrs_buf[i];
        if (current == '"' && (i == 0U || attrs_buf[i - 1U] != '\\')) {
          in_string = !in_string;
        }
        if (!in_string && (current == ',' || current == '\0')) {
          char entry[GINT_LINE_MAX];
          char *eq;
          size_t entry_len = i - start;
          graphion_runtime_attribute attribute;
          if (entry_len == 0U || entry_len >= sizeof(entry)) {
            set_diagnostic(diagnostic, line_no, 1U, "invalid graph attribute entry");
            return GINT_ERR_PARSE;
          }
          if (edge->attribute_count >= GRAPHION_RUNTIME_ATTRIBUTE_MAX) {
            set_diagnostic(diagnostic, line_no, 1U, "graph attribute capacity exceeded");
            return GINT_ERR_CAPACITY;
          }
          memcpy(entry, attrs_buf + start, entry_len);
          entry[entry_len] = '\0';
          trim_in_place(entry);
          eq = strchr(entry, '=');
          if (eq == NULL) {
            set_diagnostic(diagnostic, line_no, 1U, "graph attributes must use key=value syntax");
            return GINT_ERR_PARSE;
          }
          memset(&attribute, 0, sizeof(attribute));
          *eq = '\0';
          trim_in_place(entry);
          trim_in_place(eq + 1U);
          if (!is_valid_identifier(entry)) {
            set_diagnostic(diagnostic, line_no, 1U, "invalid graph attribute name");
            return GINT_ERR_PARSE;
          }
          memcpy(attribute.name, entry, strlen(entry) + 1U);
          if (strcmp(attribute.name, "weight") == 0) {
            graphion_runtime_value weight_value;
            if (!parse_float_literal(eq + 1U, &weight_value) && !parse_int_literal(eq + 1U, &weight_value)) {
              set_diagnostic(diagnostic, line_no, 1U, "weight must be numeric");
              return GINT_ERR_PARSE;
            }
            edge->has_weight = 1;
            edge->weight = weight_value.kind == GRAPHION_VALUE_FLOAT ? weight_value.float_value
                                                                     : (double)weight_value.int_value;
          } else if (parse_string_literal(eq + 1U, &source_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_STRING;
            memcpy(attribute.string_value, source_value.string_value, strlen(source_value.string_value) + 1U);
            edge->attributes[edge->attribute_count++] = attribute;
          } else if (parse_bool_literal(eq + 1U, &source_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_BOOL;
            attribute.bool_value = source_value.bool_value;
            edge->attributes[edge->attribute_count++] = attribute;
          } else if (parse_float_literal(eq + 1U, &source_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_FLOAT;
            attribute.float_value = source_value.float_value;
            edge->attributes[edge->attribute_count++] = attribute;
          } else if (parse_int_literal(eq + 1U, &source_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_INT;
            attribute.int_value = source_value.int_value;
            edge->attributes[edge->attribute_count++] = attribute;
          } else {
            set_diagnostic(diagnostic, line_no, 1U, "graph attributes must be scalar values");
            return GINT_ERR_PARSE;
          }
          if (current == '\0') {
            break;
          }
          start = i + 1U;
        }
        if (current == '\0') {
          break;
        }
        ++i;
      }
    }
  }
  return GINT_OK;
}

static int graph_contains_node(const graphion_runtime_value *graph, int64_t node_id) {
  size_t i;
  if (graph == NULL || graph->graph_value == NULL) {
    return 0;
  }
  for (i = 0U; i < graph->graph_value->edge_count; ++i) {
    if (graph->graph_value->edges[i].source == node_id || graph->graph_value->edges[i].target == node_id) {
      return 1;
    }
  }
  return 0;
}

static int hypergraph_contains_node(const graphion_runtime_value *hypergraph, int64_t node_id) {
  size_t i;
  size_t j;
  if (hypergraph == NULL || hypergraph->hypergraph_value == NULL) {
    return 0;
  }
  for (i = 0U; i < hypergraph->hypergraph_value->hyperedge_count; ++i) {
    for (j = 0U; j < hypergraph->hypergraph_value->hyperedges[i].node_count; ++j) {
      if (hypergraph->hypergraph_value->hyperedges[i].nodes[j] == node_id) {
        return 1;
      }
    }
  }
  return 0;
}

static size_t hypergraph_incident_count(const graphion_runtime_hypergraph_value *hypergraph, int64_t node_id) {
  size_t i;
  size_t j;
  size_t count = 0U;
  if (hypergraph == NULL) {
    return 0U;
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    for (j = 0U; j < hypergraph->hyperedges[i].node_count; ++j) {
      if (hypergraph->hyperedges[i].nodes[j] == node_id) {
        count += 1U;
        break;
      }
    }
  }
  return count;
}

static uint64_t hypergraph_incident_sum(const graphion_runtime_hypergraph_value *hypergraph, int64_t node_id) {
  size_t i;
  size_t j;
  uint64_t sum = 0U;
  if (hypergraph == NULL) {
    return 0U;
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    for (j = 0U; j < hypergraph->hyperedges[i].node_count; ++j) {
      if (hypergraph->hyperedges[i].nodes[j] == node_id) {
        sum += (uint64_t)i;
        break;
      }
    }
  }
  return sum;
}

static size_t graph_collect_nodes(const graphion_runtime_graph_value *graph,
                                  int64_t nodes[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX]) {
  size_t i;
  size_t count = 0U;
  if (graph == NULL || nodes == NULL) {
    return 0U;
  }
  for (i = 0U; i < graph->edge_count; ++i) {
    size_t j;
    int seen_source = 0;
    int seen_target = 0;
    for (j = 0U; j < count; ++j) {
      if (nodes[j] == graph->edges[i].source) {
        seen_source = 1;
      }
      if (nodes[j] == graph->edges[i].target) {
        seen_target = 1;
      }
    }
    if (!seen_source && count < GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX) {
      nodes[count++] = graph->edges[i].source;
    }
    if (!seen_target && count < GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX) {
      nodes[count++] = graph->edges[i].target;
    }
  }
  return count;
}

static int graph_find_node_index(const int64_t nodes[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX],
                                 size_t node_count,
                                 int64_t node_id) {
  size_t i;
  for (i = 0U; i < node_count; ++i) {
    if (nodes[i] == node_id) {
      return (int)i;
    }
  }
  return -1;
}

static int prepare_graph_native(graphion_runtime_graph_value *graph) {
  size_t counts[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  size_t i;

  if (graph == NULL) {
    return 0;
  }
  memset(&graph->lowered_graph, 0, sizeof(graph->lowered_graph));
  memset(graph->lowered_offsets, 0, sizeof(graph->lowered_offsets));
  memset(graph->lowered_neighbors, 0, sizeof(graph->lowered_neighbors));
  memset(graph->lowered_node_ids, 0, sizeof(graph->lowered_node_ids));
  graph->lowered_node_count = graph_collect_nodes(graph, graph->lowered_node_ids);
  if (graph->lowered_node_count == 0U) {
    return 0;
  }

  memset(counts, 0, sizeof(counts));
  for (i = 0U; i < graph->edge_count; ++i) {
    int source_index = graph_find_node_index(graph->lowered_node_ids, graph->lowered_node_count, graph->edges[i].source);
    if (source_index < 0) {
      return 0;
    }
    counts[(size_t)source_index] += 1U;
  }

  graph->lowered_offsets[0] = 0U;
  for (i = 0U; i < graph->lowered_node_count; ++i) {
    graph->lowered_offsets[i + 1U] = graph->lowered_offsets[i] + (uint32_t)counts[i];
  }

  memset(counts, 0, sizeof(counts));
  for (i = 0U; i < graph->edge_count; ++i) {
    int source_index = graph_find_node_index(graph->lowered_node_ids, graph->lowered_node_count, graph->edges[i].source);
    int target_index = graph_find_node_index(graph->lowered_node_ids, graph->lowered_node_count, graph->edges[i].target);
    uint32_t slot;
    if (source_index < 0 || target_index < 0) {
      return 0;
    }
    slot = graph->lowered_offsets[(size_t)source_index] + (uint32_t)counts[(size_t)source_index];
    graph->lowered_neighbors[slot] = (uint32_t)target_index;
    counts[(size_t)source_index] += 1U;
  }

  if (graphion_csr_graph_init(&graph->lowered_graph,
                              graph->lowered_node_count,
                              graph->edge_count,
                              graph->lowered_offsets,
                              graph->lowered_neighbors) != 0) {
    return 0;
  }
  graph->node_count = graph->lowered_node_count;
  return 1;
}

static int prepare_hypergraph_native(graphion_runtime_hypergraph_value *hypergraph) {
  size_t node_incidence_counts[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  size_t write_offsets[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  size_t i;
  size_t j;
  size_t incidence_count = 0U;

  if (hypergraph == NULL) {
    return 0;
  }
  memset(&hypergraph->lowered_hypergraph, 0, sizeof(hypergraph->lowered_hypergraph));
  memset(hypergraph->lowered_node_offsets, 0, sizeof(hypergraph->lowered_node_offsets));
  memset(hypergraph->lowered_node_hyperedges, 0, sizeof(hypergraph->lowered_node_hyperedges));
  memset(hypergraph->lowered_hyperedge_offsets, 0, sizeof(hypergraph->lowered_hyperedge_offsets));
  memset(hypergraph->lowered_hyperedge_nodes, 0, sizeof(hypergraph->lowered_hyperedge_nodes));
  memset(hypergraph->lowered_node_ids, 0, sizeof(hypergraph->lowered_node_ids));
  hypergraph->lowered_node_count = 0U;

  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    for (j = 0U; j < hypergraph->hyperedges[i].node_count; ++j) {
      int64_t node_id = hypergraph->hyperedges[i].nodes[j];
      if (graph_find_node_index(hypergraph->lowered_node_ids, hypergraph->lowered_node_count, node_id) < 0) {
        if (hypergraph->lowered_node_count >= GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX) {
          return 0;
        }
        hypergraph->lowered_node_ids[hypergraph->lowered_node_count++] = node_id;
      }
    }
  }
  if (hypergraph->lowered_node_count == 0U) {
    return 0;
  }

  memset(node_incidence_counts, 0, sizeof(node_incidence_counts));
  hypergraph->lowered_hyperedge_offsets[0] = 0U;
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    hypergraph->lowered_hyperedge_offsets[i + 1U] =
        hypergraph->lowered_hyperedge_offsets[i] + (uint32_t)hypergraph->hyperedges[i].node_count;
    for (j = 0U; j < hypergraph->hyperedges[i].node_count; ++j) {
      int node_index =
          graph_find_node_index(hypergraph->lowered_node_ids, hypergraph->lowered_node_count, hypergraph->hyperedges[i].nodes[j]);
      if (node_index < 0) {
        return 0;
      }
      hypergraph->lowered_hyperedge_nodes[incidence_count++] = (uint32_t)node_index;
      node_incidence_counts[(size_t)node_index] += 1U;
    }
  }

  hypergraph->lowered_node_offsets[0] = 0U;
  for (i = 0U; i < hypergraph->lowered_node_count; ++i) {
    hypergraph->lowered_node_offsets[i + 1U] =
        hypergraph->lowered_node_offsets[i] + (uint32_t)node_incidence_counts[i];
    write_offsets[i] = hypergraph->lowered_node_offsets[i];
  }

  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    for (j = 0U; j < hypergraph->hyperedges[i].node_count; ++j) {
      int node_index =
          graph_find_node_index(hypergraph->lowered_node_ids, hypergraph->lowered_node_count, hypergraph->hyperedges[i].nodes[j]);
      if (node_index < 0) {
        return 0;
      }
      hypergraph->lowered_node_hyperedges[write_offsets[(size_t)node_index]++] = (uint32_t)i;
    }
  }

  if (graphion_hypergraph_init(&hypergraph->lowered_hypergraph,
                               hypergraph->lowered_node_count,
                               hypergraph->hyperedge_count,
                               incidence_count,
                               hypergraph->lowered_node_offsets,
                               hypergraph->lowered_node_hyperedges,
                               hypergraph->lowered_hyperedge_offsets,
                               hypergraph->lowered_hyperedge_nodes) != 0) {
    return 0;
  }
  hypergraph->node_count = hypergraph->lowered_node_count;
  return 1;
}

static int graph_bfs_visit_order(const graphion_runtime_graph_value *graph,
                                 int64_t source,
                                 graphion_runtime_int_sequence_value *sequence,
                                 size_t *level_count) {
  int64_t nodes[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  int visited[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  int64_t queue[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  int32_t levels[GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX];
  size_t node_count;
  size_t head = 0U;
  size_t tail = 0U;
  size_t i;
  int source_index;

  if (graph == NULL || sequence == NULL) {
    return 0;
  }
  memset(sequence, 0, sizeof(*sequence));
  if (level_count != NULL) {
    *level_count = 0U;
  }
  node_count = graph_collect_nodes(graph, nodes);
  if (node_count == 0U) {
    return 0;
  }
  memset(visited, 0, sizeof(visited));
  for (i = 0U; i < GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX; ++i) {
    levels[i] = -1;
  }
  source_index = graph_find_node_index(nodes, node_count, source);
  if (source_index < 0) {
    return 0;
  }
  queue[tail++] = source;
  visited[(size_t)source_index] = 1;
  levels[(size_t)source_index] = 0;

  while (head < tail && sequence->count < GRAPHION_RUNTIME_SEQUENCE_ITEM_MAX) {
    const int64_t current = queue[head++];
    const int current_index = graph_find_node_index(nodes, node_count, current);
    sequence->items[sequence->count++] = current;
    for (i = 0U; i < graph->edge_count; ++i) {
      if (graph->edges[i].source == current) {
        const int64_t neighbor = graph->edges[i].target;
        const int neighbor_index = graph_find_node_index(nodes, node_count, neighbor);
        if (neighbor_index >= 0 && visited[(size_t)neighbor_index] == 0) {
          visited[(size_t)neighbor_index] = 1;
          levels[(size_t)neighbor_index] = levels[(size_t)current_index] + 1;
          queue[tail++] = neighbor;
        }
      }
    }
  }

  if (level_count != NULL) {
    size_t max_level = 0U;
    for (i = 0U; i < node_count; ++i) {
      if (levels[i] >= 0 && (size_t)levels[i] > max_level) {
        max_level = (size_t)levels[i];
      }
    }
    *level_count = sequence->count == 0U ? 0U : max_level + 1U;
  }
  return 1;
}

static const graphion_runtime_hyperedge *find_hyperedge_by_id(const graphion_runtime_hypergraph_value *hypergraph,
                                                              int64_t hyperedge_id) {
  if (hypergraph == NULL || hyperedge_id < 0 || (size_t)hyperedge_id >= hypergraph->hyperedge_count) {
    return NULL;
  }
  return &hypergraph->hyperedges[hyperedge_id];
}

static int parse_hyperedge_line(const char *text,
                                graphion_runtime_hyperedge *hyperedge,
                                graphion_runtime_diagnostic *diagnostic,
                                size_t line_no) {
  const char *open_bracket;
  const char *close_bracket;
  const char *attrs;
  char nodes_buf[GINT_LINE_MAX];
  size_t start = 0U;
  size_t i = 0U;

  if (text == NULL || hyperedge == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  open_bracket = strchr(text, '[');
  close_bracket = open_bracket != NULL ? strchr(open_bracket + 1U, ']') : NULL;
  attrs = close_bracket != NULL ? strchr(close_bracket + 1U, '[') : NULL;
  if (open_bracket == NULL || close_bracket == NULL || close_bracket < open_bracket) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid hyperedge declaration");
    return GINT_ERR_PARSE;
  }
  memset(hyperedge, 0, sizeof(*hyperedge));
  if ((size_t)(close_bracket - open_bracket - 1) >= sizeof(nodes_buf)) {
    set_diagnostic(diagnostic, line_no, 1U, "hyperedge node list too long");
    return GINT_ERR_PARSE;
  }
  memcpy(nodes_buf, open_bracket + 1, (size_t)(close_bracket - open_bracket - 1));
  nodes_buf[close_bracket - open_bracket - 1] = '\0';
  trim_in_place(nodes_buf);
  if (nodes_buf[0] == '\0') {
    set_diagnostic(diagnostic, line_no, 1U, "hyperedge node list cannot be empty");
    return GINT_ERR_PARSE;
  }
  while (1) {
    const char current = nodes_buf[i];
    if (current == ',' || current == '\0') {
      char token[GRAPHION_RUNTIME_NAME_MAX];
      graphion_runtime_value node_value;
      size_t len = i - start;
      if (len == 0U || len >= sizeof(token)) {
        set_diagnostic(diagnostic, line_no, 1U, "invalid hyperedge node id");
        return GINT_ERR_PARSE;
      }
      if (hyperedge->node_count >= GRAPHION_RUNTIME_HYPEREDGE_NODE_MAX) {
        set_diagnostic(diagnostic, line_no, 1U, "hyperedge node capacity exceeded");
        return GINT_ERR_CAPACITY;
      }
      memcpy(token, nodes_buf + start, len);
      token[len] = '\0';
      trim_in_place(token);
      if (!parse_int_literal(token, &node_value)) {
        set_diagnostic(diagnostic, line_no, 1U, "hypergraph node ids must be integers");
        return GINT_ERR_PARSE;
      }
      hyperedge->nodes[hyperedge->node_count] = node_value.int_value;
      hyperedge->node_count += 1U;
      if (current == '\0') {
        break;
      }
      start = i + 1U;
    }
    if (current == '\0') {
      break;
    }
    ++i;
  }
  if (attrs != NULL) {
    const char *attrs_close = strrchr(attrs, ']');
    char attrs_buf[GINT_LINE_MAX];
    size_t attrs_len;
    start = 0U;
    i = 0U;
    if (attrs_close == NULL || attrs_close < attrs) {
      set_diagnostic(diagnostic, line_no, 1U, "invalid hyperedge attribute list");
      return GINT_ERR_PARSE;
    }
    attrs_len = (size_t)(attrs_close - attrs - 1);
    if (attrs_len >= sizeof(attrs_buf)) {
      set_diagnostic(diagnostic, line_no, 1U, "hyperedge attribute list too long");
      return GINT_ERR_PARSE;
    }
    memcpy(attrs_buf, attrs + 1U, attrs_len);
    attrs_buf[attrs_len] = '\0';
    trim_in_place(attrs_buf);
    if (attrs_buf[0] != '\0') {
      int in_string = 0;
      while (1) {
        const char current = attrs_buf[i];
        if (current == '"' && (i == 0U || attrs_buf[i - 1U] != '\\')) {
          in_string = !in_string;
        }
        if (!in_string && (current == ',' || current == '\0')) {
          char entry[GINT_LINE_MAX];
          char *eq;
          size_t entry_len = i - start;
          graphion_runtime_attribute attribute;
          graphion_runtime_value parsed_value;
          if (entry_len == 0U || entry_len >= sizeof(entry)) {
            set_diagnostic(diagnostic, line_no, 1U, "invalid hyperedge attribute entry");
            return GINT_ERR_PARSE;
          }
          if (hyperedge->attribute_count >= GRAPHION_RUNTIME_ATTRIBUTE_MAX) {
            set_diagnostic(diagnostic, line_no, 1U, "hyperedge attribute capacity exceeded");
            return GINT_ERR_CAPACITY;
          }
          memcpy(entry, attrs_buf + start, entry_len);
          entry[entry_len] = '\0';
          trim_in_place(entry);
          eq = strchr(entry, '=');
          if (eq == NULL) {
            set_diagnostic(diagnostic, line_no, 1U, "hyperedge attributes must use key=value syntax");
            return GINT_ERR_PARSE;
          }
          memset(&attribute, 0, sizeof(attribute));
          *eq = '\0';
          trim_in_place(entry);
          trim_in_place(eq + 1U);
          if (!is_valid_identifier(entry)) {
            set_diagnostic(diagnostic, line_no, 1U, "invalid hyperedge attribute name");
            return GINT_ERR_PARSE;
          }
          memcpy(attribute.name, entry, strlen(entry) + 1U);
          if (strcmp(attribute.name, "weight") == 0) {
            if (!parse_float_literal(eq + 1U, &parsed_value) && !parse_int_literal(eq + 1U, &parsed_value)) {
              set_diagnostic(diagnostic, line_no, 1U, "weight must be numeric");
              return GINT_ERR_PARSE;
            }
            hyperedge->has_weight = 1;
            hyperedge->weight = parsed_value.kind == GRAPHION_VALUE_FLOAT ? parsed_value.float_value
                                                                          : (double)parsed_value.int_value;
          } else if (parse_string_literal(eq + 1U, &parsed_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_STRING;
            memcpy(attribute.string_value, parsed_value.string_value, strlen(parsed_value.string_value) + 1U);
            hyperedge->attributes[hyperedge->attribute_count++] = attribute;
          } else if (parse_bool_literal(eq + 1U, &parsed_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_BOOL;
            attribute.bool_value = parsed_value.bool_value;
            hyperedge->attributes[hyperedge->attribute_count++] = attribute;
          } else if (parse_float_literal(eq + 1U, &parsed_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_FLOAT;
            attribute.float_value = parsed_value.float_value;
            hyperedge->attributes[hyperedge->attribute_count++] = attribute;
          } else if (parse_int_literal(eq + 1U, &parsed_value)) {
            attribute.kind = GRAPHION_ATTRIBUTE_INT;
            attribute.int_value = parsed_value.int_value;
            hyperedge->attributes[hyperedge->attribute_count++] = attribute;
          } else {
            set_diagnostic(diagnostic, line_no, 1U, "hyperedge attributes must be scalar values");
            return GINT_ERR_PARSE;
          }
          if (current == '\0') {
            break;
          }
          start = i + 1U;
        }
        if (current == '\0') {
          break;
        }
        ++i;
      }
    }
  }
  return GINT_OK;
}

static void clear_diagnostic(graphion_runtime_diagnostic *diagnostic) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = 0U;
  diagnostic->column = 0U;
  diagnostic->message = NULL;
}

static void set_diagnostic(graphion_runtime_diagnostic *diagnostic,
                           size_t line,
                           size_t column,
                           const char *message) {
  if (diagnostic == NULL) {
    return;
  }
  diagnostic->line = line;
  diagnostic->column = column;
  diagnostic->message = message;
}

static void trim_in_place(char *s) {
  size_t start = 0U;
  size_t end;
  if (s == NULL) {
    return;
  }
  while (s[start] != '\0' && isspace((unsigned char)s[start]) != 0) {
    ++start;
  }
  if (start > 0U) {
    size_t i = 0U;
    while (s[start + i] != '\0') {
      s[i] = s[start + i];
      ++i;
    }
    s[i] = '\0';
  }
  end = strlen(s);
  while (end > 0U && isspace((unsigned char)s[end - 1U]) != 0) {
    --end;
  }
  s[end] = '\0';
}

static void strip_comments(char *s) {
  size_t i = 0U;
  int in_string = 0;
  while (s[i] != '\0') {
    if (s[i] == '"' && (i == 0U || s[i - 1U] != '\\')) {
      in_string = !in_string;
    }
    if (!in_string && s[i] == '#') {
      s[i] = '\0';
      break;
    }
    if (!in_string && s[i] == '/' && s[i + 1U] == '/') {
      s[i] = '\0';
      break;
    }
    ++i;
  }
}

static int is_identifier_start(char c) {
  return isalpha((unsigned char)c) != 0 || c == '_';
}

static int is_identifier_char(char c) {
  return isalnum((unsigned char)c) != 0 || c == '_';
}

static int is_valid_identifier(const char *name) {
  size_t i;
  if (name == NULL || name[0] == '\0' || !is_identifier_start(name[0])) {
    return 0;
  }
  for (i = 1U; name[i] != '\0'; ++i) {
    if (!is_identifier_char(name[i])) {
      return 0;
    }
  }
  return 1;
}

static int is_reserved_name(const char *name) {
  static const char *reserved[] = {
      "def", "return", "print", "graph", "hypergraph", "bfs", "bfs_level", "incident_count", "incident_sum", "true", "false"};
  size_t i;
  for (i = 0U; i < sizeof(reserved) / sizeof(reserved[0]); ++i) {
    if (strcmp(name, reserved[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static graphion_runtime_binding *find_binding_mut(graphion_runtime_scope *scope, const char *name) {
  size_t i;
  if (scope == NULL || name == NULL) {
    return NULL;
  }
  for (i = 0U; i < scope->count; ++i) {
    if (strcmp(scope->bindings[i].name, name) == 0) {
      return &scope->bindings[i];
    }
  }
  return NULL;
}

static const graphion_runtime_binding *find_binding(const graphion_runtime_scope *scope, const char *name) {
  size_t i;
  if (scope == NULL || name == NULL) {
    return NULL;
  }
  for (i = 0U; i < scope->count; ++i) {
    if (strcmp(scope->bindings[i].name, name) == 0) {
      return &scope->bindings[i];
    }
  }
  return NULL;
}

static void graphion_runtime_value_dispose(graphion_runtime_value *value) {
  if (value == NULL) {
    return;
  }
  if (value->owns_graph_value != 0 && value->graph_value != NULL) {
    free(value->graph_value);
  }
  if (value->owns_hypergraph_value != 0 && value->hypergraph_value != NULL) {
    free(value->hypergraph_value);
  }
  memset(value, 0, sizeof(*value));
}

static void copy_runtime_value(graphion_runtime_value *dst,
                               const graphion_runtime_value *src,
                               int take_ownership) {
  if (dst == NULL || src == NULL) {
    return;
  }
  dst->kind = GRAPHION_VALUE_NONE;
  dst->int_value = 0;
  dst->float_value = 0.0;
  dst->bool_value = 0;
  dst->graph_value = NULL;
  dst->hypergraph_value = NULL;
  dst->owns_graph_value = 0;
  dst->owns_hypergraph_value = 0;
  dst->kind = src->kind;
  switch (src->kind) {
    case GRAPHION_VALUE_INT:
      dst->int_value = src->int_value;
      break;
    case GRAPHION_VALUE_FLOAT:
      dst->float_value = src->float_value;
      break;
    case GRAPHION_VALUE_BOOL:
      dst->bool_value = src->bool_value;
      break;
    case GRAPHION_VALUE_STRING:
      memcpy(dst->string_value, src->string_value, strlen(src->string_value) + 1U);
      break;
    case GRAPHION_VALUE_GRAPH:
      dst->graph_value = src->graph_value;
      dst->owns_graph_value = take_ownership != 0 ? src->owns_graph_value : 0;
      break;
    case GRAPHION_VALUE_HYPERGRAPH:
      dst->hypergraph_value = src->hypergraph_value;
      dst->owns_hypergraph_value = take_ownership != 0 ? src->owns_hypergraph_value : 0;
      break;
    case GRAPHION_VALUE_GRAPH_NODE:
      dst->graph_node_value = src->graph_node_value;
      break;
    case GRAPHION_VALUE_GRAPH_EDGE:
      dst->graph_edge_value = src->graph_edge_value;
      break;
    case GRAPHION_VALUE_HYPERGRAPH_NODE:
      dst->hypergraph_node_value = src->hypergraph_node_value;
      break;
    case GRAPHION_VALUE_HYPEREDGE:
      dst->hyperedge_value = src->hyperedge_value;
      break;
    case GRAPHION_VALUE_INT_SEQUENCE:
      dst->int_sequence_value.count = src->int_sequence_value.count;
      if (src->int_sequence_value.count != 0U) {
        memcpy(dst->int_sequence_value.items,
               src->int_sequence_value.items,
               src->int_sequence_value.count * sizeof(src->int_sequence_value.items[0]));
      }
      break;
    default:
      break;
  }
}

static int runtime_value_is_vm_global_compatible(const graphion_runtime_value *value) {
  if (value == NULL) {
    return 0;
  }
  switch (value->kind) {
    case GRAPHION_VALUE_NONE:
    case GRAPHION_VALUE_INT:
    case GRAPHION_VALUE_FLOAT:
    case GRAPHION_VALUE_BOOL:
    case GRAPHION_VALUE_STRING:
    case GRAPHION_VALUE_GRAPH:
    case GRAPHION_VALUE_HYPERGRAPH:
    case GRAPHION_VALUE_INT_SEQUENCE:
      return 1;
    default:
      return 0;
  }
}

static void runtime_value_to_vm_value(const graphion_runtime_value *value,
                                      const graphion_runtime_binding *binding,
                                      graphion_vm_value *vm_value) {
  if (value == NULL || vm_value == NULL) {
    return;
  }
  memset(vm_value, 0, sizeof(*vm_value));
  switch (value->kind) {
    case GRAPHION_VALUE_NONE:
      vm_value->kind = GVM_VALUE_NONE;
      break;
    case GRAPHION_VALUE_INT:
      vm_value->kind = GVM_VALUE_INT;
      vm_value->as.int_value = value->int_value;
      break;
    case GRAPHION_VALUE_FLOAT:
      vm_value->kind = GVM_VALUE_FLOAT;
      vm_value->as.float_value = value->float_value;
      break;
    case GRAPHION_VALUE_BOOL:
      vm_value->kind = GVM_VALUE_BOOL;
      vm_value->as.bool_value = value->bool_value;
      break;
    case GRAPHION_VALUE_STRING:
      vm_value->kind = GVM_VALUE_STRING;
      vm_value->as.string_value = binding != NULL ? binding->value.string_value : value->string_value;
      break;
    case GRAPHION_VALUE_GRAPH:
      vm_value->kind = GVM_VALUE_GRAPH_REF;
      vm_value->as.ref_value = value->graph_value;
      break;
    case GRAPHION_VALUE_HYPERGRAPH:
      vm_value->kind = GVM_VALUE_HYPERGRAPH_REF;
      vm_value->as.ref_value = value->hypergraph_value;
      break;
    case GRAPHION_VALUE_INT_SEQUENCE:
      vm_value->kind = GVM_VALUE_INT_SEQUENCE_REF;
      vm_value->as.ref_value = binding != NULL ? (const void *)&binding->value.int_sequence_value
                                               : (const void *)&value->int_sequence_value;
      break;
    default:
      vm_value->kind = GVM_VALUE_NONE;
      break;
  }
}

static int vm_value_to_runtime_value(const graphion_vm_value *vm_value, graphion_runtime_value *value) {
  if (vm_value == NULL || value == NULL) {
    return 0;
  }
  value->kind = GRAPHION_VALUE_NONE;
  value->int_value = 0;
  value->float_value = 0.0;
  value->bool_value = 0;
  value->graph_value = NULL;
  value->hypergraph_value = NULL;
  value->owns_graph_value = 0;
  value->owns_hypergraph_value = 0;
  switch (vm_value->kind) {
    case GVM_VALUE_NONE:
      value->kind = GRAPHION_VALUE_NONE;
      return 1;
    case GVM_VALUE_INT:
      value->kind = GRAPHION_VALUE_INT;
      value->int_value = vm_value->as.int_value;
      return 1;
    case GVM_VALUE_FLOAT:
      value->kind = GRAPHION_VALUE_FLOAT;
      value->float_value = vm_value->as.float_value;
      return 1;
    case GVM_VALUE_BOOL:
      value->kind = GRAPHION_VALUE_BOOL;
      value->bool_value = vm_value->as.bool_value;
      return 1;
    case GVM_VALUE_STRING:
      if (vm_value->as.string_value == NULL || strlen(vm_value->as.string_value) >= GRAPHION_RUNTIME_STRING_MAX) {
        return 0;
      }
      value->kind = GRAPHION_VALUE_STRING;
      memcpy(value->string_value, vm_value->as.string_value, strlen(vm_value->as.string_value) + 1U);
      return 1;
    case GVM_VALUE_GRAPH_REF:
      value->kind = GRAPHION_VALUE_GRAPH;
      value->graph_value = (graphion_runtime_graph_value *)vm_value->as.ref_value;
      return 1;
    case GVM_VALUE_HYPERGRAPH_REF:
      value->kind = GRAPHION_VALUE_HYPERGRAPH;
      value->hypergraph_value = (graphion_runtime_hypergraph_value *)vm_value->as.ref_value;
      return 1;
    case GVM_VALUE_INT_SEQUENCE_REF:
      if (vm_value->as.ref_value == NULL) {
        return 0;
      }
      value->kind = GRAPHION_VALUE_INT_SEQUENCE;
      value->int_sequence_value = *(const graphion_runtime_int_sequence_value *)vm_value->as.ref_value;
      return 1;
    default:
      return 0;
  }
}

static int materialize_binding_value(graphion_runtime_scope *scope, graphion_runtime_binding *binding) {
  if (scope == NULL || binding == NULL) {
    return 0;
  }
  if (binding->is_vm_global == 0) {
    return 1;
  }
  if (binding->value_materialized != 0) {
    return 1;
  }
  if (!vm_value_to_runtime_value(&scope->vm_globals[binding->vm_global_index], &binding->value)) {
    return 0;
  }
  binding->value_materialized = 1;
  return 1;
}

static int scope_load_value(const graphion_runtime_scope *scope, const char *name, graphion_runtime_value *value) {
  graphion_runtime_binding *binding = (graphion_runtime_binding *)find_binding(scope, name);
  if (binding == NULL || value == NULL) {
    return 0;
  }
  if (!materialize_binding_value((graphion_runtime_scope *)scope, binding)) {
    return 0;
  }
  copy_runtime_value(value, &binding->value, 0);
  return 1;
}

void graphion_runtime_scope_init(graphion_runtime_scope *scope) {
  if (scope == NULL) {
    return;
  }
  memset(scope, 0, sizeof(*scope));
}

void graphion_runtime_scope_dispose(graphion_runtime_scope *scope) {
  size_t i;
  if (scope == NULL) {
    return;
  }
  for (i = 0U; i < scope->count; ++i) {
    graphion_runtime_value_dispose(&scope->bindings[i].value);
  }
  memset(scope, 0, sizeof(*scope));
}

const graphion_runtime_value *graphion_runtime_scope_find(const graphion_runtime_scope *scope,
                                                          const char *name) {
  graphion_runtime_binding *binding = (graphion_runtime_binding *)find_binding(scope, name);
  if (binding == NULL) {
    return NULL;
  }
  if (!materialize_binding_value((graphion_runtime_scope *)scope, binding)) {
    return NULL;
  }
  return &binding->value;
}

static int load_value_from_scopes(const graphion_runtime_scope *local_scope,
                                  const graphion_runtime_scope *global_scope,
                                  const char *name,
                                  graphion_runtime_value *value) {
  if (scope_load_value(local_scope, name, value)) {
    return 1;
  }
  return scope_load_value(global_scope, name, value);
}

static int assign_value(graphion_runtime_scope *scope, const char *name, const graphion_runtime_value *value) {
  graphion_runtime_binding *binding = find_binding_mut(scope, name);
  int take_ownership;
  if (scope == NULL || name == NULL || value == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  take_ownership = value->owns_graph_value != 0 || value->owns_hypergraph_value != 0;
  if (binding != NULL) {
    graphion_runtime_value_dispose(&binding->value);
    copy_runtime_value(&binding->value, value, take_ownership);
    if (scope->vm_globals_enabled != 0 && runtime_value_is_vm_global_compatible(value)) {
      binding->is_vm_global = 1;
      binding->value_materialized = 1;
      runtime_value_to_vm_value(&binding->value, binding, &scope->vm_globals[binding->vm_global_index]);
    } else {
      binding->is_vm_global = 0;
      binding->value_materialized = 1;
      memset(&scope->vm_globals[binding->vm_global_index], 0, sizeof(scope->vm_globals[binding->vm_global_index]));
    }
    return GINT_OK;
  }
  if (scope->count >= GRAPHION_RUNTIME_BINDING_MAX) {
    return GINT_ERR_CAPACITY;
  }
  binding = &scope->bindings[scope->count];
  memset(binding, 0, sizeof(*binding));
  memcpy(binding->name, name, strlen(name) + 1U);
  binding->vm_global_index = scope->count;
  copy_runtime_value(&binding->value, value, take_ownership);
  if (scope->vm_globals_enabled != 0 && runtime_value_is_vm_global_compatible(value)) {
    binding->is_vm_global = 1;
    binding->value_materialized = 1;
    runtime_value_to_vm_value(&binding->value, binding, &scope->vm_globals[binding->vm_global_index]);
  } else {
    binding->value_materialized = 1;
  }
  scope->count += 1U;
  return GINT_OK;
}

static int parse_string_literal(const char *token, graphion_runtime_value *value) {
  size_t len;
  if (token == NULL || value == NULL) {
    return 0;
  }
  len = strlen(token);
  if (len < 2U || token[0] != '"' || token[len - 1U] != '"') {
    return 0;
  }
  if (len - 1U >= GRAPHION_RUNTIME_STRING_MAX) {
    return 0;
  }
  memset(value, 0, sizeof(*value));
  value->kind = GRAPHION_VALUE_STRING;
  memcpy(value->string_value, token + 1U, len - 2U);
  value->string_value[len - 2U] = '\0';
  return 1;
}

static int parse_bool_literal(const char *token, graphion_runtime_value *value) {
  if (token == NULL || value == NULL) {
    return 0;
  }
  memset(value, 0, sizeof(*value));
  if (strcmp(token, "true") == 0) {
    value->kind = GRAPHION_VALUE_BOOL;
    value->bool_value = 1;
    return 1;
  }
  if (strcmp(token, "false") == 0) {
    value->kind = GRAPHION_VALUE_BOOL;
    value->bool_value = 0;
    return 1;
  }
  return 0;
}

static int parse_int_literal(const char *token, graphion_runtime_value *value) {
  long long parsed;
  char *end = NULL;
  if (token == NULL || value == NULL) {
    return 0;
  }
  parsed = strtoll(token, &end, 10);
  if (end == NULL || *end != '\0') {
    return 0;
  }
  memset(value, 0, sizeof(*value));
  value->kind = GRAPHION_VALUE_INT;
  value->int_value = (int64_t)parsed;
  return 1;
}

static int token_looks_float(const char *token) {
  size_t i;
  for (i = 0U; token[i] != '\0'; ++i) {
    if (token[i] == '.' || token[i] == 'e' || token[i] == 'E') {
      return 1;
    }
  }
  return 0;
}

static int parse_float_literal(const char *token, graphion_runtime_value *value) {
  double parsed;
  char *end = NULL;
  if (token == NULL || value == NULL || !token_looks_float(token)) {
    return 0;
  }
  parsed = strtod(token, &end);
  if (end == NULL || *end != '\0') {
    return 0;
  }
  memset(value, 0, sizeof(*value));
  value->kind = GRAPHION_VALUE_FLOAT;
  value->float_value = parsed;
  return 1;
}

static int parse_source_lines(const char *source,
                              graphion_runtime_program *program,
                              graphion_runtime_diagnostic *diagnostic) {
  const char *cursor;
  size_t line_no = 1U;
  if (source == NULL || program == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  memset(program, 0, sizeof(*program));
  cursor = source;
  while (*cursor != '\0') {
    char raw[GINT_LINE_MAX];
    size_t raw_len = 0U;
    size_t indent = 0U;
    graphion_runtime_source_line *line;

    while (*cursor != '\0' && *cursor != '\n' && raw_len < (GINT_LINE_MAX - 1U)) {
      raw[raw_len++] = *cursor++;
    }
    if (*cursor == '\n') {
      ++cursor;
    }
    raw[raw_len] = '\0';
    strip_comments(raw);
    while (raw[indent] == ' ' || raw[indent] == '\t') {
      ++indent;
    }
    trim_in_place(raw);
    if (raw[0] == '\0') {
      ++line_no;
      continue;
    }
    if (program->line_count >= GINT_SOURCE_LINE_MAX) {
      set_diagnostic(diagnostic, line_no, 1U, "source contains too many statements");
      return GINT_ERR_CAPACITY;
    }
    line = &program->lines[program->line_count];
    memset(line, 0, sizeof(*line));
    line->line_no = line_no;
    line->indent = indent;
    memcpy(line->text, raw, strlen(raw) + 1U);
    program->line_count += 1U;
    ++line_no;
  }
  return GINT_OK;
}

static int parse_def_header(const char *text,
                            graphion_runtime_function *function,
                            graphion_runtime_diagnostic *diagnostic,
                            size_t line_no) {
  const char *cursor;
  const char *open_paren;
  const char *close_paren;
  size_t name_len;
  char params_buf[GINT_LINE_MAX];
  if (strncmp(text, "def ", 4U) != 0) {
    return 0;
  }
  cursor = text + 4U;
  open_paren = strchr(cursor, '(');
  close_paren = strrchr(cursor, ')');
  if (open_paren == NULL || close_paren == NULL || close_paren < open_paren ||
      close_paren[1] != ':') {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function definition");
    return GINT_ERR_PARSE;
  }
  name_len = (size_t)(open_paren - cursor);
  if (name_len == 0U || name_len >= GRAPHION_RUNTIME_NAME_MAX) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function name");
    return GINT_ERR_PARSE;
  }
  memset(function, 0, sizeof(*function));
  memcpy(function->name, cursor, name_len);
  function->name[name_len] = '\0';
  trim_in_place(function->name);
  if (!is_valid_identifier(function->name)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function name");
    return GINT_ERR_PARSE;
  }
  if (is_reserved_name(function->name)) {
    set_diagnostic(diagnostic, line_no, 1U, "reserved name cannot be used as a function");
    return GINT_ERR_RESERVED_NAME;
  }
  if ((size_t)(close_paren - open_paren - 1) >= sizeof(params_buf)) {
    set_diagnostic(diagnostic, line_no, 1U, "function parameter list too long");
    return GINT_ERR_PARSE;
  }
  memcpy(params_buf, open_paren + 1, (size_t)(close_paren - open_paren - 1));
  params_buf[close_paren - open_paren - 1] = '\0';
  trim_in_place(params_buf);
  if (params_buf[0] != '\0') {
    size_t start = 0U;
    size_t i = 0U;
    while (1) {
      const char current = params_buf[i];
      if (current == ',' || current == '\0') {
        char token[GRAPHION_RUNTIME_NAME_MAX];
        size_t len = i - start;
        if (len == 0U || len >= sizeof(token)) {
          set_diagnostic(diagnostic, line_no, 1U, "invalid function parameter");
          return GINT_ERR_PARSE;
        }
        memcpy(token, params_buf + start, len);
        token[len] = '\0';
        trim_in_place(token);
        if (!is_valid_identifier(token)) {
          set_diagnostic(diagnostic, line_no, 1U, "invalid function parameter");
          return GINT_ERR_PARSE;
        }
        if (function->param_count >= GINT_PARAM_MAX) {
          set_diagnostic(diagnostic, line_no, 1U, "too many function parameters");
          return GINT_ERR_CAPACITY;
        }
        memcpy(function->params[function->param_count], token, strlen(token) + 1U);
        function->param_count += 1U;
        if (current == '\0') {
          break;
        }
        start = i + 1U;
      }
      ++i;
    }
  }
  return GINT_OK;
}

static const graphion_runtime_function *find_function(const graphion_runtime_program *program, const char *name) {
  size_t i;
  if (program == NULL || name == NULL) {
    return NULL;
  }
  for (i = 0U; i < program->function_count; ++i) {
    if (strcmp(program->functions[i].name, name) == 0) {
      return &program->functions[i];
    }
  }
  return NULL;
}

static int index_functions(graphion_runtime_program *program, graphion_runtime_diagnostic *diagnostic) {
  size_t i = 0U;
  while (i < program->line_count) {
    const graphion_runtime_source_line *line = &program->lines[i];
    graphion_runtime_function function;
    int rc;
    if (strncmp(line->text, "def ", 4U) != 0) {
      if (strncmp(line->text, "graph ", 6U) == 0) {
        if (line->indent != 0U) {
          set_diagnostic(diagnostic, line->line_no, 1U, "nested graph declarations are not supported");
          return GINT_ERR_PARSE;
        }
        ++i;
        while (i < program->line_count && program->lines[i].indent > line->indent) {
          ++i;
        }
        continue;
      }
      if (strncmp(line->text, "hypergraph ", 11U) == 0) {
        if (line->indent != 0U) {
          set_diagnostic(diagnostic, line->line_no, 1U, "nested hypergraph declarations are not supported");
          return GINT_ERR_PARSE;
        }
        ++i;
        while (i < program->line_count && program->lines[i].indent > line->indent) {
          ++i;
        }
        continue;
      }
      if (line->indent != 0U) {
        set_diagnostic(diagnostic, line->line_no, 1U, "unexpected indentation outside function body");
        return GINT_ERR_PARSE;
      }
      ++i;
      continue;
    }
    if (line->indent != 0U) {
      set_diagnostic(diagnostic, line->line_no, 1U, "nested functions are not supported");
      return GINT_ERR_PARSE;
    }
    if (program->function_count >= GINT_FUNCTION_MAX) {
      set_diagnostic(diagnostic, line->line_no, 1U, "too many functions");
      return GINT_ERR_CAPACITY;
    }
    rc = parse_def_header(line->text, &function, diagnostic, line->line_no);
    if (rc != GINT_OK) {
      return rc;
    }
    if (find_function(program, function.name) != NULL) {
      set_diagnostic(diagnostic, line->line_no, 1U, "duplicate function definition");
      return GINT_ERR_PARSE;
    }
    function.body_start = i + 1U;
    function.body_end = function.body_start;
    while (function.body_end < program->line_count &&
           program->lines[function.body_end].indent > line->indent) {
      function.body_end += 1U;
    }
    if (function.body_end == function.body_start) {
      set_diagnostic(diagnostic, line->line_no, 1U, "function body cannot be empty");
      return GINT_ERR_PARSE;
    }
    program->functions[program->function_count] = function;
    program->function_count += 1U;
    i = function.body_end;
  }
  return GINT_OK;
}

static int split_assignment(char *line, char **lhs, char **rhs) {
  size_t i = 0U;
  int in_string = 0;
  int depth = 0;
  if (line == NULL || lhs == NULL || rhs == NULL) {
    return 0;
  }
  while (line[i] != '\0') {
    if (line[i] == '"' && (i == 0U || line[i - 1U] != '\\')) {
      in_string = !in_string;
    } else if (!in_string && line[i] == '(') {
      depth += 1;
    } else if (!in_string && line[i] == ')' && depth > 0) {
      depth -= 1;
    } else if (!in_string && depth == 0 && line[i] == '=') {
      line[i] = '\0';
      *lhs = line;
      *rhs = line + i + 1U;
      trim_in_place(*lhs);
      trim_in_place(*rhs);
      return (*lhs)[0] != '\0' && (*rhs)[0] != '\0';
    }
    ++i;
  }
  return 0;
}

static int program_global_slot_index(const graphion_runtime_program *program, const char *name, size_t *slot_out) {
  size_t i;
  if (program == NULL || name == NULL || slot_out == NULL) {
    return 0;
  }
  for (i = 0U; i < program->global_count; ++i) {
    if (strcmp(program->global_names[i], name) == 0) {
      *slot_out = i;
      return 1;
    }
  }
  return 0;
}

static int ensure_program_global_slot(graphion_runtime_program *program, const char *name, size_t *slot_out) {
  if (program == NULL || name == NULL || slot_out == NULL) {
    return 0;
  }
  if (program_global_slot_index(program, name, slot_out)) {
    return 1;
  }
  if (program->global_count >= GRAPHION_RUNTIME_BINDING_MAX) {
    return 0;
  }
  memcpy(program->global_names[program->global_count], name, strlen(name) + 1U);
  *slot_out = program->global_count;
  program->global_count += 1U;
  return 1;
}

static int prepare_simple_top_level_steps(graphion_runtime_program *program,
                                          graphion_runtime_diagnostic *diagnostic) {
  size_t i;
  if (program == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  program->prepared_step_count = 0U;
  program->prepared_top_level_only = 0;
  program->global_count = 0U;
  program->prepared_const_count = 0U;
  program->prepared_vm_program_len = 0U;
  program->prepared_vm_enabled = 0;

  for (i = 0U; i < program->line_count; ++i) {
    const graphion_runtime_source_line *line = &program->lines[i];
    char buffer[GINT_LINE_MAX];
    char *lhs = NULL;
    char *rhs = NULL;
    graphion_runtime_prepared_step *step;
    graphion_runtime_value literal;

    if (line->indent != 0U) {
      return GINT_OK;
    }
    if (strncmp(line->text, "def ", 4U) == 0 || strncmp(line->text, "graph ", 6U) == 0 ||
        strncmp(line->text, "hypergraph ", 11U) == 0 || strncmp(line->text, "print(", 6U) == 0 ||
        strncmp(line->text, "return", 6U) == 0) {
      return GINT_OK;
    }
    memcpy(buffer, line->text, strlen(line->text) + 1U);
    if (!split_assignment(buffer, &lhs, &rhs)) {
      return GINT_OK;
    }
    if (!is_valid_identifier(lhs) || is_reserved_name(lhs)) {
      return GINT_OK;
    }
    if (program->prepared_step_count >= GRAPHION_RUNTIME_PREPARED_STEP_MAX) {
      set_diagnostic(diagnostic, line->line_no, 1U, "prepared step capacity exceeded");
      return GINT_ERR_CAPACITY;
    }
    step = &program->prepared_steps[program->prepared_step_count];
    memset(step, 0, sizeof(*step));
    if (!ensure_program_global_slot(program, lhs, &step->target_slot)) {
      set_diagnostic(diagnostic, line->line_no, 1U, "prepared global slot capacity exceeded");
      return GINT_ERR_CAPACITY;
    }
    memset(&literal, 0, sizeof(literal));
    if (parse_string_literal(rhs, &literal) || parse_bool_literal(rhs, &literal) || parse_float_literal(rhs, &literal) ||
        parse_int_literal(rhs, &literal)) {
      step->kind = GRAPHION_RUNTIME_STEP_STORE_LITERAL;
      step->literal.kind = literal.kind;
      step->literal.int_value = literal.int_value;
      step->literal.float_value = literal.float_value;
      step->literal.bool_value = literal.bool_value;
      if (literal.kind == GRAPHION_VALUE_STRING) {
        memcpy(step->literal.string_value, literal.string_value, strlen(literal.string_value) + 1U);
      }
    } else if (is_valid_identifier(rhs)) {
      if (!ensure_program_global_slot(program, rhs, &step->source_slot)) {
        set_diagnostic(diagnostic, line->line_no, 1U, "prepared global slot capacity exceeded");
        return GINT_ERR_CAPACITY;
      }
      step->kind = GRAPHION_RUNTIME_STEP_STORE_COPY;
    } else {
      return GINT_OK;
    }
    program->prepared_step_count += 1U;
  }

  program->prepared_top_level_only = program->prepared_step_count == program->line_count && program->line_count != 0U;
  return GINT_OK;
}

static int compile_prepared_top_level_vm_program(graphion_runtime_program *program,
                                                 graphion_runtime_diagnostic *diagnostic) {
  size_t step_index;
  size_t insn_index = 0U;

  if (program == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  program->prepared_const_count = 0U;
  program->prepared_vm_program_len = 0U;
  program->prepared_vm_enabled = 0;
  if (program->prepared_top_level_only == 0) {
    return GINT_OK;
  }

  for (step_index = 0U; step_index < program->prepared_step_count; ++step_index) {
    const graphion_runtime_prepared_step *step = &program->prepared_steps[step_index];
    if (insn_index + 2U >= GRAPHION_RUNTIME_PREPARED_VM_INSN_MAX) {
      set_diagnostic(diagnostic, program->lines[step_index].line_no, 1U, "prepared vm program too large");
      return GINT_ERR_CAPACITY;
    }
    if (step->kind == GRAPHION_RUNTIME_STEP_STORE_LITERAL) {
      graphion_vm_value *const_value;
      if (program->prepared_const_count >= GRAPHION_RUNTIME_PREPARED_CONST_MAX) {
        set_diagnostic(diagnostic, program->lines[step_index].line_no, 1U, "prepared constant pool too large");
        return GINT_ERR_CAPACITY;
      }
      const_value = &program->prepared_const_pool[program->prepared_const_count];
      memset(const_value, 0, sizeof(*const_value));
      const_value->kind = (uint8_t)step->literal.kind;
      switch (step->literal.kind) {
        case GRAPHION_VALUE_INT:
          const_value->kind = GVM_VALUE_INT;
          const_value->as.int_value = step->literal.int_value;
          break;
        case GRAPHION_VALUE_FLOAT:
          const_value->kind = GVM_VALUE_FLOAT;
          const_value->as.float_value = step->literal.float_value;
          break;
        case GRAPHION_VALUE_BOOL:
          const_value->kind = GVM_VALUE_BOOL;
          const_value->as.bool_value = step->literal.bool_value;
          break;
        case GRAPHION_VALUE_STRING:
          const_value->kind = GVM_VALUE_STRING;
          const_value->as.string_value = step->literal.string_value;
          break;
        default:
          return GINT_OK;
      }
      program->prepared_vm_program[insn_index++] =
          (graphion_insn){GVM_OP_LOAD_CONST, 0U, 0U, (int32_t)program->prepared_const_count};
      program->prepared_const_count += 1U;
      program->prepared_vm_program[insn_index++] =
          (graphion_insn){GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)step->target_slot};
    } else if (step->kind == GRAPHION_RUNTIME_STEP_STORE_COPY) {
      program->prepared_vm_program[insn_index++] =
          (graphion_insn){GVM_OP_LOAD_GLOBAL, 0U, 0U, (int32_t)step->source_slot};
      program->prepared_vm_program[insn_index++] =
          (graphion_insn){GVM_OP_STORE_GLOBAL, 0U, 0U, (int32_t)step->target_slot};
    } else {
      return GINT_OK;
    }
  }
  if (insn_index >= GRAPHION_RUNTIME_PREPARED_VM_INSN_MAX) {
    set_diagnostic(diagnostic, 1U, 1U, "prepared vm program too large");
    return GINT_ERR_CAPACITY;
  }
  program->prepared_vm_program[insn_index++] = (graphion_insn){GVM_OP_HALT, 0U, 0U, 0};
  program->prepared_vm_program_len = insn_index;
  program->prepared_vm_enabled = 1;
  return GINT_OK;
}

static int execute_prepared_top_level_program(const graphion_runtime_program *program,
                                              graphion_runtime_scope *scope) {
  size_t i;
  if (program == NULL || scope == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  if (scope->count != 0U) {
    if (scope->count != program->global_count) {
      return GINT_ERR_INVALID_ARG;
    }
    for (i = 0U; i < program->global_count; ++i) {
      if (strcmp(scope->bindings[i].name, program->global_names[i]) != 0) {
        return GINT_ERR_INVALID_ARG;
      }
    }
  } else {
    scope->vm_globals_enabled = 1;
    scope->count = program->global_count;
    for (i = 0U; i < program->global_count; ++i) {
      graphion_runtime_binding *binding = &scope->bindings[i];
      memset(binding, 0, sizeof(*binding));
      memcpy(binding->name, program->global_names[i], strlen(program->global_names[i]) + 1U);
      binding->vm_global_index = i;
      binding->is_vm_global = 1;
      binding->value_materialized = 0;
      binding->value.kind = GRAPHION_VALUE_NONE;
      scope->vm_globals[i].kind = GVM_VALUE_NONE;
      scope->vm_globals[i].as.int_value = 0;
    }
  }
  scope->vm_globals_enabled = 1;
  for (i = 0U; i < scope->count; ++i) {
    scope->vm_globals[i].kind = GVM_VALUE_NONE;
    scope->vm_globals[i].as.int_value = 0;
  }

  if (program->prepared_vm_enabled == 0) {
    return GINT_ERR_PARSE;
  }

  if (scope->prepared_vm_ready == 0 || scope->prepared_program_key != (const void *)program) {
    graphion_vm_init(&scope->prepared_vm);
    graphion_vm_bind_constants(&scope->prepared_vm, program->prepared_const_pool, program->prepared_const_count);
    graphion_vm_bind_globals(&scope->prepared_vm, scope->vm_globals, scope->count);
    if (graphion_vm_load(&scope->prepared_vm, program->prepared_vm_program, program->prepared_vm_program_len) != GVM_OK) {
      return GINT_ERR_PARSE;
    }
    scope->prepared_program_key = (const void *)program;
    scope->prepared_vm_ready = 1;
  } else {
    graphion_vm_bind_globals(&scope->prepared_vm, scope->vm_globals, scope->count);
    graphion_vm_reset_execution(&scope->prepared_vm);
  }
  if (graphion_vm_run(&scope->prepared_vm) != GVM_OK) {
    return GINT_ERR_PARSE;
  }

  for (i = 0U; i < scope->count; ++i) {
    graphion_runtime_binding *binding = &scope->bindings[i];
    binding->is_vm_global = 1;
    binding->value_materialized = 0;
  }
  return GINT_OK;
}

static int split_call(const char *expr, char *name_out, char *args_out) {
  const char *open_paren;
  const char *close_paren;
  size_t name_len;
  if (expr == NULL || name_out == NULL || args_out == NULL) {
    return 0;
  }
  open_paren = strchr(expr, '(');
  close_paren = strrchr(expr, ')');
  if (open_paren == NULL || close_paren == NULL || close_paren < open_paren || close_paren[1] != '\0') {
    return 0;
  }
  name_len = (size_t)(open_paren - expr);
  if (name_len == 0U || name_len >= GRAPHION_RUNTIME_NAME_MAX) {
    return 0;
  }
  memcpy(name_out, expr, name_len);
  name_out[name_len] = '\0';
  trim_in_place(name_out);
  if (!is_valid_identifier(name_out)) {
    return 0;
  }
  if ((size_t)(close_paren - open_paren - 1) >= GINT_LINE_MAX) {
    return 0;
  }
  memcpy(args_out, open_paren + 1, (size_t)(close_paren - open_paren - 1));
  args_out[close_paren - open_paren - 1] = '\0';
  trim_in_place(args_out);
  return 1;
}

static int split_graph_member_access(const char *expr,
                                     char *graph_name_out,
                                     char *member_name_out,
                                     char *index_out) {
  const char *dot;
  const char *open_bracket;
  const char *close_bracket;
  size_t graph_name_len;
  size_t member_name_len;
  size_t index_len;

  if (expr == NULL || graph_name_out == NULL || member_name_out == NULL || index_out == NULL) {
    return 0;
  }
  dot = strchr(expr, '.');
  open_bracket = strchr(expr, '[');
  close_bracket = strrchr(expr, ']');
  if (dot == NULL || open_bracket == NULL || close_bracket == NULL || dot > open_bracket || close_bracket[1] != '\0') {
    return 0;
  }
  graph_name_len = (size_t)(dot - expr);
  member_name_len = (size_t)(open_bracket - (dot + 1U));
  index_len = (size_t)(close_bracket - open_bracket - 1U);
  if (graph_name_len == 0U || graph_name_len >= GRAPHION_RUNTIME_NAME_MAX ||
      member_name_len == 0U || member_name_len >= GRAPHION_RUNTIME_NAME_MAX ||
      index_len == 0U || index_len >= GINT_LINE_MAX) {
    return 0;
  }
  memcpy(graph_name_out, expr, graph_name_len);
  graph_name_out[graph_name_len] = '\0';
  trim_in_place(graph_name_out);
  memcpy(member_name_out, dot + 1U, member_name_len);
  member_name_out[member_name_len] = '\0';
  trim_in_place(member_name_out);
  memcpy(index_out, open_bracket + 1U, index_len);
  index_out[index_len] = '\0';
  trim_in_place(index_out);
  return is_valid_identifier(graph_name_out) && is_valid_identifier(member_name_out) && index_out[0] != '\0';
}

static int split_arguments(const char *args_buf, char args[GINT_ARG_MAX][GINT_LINE_MAX], size_t *arg_count) {
  size_t i = 0U;
  size_t start = 0U;
  int in_string = 0;
  int depth = 0;
  if (arg_count == NULL) {
    return 0;
  }
  *arg_count = 0U;
  if (args_buf == NULL || args_buf[0] == '\0') {
    return 1;
  }
  while (1) {
    const char current = args_buf[i];
    if (current == '"' && (i == 0U || args_buf[i - 1U] != '\\')) {
      in_string = !in_string;
    } else if (!in_string && current == '(') {
      depth += 1;
    } else if (!in_string && current == ')' && depth > 0) {
      depth -= 1;
    }
    if (!in_string && depth == 0 && (current == ',' || current == '\0')) {
      size_t len;
      if (*arg_count >= GINT_ARG_MAX) {
        return 0;
      }
      len = i - start;
      if (len >= GINT_LINE_MAX) {
        return 0;
      }
      memcpy(args[*arg_count], args_buf + start, len);
      args[*arg_count][len] = '\0';
      trim_in_place(args[*arg_count]);
      if (args[*arg_count][0] == '\0') {
        return 0;
      }
      *arg_count += 1U;
      if (current == '\0') {
        break;
      }
      start = i + 1U;
    }
    if (current == '\0') {
      break;
    }
    ++i;
  }
  return 1;
}

static size_t graph_neighbor_count(const graphion_runtime_graph_value *graph, int64_t node_id) {
  size_t i;
  size_t count = 0U;
  if (graph == NULL) {
    return 0U;
  }
  for (i = 0U; i < graph->edge_count; ++i) {
    if (graph->edges[i].source == node_id) {
      count += 1U;
    }
  }
  return count;
}

static const graphion_runtime_graph_edge *find_graph_edge_by_id(const graphion_runtime_graph_value *graph, int64_t edge_id) {
  if (graph == NULL || edge_id < 0 || (size_t)edge_id >= graph->edge_count) {
    return NULL;
  }
  return &graph->edges[edge_id];
}

static void print_attribute(FILE *output, const graphion_runtime_attribute *attribute) {
  if (output == NULL || attribute == NULL) {
    return;
  }
  fprintf(output, " %s=", attribute->name);
  switch (attribute->kind) {
    case GRAPHION_ATTRIBUTE_INT:
      fprintf(output, "%lld", (long long)attribute->int_value);
      break;
    case GRAPHION_ATTRIBUTE_FLOAT:
      fprintf(output, "%g", attribute->float_value);
      break;
    case GRAPHION_ATTRIBUTE_BOOL:
      fprintf(output, "%s", attribute->bool_value != 0 ? "true" : "false");
      break;
    case GRAPHION_ATTRIBUTE_STRING:
      fprintf(output, "\"%s\"", attribute->string_value);
      break;
    default:
      fprintf(output, "none");
      break;
  }
}

static int print_value(FILE *output, const graphion_runtime_value *value) {
  size_t i;
  if (output == NULL || value == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  switch (value->kind) {
    case GRAPHION_VALUE_INT:
      fprintf(output, "%lld\n", (long long)value->int_value);
      break;
    case GRAPHION_VALUE_FLOAT:
      fprintf(output, "%g\n", value->float_value);
      break;
    case GRAPHION_VALUE_BOOL:
      fprintf(output, "%s\n", value->bool_value != 0 ? "true" : "false");
      break;
    case GRAPHION_VALUE_STRING:
      fprintf(output, "%s\n", value->string_value);
      break;
    case GRAPHION_VALUE_GRAPH:
      fprintf(output,
              "<graph name=%s nodes=%zu edges=%zu>\n",
              value->graph_value != NULL ? value->graph_value->name : "",
              value->graph_value != NULL ? value->graph_value->node_count : 0U,
              value->graph_value != NULL ? value->graph_value->edge_count : 0U);
      break;
    case GRAPHION_VALUE_HYPERGRAPH:
      fprintf(output,
              "<hypergraph name=%s nodes=%zu hyperedges=%zu>\n",
              value->hypergraph_value != NULL ? value->hypergraph_value->name : "",
              value->hypergraph_value != NULL ? value->hypergraph_value->node_count : 0U,
              value->hypergraph_value != NULL ? value->hypergraph_value->hyperedge_count : 0U);
      break;
    case GRAPHION_VALUE_GRAPH_NODE:
      fprintf(output,
              "<node id=%lld neighbors=%zu>\n",
              (long long)value->graph_node_value.id,
              graph_neighbor_count(value->graph_node_value.graph, value->graph_node_value.id));
      break;
    case GRAPHION_VALUE_GRAPH_EDGE:
      fprintf(output,
              "<edge %lld->%lld",
              (long long)(value->graph_edge_value.edge != NULL ? value->graph_edge_value.edge->source : 0),
              (long long)(value->graph_edge_value.edge != NULL ? value->graph_edge_value.edge->target : 0));
      if (value->graph_edge_value.edge != NULL && value->graph_edge_value.edge->has_weight != 0) {
        fprintf(output, " weight=%g", value->graph_edge_value.edge->weight);
      }
      for (i = 0U;
           value->graph_edge_value.edge != NULL && i < value->graph_edge_value.edge->attribute_count;
           ++i) {
        print_attribute(output, &value->graph_edge_value.edge->attributes[i]);
      }
      fprintf(output, ">\n");
      break;
    case GRAPHION_VALUE_HYPERGRAPH_NODE:
      fprintf(output,
              "<vertex id=%lld hyperedges=%zu>\n",
              (long long)value->hypergraph_node_value.id,
              hypergraph_incident_count(value->hypergraph_node_value.hypergraph, value->hypergraph_node_value.id));
      break;
    case GRAPHION_VALUE_HYPEREDGE:
      fprintf(output,
              "<hyperedge id=%s members=%zu>\n",
              value->hyperedge_value.hyperedge != NULL ? value->hyperedge_value.hyperedge->name : "",
              value->hyperedge_value.hyperedge != NULL ? value->hyperedge_value.hyperedge->node_count : 0U);
      break;
    case GRAPHION_VALUE_INT_SEQUENCE:
      fprintf(output, "[");
      for (i = 0U; i < value->int_sequence_value.count; ++i) {
        if (i != 0U) {
          fprintf(output, ", ");
        }
        fprintf(output, "%lld", (long long)value->int_sequence_value.items[i]);
      }
      fprintf(output, "]\n");
      break;
    default:
      fprintf(output, "none\n");
      break;
  }
  return GINT_OK;
}

static int eval_expression(const char *expr,
                           const graphion_runtime_program *program,
                           graphion_runtime_scope *global_scope,
                           graphion_runtime_scope *local_scope,
                           graphion_runtime_diagnostic *diagnostic,
                           FILE *output,
                           size_t line_no,
                           graphion_runtime_value *value);

static int execute_block(const graphion_runtime_program *program,
                         size_t start,
                         size_t end,
                         graphion_runtime_scope *global_scope,
                         graphion_runtime_scope *local_scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output,
                         int allow_return,
                         int *did_return,
                         graphion_runtime_value *return_value);

static int call_function(const graphion_runtime_program *program,
                         const graphion_runtime_function *function,
                         graphion_runtime_scope *global_scope,
                         graphion_runtime_scope *caller_scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output,
                         size_t line_no,
                         const char *args_expr,
                         graphion_runtime_value *value) {
  char args_buf[GINT_LINE_MAX];
  char args[GINT_ARG_MAX][GINT_LINE_MAX];
  graphion_runtime_scope local_scope;
  graphion_runtime_value arg_values[GINT_ARG_MAX];
  graphion_runtime_value return_value;
  size_t arg_count = 0U;
  size_t i;
  int did_return = 0;
  int rc;

  if (args_expr != NULL) {
    memcpy(args_buf, args_expr, strlen(args_expr) + 1U);
  } else {
    args_buf[0] = '\0';
  }
  if (!split_arguments(args_buf, args, &arg_count)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid function call arguments");
    return GINT_ERR_PARSE;
  }
  if (arg_count != function->param_count) {
    set_diagnostic(diagnostic, line_no, 1U, "function call argument count mismatch");
    return GINT_ERR_CALL;
  }
  graphion_runtime_scope_init(&local_scope);
  for (i = 0U; i < arg_count; ++i) {
    rc = eval_expression(args[i], program, global_scope, caller_scope, diagnostic, output, line_no, &arg_values[i]);
    if (rc != GINT_OK) {
      graphion_runtime_scope_dispose(&local_scope);
      return rc;
    }
    rc = assign_value(&local_scope, function->params[i], &arg_values[i]);
    if (rc != GINT_OK) {
      set_diagnostic(diagnostic, line_no, 1U, "runtime scope capacity exceeded");
      graphion_runtime_scope_dispose(&local_scope);
      return rc;
    }
  }
  memset(&return_value, 0, sizeof(return_value));
  rc = execute_block(program,
                     function->body_start,
                     function->body_end,
                     global_scope,
                     &local_scope,
                     diagnostic,
                     output,
                     1,
                     &did_return,
                     &return_value);
  if (rc != GINT_OK) {
    graphion_runtime_scope_dispose(&local_scope);
    return rc;
  }
  if (did_return) {
    *value = return_value;
  } else {
    memset(value, 0, sizeof(*value));
    value->kind = GRAPHION_VALUE_NONE;
  }
  graphion_runtime_scope_dispose(&local_scope);
  return GINT_OK;
}

static int eval_expression(const char *expr,
                           const graphion_runtime_program *program,
                           graphion_runtime_scope *global_scope,
                           graphion_runtime_scope *local_scope,
                           graphion_runtime_diagnostic *diagnostic,
                           FILE *output,
                           size_t line_no,
                           graphion_runtime_value *value) {
  if (parse_string_literal(expr, value) || parse_bool_literal(expr, value) ||
      parse_float_literal(expr, value) || parse_int_literal(expr, value)) {
    return GINT_OK;
  }
  {
    char graph_name[GRAPHION_RUNTIME_NAME_MAX];
    char member_name[GRAPHION_RUNTIME_NAME_MAX];
    char index_expr[GINT_LINE_MAX];
    if (split_graph_member_access(expr, graph_name, member_name, index_expr)) {
      graphion_runtime_value container_value;
      graphion_runtime_value index_value;
      int rc;
      if (!load_value_from_scopes(local_scope, global_scope, graph_name, &container_value) ||
          (container_value.kind != GRAPHION_VALUE_GRAPH && container_value.kind != GRAPHION_VALUE_HYPERGRAPH)) {
        set_diagnostic(diagnostic, line_no, 1U, "member access expects a graph or hypergraph value");
        return GINT_ERR_CALL;
      }
      if (container_value.kind == GRAPHION_VALUE_GRAPH) {
        rc = eval_expression(index_expr, program, global_scope, local_scope, diagnostic, output, line_no, &index_value);
        if (rc != GINT_OK) {
          return rc;
        }
        if (strcmp(member_name, "node") == 0) {
          if (index_value.kind != GRAPHION_VALUE_INT) {
            set_diagnostic(diagnostic, line_no, 1U, "graph node index must be an integer");
            return GINT_ERR_CALL;
          }
          if (!graph_contains_node(&container_value, index_value.int_value)) {
            set_diagnostic(diagnostic, line_no, 1U, "graph node not found");
            return GINT_ERR_CALL;
          }
          memset(value, 0, sizeof(*value));
          value->kind = GRAPHION_VALUE_GRAPH_NODE;
          value->graph_node_value.id = index_value.int_value;
          value->graph_node_value.graph = container_value.graph_value;
          return GINT_OK;
        }
        if (strcmp(member_name, "edge") == 0) {
          const graphion_runtime_graph_edge *edge_value;
          if (index_value.kind != GRAPHION_VALUE_INT) {
            set_diagnostic(diagnostic, line_no, 1U, "graph edge id must be an integer");
            return GINT_ERR_CALL;
          }
          edge_value = find_graph_edge_by_id(container_value.graph_value, index_value.int_value);
          if (edge_value == NULL) {
            set_diagnostic(diagnostic, line_no, 1U, "graph edge not found");
            return GINT_ERR_CALL;
          }
          memset(value, 0, sizeof(*value));
          value->kind = GRAPHION_VALUE_GRAPH_EDGE;
          value->graph_edge_value.graph = container_value.graph_value;
          value->graph_edge_value.edge = edge_value;
          return GINT_OK;
        }
      } else if (container_value.kind == GRAPHION_VALUE_HYPERGRAPH) {
        if (strcmp(member_name, "vertex") == 0) {
          rc = eval_expression(index_expr, program, global_scope, local_scope, diagnostic, output, line_no, &index_value);
          if (rc != GINT_OK) {
            return rc;
          }
          if (index_value.kind != GRAPHION_VALUE_INT) {
            set_diagnostic(diagnostic, line_no, 1U, "hypergraph vertex index must be an integer");
            return GINT_ERR_CALL;
          }
          if (!hypergraph_contains_node(&container_value, index_value.int_value)) {
            set_diagnostic(diagnostic, line_no, 1U, "hypergraph vertex not found");
            return GINT_ERR_CALL;
          }
          memset(value, 0, sizeof(*value));
          value->kind = GRAPHION_VALUE_HYPERGRAPH_NODE;
          value->hypergraph_node_value.id = index_value.int_value;
          value->hypergraph_node_value.hypergraph = container_value.hypergraph_value;
          return GINT_OK;
        }
        if (strcmp(member_name, "hyperedge") == 0) {
          const graphion_runtime_hyperedge *hyperedge_value;
          rc = eval_expression(index_expr, program, global_scope, local_scope, diagnostic, output, line_no, &index_value);
          if (rc != GINT_OK) {
            return rc;
          }
          if (index_value.kind != GRAPHION_VALUE_INT) {
            set_diagnostic(diagnostic, line_no, 1U, "hyperedge id must be an integer");
            return GINT_ERR_CALL;
          }
          hyperedge_value = find_hyperedge_by_id(container_value.hypergraph_value, index_value.int_value);
          if (hyperedge_value == NULL) {
            set_diagnostic(diagnostic, line_no, 1U, "hyperedge not found");
            return GINT_ERR_CALL;
          }
          memset(value, 0, sizeof(*value));
          value->kind = GRAPHION_VALUE_HYPEREDGE;
          value->hyperedge_value.hypergraph = container_value.hypergraph_value;
          value->hyperedge_value.hyperedge = hyperedge_value;
          return GINT_OK;
        }
      }
      set_diagnostic(diagnostic, line_no, 1U, "unknown graph or hypergraph member access");
      return GINT_ERR_CALL;
    }
  }
  {
    char name[GRAPHION_RUNTIME_NAME_MAX];
    char args[GINT_LINE_MAX];
    if (split_call(expr, name, args)) {
      const graphion_runtime_function *function;
      char split_args[GINT_ARG_MAX][GINT_LINE_MAX];
      size_t arg_count = 0U;
      if (strcmp(name, "print") == 0) {
        set_diagnostic(diagnostic, line_no, 1U, "print cannot be used as an expression");
        return GINT_ERR_CALL;
      }
      if (strcmp(name, "bfs") == 0) {
        graphion_runtime_value graph_value;
        graphion_runtime_value source_value;
        size_t level_count = 0U;
        int rc;
        if (!split_arguments(args, split_args, &arg_count) || arg_count != 2U) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs expects graph and source node");
          return GINT_ERR_CALL;
        }
        rc = eval_expression(split_args[0], program, global_scope, local_scope, diagnostic, output, line_no, &graph_value);
        if (rc != GINT_OK) {
          return rc;
        }
        rc = eval_expression(split_args[1], program, global_scope, local_scope, diagnostic, output, line_no, &source_value);
        if (rc != GINT_OK) {
          return rc;
        }
        if (graph_value.kind != GRAPHION_VALUE_GRAPH || graph_value.graph_value == NULL) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs expects a graph as first argument");
          return GINT_ERR_CALL;
        }
        if (source_value.kind != GRAPHION_VALUE_INT) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs source must be an integer node id");
          return GINT_ERR_CALL;
        }
        if (!graph_bfs_visit_order(graph_value.graph_value, source_value.int_value, &value->int_sequence_value, &level_count)) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs source node not found");
          return GINT_ERR_CALL;
        }
        value->kind = GRAPHION_VALUE_INT_SEQUENCE;
        return GINT_OK;
      }
      if (strcmp(name, "bfs_level") == 0) {
        graphion_runtime_value graph_value;
        graphion_runtime_value source_value;
        graphion_runtime_int_sequence_value sequence;
        size_t level_count = 0U;
        int rc;
        if (!split_arguments(args, split_args, &arg_count) || arg_count != 2U) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs_level expects graph and source node");
          return GINT_ERR_CALL;
        }
        rc = eval_expression(split_args[0], program, global_scope, local_scope, diagnostic, output, line_no, &graph_value);
        if (rc != GINT_OK) {
          return rc;
        }
        rc = eval_expression(split_args[1], program, global_scope, local_scope, diagnostic, output, line_no, &source_value);
        if (rc != GINT_OK) {
          return rc;
        }
        if (graph_value.kind != GRAPHION_VALUE_GRAPH || graph_value.graph_value == NULL) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs_level expects a graph as first argument");
          return GINT_ERR_CALL;
        }
        if (source_value.kind != GRAPHION_VALUE_INT) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs_level source must be an integer node id");
          return GINT_ERR_CALL;
        }
        if (!graph_bfs_visit_order(graph_value.graph_value, source_value.int_value, &sequence, &level_count)) {
          set_diagnostic(diagnostic, line_no, 1U, "bfs_level source node not found");
          return GINT_ERR_CALL;
        }
        memset(value, 0, sizeof(*value));
        value->kind = GRAPHION_VALUE_INT;
        value->int_value = (int64_t)level_count;
        return GINT_OK;
      }
      if (strcmp(name, "incident_count") == 0 || strcmp(name, "incident_sum") == 0) {
        graphion_runtime_value hypergraph_value;
        graphion_runtime_value vertex_value;
        int rc;
        if (!split_arguments(args, split_args, &arg_count) || arg_count != 2U) {
          set_diagnostic(diagnostic, line_no, 1U, "incidence builtins expect hypergraph and vertex id");
          return GINT_ERR_CALL;
        }
        rc = eval_expression(split_args[0], program, global_scope, local_scope, diagnostic, output, line_no, &hypergraph_value);
        if (rc != GINT_OK) {
          return rc;
        }
        rc = eval_expression(split_args[1], program, global_scope, local_scope, diagnostic, output, line_no, &vertex_value);
        if (rc != GINT_OK) {
          return rc;
        }
        if (hypergraph_value.kind != GRAPHION_VALUE_HYPERGRAPH || hypergraph_value.hypergraph_value == NULL) {
          set_diagnostic(diagnostic, line_no, 1U, "incidence builtins expect a hypergraph as first argument");
          return GINT_ERR_CALL;
        }
        if (vertex_value.kind != GRAPHION_VALUE_INT) {
          set_diagnostic(diagnostic, line_no, 1U, "incidence vertex id must be an integer");
          return GINT_ERR_CALL;
        }
        if (!hypergraph_contains_node(&hypergraph_value, vertex_value.int_value)) {
          set_diagnostic(diagnostic, line_no, 1U, "hypergraph vertex not found");
          return GINT_ERR_CALL;
        }
        memset(value, 0, sizeof(*value));
        value->kind = GRAPHION_VALUE_INT;
        if (strcmp(name, "incident_count") == 0) {
          value->int_value = (int64_t)hypergraph_incident_count(hypergraph_value.hypergraph_value, vertex_value.int_value);
        } else {
          value->int_value = (int64_t)hypergraph_incident_sum(hypergraph_value.hypergraph_value, vertex_value.int_value);
        }
        return GINT_OK;
      }
      function = find_function(program, name);
      if (function == NULL) {
        set_diagnostic(diagnostic, line_no, 1U, "unknown function call");
        return GINT_ERR_CALL;
      }
      return call_function(program, function, global_scope, local_scope, diagnostic, output, line_no, args, value);
    }
  }
  if (!is_valid_identifier(expr)) {
    set_diagnostic(diagnostic, line_no, 1U, "invalid expression");
    return GINT_ERR_PARSE;
  }
  if (!load_value_from_scopes(local_scope, global_scope, expr, value)) {
    set_diagnostic(diagnostic, line_no, 1U, "unknown variable in expression");
    return GINT_ERR_UNKNOWN_VARIABLE;
  }
  return GINT_OK;
}

static int execute_statement(const graphion_runtime_program *program,
                             const graphion_runtime_source_line *line,
                             graphion_runtime_scope *global_scope,
                             graphion_runtime_scope *local_scope,
                             graphion_runtime_diagnostic *diagnostic,
                             FILE *output,
                             int allow_return,
                             int *did_return,
                             graphion_runtime_value *return_value) {
  char buffer[GINT_LINE_MAX];
  char *lhs;
  char *rhs;
  graphion_runtime_value value;
  int rc;

  memcpy(buffer, line->text, strlen(line->text) + 1U);
  if (strncmp(buffer, "print(", 6U) == 0 && buffer[strlen(buffer) - 1U] == ')') {
    buffer[strlen(buffer) - 1U] = '\0';
    trim_in_place(buffer + 6U);
    rc = eval_expression(buffer + 6U, program, global_scope, local_scope, diagnostic, output, line->line_no, &value);
    if (rc != GINT_OK) {
      return rc;
    }
    return print_value(output, &value);
  }
  if (strncmp(buffer, "return", 6U) == 0 &&
      (buffer[6] == '\0' || isspace((unsigned char)buffer[6]) != 0)) {
    if (!allow_return) {
      set_diagnostic(diagnostic, line->line_no, 1U, "return is only valid inside a function");
      return GINT_ERR_RETURN;
    }
    if (buffer[6] == '\0') {
      memset(return_value, 0, sizeof(*return_value));
      return_value->kind = GRAPHION_VALUE_NONE;
    } else {
      char *expr = buffer + 6U;
      trim_in_place(expr);
      rc = eval_expression(expr, program, global_scope, local_scope, diagnostic, output, line->line_no, return_value);
      if (rc != GINT_OK) {
        return rc;
      }
    }
    *did_return = 1;
    return GINT_OK;
  }
  if (strncmp(buffer, "def ", 4U) == 0) {
    set_diagnostic(diagnostic, line->line_no, 1U, "nested functions are not supported");
    return GINT_ERR_PARSE;
  }
  if (!split_assignment(buffer, &lhs, &rhs)) {
    set_diagnostic(diagnostic, line->line_no, 1U, "expected assignment, print, or return statement");
    return GINT_ERR_PARSE;
  }
  if (!is_valid_identifier(lhs)) {
    set_diagnostic(diagnostic, line->line_no, 1U, "invalid variable name");
    return GINT_ERR_PARSE;
  }
  if (is_reserved_name(lhs)) {
    set_diagnostic(diagnostic, line->line_no, 1U, "reserved name cannot be assigned");
    return GINT_ERR_RESERVED_NAME;
  }
  memset(&value, 0, sizeof(value));
  rc = eval_expression(rhs, program, global_scope, local_scope, diagnostic, output, line->line_no, &value);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = assign_value(local_scope != NULL ? local_scope : global_scope, lhs, &value);
  if (rc != GINT_OK) {
    set_diagnostic(diagnostic, line->line_no, 1U, "runtime scope capacity exceeded");
    return rc;
  }
  return GINT_OK;
}

static int execute_block(const graphion_runtime_program *program,
                         size_t start,
                         size_t end,
                         graphion_runtime_scope *global_scope,
                         graphion_runtime_scope *local_scope,
                         graphion_runtime_diagnostic *diagnostic,
                         FILE *output,
                         int allow_return,
                         int *did_return,
                         graphion_runtime_value *return_value) {
  size_t i = start;
  if (did_return != NULL) {
    *did_return = 0;
  }
  while (i < end) {
    const graphion_runtime_source_line *line = &program->lines[i];
    int rc;
    if (line->indent == 0U && strncmp(line->text, "graph ", 6U) == 0) {
      graphion_runtime_value graph_value;
      char graph_name[GRAPHION_RUNTIME_NAME_MAX];
      size_t body_end = i + 1U;
      graphion_runtime_graph_value *graph_payload;
      rc = parse_graph_header(line->text, graph_name, diagnostic, line->line_no);
      if (rc != GINT_OK) {
        return rc;
      }
      memset(&graph_value, 0, sizeof(graph_value));
      graph_value.kind = GRAPHION_VALUE_GRAPH;
      graph_payload = (graphion_runtime_graph_value *)calloc(1U, sizeof(*graph_payload));
      if (graph_payload == NULL) {
        set_diagnostic(diagnostic, line->line_no, 1U, "graph allocation failed");
        return GINT_ERR_CAPACITY;
      }
      graph_value.graph_value = graph_payload;
      graph_value.owns_graph_value = 1;
      memcpy(graph_payload->name, graph_name, strlen(graph_name) + 1U);
      while (body_end < end && program->lines[body_end].indent > line->indent) {
        graphion_runtime_graph_edge edge;
        if (graph_payload->edge_count >= GRAPHION_RUNTIME_GRAPH_EDGE_MAX) {
          set_diagnostic(diagnostic, program->lines[body_end].line_no, 1U, "graph edge capacity exceeded");
          graphion_runtime_value_dispose(&graph_value);
          return GINT_ERR_CAPACITY;
        }
        rc = parse_graph_edge(program->lines[body_end].text, &edge, diagnostic, program->lines[body_end].line_no);
        if (rc != GINT_OK) {
          graphion_runtime_value_dispose(&graph_value);
          return rc;
        }
        if (!graph_contains_node(&graph_value, edge.source)) {
          graph_payload->node_count += 1U;
        }
        if (!graph_contains_node(&graph_value, edge.target)) {
          graph_payload->node_count += 1U;
        }
        graph_payload->edges[graph_payload->edge_count] = edge;
        graph_payload->edge_count += 1U;
        body_end += 1U;
      }
      if (body_end == i + 1U) {
        set_diagnostic(diagnostic, line->line_no, 1U, "graph body cannot be empty");
        graphion_runtime_value_dispose(&graph_value);
        return GINT_ERR_PARSE;
      }
      if (!prepare_graph_native(graph_payload)) {
        set_diagnostic(diagnostic, line->line_no, 1U, "graph native preparation failed");
        graphion_runtime_value_dispose(&graph_value);
        return GINT_ERR_PARSE;
      }
      rc = assign_value(global_scope, graph_name, &graph_value);
      if (rc != GINT_OK) {
        set_diagnostic(diagnostic, line->line_no, 1U, "runtime scope capacity exceeded");
        graphion_runtime_value_dispose(&graph_value);
        return rc;
      }
      i = body_end;
      continue;
    }
    if (line->indent == 0U && strncmp(line->text, "hypergraph ", 11U) == 0) {
      graphion_runtime_value hypergraph_value;
      char hypergraph_name[GRAPHION_RUNTIME_NAME_MAX];
      size_t body_end = i + 1U;
      graphion_runtime_hypergraph_value *hypergraph_payload;
      rc = parse_hypergraph_header(line->text, hypergraph_name, diagnostic, line->line_no);
      if (rc != GINT_OK) {
        return rc;
      }
      memset(&hypergraph_value, 0, sizeof(hypergraph_value));
      hypergraph_value.kind = GRAPHION_VALUE_HYPERGRAPH;
      hypergraph_payload = (graphion_runtime_hypergraph_value *)calloc(1U, sizeof(*hypergraph_payload));
      if (hypergraph_payload == NULL) {
        set_diagnostic(diagnostic, line->line_no, 1U, "hypergraph allocation failed");
        return GINT_ERR_CAPACITY;
      }
      hypergraph_value.hypergraph_value = hypergraph_payload;
      hypergraph_value.owns_hypergraph_value = 1;
      memcpy(hypergraph_payload->name, hypergraph_name, strlen(hypergraph_name) + 1U);
      while (body_end < end && program->lines[body_end].indent > line->indent) {
        graphion_runtime_hyperedge hyperedge;
        size_t node_index;
        if (hypergraph_payload->hyperedge_count >= GRAPHION_RUNTIME_HYPEREDGE_MAX) {
          set_diagnostic(diagnostic, program->lines[body_end].line_no, 1U, "hyperedge capacity exceeded");
          graphion_runtime_value_dispose(&hypergraph_value);
          return GINT_ERR_CAPACITY;
        }
        rc = parse_hyperedge_line(program->lines[body_end].text, &hyperedge, diagnostic, program->lines[body_end].line_no);
        if (rc != GINT_OK) {
          graphion_runtime_value_dispose(&hypergraph_value);
          return rc;
        }
        snprintf(hyperedge.name, sizeof(hyperedge.name), "%zu", hypergraph_payload->hyperedge_count);
        for (node_index = 0U; node_index < hyperedge.node_count; ++node_index) {
          if (!hypergraph_contains_node(&hypergraph_value, hyperedge.nodes[node_index])) {
            hypergraph_payload->node_count += 1U;
          }
        }
        hypergraph_payload->hyperedges[hypergraph_payload->hyperedge_count] = hyperedge;
        hypergraph_payload->hyperedge_count += 1U;
        body_end += 1U;
      }
      if (body_end == i + 1U) {
        set_diagnostic(diagnostic, line->line_no, 1U, "hypergraph body cannot be empty");
        graphion_runtime_value_dispose(&hypergraph_value);
        return GINT_ERR_PARSE;
      }
      if (!prepare_hypergraph_native(hypergraph_payload)) {
        set_diagnostic(diagnostic, line->line_no, 1U, "hypergraph native preparation failed");
        graphion_runtime_value_dispose(&hypergraph_value);
        return GINT_ERR_PARSE;
      }
      rc = assign_value(global_scope, hypergraph_name, &hypergraph_value);
      if (rc != GINT_OK) {
        set_diagnostic(diagnostic, line->line_no, 1U, "runtime scope capacity exceeded");
        graphion_runtime_value_dispose(&hypergraph_value);
        return rc;
      }
      i = body_end;
      continue;
    }
    if (line->indent == 0U && strncmp(line->text, "def ", 4U) == 0) {
      const graphion_runtime_function *function = find_function(program, line->text + 4U);
      size_t skip_to = i + 1U;
      size_t j;
      (void)function;
      for (j = 0U; j < program->function_count; ++j) {
        if (program->functions[j].body_start == i + 1U) {
          skip_to = program->functions[j].body_end;
          break;
        }
      }
      i = skip_to;
      continue;
    }
    rc = execute_statement(program, line, global_scope, local_scope, diagnostic, output, allow_return, did_return, return_value);
    if (rc != GINT_OK) {
      return rc;
    }
    if (did_return != NULL && *did_return) {
      return GINT_OK;
    }
    i += 1U;
  }
  return GINT_OK;
}

void graphion_runtime_program_init(graphion_runtime_program *program) {
  if (program == NULL) {
    return;
  }
  memset(program, 0, sizeof(*program));
}

void graphion_runtime_program_dispose(graphion_runtime_program *program) {
  if (program == NULL) {
    return;
  }
  memset(program, 0, sizeof(*program));
}

int graphion_prepare_source(const char *source,
                            graphion_runtime_program *program,
                            graphion_runtime_diagnostic *diagnostic) {
  int rc;
  clear_diagnostic(diagnostic);
  if (source == NULL || program == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  graphion_runtime_program_init(program);
  rc = parse_source_lines(source, program, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(program);
    return rc;
  }
  rc = index_functions(program, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(program);
    return rc;
  }
  rc = prepare_simple_top_level_steps(program, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(program);
    return rc;
  }
  rc = compile_prepared_top_level_vm_program(program, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(program);
    return rc;
  }
  return GINT_OK;
}

int graphion_execute_program(const graphion_runtime_program *program,
                             graphion_runtime_scope *scope,
                             graphion_runtime_diagnostic *diagnostic,
                             FILE *output) {
  graphion_runtime_value return_value;
  int did_return = 0;

  clear_diagnostic(diagnostic);
  if (program == NULL || scope == NULL || output == NULL) {
    return GINT_ERR_INVALID_ARG;
  }
  if (program->prepared_top_level_only != 0 && scope->count == 0U) {
    return execute_prepared_top_level_program(program, scope);
  }
  scope->vm_globals_enabled = 1;
  memset(&return_value, 0, sizeof(return_value));
  return execute_block(program,
                       0U,
                       program->line_count,
                       scope,
                       scope,
                       diagnostic,
                       output,
                       0,
                       &did_return,
                       &return_value);
}

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output) {
  graphion_runtime_program program;
  int rc;

  rc = graphion_prepare_source(source, &program, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = graphion_execute_program(&program, scope, diagnostic, output);
  graphion_runtime_program_dispose(&program);
  return rc;
}

int graphion_interpret_source(const char *source,
                              graphion_runtime_scope *scope,
                              graphion_runtime_diagnostic *diagnostic) {
  return graphion_interpret_source_with_output(source, scope, diagnostic, stdout);
}
