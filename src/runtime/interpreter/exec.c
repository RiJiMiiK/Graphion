/* SPDX-License-Identifier: MIT */

#include "runtime/interpreter/exec_internal.h"

#include "runtime/interpreter/program.h"
#include "vm/internal/core/value.h"

typedef struct {
  uint32_t from;
  uint32_t to;
  int directed;
  int bidirectional;
  int has_attrs;
  graphion_vm_value attrs;
} runtime_graph_edge;

typedef struct {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  uint32_t id;
} runtime_graph_named_node;

typedef struct {
  size_t start;
  size_t vertex_count;
  int has_attrs;
  graphion_vm_value attrs;
} runtime_hyperedge_builder;

typedef struct {
  unsigned char used_ids[GRAPHION_RUNTIME_PROGRAM_MAX];
  unsigned char has_node_attrs[GRAPHION_RUNTIME_PROGRAM_MAX];
  graphion_vm_value node_attrs[GRAPHION_RUNTIME_PROGRAM_MAX];
  graphion_vm_value node_attr_defaults;
  graphion_vm_value edge_attr_defaults;
  runtime_graph_named_node named_nodes[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t named_node_count;
  runtime_graph_edge edges[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t edge_count;
  runtime_hyperedge_builder hyperedges[GRAPHION_RUNTIME_PROGRAM_MAX];
  uint32_t hyperedge_vertices[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t hyperedge_vertex_count;
  size_t hyperedge_count;
  uint32_t max_id;
  int has_nodes;
  int has_directed_edges;
  int has_undirected_edges;
  int has_node_attr_defaults;
  int has_edge_attr_defaults;
} runtime_graph_builder;

typedef struct {
  graphion_struct_field_value fields[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t field_count;
} runtime_struct_builder;

static int graph_header_ends_with_colon(const char *text) {
  const char *cursor = text;
  skip_spaces(&cursor);
  if (strncmp(cursor, "graph", 5U) != 0 || is_ident_char(cursor[5])) {
    return 0;
  }
  while (*cursor != '\0') {
    cursor++;
  }
  while (cursor > text && (cursor[-1] == ' ' || cursor[-1] == '\t' || cursor[-1] == '\r')) {
    cursor--;
  }
  return cursor > text && cursor[-1] == ':';
}

static int hypergraph_header_ends_with_colon(const char *text) {
  const char *cursor = text;
  skip_spaces(&cursor);
  if (strncmp(cursor, "hypergraph", 10U) != 0 || is_ident_char(cursor[10])) {
    return 0;
  }
  while (*cursor != '\0') {
    cursor++;
  }
  while (cursor > text && (cursor[-1] == ' ' || cursor[-1] == '\t' || cursor[-1] == '\r')) {
    cursor--;
  }
  return cursor > text && cursor[-1] == ':';
}

static int struct_header_ends_with_colon(const char *text) {
  const char *cursor = text;
  skip_spaces(&cursor);
  if (strncmp(cursor, "struct", 6U) != 0 || is_ident_char(cursor[6])) {
    return 0;
  }
  while (*cursor != '\0') {
    cursor++;
  }
  while (cursor > text && (cursor[-1] == ' ' || cursor[-1] == '\t' || cursor[-1] == '\r')) {
    cursor--;
  }
  return cursor > text && cursor[-1] == ':';
}

static void runtime_graph_builder_init(runtime_graph_builder *builder) {
  size_t i;
  if (builder == NULL) {
    return;
  }
  memset(builder, 0, sizeof(*builder));
  vm_value_set_none(&builder->node_attr_defaults);
  vm_value_set_none(&builder->edge_attr_defaults);
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    vm_value_set_none(&builder->node_attrs[i]);
    vm_value_set_none(&builder->edges[i].attrs);
  }
}

static void runtime_graph_builder_dispose(runtime_graph_builder *builder) {
  size_t i;
  if (builder == NULL) {
    return;
  }
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (builder->has_node_attrs[i]) {
      vm_value_dispose_owned(&builder->node_attrs[i]);
      builder->has_node_attrs[i] = 0U;
    }
  }
  if (builder->has_node_attr_defaults) {
    vm_value_dispose_owned(&builder->node_attr_defaults);
    builder->has_node_attr_defaults = 0;
  }
  if (builder->has_edge_attr_defaults) {
    vm_value_dispose_owned(&builder->edge_attr_defaults);
    builder->has_edge_attr_defaults = 0;
  }
  for (i = 0U; i < builder->edge_count; ++i) {
    if (builder->edges[i].has_attrs) {
      vm_value_dispose_owned(&builder->edges[i].attrs);
      builder->edges[i].has_attrs = 0;
    }
  }
  for (i = 0U; i < builder->hyperedge_count; ++i) {
    if (builder->hyperedges[i].has_attrs) {
      vm_value_dispose_owned(&builder->hyperedges[i].attrs);
      builder->hyperedges[i].has_attrs = 0;
    }
  }
}

static void runtime_struct_builder_init(runtime_struct_builder *builder) {
  size_t i;
  if (builder == NULL) {
    return;
  }
  memset(builder, 0, sizeof(*builder));
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    vm_value_set_none(&builder->fields[i].default_value);
  }
}

static void runtime_struct_builder_dispose(runtime_struct_builder *builder) {
  size_t i;
  if (builder == NULL) {
    return;
  }
  for (i = 0U; i < builder->field_count; ++i) {
    if (builder->fields[i].has_default) {
      vm_value_dispose_owned(&builder->fields[i].default_value);
      builder->fields[i].has_default = 0U;
    }
  }
}

static int runtime_graph_mark_id(runtime_graph_builder *builder,
                                 uint32_t id,
                                 int fail_if_exists,
                                 unsigned int line,
                                 graphion_runtime_diagnostic *diagnostic) {
  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if ((size_t)id >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "graph node id is too large", GINT_ERR_CAPACITY);
  }
  if (builder->used_ids[id] && fail_if_exists) {
    return fail(diagnostic, line, 1U, "duplicate graph node id", GINT_ERR_PARSE);
  }
  builder->used_ids[id] = 1U;
  if (!builder->has_nodes || id > builder->max_id) {
    builder->max_id = id;
  }
  builder->has_nodes = 1;
  return GINT_OK;
}

static int runtime_graph_next_free_id(runtime_graph_builder *builder,
                                      uint32_t *id_out,
                                      unsigned int line,
                                      graphion_runtime_diagnostic *diagnostic) {
  size_t i;
  if (builder == NULL || id_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (!builder->used_ids[i]) {
      *id_out = (uint32_t)i;
      return runtime_graph_mark_id(builder, (uint32_t)i, 0, line, diagnostic);
    }
  }
  return fail(diagnostic, line, 1U, "too many graph nodes", GINT_ERR_CAPACITY);
}

static int runtime_graph_find_named_id(const runtime_graph_builder *builder, const char *name, uint32_t *id_out) {
  size_t i;
  if (builder == NULL || name == NULL || id_out == NULL) {
    return 0;
  }
  for (i = 0U; i < builder->named_node_count; ++i) {
    if (strcmp(builder->named_nodes[i].name, name) == 0) {
      *id_out = builder->named_nodes[i].id;
      return 1;
    }
  }
  return 0;
}

static int runtime_graph_add_named_node(runtime_graph_builder *builder,
                                        const char *name,
                                        uint32_t *id_out,
                                        unsigned int line,
                                        graphion_runtime_diagnostic *diagnostic) {
  uint32_t id = 0U;
  if (builder == NULL || name == NULL || id_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (runtime_graph_find_named_id(builder, name, id_out)) {
    return GINT_OK;
  }
  if (builder->named_node_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "too many graph nodes", GINT_ERR_CAPACITY);
  }
  if (strlen(name) >= GRAPHION_RUNTIME_NAME_MAX) {
    return fail(diagnostic, line, 1U, "graph node name too long", GINT_ERR_CAPACITY);
  }
  {
    const int rc = runtime_graph_next_free_id(builder, &id, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  copy_name(builder->named_nodes[builder->named_node_count].name, name);
  builder->named_nodes[builder->named_node_count].id = id;
  builder->named_node_count += 1U;
  *id_out = id;
  return GINT_OK;
}

static const char *runtime_graph_name_for_id(const runtime_graph_builder *builder, uint32_t id) {
  size_t i;

  if (builder == NULL) {
    return NULL;
  }
  for (i = 0U; i < builder->named_node_count; ++i) {
    if (builder->named_nodes[i].id == id) {
      return builder->named_nodes[i].name;
    }
  }
  return NULL;
}

static int runtime_graph_id_from_value(runtime_graph_builder *builder,
                                       const graphion_runtime_value *value,
                                       int fail_if_existing_id,
                                       int assign_named_ids,
                                       uint32_t *id_out,
                                       unsigned int line,
                                       graphion_runtime_diagnostic *diagnostic) {
  if (builder == NULL || value == NULL || id_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (value->kind == GVM_VALUE_INT) {
    if (value->as.int_value < 0) {
      return fail(diagnostic, line, 1U, "graph node id must be non-negative", GINT_ERR_PARSE);
    }
    if ((uint64_t)value->as.int_value >= GRAPHION_RUNTIME_PROGRAM_MAX) {
      return fail(diagnostic, line, 1U, "graph node id is too large", GINT_ERR_CAPACITY);
    }
    *id_out = (uint32_t)value->as.int_value;
    return runtime_graph_mark_id(builder, *id_out, fail_if_existing_id, line, diagnostic);
  }
  if (value->kind == GVM_VALUE_STRING) {
    const char *name = value->as.string_value != NULL ? value->as.string_value : "";
    if (!assign_named_ids) {
      *id_out = 0U;
      return GINT_OK;
    }
    return runtime_graph_add_named_node(builder, name, id_out, line, diagnostic);
  }
  return fail(diagnostic, line, 1U, "graph node variable must be int or string", GINT_ERR_PARSE);
}

static int parse_graph_node_ref(const char **cursor,
                                runtime_graph_builder *builder,
                                const graphion_runtime_scope *scope,
                                int fail_if_existing_id,
                                int assign_named_ids,
                                uint32_t *id_out,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  char name[GRAPHION_RUNTIME_NAME_MAX];
  char *end = NULL;
  int64_t id;

  if (cursor == NULL || *cursor == NULL || builder == NULL || id_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(cursor);

  if (**cursor == '"') {
    size_t len = 0U;
    (*cursor)++;
    while ((*cursor)[len] != '\0' && (*cursor)[len] != '"') {
      len++;
    }
    if ((*cursor)[len] != '"') {
      return fail(diagnostic, line, 1U, "unterminated string literal", GINT_ERR_PARSE);
    }
    if (len >= sizeof(name)) {
      return fail(diagnostic, line, 1U, "graph node name too long", GINT_ERR_CAPACITY);
    }
    memcpy(name, *cursor, len);
    name[len] = '\0';
    *cursor += len + 1U;
    if (!assign_named_ids) {
      *id_out = 0U;
      return GINT_OK;
    }
    return runtime_graph_add_named_node(builder, name, id_out, line, diagnostic);
  }

  if (**cursor == '-' || (**cursor >= '0' && **cursor <= '9')) {
    id = strtoll(*cursor, &end, 10);
    if (end == *cursor) {
      return fail(diagnostic, line, 1U, "expected graph node name or id", GINT_ERR_PARSE);
    }
    if (id < 0) {
      return fail(diagnostic, line, 1U, "graph node id must be non-negative", GINT_ERR_PARSE);
    }
    if ((uint64_t)id >= GRAPHION_RUNTIME_PROGRAM_MAX) {
      return fail(diagnostic, line, 1U, "graph node id is too large", GINT_ERR_CAPACITY);
    }
    *cursor = end;
    *id_out = (uint32_t)id;
    return runtime_graph_mark_id(builder, (uint32_t)id, fail_if_existing_id, line, diagnostic);
  }

  if (is_ident_start_char(**cursor)) {
    const graphion_runtime_value *value;
    size_t len = 0U;
    while (is_ident_char((*cursor)[len])) {
      len++;
    }
    if (len >= sizeof(name)) {
      return fail(diagnostic, line, 1U, "graph node name too long", GINT_ERR_CAPACITY);
    }
    memcpy(name, *cursor, len);
    name[len] = '\0';
    *cursor += len;
    value = graphion_runtime_scope_find(scope, name);
    if (value == NULL) {
      return fail(diagnostic, line, 1U, "unknown graph node variable", GINT_ERR_UNKNOWN_VARIABLE);
    }
    return runtime_graph_id_from_value(builder, value, fail_if_existing_id, assign_named_ids, id_out, line, diagnostic);
  }

  return fail(diagnostic, line, 1U, "expected graph node name or id", GINT_ERR_PARSE);
}

static int graph_block_line_has_edge(const char *text) {
  const char *cursor = text;

  if (text == NULL) {
    return 0;
  }
  skip_spaces(&cursor);
  if (*cursor == '"') {
    cursor++;
    while (*cursor != '\0' && *cursor != '"') {
      cursor++;
    }
    if (*cursor == '"') {
      cursor++;
    }
  } else {
    while (*cursor != '\0' && *cursor != '-' && *cursor != '<' &&
           !(*cursor == ' ' || *cursor == '\t' || *cursor == '\r')) {
      cursor++;
    }
  }
  skip_spaces(&cursor);
  return *cursor == '-' || (cursor[0] == '<' && cursor[1] == '-' && cursor[2] == '>');
}

static int validate_graph_edge_attrs(const graphion_vm_value *attrs,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic);

static int runtime_graph_set_node_attrs(runtime_graph_builder *builder,
                                        uint32_t node_id,
                                        const graphion_vm_value *attrs,
                                        unsigned int line,
                                        graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || attrs == NULL || (size_t)node_id >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (attrs->kind != GVM_VALUE_DICT) {
    return fail(diagnostic, line, 1U, "graph node attributes must be a dict literal", GINT_ERR_PARSE);
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, attrs);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone graph node attributes", GINT_ERR_CAPACITY);
  }
  if (builder->has_node_attrs[node_id]) {
    vm_value_dispose_owned(&builder->node_attrs[node_id]);
  }
  builder->node_attrs[node_id] = cloned;
  builder->has_node_attrs[node_id] = 1U;
  return GINT_OK;
}

static int runtime_graph_set_node_attr_defaults(runtime_graph_builder *builder,
                                                const graphion_vm_value *defaults,
                                                unsigned int line,
                                                graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || defaults == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->has_node_attr_defaults) {
    return fail(diagnostic, line, 1U, "duplicate graph node attribute defaults", GINT_ERR_PARSE);
  }
  if (defaults->kind != GVM_VALUE_DICT) {
    return fail(diagnostic, line, 1U, "graph node attribute defaults must be a dict literal", GINT_ERR_PARSE);
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, defaults);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone graph node attribute defaults", GINT_ERR_CAPACITY);
  }
  builder->node_attr_defaults = cloned;
  builder->has_node_attr_defaults = 1;
  return GINT_OK;
}

static int runtime_hypergraph_set_vertex_attrs(runtime_graph_builder *builder,
                                               uint32_t vertex_id,
                                               const graphion_vm_value *attrs,
                                               unsigned int line,
                                               graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || attrs == NULL || (size_t)vertex_id >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (attrs->kind != GVM_VALUE_DICT) {
    return fail(diagnostic, line, 1U, "hypergraph vertex attributes must be a dict literal", GINT_ERR_PARSE);
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, attrs);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone hypergraph vertex attributes", GINT_ERR_CAPACITY);
  }
  if (builder->has_node_attrs[vertex_id]) {
    vm_value_dispose_owned(&builder->node_attrs[vertex_id]);
  }
  builder->node_attrs[vertex_id] = cloned;
  builder->has_node_attrs[vertex_id] = 1U;
  return GINT_OK;
}

static int runtime_hypergraph_set_vertex_attr_defaults(runtime_graph_builder *builder,
                                                       const graphion_vm_value *defaults,
                                                       unsigned int line,
                                                       graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || defaults == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->has_node_attr_defaults) {
    return fail(diagnostic, line, 1U, "duplicate hypergraph vertex attribute defaults", GINT_ERR_PARSE);
  }
  if (defaults->kind != GVM_VALUE_DICT) {
    return fail(diagnostic, line, 1U, "hypergraph vertex attribute defaults must be a dict literal", GINT_ERR_PARSE);
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, defaults);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone hypergraph vertex attribute defaults", GINT_ERR_CAPACITY);
  }
  builder->node_attr_defaults = cloned;
  builder->has_node_attr_defaults = 1;
  return GINT_OK;
}

static int runtime_graph_set_edge_attr_defaults(runtime_graph_builder *builder,
                                                const graphion_vm_value *defaults,
                                                unsigned int line,
                                                graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || defaults == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->has_edge_attr_defaults) {
    return fail(diagnostic, line, 1U, "duplicate graph edge attribute defaults", GINT_ERR_PARSE);
  }
  rc = validate_graph_edge_attrs(defaults, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, defaults);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone graph edge attribute defaults", GINT_ERR_CAPACITY);
  }
  builder->edge_attr_defaults = cloned;
  builder->has_edge_attr_defaults = 1;
  return GINT_OK;
}

static int runtime_graph_set_edge_attrs(runtime_graph_builder *builder,
                                        size_t edge_index,
                                        const graphion_vm_value *attrs,
                                        unsigned int line,
                                        graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || attrs == NULL || edge_index >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  rc = validate_graph_edge_attrs(attrs, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, attrs);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone graph edge attributes", GINT_ERR_CAPACITY);
  }
  if (builder->edges[edge_index].has_attrs) {
    vm_value_dispose_owned(&builder->edges[edge_index].attrs);
  }
  builder->edges[edge_index].attrs = cloned;
  builder->edges[edge_index].has_attrs = 1;
  return GINT_OK;
}

static int runtime_hypergraph_set_hyperedge_attr_defaults(runtime_graph_builder *builder,
                                                          const graphion_vm_value *defaults,
                                                          unsigned int line,
                                                          graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || defaults == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->has_edge_attr_defaults) {
    return fail(diagnostic, line, 1U, "duplicate hypergraph hyperedge attribute defaults", GINT_ERR_PARSE);
  }
  if (defaults->kind != GVM_VALUE_DICT) {
    return fail(diagnostic, line, 1U, "hypergraph hyperedge attribute defaults must be a dict literal", GINT_ERR_PARSE);
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, defaults);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone hypergraph hyperedge attribute defaults", GINT_ERR_CAPACITY);
  }
  builder->edge_attr_defaults = cloned;
  builder->has_edge_attr_defaults = 1;
  return GINT_OK;
}

static int runtime_hypergraph_set_hyperedge_attrs(runtime_graph_builder *builder,
                                                  size_t hyperedge_index,
                                                  const graphion_vm_value *attrs,
                                                  unsigned int line,
                                                  graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value cloned;
  int rc;

  if (builder == NULL || attrs == NULL || hyperedge_index >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (attrs->kind != GVM_VALUE_DICT) {
    return fail(diagnostic, line, 1U, "hypergraph hyperedge attributes must be a dict literal", GINT_ERR_PARSE);
  }
  vm_value_set_none(&cloned);
  rc = vm_value_clone(&cloned, attrs);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to clone hypergraph hyperedge attributes", GINT_ERR_CAPACITY);
  }
  if (builder->hyperedges[hyperedge_index].has_attrs) {
    vm_value_dispose_owned(&builder->hyperedges[hyperedge_index].attrs);
  }
  builder->hyperedges[hyperedge_index].attrs = cloned;
  builder->hyperedges[hyperedge_index].has_attrs = 1;
  return GINT_OK;
}

static int graph_node_attr_keys_match(const runtime_graph_builder *builder, size_t reference_id, size_t node_id) {
  if (builder == NULL || reference_id >= GRAPHION_RUNTIME_PROGRAM_MAX || node_id >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return 0;
  }
  if (!builder->has_node_attrs[reference_id] && !builder->has_node_attrs[node_id]) {
    return 1;
  }
  if (!builder->has_node_attrs[reference_id] || !builder->has_node_attrs[node_id]) {
    return 0;
  }
  return vm_value_dict_keys_equal(&builder->node_attrs[reference_id], &builder->node_attrs[node_id]);
}

static int apply_graph_node_attr_defaults(runtime_graph_builder *builder,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (!builder->has_node_attr_defaults) {
    return GINT_OK;
  }
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (!builder->used_ids[i]) {
      continue;
    }
    if (!builder->has_node_attrs[i]) {
      const int rc = runtime_graph_set_node_attrs(builder,
                                                  (uint32_t)i,
                                                  &builder->node_attr_defaults,
                                                  line,
                                                  diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
    } else {
      int rc;
      if (!vm_value_dict_keys_subset(&builder->node_attrs[i], &builder->node_attr_defaults)) {
        return fail(diagnostic, line, 1U, "graph node attributes must use declared default keys", GINT_ERR_PARSE);
      }
      rc = vm_value_dict_merge_defaults(&builder->node_attrs[i], &builder->node_attr_defaults);
      if (rc != GVM_OK) {
        return fail(diagnostic, line, 1U, "failed to apply graph node attribute defaults", GINT_ERR_CAPACITY);
      }
    }
  }
  return GINT_OK;
}

static int apply_graph_edge_attr_defaults(runtime_graph_builder *builder,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (!builder->has_edge_attr_defaults) {
    return GINT_OK;
  }
  for (i = 0U; i < builder->edge_count; ++i) {
    if (!builder->edges[i].has_attrs) {
      const int rc = runtime_graph_set_edge_attrs(builder, i, &builder->edge_attr_defaults, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
    } else {
      int rc;
      if (!vm_value_dict_keys_subset(&builder->edges[i].attrs, &builder->edge_attr_defaults)) {
        return fail(diagnostic, line, 1U, "graph edge attributes must use declared default keys", GINT_ERR_PARSE);
      }
      rc = vm_value_dict_merge_defaults(&builder->edges[i].attrs, &builder->edge_attr_defaults);
      if (rc != GVM_OK) {
        return fail(diagnostic, line, 1U, "failed to apply graph edge attribute defaults", GINT_ERR_CAPACITY);
      }
    }
  }
  return GINT_OK;
}

static int validate_graph_node_attr_schema(const runtime_graph_builder *builder,
                                           unsigned int line,
                                           graphion_runtime_diagnostic *diagnostic) {
  size_t reference_id = GRAPHION_RUNTIME_PROGRAM_MAX;
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (builder->used_ids[i]) {
      reference_id = i;
      break;
    }
  }
  if (reference_id == GRAPHION_RUNTIME_PROGRAM_MAX) {
    return GINT_OK;
  }
  for (i = reference_id + 1U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (builder->used_ids[i] && !graph_node_attr_keys_match(builder, reference_id, i)) {
      return fail(diagnostic, line, 1U, "graph node attributes must use the same keys", GINT_ERR_PARSE);
    }
  }
  return GINT_OK;
}

static int apply_hypergraph_vertex_attr_defaults(runtime_graph_builder *builder,
                                                 unsigned int line,
                                                 graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (!builder->has_node_attr_defaults) {
    return GINT_OK;
  }
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (!builder->used_ids[i]) {
      continue;
    }
    if (!builder->has_node_attrs[i]) {
      const int rc = runtime_hypergraph_set_vertex_attrs(builder,
                                                         (uint32_t)i,
                                                         &builder->node_attr_defaults,
                                                         line,
                                                         diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
    } else {
      int rc;
      if (!vm_value_dict_keys_subset(&builder->node_attrs[i], &builder->node_attr_defaults)) {
        return fail(diagnostic, line, 1U, "hypergraph vertex attributes must use declared default keys", GINT_ERR_PARSE);
      }
      rc = vm_value_dict_merge_defaults(&builder->node_attrs[i], &builder->node_attr_defaults);
      if (rc != GVM_OK) {
        return fail(diagnostic, line, 1U, "failed to apply hypergraph vertex attribute defaults", GINT_ERR_CAPACITY);
      }
    }
  }
  return GINT_OK;
}

static int validate_hypergraph_vertex_attr_schema(const runtime_graph_builder *builder,
                                                  unsigned int line,
                                                  graphion_runtime_diagnostic *diagnostic) {
  size_t reference_id = GRAPHION_RUNTIME_PROGRAM_MAX;
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->has_node_attr_defaults) {
    return GINT_OK;
  }
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (builder->used_ids[i]) {
      reference_id = i;
      break;
    }
  }
  if (reference_id == GRAPHION_RUNTIME_PROGRAM_MAX) {
    return GINT_OK;
  }
  for (i = reference_id + 1U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (builder->used_ids[i] && !graph_node_attr_keys_match(builder, reference_id, i)) {
      return fail(diagnostic, line, 1U, "hypergraph vertex attributes must use the same keys", GINT_ERR_PARSE);
    }
  }
  return GINT_OK;
}

static int graph_edge_attr_keys_match(const runtime_graph_builder *builder, size_t reference_index, size_t edge_index) {
  if (builder == NULL || reference_index >= builder->edge_count || edge_index >= builder->edge_count) {
    return 0;
  }
  if (!builder->edges[reference_index].has_attrs && !builder->edges[edge_index].has_attrs) {
    return 1;
  }
  if (!builder->edges[reference_index].has_attrs || !builder->edges[edge_index].has_attrs) {
    return 0;
  }
  return vm_value_dict_keys_equal(&builder->edges[reference_index].attrs, &builder->edges[edge_index].attrs);
}

static int validate_graph_edge_attr_schema(const runtime_graph_builder *builder,
                                           unsigned int line,
                                           graphion_runtime_diagnostic *diagnostic) {
  size_t reference_index = GRAPHION_RUNTIME_PROGRAM_MAX;
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->edge_count == 0U || builder->has_edge_attr_defaults) {
    return GINT_OK;
  }
  for (i = 0U; i < builder->edge_count; ++i) {
    if (builder->edges[i].has_attrs) {
      reference_index = i;
      break;
    }
  }
  if (reference_index == GRAPHION_RUNTIME_PROGRAM_MAX) {
    return GINT_OK;
  }
  for (i = 0U; i < builder->edge_count; ++i) {
    if (!graph_edge_attr_keys_match(builder, reference_index, i)) {
      return fail(diagnostic, line, 1U, "graph edge attributes must use the same keys", GINT_ERR_PARSE);
    }
  }
  return GINT_OK;
}

static int hypergraph_hyperedge_attr_keys_match(const runtime_graph_builder *builder,
                                                size_t reference_index,
                                                size_t hyperedge_index) {
  if (builder == NULL || reference_index >= builder->hyperedge_count || hyperedge_index >= builder->hyperedge_count) {
    return 0;
  }
  if (!builder->hyperedges[reference_index].has_attrs && !builder->hyperedges[hyperedge_index].has_attrs) {
    return 1;
  }
  if (!builder->hyperedges[reference_index].has_attrs || !builder->hyperedges[hyperedge_index].has_attrs) {
    return 0;
  }
  return vm_value_dict_keys_equal(&builder->hyperedges[reference_index].attrs,
                                  &builder->hyperedges[hyperedge_index].attrs);
}

static int apply_hypergraph_hyperedge_attr_defaults(runtime_graph_builder *builder,
                                                    unsigned int line,
                                                    graphion_runtime_diagnostic *diagnostic) {
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (!builder->has_edge_attr_defaults) {
    return GINT_OK;
  }
  for (i = 0U; i < builder->hyperedge_count; ++i) {
    if (!builder->hyperedges[i].has_attrs) {
      const int rc = runtime_hypergraph_set_hyperedge_attrs(builder, i, &builder->edge_attr_defaults, line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
    } else {
      int rc;
      if (!vm_value_dict_keys_subset(&builder->hyperedges[i].attrs, &builder->edge_attr_defaults)) {
        return fail(diagnostic, line, 1U, "hypergraph hyperedge attributes must use declared default keys", GINT_ERR_PARSE);
      }
      rc = vm_value_dict_merge_defaults(&builder->hyperedges[i].attrs, &builder->edge_attr_defaults);
      if (rc != GVM_OK) {
        return fail(diagnostic, line, 1U, "failed to apply hypergraph hyperedge attribute defaults", GINT_ERR_CAPACITY);
      }
    }
  }
  return GINT_OK;
}

static int validate_hypergraph_hyperedge_attr_schema(const runtime_graph_builder *builder,
                                                     unsigned int line,
                                                     graphion_runtime_diagnostic *diagnostic) {
  size_t reference_index = GRAPHION_RUNTIME_PROGRAM_MAX;
  size_t i;

  if (builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->hyperedge_count == 0U || builder->has_edge_attr_defaults) {
    return GINT_OK;
  }
  for (i = 0U; i < builder->hyperedge_count; ++i) {
    if (builder->hyperedges[i].has_attrs) {
      reference_index = i;
      break;
    }
  }
  if (reference_index == GRAPHION_RUNTIME_PROGRAM_MAX) {
    return GINT_OK;
  }
  for (i = 0U; i < builder->hyperedge_count; ++i) {
    if (!hypergraph_hyperedge_attr_keys_match(builder, reference_index, i)) {
      return fail(diagnostic, line, 1U, "hypergraph hyperedge attributes must use the same keys", GINT_ERR_PARSE);
    }
  }
  return GINT_OK;
}

static int parse_graph_node_attrs(const char *text,
                                  runtime_graph_builder *builder,
                                  uint32_t node_id,
                                  graphion_runtime_scope *scope,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value attrs;
  int rc;

  vm_value_set_none(&attrs);
  rc = evaluate_expression_text_to_value(text, strlen(text), scope, line, diagnostic, &attrs);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = runtime_graph_set_node_attrs(builder, node_id, &attrs, line, diagnostic);
  vm_value_dispose_owned(&attrs);
  return rc;
}

static int parse_hypergraph_vertex_attrs(const char *text,
                                         runtime_graph_builder *builder,
                                         uint32_t vertex_id,
                                         graphion_runtime_scope *scope,
                                         unsigned int line,
                                         graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value attrs;
  int rc;

  vm_value_set_none(&attrs);
  rc = evaluate_expression_text_to_value(text, strlen(text), scope, line, diagnostic, &attrs);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = runtime_hypergraph_set_vertex_attrs(builder, vertex_id, &attrs, line, diagnostic);
  vm_value_dispose_owned(&attrs);
  return rc;
}

static int parse_hypergraph_attr_defaults_line(const char *text,
                                               runtime_graph_builder *builder,
                                               graphion_runtime_scope *scope,
                                               unsigned int line,
                                               graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  graphion_vm_value defaults;
  int rc;

  if (text == NULL || builder == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(&cursor);
  if (strncmp(cursor, "defaults", 8U) != 0 || is_ident_char(cursor[8])) {
    return fail(diagnostic, line, 1U, "expected 'defaults vertex'", GINT_ERR_PARSE);
  }
  cursor += 8;
  skip_spaces(&cursor);
  if (strncmp(cursor, "vertex", 6U) == 0 && !is_ident_char(cursor[6])) {
    cursor += 6;
    skip_spaces(&cursor);
    vm_value_set_none(&defaults);
    rc = evaluate_expression_text_to_value(cursor, strlen(cursor), scope, line, diagnostic, &defaults);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = runtime_hypergraph_set_vertex_attr_defaults(builder, &defaults, line, diagnostic);
    vm_value_dispose_owned(&defaults);
    return rc;
  }
  if (strncmp(cursor, "hyperedge", 9U) == 0 && !is_ident_char(cursor[9])) {
    cursor += 9;
    skip_spaces(&cursor);
    vm_value_set_none(&defaults);
    rc = evaluate_expression_text_to_value(cursor, strlen(cursor), scope, line, diagnostic, &defaults);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = runtime_hypergraph_set_hyperedge_attr_defaults(builder, &defaults, line, diagnostic);
    vm_value_dispose_owned(&defaults);
    return rc;
  }
  return fail(diagnostic, line, 1U, "expected 'vertex' or 'hyperedge' after defaults", GINT_ERR_PARSE);
}

static int parse_hypergraph_edge_line(const char *text,
                                      runtime_graph_builder *builder,
                                      int reserve_only,
                                      graphion_runtime_scope *scope,
                                      unsigned int line,
                                      graphion_runtime_diagnostic *diagnostic) {
  graphion_vm_value vertices;
  graphion_vm_value attrs;
  char list_text[512];
  const char *list_start;
  const char *scan;
  const char *attrs_text;
  size_t list_len;
  size_t vertex_count = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int has_attrs = 0;
  int rc;

  if (text == NULL || builder == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(&text);
  list_start = text;
  scan = text;
  while (*scan != '\0') {
    if (in_string) {
      if (*scan == '"') {
        in_string = 0;
      }
      scan++;
      continue;
    }
    if (*scan == '"') {
      in_string = 1;
      scan++;
      continue;
    }
    if (*scan == '[') {
      depth++;
    } else if (*scan == ']') {
      depth--;
      if (depth == 0) {
        scan++;
        break;
      }
    }
    scan++;
  }
  if (depth != 0) {
    return fail(diagnostic, line, 1U, "expected ']' after hyperedge vertex list", GINT_ERR_PARSE);
  }
  list_len = (size_t)(scan - list_start);
  if (list_len >= sizeof(list_text)) {
    return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
  }
  memcpy(list_text, list_start, list_len);
  list_text[list_len] = '\0';
  attrs_text = scan;
  skip_spaces(&attrs_text);
  has_attrs = *attrs_text != '\0' ? 1 : 0;

  vm_value_set_none(&vertices);
  vm_value_set_none(&attrs);
  rc = evaluate_expression_text_to_value(list_text, strlen(list_text), scope, line, diagnostic, &vertices);
  if (rc != GINT_OK) {
    return rc;
  }
  if (!vm_value_list_length(&vertices, &vertex_count)) {
    vm_value_dispose_owned(&vertices);
    return fail(diagnostic, line, 1U, "hyperedge must be a list of vertices", GINT_ERR_PARSE);
  }
  if (vertex_count == 0U) {
    vm_value_dispose_owned(&vertices);
    return fail(diagnostic, line, 1U, "hyperedge must contain at least one vertex", GINT_ERR_PARSE);
  }
  if (vertex_count > GRAPHION_RUNTIME_PROGRAM_MAX) {
    vm_value_dispose_owned(&vertices);
    return fail(diagnostic, line, 1U, "too many hyperedge vertices", GINT_ERR_CAPACITY);
  }
  if (!reserve_only && builder->hyperedge_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    vm_value_dispose_owned(&vertices);
    return fail(diagnostic, line, 1U, "too many hyperedges", GINT_ERR_CAPACITY);
  }
  if (!reserve_only && builder->hyperedge_vertex_count + vertex_count > GRAPHION_RUNTIME_PROGRAM_MAX) {
    vm_value_dispose_owned(&vertices);
    return fail(diagnostic, line, 1U, "too many hyperedge incidences", GINT_ERR_CAPACITY);
  }
  if (!reserve_only) {
    builder->hyperedges[builder->hyperedge_count].start = builder->hyperedge_vertex_count;
  }
  for (i = 0U; i < vertex_count; ++i) {
    graphion_vm_value vertex_value;
    uint32_t vertex_id = 0U;

    vm_value_set_none(&vertex_value);
    rc = vm_value_list_clone_item(&vertices, i, &vertex_value);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&vertices);
      return fail(diagnostic, line, 1U, "invalid hyperedge vertex", GINT_ERR_PARSE);
    }
    rc = runtime_graph_id_from_value(builder, &vertex_value, 0, !reserve_only, &vertex_id, line, diagnostic);
    vm_value_dispose_owned(&vertex_value);
    if (rc != GINT_OK) {
      vm_value_dispose_owned(&vertices);
      return rc;
    }
    if (!reserve_only) {
      builder->hyperedge_vertices[builder->hyperedge_vertex_count + i] = vertex_id;
    }
  }
  if (!reserve_only) {
    builder->hyperedges[builder->hyperedge_count].vertex_count = vertex_count;
    if (has_attrs) {
      rc = evaluate_expression_text_to_value(attrs_text, strlen(attrs_text), scope, line, diagnostic, &attrs);
      if (rc != GINT_OK) {
        vm_value_dispose_owned(&vertices);
        return rc;
      }
      rc = runtime_hypergraph_set_hyperedge_attrs(builder, builder->hyperedge_count, &attrs, line, diagnostic);
      vm_value_dispose_owned(&attrs);
      if (rc != GINT_OK) {
        vm_value_dispose_owned(&vertices);
        return rc;
      }
    }
    builder->hyperedge_vertex_count += vertex_count;
    builder->hyperedge_count += 1U;
  }
  vm_value_dispose_owned(&vertices);
  return GINT_OK;
}

static int graph_block_line_is_defaults(const char *text) {
  const char *cursor = text;

  if (text == NULL) {
    return 0;
  }
  skip_spaces(&cursor);
  return strncmp(cursor, "defaults", 8U) == 0 && !is_ident_char(cursor[8]);
}

static int parse_graph_attr_defaults_line(const char *text,
                                          runtime_graph_builder *builder,
                                          graphion_runtime_scope *scope,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  graphion_vm_value defaults;
  int rc;

  if (text == NULL || builder == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(&cursor);
  if (strncmp(cursor, "defaults", 8U) != 0 || is_ident_char(cursor[8])) {
    return fail(diagnostic, line, 1U, "expected 'defaults node'", GINT_ERR_PARSE);
  }
  cursor += 8;
  skip_spaces(&cursor);
  if (strncmp(cursor, "node", 4U) == 0 && !is_ident_char(cursor[4])) {
    cursor += 4;
    skip_spaces(&cursor);
    vm_value_set_none(&defaults);
    rc = evaluate_expression_text_to_value(cursor, strlen(cursor), scope, line, diagnostic, &defaults);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = runtime_graph_set_node_attr_defaults(builder, &defaults, line, diagnostic);
    vm_value_dispose_owned(&defaults);
    return rc;
  }
  if (strncmp(cursor, "edge", 4U) == 0 && !is_ident_char(cursor[4])) {
    cursor += 4;
    skip_spaces(&cursor);
    vm_value_set_none(&defaults);
    rc = evaluate_expression_text_to_value(cursor, strlen(cursor), scope, line, diagnostic, &defaults);
    if (rc != GINT_OK) {
      return rc;
    }
    rc = runtime_graph_set_edge_attr_defaults(builder, &defaults, line, diagnostic);
    vm_value_dispose_owned(&defaults);
    return rc;
  }
  return fail(diagnostic, line, 1U, "expected 'node' or 'edge' after defaults", GINT_ERR_PARSE);
}

static int validate_graph_edge_attrs(const graphion_vm_value *attrs,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic) {
  uint8_t weight_kind = GVM_VALUE_NONE;
  int has_weight = 0;

  if (attrs == NULL || attrs->kind != GVM_VALUE_DICT) {
    return fail(diagnostic, line, 1U, "graph edge attributes must be a dict literal", GINT_ERR_PARSE);
  }
  if (!vm_value_dict_key_kind(attrs, "weight", &weight_kind, &has_weight)) {
    return fail(diagnostic, line, 1U, "invalid graph edge attributes", GINT_ERR_PARSE);
  }
  if (has_weight && weight_kind != GVM_VALUE_INT && weight_kind != GVM_VALUE_FLOAT) {
    return fail(diagnostic, line, 1U, "graph edge weight must be int or float", GINT_ERR_PARSE);
  }
  return GINT_OK;
}

static int parse_graph_edge_attrs(const char *text,
                                  graphion_runtime_scope *scope,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic,
                                  graphion_vm_value *attrs_out) {
  char weight_expression[512];
  const char *expression_text = text;
  graphion_vm_value expression_value;
  int rc;

  if (text == NULL || scope == NULL || attrs_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(&expression_text);
  vm_value_set_none(attrs_out);
  vm_value_set_none(&expression_value);
  rc = evaluate_expression_text_to_value(expression_text, strlen(expression_text), scope, line, diagnostic, &expression_value);
  if (rc != GINT_OK) {
    return rc;
  }
  if (expression_value.kind == GVM_VALUE_DICT) {
    rc = vm_value_clone(attrs_out, &expression_value);
    vm_value_dispose_owned(&expression_value);
    if (rc != GVM_OK) {
      return fail(diagnostic, line, 1U, "failed to clone graph edge attributes", GINT_ERR_CAPACITY);
    }
    rc = validate_graph_edge_attrs(attrs_out, line, diagnostic);
    if (rc != GINT_OK) {
      vm_value_dispose_owned(attrs_out);
      return rc;
    }
    return GINT_OK;
  }
  if (expression_value.kind != GVM_VALUE_INT && expression_value.kind != GVM_VALUE_FLOAT) {
    vm_value_dispose_owned(&expression_value);
    return fail(diagnostic, line, 1U, "graph edge weight expression must be int, float, or dict", GINT_ERR_PARSE);
  }
  vm_value_dispose_owned(&expression_value);
  {
    const int written = snprintf(weight_expression, sizeof(weight_expression), "{\"weight\": %s}", expression_text);
    if (written < 0 || (size_t)written >= sizeof(weight_expression)) {
      return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
    }
  }
  rc = evaluate_expression_text_to_value(weight_expression, strlen(weight_expression), scope, line, diagnostic, attrs_out);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = validate_graph_edge_attrs(attrs_out, line, diagnostic);
  if (rc != GINT_OK) {
    vm_value_dispose_owned(attrs_out);
    return rc;
  }
  return GINT_OK;
}

static int parse_graph_block_line(const char *text,
                                  runtime_graph_builder *builder,
                                  int reserve_only,
                                  graphion_runtime_scope *scope,
                                  unsigned int line,
                                  graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  uint32_t left = 0U;
  uint32_t right = 0U;
  int has_edge = graph_block_line_has_edge(text);
  int bidirectional = 0;
  int directed_syntax = 0;
  graphion_vm_value edge_attrs;
  int has_edge_attrs = 0;
  int rc;

  vm_value_set_none(&edge_attrs);
  if (text == NULL || builder == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  rc = parse_graph_node_ref(&cursor,
                            builder,
                            scope,
                            reserve_only && !has_edge ? 1 : 0,
                            !reserve_only,
                            &left,
                            line,
                            diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor == '{' && !has_edge) {
    if (reserve_only) {
      return GINT_OK;
    }
    return parse_graph_node_attrs(cursor, builder, left, scope, line, diagnostic);
  }
  if (*cursor == '\0' && !has_edge) {
    return GINT_OK;
  }
  if (*cursor == '-') {
    cursor++;
    if (*cursor == '>') {
      directed_syntax = 1;
      bidirectional = 0;
      cursor++;
    } else {
      directed_syntax = 0;
      bidirectional = 1;
    }
  } else if (cursor[0] == '<' && cursor[1] == '-' && cursor[2] == '>') {
    directed_syntax = 1;
    bidirectional = 1;
    cursor += 3;
  } else {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after graph node", GINT_ERR_PARSE);
  }
  if (directed_syntax) {
    if (builder->has_undirected_edges) {
      return fail(diagnostic, line, 1U, "directed graph cannot use undirected '-' edges", GINT_ERR_PARSE);
    }
    builder->has_directed_edges = 1;
  } else {
    if (builder->has_directed_edges) {
      return fail(diagnostic, line, 1U, "directed graph cannot use undirected '-' edges", GINT_ERR_PARSE);
    }
    builder->has_undirected_edges = 1;
  }
  rc = parse_graph_node_ref(&cursor, builder, scope, 0, !reserve_only, &right, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    if (reserve_only) {
      return GINT_OK;
    }
    rc = parse_graph_edge_attrs(cursor, scope, line, diagnostic, &edge_attrs);
    if (rc != GINT_OK) {
      return rc;
    }
    has_edge_attrs = 1;
    cursor += strlen(cursor);
    skip_spaces(&cursor);
  }
  if (*cursor != '\0') {
    vm_value_dispose_owned(&edge_attrs);
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after graph edge", GINT_ERR_PARSE);
  }
  if (reserve_only) {
    vm_value_dispose_owned(&edge_attrs);
    return GINT_OK;
  }
  if (builder->edge_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    vm_value_dispose_owned(&edge_attrs);
    return fail(diagnostic, line, 1U, "too many graph edges", GINT_ERR_CAPACITY);
  }
  builder->edges[builder->edge_count].from = left;
  builder->edges[builder->edge_count].to = right;
  builder->edges[builder->edge_count].directed = directed_syntax;
  builder->edges[builder->edge_count].bidirectional = bidirectional;
  builder->edges[builder->edge_count].has_attrs = has_edge_attrs;
  if (has_edge_attrs) {
    builder->edges[builder->edge_count].attrs = edge_attrs;
  }
  builder->edge_count += 1U;
  return GINT_OK;
}

static int parse_graph_name_from_header(const char *text,
                                        char target[GRAPHION_RUNTIME_NAME_MAX],
                                        unsigned int line,
                                        graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  int rc;

  skip_spaces(&cursor);
  if (strncmp(cursor, "graph", 5U) != 0 || is_ident_char(cursor[5])) {
    return fail(diagnostic, line, 1U, "expected 'graph'", GINT_ERR_PARSE);
  }
  cursor += 5;
  if (*cursor != ' ' && *cursor != '\t') {
    return fail(diagnostic, line, 1U, "expected graph name", GINT_ERR_PARSE);
  }
  rc = parse_identifier_token(&cursor, target, GRAPHION_RUNTIME_NAME_MAX, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after graph declaration", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after graph declaration", GINT_ERR_PARSE);
  }
  return GINT_OK;
}

static int parse_hypergraph_name_from_header(const char *text,
                                             char target[GRAPHION_RUNTIME_NAME_MAX],
                                             unsigned int line,
                                             graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  int rc;

  skip_spaces(&cursor);
  if (strncmp(cursor, "hypergraph", 10U) != 0 || is_ident_char(cursor[10])) {
    return fail(diagnostic, line, 1U, "expected 'hypergraph'", GINT_ERR_PARSE);
  }
  cursor += 10;
  if (*cursor != ' ' && *cursor != '\t') {
    return fail(diagnostic, line, 1U, "expected hypergraph name", GINT_ERR_PARSE);
  }
  rc = parse_identifier_token(&cursor, target, GRAPHION_RUNTIME_NAME_MAX, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after hypergraph declaration", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after hypergraph declaration", GINT_ERR_PARSE);
  }
  return GINT_OK;
}

static int parse_struct_name_from_header(const char *text,
                                         char target[GRAPHION_RUNTIME_NAME_MAX],
                                         unsigned int line,
                                         graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  int rc;

  skip_spaces(&cursor);
  if (strncmp(cursor, "struct", 6U) != 0 || is_ident_char(cursor[6])) {
    return fail(diagnostic, line, 1U, "expected 'struct'", GINT_ERR_PARSE);
  }
  cursor += 6;
  if (*cursor != ' ' && *cursor != '\t') {
    return fail(diagnostic, line, 1U, "expected struct name", GINT_ERR_PARSE);
  }
  rc = parse_identifier_token(&cursor, target, GRAPHION_RUNTIME_NAME_MAX, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (is_reserved_name(target)) {
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after struct declaration", GINT_ERR_PARSE);
  }
  cursor++;
  skip_spaces(&cursor);
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after struct declaration", GINT_ERR_PARSE);
  }
  return GINT_OK;
}

static int runtime_struct_type_name_is_builtin(const char *name) {
  static const char *supported[] = {"int",   "float", "bool",  "string", "bits",  "list",
                                    "dict",  "tuple", "set",   "graph",  "hypergraph", "any"};
  size_t i;
  if (name == NULL || *name == '\0') {
    return 0;
  }
  for (i = 0U; i < sizeof(supported) / sizeof(supported[0]); ++i) {
    if (strcmp(name, supported[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static int runtime_struct_type_name_is_supported(const char *name, const graphion_runtime_scope *scope) {
  const graphion_runtime_value *value;
  if (runtime_struct_type_name_is_builtin(name)) {
    return 1;
  }
  value = graphion_runtime_scope_find(scope, name);
  return value != NULL && value->kind == GVM_VALUE_STRUCT_TYPE;
}

static int runtime_value_matches_struct_type(const graphion_vm_value *value, const char *type_name) {
  if (value == NULL || type_name == NULL || strcmp(type_name, "any") == 0) {
    return 1;
  }
  if (strcmp(type_name, "int") == 0) {
    return value->kind == GVM_VALUE_INT;
  }
  if (strcmp(type_name, "float") == 0) {
    return value->kind == GVM_VALUE_FLOAT;
  }
  if (strcmp(type_name, "bool") == 0) {
    return value->kind == GVM_VALUE_BOOL;
  }
  if (strcmp(type_name, "string") == 0) {
    return value->kind == GVM_VALUE_STRING;
  }
  if (strcmp(type_name, "bits") == 0) {
    return value->kind == GVM_VALUE_BITS;
  }
  if (strcmp(type_name, "list") == 0) {
    return value->kind == GVM_VALUE_LIST;
  }
  if (strcmp(type_name, "dict") == 0) {
    return value->kind == GVM_VALUE_DICT;
  }
  if (strcmp(type_name, "tuple") == 0) {
    return value->kind == GVM_VALUE_TUPLE;
  }
  if (strcmp(type_name, "set") == 0) {
    return value->kind == GVM_VALUE_SET;
  }
  if (strcmp(type_name, "graph") == 0) {
    return value->kind == GVM_VALUE_GRAPH_REF;
  }
  if (strcmp(type_name, "hypergraph") == 0) {
    return value->kind == GVM_VALUE_HYPERGRAPH_REF;
  }
  if (value->kind == GVM_VALUE_STRUCT) {
    const graphion_struct_instance_value *instance = (const graphion_struct_instance_value *)value->as.ref_value;
    return instance != NULL && strcmp(instance->type_name, type_name) == 0;
  }
  return 0;
}

static int build_runtime_graph_value(const runtime_graph_builder *builder,
                                     graphion_vm_value *value_out,
                                     unsigned int line,
                                     graphion_runtime_diagnostic *diagnostic) {
  graphion_graph_value *graph_value = NULL;
  graphion_csr_graph *graph = NULL;
  uint32_t *offsets = NULL;
  uint32_t *neighbors = NULL;
  size_t node_count;
  size_t visible_node_count = 0U;
  size_t visible_node_attr_key_count = 0U;
  size_t visible_edge_attr_key_count = 0U;
  size_t adjacency_count;
  size_t i;

  if (builder == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  node_count = builder->has_nodes ? (size_t)builder->max_id + 1U : 0U;
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (builder->used_ids[i]) {
      visible_node_count += 1U;
    }
  }
  adjacency_count = 0U;
  for (i = 0U; i < builder->edge_count; ++i) {
    adjacency_count += builder->edges[i].bidirectional ? 2U : 1U;
  }
  graph_value = (graphion_graph_value *)calloc(1U, sizeof(*graph_value));
  if (graph_value == NULL) {
    return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  graph = &graph_value->csr;
  if (node_count > 0U) {
    offsets = (uint32_t *)calloc(node_count + 1U, sizeof(*offsets));
    if (offsets == NULL) {
      free(graph_value);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    graph->offsets = offsets;
  }
  if (node_count > 0U) {
    size_t node_index = 0U;

    if (visible_node_count > 0U) {
      graph_value->nodes = (graphion_graph_node_value *)calloc(visible_node_count, sizeof(*graph_value->nodes));
      if (graph_value->nodes == NULL) {
        free((void *)graph->offsets);
        free(graph_value);
        return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
      }
      graph_value->node_count = visible_node_count;
      for (i = 0U; i < node_count; ++i) {
        if (builder->used_ids[i]) {
          const char *name = runtime_graph_name_for_id(builder, (uint32_t)i);
          graph_value->nodes[node_index].id = (uint32_t)i;
          if (name != NULL) {
            const size_t name_len = strlen(name);
            char *copy = (char *)malloc(name_len + 1U);
            if (copy == NULL) {
              graphion_vm_value cleanup;
              vm_value_set_none(&cleanup);
              cleanup.kind = GVM_VALUE_GRAPH_REF;
              cleanup.as.ref_value = graph_value;
              vm_value_dispose_owned(&cleanup);
              return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
            }
            memcpy(copy, name, name_len + 1U);
            graph_value->nodes[node_index].name = copy;
          }
          node_index += 1U;
        }
      }
    }
    graph_value->node_attrs = (graphion_vm_value *)calloc(node_count, sizeof(*graph_value->node_attrs));
    if (graph_value->node_attrs == NULL) {
      graphion_vm_value cleanup;
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_GRAPH_REF;
      cleanup.as.ref_value = graph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    graph_value->node_attr_count = node_count;
    for (i = 0U; i < node_count; ++i) {
      vm_value_set_none(&graph_value->node_attrs[i]);
    }
    for (i = 0U; i < node_count; ++i) {
      if (builder->has_node_attrs[i]) {
        const int rc = vm_value_clone(&graph_value->node_attrs[i], &builder->node_attrs[i]);
        if (rc != GVM_OK) {
          graphion_vm_value cleanup;
          vm_value_set_none(&cleanup);
          cleanup.kind = GVM_VALUE_GRAPH_REF;
          cleanup.as.ref_value = graph_value;
          vm_value_dispose_owned(&cleanup);
          return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
        }
      }
    }
  }
  if (builder->has_node_attr_defaults) {
    (void)vm_value_dict_length(&builder->node_attr_defaults, &visible_node_attr_key_count);
  } else {
    for (i = 0U; i < node_count; ++i) {
      if (builder->has_node_attrs[i]) {
        (void)vm_value_dict_length(&builder->node_attrs[i], &visible_node_attr_key_count);
        break;
      }
    }
  }
  if (adjacency_count > 0U) {
    neighbors = (uint32_t *)calloc(adjacency_count, sizeof(*neighbors));
    if (neighbors == NULL) {
      graphion_vm_value cleanup;
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_GRAPH_REF;
      cleanup.as.ref_value = graph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    for (i = 0U; i < builder->edge_count; ++i) {
      offsets[builder->edges[i].from + 1U] += 1U;
      if (builder->edges[i].bidirectional) {
        offsets[builder->edges[i].to + 1U] += 1U;
      }
    }
    for (i = 1U; i <= node_count; ++i) {
      offsets[i] += offsets[i - 1U];
    }
    {
      uint32_t *cursor = (uint32_t *)calloc(node_count, sizeof(*cursor));
      if (cursor == NULL) {
        free(neighbors);
        {
          graphion_vm_value cleanup;
          vm_value_set_none(&cleanup);
          cleanup.kind = GVM_VALUE_GRAPH_REF;
          cleanup.as.ref_value = graph_value;
          vm_value_dispose_owned(&cleanup);
        }
        return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
      }
      memcpy(cursor, offsets, node_count * sizeof(*cursor));
      for (i = 0U; i < builder->edge_count; ++i) {
        const uint32_t from = builder->edges[i].from;
        const uint32_t to = builder->edges[i].to;
        neighbors[cursor[from]++] = to;
        if (builder->edges[i].bidirectional) {
          neighbors[cursor[to]++] = from;
        }
      }
      free(cursor);
    }
  }
  if (builder->edge_count > 0U) {
    graph_value->edges = (graphion_graph_edge_value *)calloc(builder->edge_count, sizeof(*graph_value->edges));
    if (graph_value->edges == NULL) {
      graphion_vm_value cleanup;
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_GRAPH_REF;
      cleanup.as.ref_value = graph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    graph_value->edge_count = builder->edge_count;
    for (i = 0U; i < builder->edge_count; ++i) {
      graph_value->edges[i].from = builder->edges[i].from;
      graph_value->edges[i].to = builder->edges[i].to;
      graph_value->edges[i].directed = builder->edges[i].directed ? 1U : 0U;
      graph_value->edges[i].bidirectional = builder->edges[i].bidirectional ? 1U : 0U;
    }
    graph_value->edge_attrs = (graphion_vm_value *)calloc(builder->edge_count, sizeof(*graph_value->edge_attrs));
    if (graph_value->edge_attrs == NULL) {
      graphion_vm_value cleanup;
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_GRAPH_REF;
      cleanup.as.ref_value = graph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    graph_value->edge_attr_count = builder->edge_count;
    for (i = 0U; i < builder->edge_count; ++i) {
      vm_value_set_none(&graph_value->edge_attrs[i]);
      if (builder->edges[i].has_attrs) {
        const int rc = vm_value_clone(&graph_value->edge_attrs[i], &builder->edges[i].attrs);
        if (rc != GVM_OK) {
          graphion_vm_value cleanup;
          vm_value_set_none(&cleanup);
          cleanup.kind = GVM_VALUE_GRAPH_REF;
          cleanup.as.ref_value = graph_value;
          vm_value_dispose_owned(&cleanup);
          return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
        }
        if (visible_edge_attr_key_count == 0U) {
          (void)vm_value_dict_length(&builder->edges[i].attrs, &visible_edge_attr_key_count);
        }
      }
    }
  }
  graph->node_count = node_count;
  graph->edge_count = adjacency_count;
  graph->neighbors = neighbors;
  graph->weights = NULL;
  graph->edge_attrs = NULL;
  vm_value_set_none(value_out);
  value_out->kind = GVM_VALUE_GRAPH_REF;
  value_out->reserved[0] = builder->has_directed_edges ? 1U : 0U;
  value_out->reserved[1] = (uint8_t)(visible_node_count & 0xFFU);
  value_out->reserved[2] = (uint8_t)((visible_node_count >> 8U) & 0xFFU);
  value_out->reserved[3] = (uint8_t)(builder->edge_count & 0xFFU);
  value_out->reserved[4] = (uint8_t)((builder->edge_count >> 8U) & 0xFFU);
  value_out->reserved[5] = (uint8_t)(visible_node_attr_key_count & 0xFFU);
  value_out->reserved[6] = (uint8_t)((visible_node_attr_key_count >> 8U) & 0xFFU);
  value_out->as.ref_value = graph_value;
  (void)visible_edge_attr_key_count;
  return GINT_OK;
}

static int parse_hypergraph_vertex_line(const char *text,
                                        runtime_graph_builder *builder,
                                        int reserve_only,
                                        graphion_runtime_scope *scope,
                                        unsigned int line,
                                        graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  uint32_t vertex_id = 0U;
  int rc;

  if (text == NULL || builder == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  rc = parse_graph_node_ref(&cursor, builder, scope, reserve_only ? 1 : 0, !reserve_only, &vertex_id, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor == '{') {
    if (reserve_only) {
      return GINT_OK;
    }
    return parse_hypergraph_vertex_attrs(cursor, builder, vertex_id, scope, line, diagnostic);
  }
  if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after hypergraph vertex", GINT_ERR_PARSE);
  }
  (void)vertex_id;
  return GINT_OK;
}

static int parse_hypergraph_block_line(const char *text,
                                       runtime_graph_builder *builder,
                                       int reserve_only,
                                       graphion_runtime_scope *scope,
                                       unsigned int line,
                                       graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;

  if (text == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  skip_spaces(&cursor);
  if (*cursor == '[') {
    return parse_hypergraph_edge_line(cursor, builder, reserve_only, scope, line, diagnostic);
  }
  return parse_hypergraph_vertex_line(cursor, builder, reserve_only, scope, line, diagnostic);
}

static int build_runtime_hypergraph_value(const runtime_graph_builder *builder,
                                          graphion_vm_value *value_out,
                                          unsigned int line,
                                          graphion_runtime_diagnostic *diagnostic) {
  graphion_hypergraph_value *hypergraph_value = NULL;
  graphion_hypergraph *hypergraph = NULL;
  uint32_t *offsets = NULL;
  uint32_t *node_hyperedge_offsets = NULL;
  uint32_t *node_hyperedges = NULL;
  uint32_t *hyperedge_offsets = NULL;
  uint32_t *hyperedge_vertices = NULL;
  size_t dense_vertex_count;
  size_t visible_vertex_count = 0U;
  size_t visible_vertex_attr_key_count = 0U;
  size_t visible_hyperedge_attr_key_count = 0U;
  size_t incidence_count = 0U;
  size_t i;

  if (builder == NULL || value_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  dense_vertex_count = builder->has_nodes ? (size_t)builder->max_id + 1U : 0U;
  for (i = 0U; i < GRAPHION_RUNTIME_PROGRAM_MAX; ++i) {
    if (builder->used_ids[i]) {
      visible_vertex_count += 1U;
    }
  }
  hypergraph_value = (graphion_hypergraph_value *)calloc(1U, sizeof(*hypergraph_value));
  if (hypergraph_value == NULL) {
    return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
  }
  hypergraph = &hypergraph_value->hypergraph;
  for (i = 0U; i < builder->hyperedge_count; ++i) {
    incidence_count += builder->hyperedges[i].vertex_count;
  }
  if (dense_vertex_count > 0U) {
    size_t vertex_index = 0U;
    offsets = (uint32_t *)calloc(dense_vertex_count + 1U, sizeof(*offsets));
    if (offsets == NULL) {
      free(hypergraph_value);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    hypergraph->node_offsets = offsets;
    hypergraph_value->vertices =
        (graphion_graph_node_value *)calloc(visible_vertex_count, sizeof(*hypergraph_value->vertices));
    if (hypergraph_value->vertices == NULL) {
      graphion_vm_value cleanup;
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
      cleanup.as.ref_value = hypergraph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    hypergraph_value->vertex_count = visible_vertex_count;
    for (i = 0U; i < dense_vertex_count; ++i) {
      if (builder->used_ids[i]) {
        const char *name = runtime_graph_name_for_id(builder, (uint32_t)i);
        hypergraph_value->vertices[vertex_index].id = (uint32_t)i;
        if (name != NULL) {
          const size_t name_len = strlen(name);
          char *copy = (char *)malloc(name_len + 1U);
          if (copy == NULL) {
            graphion_vm_value cleanup;
            vm_value_set_none(&cleanup);
            cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
            cleanup.as.ref_value = hypergraph_value;
            vm_value_dispose_owned(&cleanup);
            return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
          }
          memcpy(copy, name, name_len + 1U);
          hypergraph_value->vertices[vertex_index].name = copy;
        }
        vertex_index += 1U;
      }
    }
    hypergraph_value->vertex_attrs = (graphion_vm_value *)calloc(dense_vertex_count, sizeof(*hypergraph_value->vertex_attrs));
    if (hypergraph_value->vertex_attrs == NULL) {
      graphion_vm_value cleanup;
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
      cleanup.as.ref_value = hypergraph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    hypergraph_value->vertex_attr_count = dense_vertex_count;
    for (i = 0U; i < dense_vertex_count; ++i) {
      vm_value_set_none(&hypergraph_value->vertex_attrs[i]);
      if (builder->has_node_attrs[i]) {
        const int rc = vm_value_clone(&hypergraph_value->vertex_attrs[i], &builder->node_attrs[i]);
        if (rc != GVM_OK) {
          graphion_vm_value cleanup;
          vm_value_set_none(&cleanup);
          cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
          cleanup.as.ref_value = hypergraph_value;
          vm_value_dispose_owned(&cleanup);
          return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
        }
      }
    }
  }
  if (builder->has_node_attr_defaults) {
    (void)vm_value_dict_length(&builder->node_attr_defaults, &visible_vertex_attr_key_count);
  } else {
    for (i = 0U; i < dense_vertex_count; ++i) {
      if (builder->has_node_attrs[i]) {
        (void)vm_value_dict_length(&builder->node_attrs[i], &visible_vertex_attr_key_count);
        break;
      }
    }
  }
  if (builder->hyperedge_count > 0U) {
    hyperedge_offsets = (uint32_t *)calloc(builder->hyperedge_count + 1U, sizeof(*hyperedge_offsets));
    hyperedge_vertices = (uint32_t *)calloc(incidence_count, sizeof(*hyperedge_vertices));
    node_hyperedge_offsets = (uint32_t *)calloc(dense_vertex_count + 1U, sizeof(*node_hyperedge_offsets));
    node_hyperedges = (uint32_t *)calloc(incidence_count, sizeof(*node_hyperedges));
    if (hyperedge_offsets == NULL || hyperedge_vertices == NULL || node_hyperedge_offsets == NULL ||
        node_hyperedges == NULL) {
      graphion_vm_value cleanup;
      free(hyperedge_offsets);
      free(hyperedge_vertices);
      free(node_hyperedge_offsets);
      free(node_hyperedges);
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
      cleanup.as.ref_value = hypergraph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    {
      size_t write_index = 0U;
      for (i = 0U; i < builder->hyperedge_count; ++i) {
        size_t j;
        hyperedge_offsets[i] = (uint32_t)write_index;
        for (j = 0U; j < builder->hyperedges[i].vertex_count; ++j) {
          const uint32_t vertex_id = builder->hyperedge_vertices[builder->hyperedges[i].start + j];
          hyperedge_vertices[write_index++] = vertex_id;
          node_hyperedge_offsets[vertex_id + 1U] += 1U;
        }
      }
      hyperedge_offsets[builder->hyperedge_count] = (uint32_t)write_index;
    }
    for (i = 1U; i <= dense_vertex_count; ++i) {
      node_hyperedge_offsets[i] += node_hyperedge_offsets[i - 1U];
    }
    {
      uint32_t *cursor = (uint32_t *)calloc(dense_vertex_count, sizeof(*cursor));
      if (cursor == NULL) {
        graphion_vm_value cleanup;
        free(hyperedge_offsets);
        free(hyperedge_vertices);
        free(node_hyperedge_offsets);
        free(node_hyperedges);
        vm_value_set_none(&cleanup);
        cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
        cleanup.as.ref_value = hypergraph_value;
        vm_value_dispose_owned(&cleanup);
        return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
      }
      memcpy(cursor, node_hyperedge_offsets, dense_vertex_count * sizeof(*cursor));
      for (i = 0U; i < builder->hyperedge_count; ++i) {
        size_t j;
        for (j = 0U; j < builder->hyperedges[i].vertex_count; ++j) {
          const uint32_t vertex_id = builder->hyperedge_vertices[builder->hyperedges[i].start + j];
          node_hyperedges[cursor[vertex_id]++] = (uint32_t)i;
        }
      }
      free(cursor);
    }
    hypergraph_value->hyperedge_attrs =
        (graphion_vm_value *)calloc(builder->hyperedge_count, sizeof(*hypergraph_value->hyperedge_attrs));
    if (hypergraph_value->hyperedge_attrs == NULL) {
      graphion_vm_value cleanup;
      vm_value_set_none(&cleanup);
      cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
      cleanup.as.ref_value = hypergraph_value;
      vm_value_dispose_owned(&cleanup);
      return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
    }
    hypergraph_value->hyperedge_attr_count = builder->hyperedge_count;
    for (i = 0U; i < builder->hyperedge_count; ++i) {
      vm_value_set_none(&hypergraph_value->hyperedge_attrs[i]);
      if (builder->hyperedges[i].has_attrs) {
        const int rc = vm_value_clone(&hypergraph_value->hyperedge_attrs[i], &builder->hyperedges[i].attrs);
        if (rc != GVM_OK) {
          graphion_vm_value cleanup;
          vm_value_set_none(&cleanup);
          cleanup.kind = GVM_VALUE_HYPERGRAPH_REF;
          cleanup.as.ref_value = hypergraph_value;
          vm_value_dispose_owned(&cleanup);
          return fail(diagnostic, line, 1U, "out of memory", GINT_ERR_CAPACITY);
        }
        if (visible_hyperedge_attr_key_count == 0U) {
          (void)vm_value_dict_length(&builder->hyperedges[i].attrs, &visible_hyperedge_attr_key_count);
        }
      }
    }
  }
  hypergraph->node_count = dense_vertex_count;
  hypergraph->hyperedge_count = builder->hyperedge_count;
  hypergraph->incidence_count = incidence_count;
  hypergraph->node_hyperedges = node_hyperedges;
  hypergraph->hyperedge_offsets = hyperedge_offsets;
  hypergraph->hyperedge_nodes = hyperedge_vertices;
  if (node_hyperedge_offsets != NULL) {
    free((void *)hypergraph->node_offsets);
    hypergraph->node_offsets = node_hyperedge_offsets;
  }
  vm_value_set_none(value_out);
  value_out->kind = GVM_VALUE_HYPERGRAPH_REF;
  value_out->reserved[1] = (uint8_t)(visible_vertex_count & 0xFFU);
  value_out->reserved[2] = (uint8_t)((visible_vertex_count >> 8U) & 0xFFU);
  value_out->reserved[5] = (uint8_t)(visible_vertex_attr_key_count & 0xFFU);
  value_out->reserved[6] = (uint8_t)((visible_vertex_attr_key_count >> 8U) & 0xFFU);
  value_out->as.ref_value = hypergraph_value;
  (void)visible_hyperedge_attr_key_count;
  return GINT_OK;
}

static int parse_struct_field_line(const char *text,
                                   runtime_struct_builder *builder,
                                   graphion_runtime_scope *scope,
                                   unsigned int line,
                                   graphion_runtime_diagnostic *diagnostic) {
  const char *cursor = text;
  graphion_struct_field_value *field;
  size_t i;
  int rc;

  if (text == NULL || builder == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  if (builder->field_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
    return fail(diagnostic, line, 1U, "too many struct fields", GINT_ERR_CAPACITY);
  }
  field = &builder->fields[builder->field_count];
  rc = parse_identifier_token(&cursor, field->name, sizeof(field->name), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  for (i = 0U; i < builder->field_count; ++i) {
    if (strcmp(builder->fields[i].name, field->name) == 0) {
      return fail(diagnostic, line, 1U, "duplicate struct field", GINT_ERR_PARSE);
    }
  }
  skip_spaces(&cursor);
  if (*cursor != ':') {
    return fail(diagnostic, line, 1U, "expected ':' after struct field name", GINT_ERR_PARSE);
  }
  cursor++;
  rc = parse_identifier_token(&cursor, field->type_name, sizeof(field->type_name), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  if (!runtime_struct_type_name_is_supported(field->type_name, scope)) {
    return fail(diagnostic, line, 1U, "unsupported struct field type", GINT_ERR_PARSE);
  }
  skip_spaces(&cursor);
  if (*cursor == '=') {
    cursor++;
    vm_value_set_none(&field->default_value);
    rc = evaluate_expression_text_to_value(cursor, strlen(cursor), scope, line, diagnostic, &field->default_value);
    if (rc != GINT_OK) {
      return rc;
    }
    if (!runtime_value_matches_struct_type(&field->default_value, field->type_name)) {
      vm_value_dispose_owned(&field->default_value);
      return fail(diagnostic, line, 1U, "struct field default has wrong type", GINT_ERR_PARSE);
    }
    field->has_default = 1U;
  } else if (*cursor != '\0') {
    return fail(diagnostic, line, 1U, "unexpected trailing tokens after struct field", GINT_ERR_PARSE);
  }
  builder->field_count += 1U;
  return GINT_OK;
}

static int collect_struct_block(const runtime_source_line *lines,
                                size_t count,
                                size_t start_index,
                                unsigned int current_indent,
                                size_t *end_index_out,
                                runtime_struct_builder *builder,
                                graphion_runtime_scope *scope,
                                unsigned int line,
                                graphion_runtime_diagnostic *diagnostic) {
  size_t body_start;
  size_t body_end;
  unsigned int body_indent;
  size_t i;

  if (lines == NULL || end_index_out == NULL || builder == NULL || scope == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  runtime_struct_builder_init(builder);
  body_start = find_next_nonblank_line(lines, count, start_index + 1U);
  if (body_start >= count || lines[body_start].indent <= current_indent) {
    return fail(diagnostic, line, 1U, "expected indented struct field block", GINT_ERR_PARSE);
  }
  body_indent = lines[body_start].indent;
  body_end = scan_block_end(lines, count, body_start, body_indent);
  for (i = body_start; i < body_end; ++i) {
    int rc;
    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    rc = parse_struct_field_line(line_content(&lines[i]), builder, scope, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  if (builder->field_count == 0U) {
    return fail(diagnostic, line, 1U, "struct requires at least one field", GINT_ERR_PARSE);
  }
  *end_index_out = body_end - 1U;
  return GINT_OK;
}

static int collect_graph_block(const runtime_source_line *lines,
                               size_t count,
                               size_t start_index,
                               unsigned int current_indent,
                               size_t *end_index_out,
                               runtime_graph_builder *builder,
                               graphion_runtime_scope *scope,
                               unsigned int line,
                               graphion_runtime_diagnostic *diagnostic) {
  size_t body_start;
  size_t body_end;
  unsigned int body_indent;
  size_t i;

  if (lines == NULL || end_index_out == NULL || builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  runtime_graph_builder_init(builder);

  body_start = find_next_nonblank_line(lines, count, start_index + 1U);
  if (body_start >= count || lines[body_start].indent <= current_indent) {
    return fail(diagnostic, line, 1U, "expected indented graph node block", GINT_ERR_PARSE);
  }
  body_indent = lines[body_start].indent;
  body_end = scan_block_end(lines, count, body_start, body_indent);

  for (i = body_start; i < body_end; ++i) {
    int rc;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (!graph_block_line_is_defaults(line_content(&lines[i]))) {
      continue;
    }
    rc = parse_graph_attr_defaults_line(line_content(&lines[i]), builder, scope, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }

  for (i = body_start; i < body_end; ++i) {
    int rc;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (graph_block_line_is_defaults(line_content(&lines[i]))) {
      continue;
    }
    rc = parse_graph_block_line(line_content(&lines[i]), builder, 1, scope, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }

  for (i = body_start; i < body_end; ++i) {
    int rc;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (graph_block_line_is_defaults(line_content(&lines[i]))) {
      continue;
    }
    rc = parse_graph_block_line(line_content(&lines[i]), builder, 0, scope, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = apply_graph_node_attr_defaults(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = apply_graph_edge_attr_defaults(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = validate_graph_node_attr_schema(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = validate_graph_edge_attr_schema(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  *end_index_out = body_end - 1U;
  return GINT_OK;
}

static int collect_hypergraph_block(const runtime_source_line *lines,
                                    size_t count,
                                    size_t start_index,
                                    unsigned int current_indent,
                                    size_t *end_index_out,
                                    runtime_graph_builder *builder,
                                    graphion_runtime_scope *scope,
                                    unsigned int line,
                                    graphion_runtime_diagnostic *diagnostic) {
  size_t body_start;
  size_t body_end;
  unsigned int body_indent;
  size_t i;

  if (lines == NULL || end_index_out == NULL || builder == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  runtime_graph_builder_init(builder);

  body_start = find_next_nonblank_line(lines, count, start_index + 1U);
  if (body_start >= count || lines[body_start].indent <= current_indent) {
    return fail(diagnostic, line, 1U, "expected indented hypergraph vertex block", GINT_ERR_PARSE);
  }
  body_indent = lines[body_start].indent;
  body_end = scan_block_end(lines, count, body_start, body_indent);

  for (i = body_start; i < body_end; ++i) {
    int rc;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (!graph_block_line_is_defaults(line_content(&lines[i]))) {
      continue;
    }
    rc = parse_hypergraph_attr_defaults_line(line_content(&lines[i]), builder, scope, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }

  for (i = body_start; i < body_end; ++i) {
    int rc;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (graph_block_line_is_defaults(line_content(&lines[i]))) {
      continue;
    }
    rc = parse_hypergraph_block_line(line_content(&lines[i]), builder, 1, scope, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }

  for (i = body_start; i < body_end; ++i) {
    int rc;

    if (line_is_blank(&lines[i])) {
      continue;
    }
    if (lines[i].indent != body_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (graph_block_line_is_defaults(line_content(&lines[i]))) {
      continue;
    }
    rc = parse_hypergraph_block_line(line_content(&lines[i]), builder, 0, scope, lines[i].line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = apply_hypergraph_vertex_attr_defaults(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = validate_hypergraph_vertex_attr_schema(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = apply_hypergraph_hyperedge_attr_defaults(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  {
    const int rc = validate_hypergraph_hyperedge_attr_schema(builder, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
  }
  *end_index_out = body_end - 1U;
  return GINT_OK;
}

static int execute_graph_block_declaration(const char *statement_source,
                                           const runtime_graph_builder *builder,
                                           graphion_runtime_scope *scope,
                                           unsigned int line,
                                           graphion_runtime_diagnostic *diagnostic) {
  char target[GRAPHION_RUNTIME_NAME_MAX];
  graphion_vm_value graph_value;
  int existing;
  size_t target_index;
  int rc;

  vm_value_set_none(&graph_value);
  rc = parse_graph_name_from_header(statement_source, target, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = build_runtime_graph_value(builder, &graph_value, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  existing = scope_find_global_index(scope, target);
  if (existing >= 0) {
    target_index = (size_t)existing;
  } else {
    rc = graphion_runtime_scope_reserve_globals(scope, scope->global_count + 1U, line, diagnostic);
    if (rc != GINT_OK) {
      vm_value_dispose_owned(&graph_value);
      return rc;
    }
    target_index = scope->global_count;
    copy_name(scope->global_names[target_index], target);
    scope->global_count += 1U;
  }
  runtime_free_string(&scope->owned_string_values[target_index]);
  if (scope->globals[target_index].kind == GVM_VALUE_LIST || scope->globals[target_index].kind == GVM_VALUE_DICT ||
      scope->globals[target_index].kind == GVM_VALUE_TUPLE || scope->globals[target_index].kind == GVM_VALUE_SET ||
      scope->globals[target_index].kind == GVM_VALUE_GRAPH_REF ||
      scope->globals[target_index].kind == GVM_VALUE_HYPERGRAPH_REF ||
      scope->globals[target_index].kind == GVM_VALUE_STRUCT_TYPE || scope->globals[target_index].kind == GVM_VALUE_STRUCT) {
    vm_value_dispose_owned(&scope->globals[target_index]);
  } else {
    vm_value_set_none(&scope->globals[target_index]);
  }
  scope->globals[target_index] = graph_value;
  return GINT_OK;
}

static int runtime_scope_set_owned_value(graphion_runtime_scope *scope,
                                         const char *name,
                                         graphion_vm_value *value,
                                         unsigned int line,
                                         graphion_runtime_diagnostic *diagnostic) {
  int existing;
  size_t target_index;
  int rc;

  if (scope == NULL || name == NULL || value == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }
  existing = scope_find_global_index(scope, name);
  if (existing >= 0) {
    target_index = (size_t)existing;
  } else {
    rc = graphion_runtime_scope_reserve_globals(scope, scope->global_count + 1U, line, diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    target_index = scope->global_count;
    copy_name(scope->global_names[target_index], name);
    scope->global_count += 1U;
  }
  runtime_free_string(&scope->owned_string_values[target_index]);
  vm_value_dispose_owned(&scope->globals[target_index]);
  scope->globals[target_index] = *value;
  vm_value_set_none(value);
  return GINT_OK;
}

static int execute_hypergraph_block_declaration(const char *statement_source,
                                                const runtime_graph_builder *builder,
                                                graphion_runtime_scope *scope,
                                                unsigned int line,
                                                graphion_runtime_diagnostic *diagnostic) {
  char target[GRAPHION_RUNTIME_NAME_MAX];
  graphion_vm_value hypergraph_value;
  int existing;
  size_t target_index;
  int rc;

  vm_value_set_none(&hypergraph_value);
  rc = parse_hypergraph_name_from_header(statement_source, target, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = build_runtime_hypergraph_value(builder, &hypergraph_value, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  existing = scope_find_global_index(scope, target);
  if (existing >= 0) {
    target_index = (size_t)existing;
  } else {
    rc = graphion_runtime_scope_reserve_globals(scope, scope->global_count + 1U, line, diagnostic);
    if (rc != GINT_OK) {
      vm_value_dispose_owned(&hypergraph_value);
      return rc;
    }
    target_index = scope->global_count;
    copy_name(scope->global_names[target_index], target);
    scope->global_count += 1U;
  }
  runtime_free_string(&scope->owned_string_values[target_index]);
  if (scope->globals[target_index].kind == GVM_VALUE_LIST || scope->globals[target_index].kind == GVM_VALUE_DICT ||
      scope->globals[target_index].kind == GVM_VALUE_TUPLE || scope->globals[target_index].kind == GVM_VALUE_SET ||
      scope->globals[target_index].kind == GVM_VALUE_GRAPH_REF ||
      scope->globals[target_index].kind == GVM_VALUE_HYPERGRAPH_REF ||
      scope->globals[target_index].kind == GVM_VALUE_STRUCT_TYPE || scope->globals[target_index].kind == GVM_VALUE_STRUCT) {
    vm_value_dispose_owned(&scope->globals[target_index]);
  } else {
    vm_value_set_none(&scope->globals[target_index]);
  }
  scope->globals[target_index] = hypergraph_value;
  return GINT_OK;
}

static int execute_struct_block_declaration(const char *statement_source,
                                            const runtime_struct_builder *builder,
                                            graphion_runtime_scope *scope,
                                            unsigned int line,
                                            graphion_runtime_diagnostic *diagnostic) {
  char target[GRAPHION_RUNTIME_NAME_MAX];
  graphion_vm_value struct_value;
  int rc;

  vm_value_set_none(&struct_value);
  rc = parse_struct_name_from_header(statement_source, target, line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = vm_value_set_struct_type(&struct_value, target, builder->fields, builder->field_count);
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to create struct type", GINT_ERR_CAPACITY);
  }
  return runtime_scope_set_owned_value(scope, target, &struct_value, line, diagnostic);
}

static int execute_struct_instance_assignment(const char *statement_source,
                                              graphion_runtime_scope *scope,
                                              unsigned int line,
                                              graphion_runtime_diagnostic *diagnostic,
                                              int *handled_out) {
  const char *cursor = statement_source;
  const char *dict_start;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  char type_name[GRAPHION_RUNTIME_NAME_MAX];
  const graphion_runtime_value *type_value;
  graphion_vm_value overrides;
  graphion_vm_value instance;
  int rc;

  if (handled_out != NULL) {
    *handled_out = 0;
  }
  vm_value_set_none(&overrides);
  vm_value_set_none(&instance);
  rc = parse_identifier_token(&cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&cursor);
  if (*cursor != '=') {
    return GINT_OK;
  }
  cursor++;
  skip_spaces(&cursor);
  if (!is_ident_start_char(*cursor)) {
    return GINT_OK;
  }
  rc = parse_identifier_token(&cursor, type_name, sizeof(type_name), line, diagnostic);
  if (rc != GINT_OK) {
    return GINT_OK;
  }
  type_value = graphion_runtime_scope_find(scope, type_name);
  if (type_value == NULL || type_value->kind != GVM_VALUE_STRUCT_TYPE) {
    return GINT_OK;
  }
  skip_spaces(&cursor);
  if (*cursor != '{') {
    return fail(diagnostic, line, 1U, "expected struct instance field dictionary", GINT_ERR_PARSE);
  }
  dict_start = cursor;
  rc = evaluate_expression_text_to_value(dict_start, strlen(dict_start), scope, line, diagnostic, &overrides);
  if (rc != GINT_OK) {
    return rc;
  }
  if (overrides.kind != GVM_VALUE_DICT) {
    vm_value_dispose_owned(&overrides);
    return fail(diagnostic, line, 1U, "struct instance fields must be a dict literal", GINT_ERR_PARSE);
  }
  rc = vm_value_instantiate_struct(&instance, type_value, &overrides);
  vm_value_dispose_owned(&overrides);
  if (rc == GVM_ERR_MISSING_KEY) {
    return fail(diagnostic, line, 1U, "missing or unknown struct field", GINT_ERR_RUN);
  }
  if (rc == GVM_ERR_TYPE_MISMATCH) {
    return fail(diagnostic, line, 1U, "struct field value has wrong type", GINT_ERR_RUN);
  }
  if (rc != GVM_OK) {
    return fail(diagnostic, line, 1U, "failed to create struct instance", GINT_ERR_CAPACITY);
  }
  if (is_reserved_name(target)) {
    vm_value_dispose_owned(&instance);
    return fail(diagnostic, line, 1U, "reserved name cannot be assigned", GINT_ERR_RESERVED_NAME);
  }
  rc = runtime_scope_set_owned_value(scope, target, &instance, line, diagnostic);
  if (rc != GINT_OK) {
    vm_value_dispose_owned(&instance);
    return rc;
  }
  if (handled_out != NULL) {
    *handled_out = 1;
  }
  return GINT_OK;
}

static int collect_assignment_statement_text(const runtime_source_line *lines,
                                             size_t count,
                                             size_t start_index,
                                             char *buffer,
                                             size_t buffer_size,
                                             size_t *end_index_out,
                                             unsigned int line,
                                             graphion_runtime_diagnostic *diagnostic) {
  const runtime_source_line *start_line;
  const char *cursor;
  const char *rhs_cursor;
  char target[GRAPHION_RUNTIME_NAME_MAX];
  size_t write_index = 0U;
  size_t i;
  int depth = 0;
  int in_string = 0;
  int multiline_allowed = 0;
  int saw_nonblank_continuation = 0;
  int rc;

  if (lines == NULL || start_index >= count || buffer == NULL || buffer_size == 0U || end_index_out == NULL) {
    return fail(diagnostic, line, 1U, "invalid runtime argument", GINT_ERR_INVALID_ARG);
  }

  start_line = &lines[start_index];
  cursor = line_content(start_line);
  rhs_cursor = cursor;
  rc = parse_identifier_token(&rhs_cursor, target, sizeof(target), line, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  skip_spaces(&rhs_cursor);
  if (*rhs_cursor == '[') {
    int bracket_depth = 0;
    int bracket_in_string = 0;
    do {
      if (*rhs_cursor == '\0') {
        return fail(diagnostic, line, 1U, "expected ']' after assignment target index", GINT_ERR_PARSE);
      }
      if (bracket_in_string) {
        if (*rhs_cursor == '"') {
          bracket_in_string = 0;
        }
      } else if (*rhs_cursor == '"') {
        bracket_in_string = 1;
      } else if (*rhs_cursor == '[') {
        bracket_depth++;
      } else if (*rhs_cursor == ']') {
        bracket_depth--;
      }
      rhs_cursor++;
    } while (bracket_depth > 0);
    skip_spaces(&rhs_cursor);
  }
  if (rhs_cursor[0] == '*' && rhs_cursor[1] == '*' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if (rhs_cursor[0] == '/' && rhs_cursor[1] == '/' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if (rhs_cursor[0] == '<' && rhs_cursor[1] == '<' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if (rhs_cursor[0] == '>' && rhs_cursor[1] == '>' && rhs_cursor[2] == '=') {
    rhs_cursor += 3;
  } else if ((rhs_cursor[0] == '+' || rhs_cursor[0] == '-' || rhs_cursor[0] == '*' || rhs_cursor[0] == '/' ||
              rhs_cursor[0] == '%' || rhs_cursor[0] == '&' || rhs_cursor[0] == '|' || rhs_cursor[0] == '^') &&
             rhs_cursor[1] == '=') {
    rhs_cursor += 2;
  } else if (*rhs_cursor == '=') {
    rhs_cursor++;
  } else {
    return fail(diagnostic, line, 1U, "expected '='", GINT_ERR_PARSE);
  }
  skip_spaces(&rhs_cursor);
  multiline_allowed = *rhs_cursor == '(' ? 1 : 0;

  for (i = start_index; i < count; ++i) {
    const char *scan = i == start_index ? cursor : line_content(&lines[i]);

    if (i > start_index) {
      if (line_is_blank(&lines[i])) {
        continue;
      }
      saw_nonblank_continuation = 1;
      if (!multiline_allowed) {
        return fail(diagnostic, line, 1U, "multiline assignment expression requires grouping parentheses", GINT_ERR_PARSE);
      }
      if (write_index > 0U && buffer[write_index - 1U] != ' ') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = ' ';
      }
    }

    while (*scan != '\0') {
      if (in_string) {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (*scan == '"') {
          in_string = 0;
        }
        scan++;
        continue;
      }
      if (*scan == '"') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        in_string = 1;
        scan++;
        continue;
      }
      if (*scan == '(') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        depth++;
        scan++;
        continue;
      }
      if (*scan == ')') {
        if (write_index + 1U >= buffer_size) {
          return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
        }
        buffer[write_index++] = *scan;
        if (depth > 0) {
          depth--;
        }
        scan++;
        continue;
      }
      if (write_index + 1U >= buffer_size) {
        return fail(diagnostic, line, 1U, "source line too long", GINT_ERR_CAPACITY);
      }
      buffer[write_index++] = *scan;
      scan++;
    }

    while (write_index > 0U &&
           (buffer[write_index - 1U] == ' ' || buffer[write_index - 1U] == '\t' || buffer[write_index - 1U] == '\r')) {
      write_index--;
    }
    if (depth == 0) {
      if (i == start_index && ternary_line_looks_incomplete(buffer, write_index) &&
          find_next_nonblank_line(lines, count, i + 1U) < count) {
        return fail(diagnostic, line, 1U, "multiline assignment expression requires grouping parentheses", GINT_ERR_PARSE);
      }
      buffer[write_index] = '\0';
      *end_index_out = i;
      return GINT_OK;
    }
  }

  if (!saw_nonblank_continuation) {
    buffer[write_index] = '\0';
    *end_index_out = start_index;
    return GINT_OK;
  }
  return fail(diagnostic, line, 1U, "expected ')' after expression", GINT_ERR_PARSE);
}

static int execute_statement_source_line(const runtime_source_line *lines,
                                         size_t count,
                                         size_t *index,
                                         graphion_runtime_scope *scope,
                                         graphion_runtime_diagnostic *diagnostic,
                                         FILE *output) {
  char statement_text[512];
  const char *statement_source = line_content(&lines[*index]);
  graphion_runtime_program program;
  size_t statement_end = *index;
  runtime_graph_builder graph_builder;
  runtime_struct_builder struct_builder;
  int graph_block_declaration = 0;
  int hypergraph_block_declaration = 0;
  int struct_block_declaration = 0;
  int rc;

  runtime_graph_builder_init(&graph_builder);
  runtime_struct_builder_init(&struct_builder);
  if (strncmp(statement_source, "graph", 5U) == 0 && !is_ident_char(statement_source[5]) &&
      graph_header_ends_with_colon(statement_source)) {
    graph_block_declaration = 1;
    rc = collect_graph_block(lines,
                             count,
                             *index,
                             lines[*index].indent,
                             &statement_end,
                             &graph_builder,
                             scope,
                             lines[*index].line,
                             diagnostic);
    if (rc != GINT_OK) {
      runtime_graph_builder_dispose(&graph_builder);
      return rc;
    }
  } else if (strncmp(statement_source, "hypergraph", 10U) == 0 && !is_ident_char(statement_source[10]) &&
             hypergraph_header_ends_with_colon(statement_source)) {
    hypergraph_block_declaration = 1;
    rc = collect_hypergraph_block(lines,
                                  count,
                                  *index,
                                  lines[*index].indent,
                                  &statement_end,
                                  &graph_builder,
                                  scope,
                                  lines[*index].line,
                                  diagnostic);
    if (rc != GINT_OK) {
      runtime_graph_builder_dispose(&graph_builder);
      return rc;
    }
  } else if (strncmp(statement_source, "struct", 6U) == 0 && !is_ident_char(statement_source[6]) &&
             struct_header_ends_with_colon(statement_source)) {
    struct_block_declaration = 1;
    rc = collect_struct_block(lines,
                              count,
                              *index,
                              lines[*index].indent,
                              &statement_end,
                              &struct_builder,
                              scope,
                              lines[*index].line,
                              diagnostic);
    if (rc != GINT_OK) {
      runtime_struct_builder_dispose(&struct_builder);
      return rc;
    }
  } else if (!(strncmp(statement_source, "print", 5U) == 0 && !is_ident_char(statement_source[5])) &&
             !(strncmp(statement_source, "struct", 6U) == 0 && !is_ident_char(statement_source[6])) &&
             !(strncmp(statement_source, "hypergraph", 10U) == 0 && !is_ident_char(statement_source[10])) &&
             !(strncmp(statement_source, "graph", 5U) == 0 && !is_ident_char(statement_source[5])) &&
             !(strncmp(statement_source, "add_node", 8U) == 0 && !is_ident_char(statement_source[8])) &&
             !(strncmp(statement_source, "add_edge", 8U) == 0 && !is_ident_char(statement_source[8])) &&
             !(strncmp(statement_source, "add_vertex", 10U) == 0 && !is_ident_char(statement_source[10])) &&
             !(strncmp(statement_source, "add_hyperedge", 13U) == 0 && !is_ident_char(statement_source[13])) &&
             !(strncmp(statement_source, "remove_vertex", 13U) == 0 && !is_ident_char(statement_source[13])) &&
             !(strncmp(statement_source, "remove_hyperedge", 16U) == 0 && !is_ident_char(statement_source[16])) &&
             !(strncmp(statement_source, "remove_node", 11U) == 0 && !is_ident_char(statement_source[11])) &&
             !(strncmp(statement_source, "remove_edge", 11U) == 0 && !is_ident_char(statement_source[11])) &&
             !(strncmp(statement_source, "set_node_attrs", 14U) == 0 && !is_ident_char(statement_source[14])) &&
             !(strncmp(statement_source, "set_edge_attrs", 14U) == 0 && !is_ident_char(statement_source[14])) &&
             !(strncmp(statement_source, "set_edge_weight", 15U) == 0 && !is_ident_char(statement_source[15])) &&
             !(strncmp(statement_source, "set_vertex_attrs", 16U) == 0 && !is_ident_char(statement_source[16])) &&
             !(strncmp(statement_source, "set_hyperedge_attrs", 19U) == 0 && !is_ident_char(statement_source[19]))) {
    rc = collect_assignment_statement_text(lines,
                                           count,
                                           *index,
                                           statement_text,
                                           sizeof(statement_text),
                                           &statement_end,
                                           lines[*index].line,
                                           diagnostic);
    if (rc != GINT_OK) {
      return rc;
    }
    statement_source = statement_text;
  }
  if (graph_block_declaration) {
    rc = execute_graph_block_declaration(statement_source, &graph_builder, scope, lines[*index].line, diagnostic);
    runtime_graph_builder_dispose(&graph_builder);
    if (rc != GINT_OK) {
      return rc;
    }
    *index = statement_end + 1U;
    return GINT_OK;
  }
  if (hypergraph_block_declaration) {
    rc = execute_hypergraph_block_declaration(statement_source, &graph_builder, scope, lines[*index].line, diagnostic);
    runtime_graph_builder_dispose(&graph_builder);
    if (rc != GINT_OK) {
      return rc;
    }
    *index = statement_end + 1U;
    return GINT_OK;
  }
  if (struct_block_declaration) {
    rc = execute_struct_block_declaration(statement_source, &struct_builder, scope, lines[*index].line, diagnostic);
    runtime_struct_builder_dispose(&struct_builder);
    if (rc != GINT_OK) {
      return rc;
    }
    *index = statement_end + 1U;
    return GINT_OK;
  }
  {
    int struct_assignment_handled = 0;
    rc = execute_struct_instance_assignment(statement_source, scope, lines[*index].line, diagnostic, &struct_assignment_handled);
    if (rc != GINT_OK) {
      return rc;
    }
    if (struct_assignment_handled) {
      *index = statement_end + 1U;
      return GINT_OK;
    }
  }
  rc = seed_program_from_scope(&program, scope, lines[*index].line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  rc = parse_statement_line(statement_source, scope, &program, lines[*index].line, diagnostic);
  if (rc != GINT_OK) {
    graphion_runtime_program_dispose(&program);
    return rc;
  }
  if (program.program_len > 0U) {
    rc = graphion_execute_program(&program, scope, diagnostic, output);
    graphion_runtime_program_dispose(&program);
    if (rc != GINT_OK) {
      return rc;
    }
  } else {
    graphion_runtime_program_dispose(&program);
  }
  *index = statement_end + 1U;
  return GINT_OK;
}

int execute_block(const runtime_source_line *lines,
                  size_t count,
                  size_t *index,
                  unsigned int block_indent,
                  graphion_runtime_scope *scope,
                  graphion_runtime_diagnostic *diagnostic,
                  FILE *output);

static int execute_if_chain(const runtime_source_line *lines,
                            size_t count,
                            size_t *index,
                            unsigned int current_indent,
                            graphion_runtime_scope *scope,
                            graphion_runtime_diagnostic *diagnostic,
                            FILE *output) {
  size_t clause_index = *index;
  int branch_taken = 0;
  int seen_else = 0;
  int first_clause = 1;
  while (clause_index < count) {
    const runtime_source_line *clause_line;
    const char *cursor;
    size_t body_start;
    size_t body_end;
    unsigned int body_indent;
    int is_else_clause;
    if (line_is_blank(&lines[clause_index])) {
      clause_index++;
      continue;
    }
    if (lines[clause_index].indent != current_indent) {
      break;
    }
    clause_line = &lines[clause_index];
    cursor = line_content(clause_line);
    is_else_clause = line_is_else_clause(clause_line);
    if (!line_is_if_clause(clause_line) && !line_is_elif_clause(clause_line) && !is_else_clause) {
      break;
    }
    if (!first_clause && line_is_if_clause(clause_line)) {
      break;
    }
    if (seen_else && line_is_if_clause(clause_line)) {
      break;
    }
    if (seen_else && (line_is_elif_clause(clause_line) || is_else_clause)) {
      return fail(diagnostic, clause_line->line, 1U, "else must be last in if chain", GINT_ERR_PARSE);
    }
    if (is_else_clause) {
      int rc = parse_else_header(cursor, clause_line->line, diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      body_start = find_next_nonblank_line(lines, count, clause_index + 1U);
      if (body_start >= count || lines[body_start].indent <= current_indent) {
        return fail(diagnostic, clause_line->line, 1U, "expected indented block after else", GINT_ERR_PARSE);
      }
      body_indent = lines[body_start].indent;
      body_end = scan_block_end(lines, count, body_start, body_indent);
      if (body_end < count && !line_is_blank(&lines[body_end]) && lines[body_end].indent > current_indent &&
          lines[body_end].indent < body_indent) {
        return fail(diagnostic, lines[body_end].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
      }
      seen_else = 1;
      if (!branch_taken) {
        size_t exec_index = body_start;
        rc = execute_block(lines, count, &exec_index, body_indent, scope, diagnostic, output);
        if (rc != GINT_OK) {
          return rc;
        }
        body_end = exec_index;
        branch_taken = 1;
      }
    } else {
      char condition_text[512];
      size_t header_end_index = clause_index;
      int rc = collect_control_condition_text(lines,
                                              count,
                                              clause_index,
                                              line_is_elif_clause(clause_line) ? "elif" : "if",
                                              condition_text,
                                              sizeof(condition_text),
                                              &header_end_index,
                                              clause_line->line,
                                              diagnostic);
      if (rc != GINT_OK) {
        return rc;
      }
      body_start = find_next_nonblank_line(lines, count, header_end_index + 1U);
      if (body_start >= count || lines[body_start].indent <= current_indent) {
        return fail(diagnostic,
                    clause_line->line,
                    1U,
                    line_is_elif_clause(clause_line) ? "expected indented block after elif" :
                    "expected indented block after if",
                    GINT_ERR_PARSE);
      }
      body_indent = lines[body_start].indent;
      body_end = scan_block_end(lines, count, body_start, body_indent);
      if (body_end < count && !line_is_blank(&lines[body_end]) && lines[body_end].indent > current_indent &&
          lines[body_end].indent < body_indent) {
        return fail(diagnostic, lines[body_end].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
      }
      if (!branch_taken) {
        int condition_true = 0;
        rc = evaluate_condition_text(condition_text,
                                     strlen(condition_text),
                                     scope,
                                     clause_line->line,
                                     diagnostic,
                                     &condition_true);
        if (rc != GINT_OK) {
          return rc;
        }
        if (condition_true) {
          size_t exec_index = body_start;
          rc = execute_block(lines, count, &exec_index, body_indent, scope, diagnostic, output);
          if (rc != GINT_OK) {
            return rc;
          }
          body_end = exec_index;
          branch_taken = 1;
        }
      }
    }
    clause_index = body_end;
    first_clause = 0;
  }
  *index = clause_index;
  return GINT_OK;
}

static int execute_match_statement(const runtime_source_line *lines,
                                   size_t count,
                                   size_t *index,
                                   unsigned int current_indent,
                                   graphion_runtime_scope *scope,
                                   graphion_runtime_diagnostic *diagnostic,
                                   FILE *output) {
  char match_expression[512];
  graphion_vm_value match_value = {GVM_VALUE_NONE, {0}, {0}};
  runtime_match_case_value seen_cases[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t seen_case_count = 0U;
  size_t header_end_index = *index;
  size_t clause_index;
  unsigned int branch_indent;
  int branch_taken = 0;
  int seen_default = 0;
  int rc;
  size_t i;

  for (i = 0U; i < GRAPHION_RUNTIME_WARNING_MAX; ++i) {
    runtime_match_case_value_init(&seen_cases[i]);
  }

  rc = collect_match_expression_text(lines,
                                     count,
                                     *index,
                                     match_expression,
                                     sizeof(match_expression),
                                     &header_end_index,
                                     lines[*index].line,
                                     diagnostic);
  if (rc != GINT_OK) {
    goto cleanup;
  }

  rc = evaluate_expression_text_to_value(match_expression,
                                         strlen(match_expression),
                                         scope,
                                         lines[*index].line,
                                         diagnostic,
                                         &match_value);
  if (rc != GINT_OK) {
    goto cleanup;
  }

  clause_index = find_next_nonblank_line(lines, count, header_end_index + 1U);
  if (clause_index >= count || lines[clause_index].indent <= current_indent) {
    rc = fail(diagnostic, lines[*index].line, 1U, "expected indented match block", GINT_ERR_PARSE);
    goto cleanup;
  }
  branch_indent = lines[clause_index].indent;

  while (clause_index < count) {
    size_t label_start = clause_index;
    size_t label_index = clause_index;
    size_t body_start;
    size_t body_end;
    int label_matches = 0;
    int is_default = 0;

    if (line_is_blank(&lines[clause_index])) {
      clause_index++;
      continue;
    }
    if (lines[clause_index].indent < branch_indent) {
      break;
    }
    if (lines[clause_index].indent > branch_indent) {
      rc = fail(diagnostic, lines[clause_index].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
      goto cleanup;
    }

    if (line_is_default_clause(&lines[clause_index])) {
      is_default = 1;
      if (seen_default) {
        rc = fail(diagnostic, lines[clause_index].line, 1U, "default can only appear once", GINT_ERR_PARSE);
        goto cleanup;
      }
      rc = parse_default_header(line_content(&lines[clause_index]), lines[clause_index].line, diagnostic);
      if (rc != GINT_OK) {
        goto cleanup;
      }
      seen_default = 1;
      label_index = clause_index + 1U;
    } else {
      while (label_index < count) {
        runtime_match_case_value case_value;
        graphion_vm_value parsed_value;
        int compatible = 0;
        int equal = 0;
        size_t existing_index;

        runtime_match_case_value_init(&case_value);
        if (line_is_blank(&lines[label_index])) {
          label_index++;
          continue;
        }
        if (lines[label_index].indent != branch_indent || line_is_default_clause(&lines[label_index])) {
          break;
        }
        rc = parse_match_case_header(line_content(&lines[label_index]),
                                     &parsed_value,
                                     &case_value,
                                     lines[label_index].line,
                                     diagnostic);
        if (rc != GINT_OK) {
          runtime_match_case_value_dispose(&case_value);
          goto cleanup;
        }
        for (existing_index = 0U; existing_index < seen_case_count; ++existing_index) {
          scalar_values_match_equal(&seen_cases[existing_index].value, &parsed_value, &compatible, &equal);
          if (compatible && equal) {
            runtime_match_case_value_dispose(&case_value);
            rc = fail(diagnostic, lines[label_index].line, 1U, "duplicate match case", GINT_ERR_PARSE);
            goto cleanup;
          }
        }
        if (seen_case_count >= GRAPHION_RUNTIME_PROGRAM_MAX) {
          runtime_match_case_value_dispose(&case_value);
          rc = fail(diagnostic, lines[label_index].line, 1U, "too many match cases", GINT_ERR_CAPACITY);
          goto cleanup;
        }
        seen_cases[seen_case_count++] = case_value;
        scalar_values_match_equal(&match_value, &parsed_value, &compatible, &equal);
        if (compatible && equal) {
          label_matches = 1;
        }
        label_index++;
        if (label_index >= count || line_is_blank(&lines[label_index]) || lines[label_index].indent != branch_indent) {
          break;
        }
      }
      if (label_index == label_start) {
        rc = fail(diagnostic, lines[clause_index].line, 1U, "expected scalar literal", GINT_ERR_PARSE);
        goto cleanup;
      }
    }

    body_start = find_next_nonblank_line(lines, count, label_index);
    if (body_start >= count || lines[body_start].indent <= branch_indent) {
      rc = fail(diagnostic,
                lines[label_start].line,
                1U,
                is_default ? "expected indented block after default" : "expected indented block after match case",
                GINT_ERR_PARSE);
      goto cleanup;
    }
    body_end = scan_block_end(lines, count, body_start, lines[body_start].indent);

    if (is_default && find_next_nonblank_line(lines, count, body_end) < count &&
        lines[find_next_nonblank_line(lines, count, body_end)].indent == branch_indent) {
      rc = fail(diagnostic, lines[label_start].line, 1U, "default must be last in match", GINT_ERR_PARSE);
      goto cleanup;
    }

    if (!branch_taken && (is_default || label_matches)) {
      size_t exec_index = body_start;
      rc = execute_block(lines, count, &exec_index, lines[body_start].indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        goto cleanup;
      }
      body_end = exec_index;
      branch_taken = 1;
    }

    clause_index = body_end;
  }

  *index = clause_index;
  rc = GINT_OK;

cleanup:
  vm_value_dispose_owned(&match_value);
  for (i = 0U; i < seen_case_count; ++i) {
    runtime_match_case_value_dispose(&seen_cases[i]);
  }
  return rc;
}

int execute_block(const runtime_source_line *lines,
                  size_t count,
                  size_t *index,
                  unsigned int block_indent,
                  graphion_runtime_scope *scope,
                  graphion_runtime_diagnostic *diagnostic,
                  FILE *output) {
  size_t i = *index;
  while (i < count) {
    if (line_is_blank(&lines[i])) {
      i++;
      continue;
    }
    if (lines[i].indent < block_indent) {
      break;
    }
    if (lines[i].indent > block_indent) {
      return fail(diagnostic, lines[i].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    if (line_is_elif_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "elif without matching if", GINT_ERR_PARSE);
    }
    if (line_is_else_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "else without matching if", GINT_ERR_PARSE);
    }
    if (line_is_default_clause(&lines[i])) {
      return fail(diagnostic, lines[i].line, 1U, "default without matching match", GINT_ERR_PARSE);
    }
    if (line_is_if_clause(&lines[i])) {
      int rc = execute_if_chain(lines, count, &i, block_indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
      continue;
    }
    if (line_is_match_clause(&lines[i])) {
      int rc = execute_match_statement(lines, count, &i, block_indent, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
      continue;
    }
    {
      int rc = execute_statement_source_line(lines, count, &i, scope, diagnostic, output);
      if (rc != GINT_OK) {
        return rc;
      }
    }
  }
  *index = i;
  return GINT_OK;
}

int graphion_interpret_source_with_output(const char *source,
                                          graphion_runtime_scope *scope,
                                          graphion_runtime_diagnostic *diagnostic,
                                          FILE *output) {
  runtime_source_line lines[GRAPHION_RUNTIME_PROGRAM_MAX];
  size_t line_count = 0U;
  size_t index = 0U;
  int rc;
  if (source == NULL || scope == NULL) {
    clear_diagnostic(diagnostic);
    return GINT_ERR_INVALID_ARG;
  }
  clear_diagnostic(diagnostic);
  rc = split_source_lines(source, lines, GRAPHION_RUNTIME_PROGRAM_MAX, &line_count, diagnostic);
  if (rc != GINT_OK) {
    return rc;
  }
  rc = execute_block(lines, line_count, &index, 0U, scope, diagnostic, output);
  if (rc != GINT_OK) {
    return rc;
  }
  while (index < line_count) {
    if (!line_is_blank(&lines[index])) {
      return fail(diagnostic, lines[index].line, 1U, "unexpected indentation", GINT_ERR_PARSE);
    }
    index++;
  }
  return GINT_OK;
}
