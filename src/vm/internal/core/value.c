/* SPDX-License-Identifier: MIT */
// cppcheck-suppress-file constVariablePointer
// cppcheck-suppress-file variableScope

#include "vm/internal/core/value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t count;
  size_t capacity;
  graphion_vm_value *items;
} graphion_vm_list;

typedef struct {
  char *key;
  graphion_vm_value value;
} graphion_vm_dict_entry;

typedef struct {
  size_t count;
  size_t capacity;
  graphion_vm_dict_entry *entries;
} graphion_vm_dict;

int is_valid_reg(uint8_t reg) { return reg < 16U ? 1 : 0; }

int64_t wrap_add_i64(int64_t lhs, int64_t rhs) {
  const uint64_t ulhs = (uint64_t)lhs;
  const uint64_t urhs = (uint64_t)rhs;
  return (int64_t)(ulhs + urhs);
}

int64_t wrap_sub_i64(int64_t lhs, int64_t rhs) {
  const uint64_t ulhs = (uint64_t)lhs;
  const uint64_t urhs = (uint64_t)rhs;
  return (int64_t)(ulhs - urhs);
}

int64_t wrap_mul_i64(int64_t lhs, int64_t rhs) {
  const uint64_t ulhs = (uint64_t)lhs;
  const uint64_t urhs = (uint64_t)rhs;
  return (int64_t)(ulhs * urhs);
}

static char *vm_strdup_text(const char *text) {
  size_t len;
  char *copy;
  if (text == NULL) {
    return NULL;
  }
  len = strlen(text);
  copy = (char *)malloc(len + 1U);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, text, len + 1U);
  return copy;
}

static graphion_vm_list *vm_list_create(void) {
  graphion_vm_list *list = (graphion_vm_list *)calloc(1U, sizeof(*list));
  return list;
}

static graphion_vm_dict *vm_dict_create(void) {
  graphion_vm_dict *dict = (graphion_vm_dict *)calloc(1U, sizeof(*dict));
  return dict;
}

static graphion_struct_type_value *vm_struct_type_create(void) {
  return (graphion_struct_type_value *)calloc(1U, sizeof(graphion_struct_type_value));
}

static graphion_struct_instance_value *vm_struct_instance_create(void) {
  return (graphion_struct_instance_value *)calloc(1U, sizeof(graphion_struct_instance_value));
}

static graphion_csr_graph *vm_graph_create_empty(void) {
  graphion_graph_value *graph = (graphion_graph_value *)calloc(1U, sizeof(*graph));
  // cppcheck-suppress memleak
  return graph != NULL ? &graph->csr : NULL;
}

static graphion_hypergraph *vm_hypergraph_create_empty(void) {
  graphion_hypergraph_value *hypergraph = (graphion_hypergraph_value *)calloc(1U, sizeof(*hypergraph));
  // cppcheck-suppress memleak
  return hypergraph != NULL ? &hypergraph->hypergraph : NULL;
}

static graphion_csr_graph *vm_graph_create_nodes(size_t node_count) {
  graphion_csr_graph *graph;
  uint32_t *offsets;

  graph = vm_graph_create_empty();
  if (graph == NULL) {
    return NULL;
  }
  if (node_count == 0U) {
    return graph;
  }
  offsets = (uint32_t *)calloc(node_count + 1U, sizeof(*offsets));
  if (offsets == NULL) {
    free(graph);
    return NULL;
  }
  graph->node_count = node_count;
  graph->edge_count = 0U;
  graph->offsets = offsets;
  graph->neighbors = NULL;
  graph->weights = NULL;
  graph->edge_attrs = NULL;
  return graph;
}

static size_t vm_graph_visible_node_count(const graphion_vm_value *value, const graphion_csr_graph *graph) {
  size_t count;
  if (value == NULL) {
    return 0U;
  }
  count = (size_t)value->reserved[1] | ((size_t)value->reserved[2] << 8U);
  if (count != 0U) {
    return count;
  }
  return graph != NULL ? graph->node_count : 0U;
}

static size_t vm_graph_visible_edge_count(const graphion_vm_value *value, const graphion_csr_graph *graph) {
  const graphion_graph_value *graph_value;
  size_t count;
  if (value == NULL) {
    return 0U;
  }
  if (value->kind == GVM_VALUE_GRAPH_REF && value->as.ref_value != NULL) {
    graph_value = (const graphion_graph_value *)value->as.ref_value;
    if (graph_value->edge_count > 0U) {
      return graph_value->edge_count;
    }
  }
  count = (size_t)value->reserved[3] | ((size_t)value->reserved[4] << 8U);
  if (count != 0U) {
    return count;
  }
  if (graph == NULL) {
    return 0U;
  }
  return value->reserved[0] != 0U ? graph->edge_count : graph->edge_count / 2U;
}

static size_t vm_graph_visible_node_attr_key_count(const graphion_vm_value *value) {
  if (value == NULL) {
    return 0U;
  }
  return (size_t)value->reserved[5] | ((size_t)value->reserved[6] << 8U);
}

static size_t vm_graph_visible_edge_attr_key_count(const graphion_vm_value *value) {
  const graphion_graph_value *graph_value;
  size_t i;
  size_t len = 0U;

  if (value == NULL || value->kind != GVM_VALUE_GRAPH_REF || value->as.ref_value == NULL) {
    return 0U;
  }
  graph_value = (const graphion_graph_value *)value->as.ref_value;
  for (i = 0U; i < graph_value->edge_attr_count; ++i) {
    if (graph_value->edge_attrs[i].kind == GVM_VALUE_DICT &&
        vm_value_dict_length(&graph_value->edge_attrs[i], &len) && len > 0U) {
      return len;
    }
  }
  return 0U;
}

static size_t vm_hypergraph_visible_vertex_attr_key_count(const graphion_vm_value *value) {
  if (value == NULL) {
    return 0U;
  }
  return (size_t)value->reserved[5] | ((size_t)value->reserved[6] << 8U);
}

static size_t vm_hypergraph_visible_hyperedge_attr_key_count(const graphion_vm_value *value) {
  const graphion_hypergraph_value *hypergraph_value;
  size_t i;
  size_t len = 0U;

  if (value == NULL || value->kind != GVM_VALUE_HYPERGRAPH_REF || value->as.ref_value == NULL) {
    return 0U;
  }
  hypergraph_value = (const graphion_hypergraph_value *)value->as.ref_value;
  for (i = 0U; i < hypergraph_value->hyperedge_attr_count; ++i) {
    if (hypergraph_value->hyperedge_attrs[i].kind == GVM_VALUE_DICT &&
        vm_value_dict_length(&hypergraph_value->hyperedge_attrs[i], &len) && len > 0U) {
      return len;
    }
  }
  return 0U;
}

static size_t vm_hypergraph_active_hyperedge_count(const graphion_hypergraph *hypergraph) {
  size_t count = 0U;
  size_t i;

  if (hypergraph == NULL || hypergraph->hyperedge_offsets == NULL) {
    return 0U;
  }
  for (i = 0U; i < hypergraph->hyperedge_count; ++i) {
    if (hypergraph->hyperedge_offsets[i] < hypergraph->hyperedge_offsets[i + 1U]) {
      count += 1U;
    }
  }
  return count;
}

static void vm_value_clear(graphion_vm_value *value) {
  if (value == NULL) {
    return;
  }
  memset(value, 0, sizeof(*value));
  value->kind = GVM_VALUE_NONE;
}

void vm_value_set_int(graphion_vm_value *value, int64_t int_value) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
  value->kind = GVM_VALUE_INT;
  value->as.int_value = int_value;
}

void vm_value_set_float(graphion_vm_value *value, double float_value) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
  value->kind = GVM_VALUE_FLOAT;
  value->as.float_value = float_value;
}

void vm_value_set_bool(graphion_vm_value *value, int bool_value) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
  value->kind = GVM_VALUE_BOOL;
  value->as.bool_value = bool_value != 0 ? 1 : 0;
}

void vm_value_set_bits(graphion_vm_value *value, uint64_t bits_value, uint8_t width) {
  if (value == NULL) {
    return;
  }
  vm_value_clear(value);
  value->kind = GVM_VALUE_BITS;
  value->reserved[0] = width;
  value->as.int_value = (int64_t)bits_value;
}

void vm_value_copy(graphion_vm_value *dst, const graphion_vm_value *src) {
  if (dst == NULL || src == NULL) {
    return;
  }
  *dst = *src;
}

static int vm_list_append_value(graphion_vm_list *list, const graphion_vm_value *src);
static size_t vm_dict_find_index(const graphion_vm_dict *dict, const char *key);
int vm_dict_get_element(graphion_vm *vm, uint8_t dict_reg, uint8_t key_reg);

static int vm_value_is_sequence_kind(uint8_t kind) {
  return kind == GVM_VALUE_LIST || kind == GVM_VALUE_TUPLE;
}

static int vm_value_is_list_storage_kind(uint8_t kind) {
  return vm_value_is_sequence_kind(kind) || kind == GVM_VALUE_SET;
}

static int vm_value_is_compound_kind(uint8_t kind) {
  return vm_value_is_list_storage_kind(kind) || kind == GVM_VALUE_DICT || kind == GVM_VALUE_GRAPH_REF ||
         kind == GVM_VALUE_HYPERGRAPH_REF || kind == GVM_VALUE_STRUCT_TYPE || kind == GVM_VALUE_STRUCT;
}

void vm_value_dispose_owned(graphion_vm_value *value) {
  size_t i;
  graphion_vm_list *list;
  graphion_vm_dict *dict;
  graphion_csr_graph *graph;
  graphion_graph_value *graph_value;
  graphion_hypergraph *hypergraph;
  graphion_hypergraph_value *hypergraph_value;
  graphion_struct_type_value *struct_type;
  graphion_struct_instance_value *struct_instance;

  if (value == NULL) {
    return;
  }
  if (value->kind == GVM_VALUE_STRING && value->as.string_value != NULL) {
    free((void *)value->as.string_value);
  } else if (vm_value_is_list_storage_kind(value->kind)) {
    list = (graphion_vm_list *)value->as.ref_value;
    if (list != NULL) {
      for (i = 0U; i < list->count; ++i) {
        vm_value_dispose_owned(&list->items[i]);
      }
      free(list->items);
      free(list);
    }
  } else if (value->kind == GVM_VALUE_DICT) {
    dict = (graphion_vm_dict *)value->as.ref_value;
    if (dict != NULL) {
      for (i = 0U; i < dict->count; ++i) {
        free(dict->entries[i].key);
        vm_value_dispose_owned(&dict->entries[i].value);
      }
      free(dict->entries);
      free(dict);
    }
  } else if (value->kind == GVM_VALUE_GRAPH_REF) {
    graph = (graphion_csr_graph *)value->as.ref_value;
    if (graph != NULL) {
      graph_value = (graphion_graph_value *)value->as.ref_value;
      if (graph_value->nodes != NULL) {
        for (i = 0U; i < graph_value->node_count; ++i) {
          free((void *)graph_value->nodes[i].name);
        }
        free(graph_value->nodes);
      }
      if (graph_value->node_attrs != NULL) {
        for (i = 0U; i < graph_value->node_attr_count; ++i) {
          vm_value_dispose_owned(&graph_value->node_attrs[i]);
        }
        free(graph_value->node_attrs);
      }
      if (graph_value->edge_attrs != NULL) {
        for (i = 0U; i < graph_value->edge_attr_count; ++i) {
          vm_value_dispose_owned(&graph_value->edge_attrs[i]);
        }
        free(graph_value->edge_attrs);
      }
      free(graph_value->edges);
      free((void *)graph->offsets);
      free((void *)graph->neighbors);
      free((void *)graph->weights);
      free((void *)graph->edge_attrs);
    }
    free(graph);
  } else if (value->kind == GVM_VALUE_HYPERGRAPH_REF) {
    hypergraph = (graphion_hypergraph *)value->as.ref_value;
    if (hypergraph != NULL) {
      hypergraph_value = (graphion_hypergraph_value *)value->as.ref_value;
      if (hypergraph_value->vertices != NULL) {
        for (i = 0U; i < hypergraph_value->vertex_count; ++i) {
          free((void *)hypergraph_value->vertices[i].name);
        }
        free(hypergraph_value->vertices);
      }
      if (hypergraph_value->vertex_attrs != NULL) {
        for (i = 0U; i < hypergraph_value->vertex_attr_count; ++i) {
          vm_value_dispose_owned(&hypergraph_value->vertex_attrs[i]);
        }
        free(hypergraph_value->vertex_attrs);
      }
      if (hypergraph_value->hyperedge_attrs != NULL) {
        for (i = 0U; i < hypergraph_value->hyperedge_attr_count; ++i) {
          vm_value_dispose_owned(&hypergraph_value->hyperedge_attrs[i]);
        }
        free(hypergraph_value->hyperedge_attrs);
      }
      free((void *)hypergraph->node_offsets);
      free((void *)hypergraph->node_hyperedges);
      free((void *)hypergraph->hyperedge_offsets);
      free((void *)hypergraph->hyperedge_nodes);
      free(hypergraph);
    }
  } else if (value->kind == GVM_VALUE_STRUCT_TYPE) {
    struct_type = (graphion_struct_type_value *)value->as.ref_value;
    if (struct_type != NULL) {
      for (i = 0U; i < struct_type->field_count; ++i) {
        if (struct_type->fields[i].has_default) {
          vm_value_dispose_owned(&struct_type->fields[i].default_value);
        }
      }
      free(struct_type->fields);
      free(struct_type);
    }
  } else if (value->kind == GVM_VALUE_STRUCT) {
    struct_instance = (graphion_struct_instance_value *)value->as.ref_value;
    if (struct_instance != NULL) {
      vm_value_dispose_owned(&struct_instance->fields);
      free(struct_instance);
    }
  }
  vm_value_clear(value);
}

int vm_value_clone(graphion_vm_value *dst, const graphion_vm_value *src) {
  size_t i;
  graphion_vm_list *src_list;
  graphion_vm_list *dst_list;
  graphion_vm_dict *src_dict;
  graphion_vm_dict *dst_dict;
  graphion_csr_graph *src_graph;
  graphion_csr_graph *dst_graph;
  graphion_graph_value *src_graph_value;
  graphion_graph_value *dst_graph_value;
  graphion_hypergraph *src_hypergraph;
  graphion_hypergraph *dst_hypergraph;
  graphion_hypergraph_value *src_hypergraph_value;
  graphion_hypergraph_value *dst_hypergraph_value;
  graphion_struct_type_value *src_struct_type;
  graphion_struct_type_value *dst_struct_type;
  graphion_struct_instance_value *src_struct_instance;
  graphion_struct_instance_value *dst_struct_instance;

  if (dst == NULL || src == NULL) {
    return GVM_ERR_INVALID_ARG;
  }

  vm_value_clear(dst);
  if (src->kind == GVM_VALUE_STRING) {
    char *copy = vm_strdup_text(src->as.string_value != NULL ? src->as.string_value : "");
    if (copy == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    dst->kind = GVM_VALUE_STRING;
    dst->as.string_value = copy;
    return GVM_OK;
  }
  if (!vm_value_is_compound_kind(src->kind)) {
    *dst = *src;
    return GVM_OK;
  }

  if (vm_value_is_list_storage_kind(src->kind)) {
    src_list = (graphion_vm_list *)src->as.ref_value;
    dst_list = vm_list_create();
    if (dst_list == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    if (src_list != NULL && src_list->count > 0U) {
      dst_list->items = (graphion_vm_value *)calloc(src_list->count, sizeof(*dst_list->items));
      if (dst_list->items == NULL) {
        free(dst_list);
        return GVM_ERR_INVALID_ARG;
      }
      dst_list->capacity = src_list->count;
      for (i = 0U; i < src_list->count; ++i) {
        int rc = vm_value_clone(&dst_list->items[i], &src_list->items[i]);
        if (rc != GVM_OK) {
          size_t j;
          for (j = 0U; j < i; ++j) {
            vm_value_dispose_owned(&dst_list->items[j]);
          }
          free(dst_list->items);
          free(dst_list);
          return rc;
        }
      }
      dst_list->count = src_list->count;
    }
    dst->kind = src->kind;
    dst->as.ref_value = dst_list;
    return GVM_OK;
  }

  if (src->kind == GVM_VALUE_GRAPH_REF) {
    size_t offset_count;
    src_graph = (graphion_csr_graph *)src->as.ref_value;
    src_graph_value = (graphion_graph_value *)src->as.ref_value;
    dst_graph = vm_graph_create_nodes(src_graph != NULL ? src_graph->node_count : 0U);
    if (dst_graph == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    dst_graph_value = (graphion_graph_value *)dst_graph;
    if (src_graph != NULL) {
      dst_graph->edge_count = src_graph->edge_count;
      if (src_graph->edge_count > 0U) {
        uint32_t *neighbors = (uint32_t *)calloc(src_graph->edge_count, sizeof(*neighbors));
        if (neighbors == NULL) {
          vm_value_clear(dst);
          free((void *)dst_graph->offsets);
          free(dst_graph);
          return GVM_ERR_INVALID_ARG;
        }
        memcpy(neighbors, src_graph->neighbors, src_graph->edge_count * sizeof(*neighbors));
        dst_graph->neighbors = neighbors;
      }
      offset_count = src_graph->node_count + 1U;
      if (src_graph->offsets != NULL && dst_graph->offsets != NULL && offset_count > 0U) {
        memcpy((void *)dst_graph->offsets, src_graph->offsets, offset_count * sizeof(*dst_graph->offsets));
      }
      if (src_graph_value->node_attrs != NULL && src_graph_value->node_attr_count > 0U) {
        dst_graph_value->node_attrs =
            (graphion_vm_value *)calloc(src_graph_value->node_attr_count, sizeof(*dst_graph_value->node_attrs));
        if (dst_graph_value->node_attrs == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_GRAPH_REF, {0}, {.ref_value = dst_graph}});
          return GVM_ERR_INVALID_ARG;
        }
        dst_graph_value->node_attr_count = src_graph_value->node_attr_count;
        for (i = 0U; i < src_graph_value->node_attr_count; ++i) {
          int rc = vm_value_clone(&dst_graph_value->node_attrs[i], &src_graph_value->node_attrs[i]);
          if (rc != GVM_OK) {
            graphion_vm_value cleanup;
            vm_value_clear(&cleanup);
            cleanup.kind = GVM_VALUE_GRAPH_REF;
            cleanup.as.ref_value = dst_graph;
            vm_value_dispose_owned(&cleanup);
            return rc;
          }
        }
      }
      if (src_graph_value->nodes != NULL && src_graph_value->node_count > 0U) {
        dst_graph_value->nodes =
            (graphion_graph_node_value *)calloc(src_graph_value->node_count, sizeof(*dst_graph_value->nodes));
        if (dst_graph_value->nodes == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_GRAPH_REF, {0}, {.ref_value = dst_graph}});
          return GVM_ERR_INVALID_ARG;
        }
        dst_graph_value->node_count = src_graph_value->node_count;
        for (i = 0U; i < src_graph_value->node_count; ++i) {
          dst_graph_value->nodes[i].id = src_graph_value->nodes[i].id;
          if (src_graph_value->nodes[i].name != NULL) {
            dst_graph_value->nodes[i].name = vm_strdup_text(src_graph_value->nodes[i].name);
            if (dst_graph_value->nodes[i].name == NULL) {
              graphion_vm_value cleanup;
              vm_value_clear(&cleanup);
              cleanup.kind = GVM_VALUE_GRAPH_REF;
              cleanup.as.ref_value = dst_graph;
              vm_value_dispose_owned(&cleanup);
              return GVM_ERR_INVALID_ARG;
            }
          }
        }
      }
      if (src_graph_value->edge_attrs != NULL && src_graph_value->edge_attr_count > 0U) {
        dst_graph_value->edge_attrs =
            (graphion_vm_value *)calloc(src_graph_value->edge_attr_count, sizeof(*dst_graph_value->edge_attrs));
        if (dst_graph_value->edge_attrs == NULL) {
          graphion_vm_value cleanup;
          vm_value_clear(&cleanup);
          cleanup.kind = GVM_VALUE_GRAPH_REF;
          cleanup.as.ref_value = dst_graph;
          vm_value_dispose_owned(&cleanup);
          return GVM_ERR_INVALID_ARG;
        }
        dst_graph_value->edge_attr_count = src_graph_value->edge_attr_count;
        for (i = 0U; i < src_graph_value->edge_attr_count; ++i) {
          int rc = vm_value_clone(&dst_graph_value->edge_attrs[i], &src_graph_value->edge_attrs[i]);
          if (rc != GVM_OK) {
            graphion_vm_value cleanup;
            vm_value_clear(&cleanup);
            cleanup.kind = GVM_VALUE_GRAPH_REF;
            cleanup.as.ref_value = dst_graph;
            vm_value_dispose_owned(&cleanup);
            return rc;
          }
        }
      }
      if (src_graph_value->edges != NULL && src_graph_value->edge_count > 0U) {
        dst_graph_value->edges =
            (graphion_graph_edge_value *)calloc(src_graph_value->edge_count, sizeof(*dst_graph_value->edges));
        if (dst_graph_value->edges == NULL) {
          graphion_vm_value cleanup;
          vm_value_clear(&cleanup);
          cleanup.kind = GVM_VALUE_GRAPH_REF;
          cleanup.as.ref_value = dst_graph;
          vm_value_dispose_owned(&cleanup);
          return GVM_ERR_INVALID_ARG;
        }
        memcpy(dst_graph_value->edges, src_graph_value->edges, src_graph_value->edge_count * sizeof(*dst_graph_value->edges));
        dst_graph_value->edge_count = src_graph_value->edge_count;
      }
    }
    dst->kind = GVM_VALUE_GRAPH_REF;
    memcpy(dst->reserved, src->reserved, sizeof(dst->reserved));
    dst->as.ref_value = dst_graph;
    return GVM_OK;
  }

  if (src->kind == GVM_VALUE_HYPERGRAPH_REF) {
    dst_hypergraph = vm_hypergraph_create_empty();
    if (dst_hypergraph == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    src_hypergraph = (graphion_hypergraph *)src->as.ref_value;
    src_hypergraph_value = (graphion_hypergraph_value *)src->as.ref_value;
    dst_hypergraph_value = (graphion_hypergraph_value *)dst_hypergraph;
    if (src_hypergraph != NULL) {
      dst_hypergraph->node_count = src_hypergraph->node_count;
      dst_hypergraph->hyperedge_count = src_hypergraph->hyperedge_count;
      dst_hypergraph->incidence_count = src_hypergraph->incidence_count;
      if (src_hypergraph->node_offsets != NULL && src_hypergraph->node_count > 0U) {
        const size_t count = src_hypergraph->node_count + 1U;
        uint32_t *copy = (uint32_t *)calloc(count, sizeof(*copy));
        if (copy == NULL) {
          free(dst_hypergraph);
          return GVM_ERR_INVALID_ARG;
        }
        memcpy(copy, src_hypergraph->node_offsets, count * sizeof(*copy));
        dst_hypergraph->node_offsets = copy;
      }
      if (src_hypergraph->node_hyperedges != NULL && src_hypergraph->incidence_count > 0U) {
        uint32_t *copy = (uint32_t *)calloc(src_hypergraph->incidence_count, sizeof(*copy));
        if (copy == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
          return GVM_ERR_INVALID_ARG;
        }
        memcpy(copy, src_hypergraph->node_hyperedges, src_hypergraph->incidence_count * sizeof(*copy));
        dst_hypergraph->node_hyperedges = copy;
      }
      if (src_hypergraph->hyperedge_offsets != NULL && src_hypergraph->hyperedge_count > 0U) {
        const size_t count = src_hypergraph->hyperedge_count + 1U;
        uint32_t *copy = (uint32_t *)calloc(count, sizeof(*copy));
        if (copy == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
          return GVM_ERR_INVALID_ARG;
        }
        memcpy(copy, src_hypergraph->hyperedge_offsets, count * sizeof(*copy));
        dst_hypergraph->hyperedge_offsets = copy;
      }
      if (src_hypergraph->hyperedge_nodes != NULL && src_hypergraph->incidence_count > 0U) {
        uint32_t *copy = (uint32_t *)calloc(src_hypergraph->incidence_count, sizeof(*copy));
        if (copy == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
          return GVM_ERR_INVALID_ARG;
        }
        memcpy(copy, src_hypergraph->hyperedge_nodes, src_hypergraph->incidence_count * sizeof(*copy));
        dst_hypergraph->hyperedge_nodes = copy;
      }
      if (src_hypergraph_value->vertices != NULL && src_hypergraph_value->vertex_count > 0U) {
        dst_hypergraph_value->vertices =
            (graphion_graph_node_value *)calloc(src_hypergraph_value->vertex_count,
                                                sizeof(*dst_hypergraph_value->vertices));
        if (dst_hypergraph_value->vertices == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
          return GVM_ERR_INVALID_ARG;
        }
        dst_hypergraph_value->vertex_count = src_hypergraph_value->vertex_count;
        for (i = 0U; i < src_hypergraph_value->vertex_count; ++i) {
          dst_hypergraph_value->vertices[i].id = src_hypergraph_value->vertices[i].id;
          if (src_hypergraph_value->vertices[i].name != NULL) {
            dst_hypergraph_value->vertices[i].name = vm_strdup_text(src_hypergraph_value->vertices[i].name);
            if (dst_hypergraph_value->vertices[i].name == NULL) {
              vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
              return GVM_ERR_INVALID_ARG;
            }
          }
        }
      }
      if (src_hypergraph_value->vertex_attrs != NULL && src_hypergraph_value->vertex_attr_count > 0U) {
        dst_hypergraph_value->vertex_attrs =
            (graphion_vm_value *)calloc(src_hypergraph_value->vertex_attr_count,
                                        sizeof(*dst_hypergraph_value->vertex_attrs));
        if (dst_hypergraph_value->vertex_attrs == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
          return GVM_ERR_INVALID_ARG;
        }
        dst_hypergraph_value->vertex_attr_count = src_hypergraph_value->vertex_attr_count;
        for (i = 0U; i < src_hypergraph_value->vertex_attr_count; ++i) {
          int rc = vm_value_clone(&dst_hypergraph_value->vertex_attrs[i], &src_hypergraph_value->vertex_attrs[i]);
          if (rc != GVM_OK) {
            vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
            return rc;
          }
        }
      }
      if (src_hypergraph_value->hyperedge_attrs != NULL && src_hypergraph_value->hyperedge_attr_count > 0U) {
        dst_hypergraph_value->hyperedge_attrs =
            (graphion_vm_value *)calloc(src_hypergraph_value->hyperedge_attr_count,
                                        sizeof(*dst_hypergraph_value->hyperedge_attrs));
        if (dst_hypergraph_value->hyperedge_attrs == NULL) {
          vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
          return GVM_ERR_INVALID_ARG;
        }
        dst_hypergraph_value->hyperedge_attr_count = src_hypergraph_value->hyperedge_attr_count;
        for (i = 0U; i < src_hypergraph_value->hyperedge_attr_count; ++i) {
          int rc = vm_value_clone(&dst_hypergraph_value->hyperedge_attrs[i],
                                  &src_hypergraph_value->hyperedge_attrs[i]);
          if (rc != GVM_OK) {
            vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_HYPERGRAPH_REF, {0}, {.ref_value = dst_hypergraph}});
            return rc;
          }
        }
      }
    }
    dst->kind = GVM_VALUE_HYPERGRAPH_REF;
    memcpy(dst->reserved, src->reserved, sizeof(dst->reserved));
    dst->as.ref_value = dst_hypergraph;
    return GVM_OK;
  }

  if (src->kind == GVM_VALUE_STRUCT_TYPE) {
    src_struct_type = (graphion_struct_type_value *)src->as.ref_value;
    dst_struct_type = vm_struct_type_create();
    if (dst_struct_type == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    if (src_struct_type != NULL) {
      memcpy(dst_struct_type->name, src_struct_type->name, sizeof(dst_struct_type->name));
      if (src_struct_type->field_count > 0U) {
        dst_struct_type->fields =
            (graphion_struct_field_value *)calloc(src_struct_type->field_count, sizeof(*dst_struct_type->fields));
        if (dst_struct_type->fields == NULL) {
          free(dst_struct_type);
          return GVM_ERR_INVALID_ARG;
        }
        dst_struct_type->field_count = src_struct_type->field_count;
        for (i = 0U; i < src_struct_type->field_count; ++i) {
          memcpy(&dst_struct_type->fields[i], &src_struct_type->fields[i], sizeof(dst_struct_type->fields[i]));
          if (src_struct_type->fields[i].has_default) {
            int rc = vm_value_clone(&dst_struct_type->fields[i].default_value,
                                    &src_struct_type->fields[i].default_value);
            if (rc != GVM_OK) {
              graphion_vm_value cleanup;
              vm_value_clear(&cleanup);
              cleanup.kind = GVM_VALUE_STRUCT_TYPE;
              cleanup.as.ref_value = dst_struct_type;
              vm_value_dispose_owned(&cleanup);
              return rc;
            }
          }
        }
      }
    }
    dst->kind = GVM_VALUE_STRUCT_TYPE;
    dst->as.ref_value = dst_struct_type;
    return GVM_OK;
  }

  if (src->kind == GVM_VALUE_STRUCT) {
    src_struct_instance = (graphion_struct_instance_value *)src->as.ref_value;
    dst_struct_instance = vm_struct_instance_create();
    if (dst_struct_instance == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    if (src_struct_instance != NULL) {
      memcpy(dst_struct_instance->type_name, src_struct_instance->type_name, sizeof(dst_struct_instance->type_name));
      if (vm_value_clone(&dst_struct_instance->fields, &src_struct_instance->fields) != GVM_OK) {
        free(dst_struct_instance);
        return GVM_ERR_INVALID_ARG;
      }
    }
    dst->kind = GVM_VALUE_STRUCT;
    dst->as.ref_value = dst_struct_instance;
    return GVM_OK;
  }

  src_dict = (graphion_vm_dict *)src->as.ref_value;
  dst_dict = vm_dict_create();
  if (dst_dict == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (src_dict != NULL && src_dict->count > 0U) {
    dst_dict->entries = (graphion_vm_dict_entry *)calloc(src_dict->count, sizeof(*dst_dict->entries));
    if (dst_dict->entries == NULL) {
      free(dst_dict);
      return GVM_ERR_INVALID_ARG;
    }
    dst_dict->capacity = src_dict->count;
    for (i = 0U; i < src_dict->count; ++i) {
      int rc;
      dst_dict->entries[i].key = vm_strdup_text(src_dict->entries[i].key != NULL ? src_dict->entries[i].key : "");
      if (dst_dict->entries[i].key == NULL) {
        size_t j;
        for (j = 0U; j < i; ++j) {
          free(dst_dict->entries[j].key);
          vm_value_dispose_owned(&dst_dict->entries[j].value);
        }
        free(dst_dict->entries);
        free(dst_dict);
        return GVM_ERR_INVALID_ARG;
      }
      rc = vm_value_clone(&dst_dict->entries[i].value, &src_dict->entries[i].value);
      if (rc != GVM_OK) {
        size_t j;
        free(dst_dict->entries[i].key);
        for (j = 0U; j < i; ++j) {
          free(dst_dict->entries[j].key);
          vm_value_dispose_owned(&dst_dict->entries[j].value);
        }
        free(dst_dict->entries);
        free(dst_dict);
        return rc;
      }
    }
    dst_dict->count = src_dict->count;
  }
  dst->kind = GVM_VALUE_DICT;
  dst->as.ref_value = dst_dict;
  return GVM_OK;
}

int vm_value_get_int(const graphion_vm_value *value, int64_t *out_value) {
  if (value == NULL || out_value == NULL || value->kind != GVM_VALUE_INT) {
    return 0;
  }
  *out_value = value->as.int_value;
  return 1;
}

int vm_value_get_numeric(const graphion_vm_value *value,
                         int64_t *out_int,
                         double *out_float,
                         int *out_is_float) {
  if (value == NULL || out_int == NULL || out_float == NULL || out_is_float == NULL) {
    return 0;
  }
  switch (value->kind) {
    case GVM_VALUE_INT:
      *out_int = value->as.int_value;
      *out_float = (double)value->as.int_value;
      *out_is_float = 0;
      return 1;
    case GVM_VALUE_FLOAT:
      *out_int = 0;
      *out_float = value->as.float_value;
      *out_is_float = 1;
      return 1;
    default:
      return 0;
  }
}

int vm_value_list_length(const graphion_vm_value *value, size_t *len_out) {
  graphion_vm_list *list;
  if (value == NULL || len_out == NULL || value->kind != GVM_VALUE_LIST) {
    return 0;
  }
  list = (graphion_vm_list *)value->as.ref_value;
  *len_out = list != NULL ? list->count : 0U;
  return 1;
}

int vm_value_list_clone_item(const graphion_vm_value *value, size_t index, graphion_vm_value *out) {
  graphion_vm_list *list;

  if (value == NULL || out == NULL || value->kind != GVM_VALUE_LIST) {
    return GVM_ERR_INVALID_ARG;
  }
  list = (graphion_vm_list *)value->as.ref_value;
  if (list == NULL || index >= list->count) {
    return GVM_ERR_INDEX_OUT_OF_RANGE;
  }
  return vm_value_clone(out, &list->items[index]);
}

int vm_value_set_empty_list_value(graphion_vm_value *value) {
  graphion_vm_list *list;

  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  list = vm_list_create();
  if (list == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_value_dispose_owned(value);
  value->kind = GVM_VALUE_LIST;
  value->as.ref_value = list;
  return GVM_OK;
}

int vm_value_tuple_length(const graphion_vm_value *value, size_t *len_out) {
  graphion_vm_list *tuple;
  if (value == NULL || len_out == NULL || value->kind != GVM_VALUE_TUPLE) {
    return 0;
  }
  tuple = (graphion_vm_list *)value->as.ref_value;
  *len_out = tuple != NULL ? tuple->count : 0U;
  return 1;
}

int vm_value_set_length(const graphion_vm_value *value, size_t *len_out) {
  graphion_vm_list *set;
  if (value == NULL || len_out == NULL || value->kind != GVM_VALUE_SET) {
    return 0;
  }
  set = (graphion_vm_list *)value->as.ref_value;
  *len_out = set != NULL ? set->count : 0U;
  return 1;
}

int vm_value_dict_length(const graphion_vm_value *value, size_t *len_out) {
  graphion_vm_dict *dict;
  if (value == NULL || len_out == NULL || value->kind != GVM_VALUE_DICT) {
    return 0;
  }
  dict = (graphion_vm_dict *)value->as.ref_value;
  *len_out = dict != NULL ? dict->count : 0U;
  return 1;
}

int vm_value_struct_field_count(const graphion_vm_value *value, size_t *len_out) {
  graphion_struct_instance_value *instance;
  if (value == NULL || len_out == NULL || value->kind != GVM_VALUE_STRUCT) {
    return 0;
  }
  instance = (graphion_struct_instance_value *)value->as.ref_value;
  return vm_value_dict_length(instance != NULL ? &instance->fields : NULL, len_out);
}

int vm_value_set_empty_dict_value(graphion_vm_value *value) {
  graphion_vm_dict *dict;

  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  dict = vm_dict_create();
  if (dict == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_value_dispose_owned(value);
  value->kind = GVM_VALUE_DICT;
  value->as.ref_value = dict;
  return GVM_OK;
}

int vm_value_dict_keys_equal(const graphion_vm_value *lhs, const graphion_vm_value *rhs) {
  graphion_vm_dict *lhs_dict;
  graphion_vm_dict *rhs_dict;
  size_t lhs_count;
  size_t rhs_count;
  size_t i;
  size_t j;

  if (lhs == NULL || rhs == NULL || lhs->kind != GVM_VALUE_DICT || rhs->kind != GVM_VALUE_DICT) {
    return 0;
  }
  lhs_dict = (graphion_vm_dict *)lhs->as.ref_value;
  rhs_dict = (graphion_vm_dict *)rhs->as.ref_value;
  lhs_count = lhs_dict != NULL ? lhs_dict->count : 0U;
  rhs_count = rhs_dict != NULL ? rhs_dict->count : 0U;
  if (lhs_count != rhs_count) {
    return 0;
  }
  for (i = 0U; i < lhs_count; ++i) {
    const char *lhs_key = lhs_dict->entries[i].key != NULL ? lhs_dict->entries[i].key : "";
    int found = 0;
    for (j = 0U; j < rhs_count; ++j) {
      const char *rhs_key = rhs_dict->entries[j].key != NULL ? rhs_dict->entries[j].key : "";
      if (strcmp(lhs_key, rhs_key) == 0) {
        found = 1;
        break;
      }
    }
    if (!found) {
      return 0;
    }
  }
  return 1;
}

int vm_value_dict_keys_subset(const graphion_vm_value *value, const graphion_vm_value *allowed) {
  graphion_vm_dict *value_dict;
  graphion_vm_dict *allowed_dict;
  size_t value_count;
  size_t allowed_count;
  size_t i;
  size_t j;

  if (value == NULL || allowed == NULL || value->kind != GVM_VALUE_DICT || allowed->kind != GVM_VALUE_DICT) {
    return 0;
  }
  value_dict = (graphion_vm_dict *)value->as.ref_value;
  allowed_dict = (graphion_vm_dict *)allowed->as.ref_value;
  value_count = value_dict != NULL ? value_dict->count : 0U;
  allowed_count = allowed_dict != NULL ? allowed_dict->count : 0U;
  for (i = 0U; i < value_count; ++i) {
    const char *value_key = value_dict->entries[i].key != NULL ? value_dict->entries[i].key : "";
    int found = 0;
    for (j = 0U; j < allowed_count; ++j) {
      const char *allowed_key = allowed_dict->entries[j].key != NULL ? allowed_dict->entries[j].key : "";
      if (strcmp(value_key, allowed_key) == 0) {
        found = 1;
        break;
      }
    }
    if (!found) {
      return 0;
    }
  }
  return 1;
}

int vm_value_dict_merge_defaults(graphion_vm_value *value, const graphion_vm_value *defaults) {
  graphion_vm_dict *defaults_dict;
  size_t defaults_count;
  size_t i;

  if (value == NULL || defaults == NULL || value->kind != GVM_VALUE_DICT || defaults->kind != GVM_VALUE_DICT) {
    return GVM_ERR_INVALID_ARG;
  }
  defaults_dict = (graphion_vm_dict *)defaults->as.ref_value;
  defaults_count = defaults_dict != NULL ? defaults_dict->count : 0U;
  for (i = 0U; i < defaults_count; ++i) {
    graphion_vm vm;
    const char *key = defaults_dict->entries[i].key != NULL ? defaults_dict->entries[i].key : "";
    const size_t existing_index = vm_dict_find_index((graphion_vm_dict *)value->as.ref_value, key);
    int rc;

    if (existing_index != (size_t)-1) {
      continue;
    }
    graphion_vm_init(&vm);
    vm.regs[0] = *value;
    vm.regs[1] = defaults_dict->entries[i].value;
    rc = vm_dict_set_reg(&vm, 0U, key, 1U);
    *value = vm.regs[0];
    vm.regs[0].kind = GVM_VALUE_NONE;
    vm.regs[1].kind = GVM_VALUE_NONE;
    graphion_vm_dispose(&vm);
    if (rc != GVM_OK) {
      return rc;
    }
  }
  return GVM_OK;
}

int vm_value_dict_key_kind(const graphion_vm_value *value, const char *key, uint8_t *kind_out, int *found_out) {
  graphion_vm_dict *dict;
  size_t index;

  if (kind_out != NULL) {
    *kind_out = GVM_VALUE_NONE;
  }
  if (found_out != NULL) {
    *found_out = 0;
  }
  if (value == NULL || key == NULL || value->kind != GVM_VALUE_DICT) {
    return 0;
  }
  dict = (graphion_vm_dict *)value->as.ref_value;
  index = vm_dict_find_index(dict, key);
  if (index == (size_t)-1) {
    return 1;
  }
  if (kind_out != NULL) {
    *kind_out = dict->entries[index].value.kind;
  }
  if (found_out != NULL) {
    *found_out = 1;
  }
  return 1;
}

int vm_value_dict_get_clone(const graphion_vm_value *value, const char *key, graphion_vm_value *out) {
  graphion_vm_dict *dict;
  size_t index;

  if (out != NULL) {
    vm_value_clear(out);
  }
  if (value == NULL || key == NULL || out == NULL || value->kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  dict = (graphion_vm_dict *)value->as.ref_value;
  index = vm_dict_find_index(dict, key);
  if (index == (size_t)-1) {
    return GVM_ERR_MISSING_KEY;
  }
  return vm_value_clone(out, &dict->entries[index].value);
}

int vm_value_dict_set_clone(graphion_vm_value *value, const char *key, const graphion_vm_value *src) {
  graphion_vm vm;
  int rc;

  if (value == NULL || key == NULL || src == NULL || value->kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  graphion_vm_init(&vm);
  vm.regs[0] = *value;
  vm.regs[1] = *src;
  rc = vm_dict_set_reg(&vm, 0U, key, 1U);
  *value = vm.regs[0];
  vm.regs[0].kind = GVM_VALUE_NONE;
  vm.regs[1].kind = GVM_VALUE_NONE;
  graphion_vm_dispose(&vm);
  return rc;
}

int vm_value_dict_patch_existing(graphion_vm_value *value, const graphion_vm_value *patch) {
  graphion_vm_dict *dict;
  graphion_vm_dict *patch_dict;
  size_t patch_count;
  size_t i;

  if (value == NULL || patch == NULL || value->kind != GVM_VALUE_DICT || patch->kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  dict = (graphion_vm_dict *)value->as.ref_value;
  patch_dict = (graphion_vm_dict *)patch->as.ref_value;
  patch_count = patch_dict != NULL ? patch_dict->count : 0U;
  for (i = 0U; i < patch_count; ++i) {
    const char *key = patch_dict->entries[i].key != NULL ? patch_dict->entries[i].key : "";
    if (vm_dict_find_index(dict, key) == (size_t)-1) {
      return GVM_ERR_MISSING_KEY;
    }
  }
  for (i = 0U; i < patch_count; ++i) {
    const char *key = patch_dict->entries[i].key != NULL ? patch_dict->entries[i].key : "";
    const int rc = vm_value_dict_set_clone(value, key, &patch_dict->entries[i].value);
    if (rc != GVM_OK) {
      return rc;
    }
  }
  return GVM_OK;
}

static int vm_struct_field_index(const graphion_struct_type_value *type_value, const char *field_name) {
  size_t i;
  if (type_value == NULL || field_name == NULL) {
    return -1;
  }
  for (i = 0U; i < type_value->field_count; ++i) {
    if (strcmp(type_value->fields[i].name, field_name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int vm_struct_value_matches_type(const graphion_vm_value *value, const char *type_name) {
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

int vm_value_set_struct_type(graphion_vm_value *value,
                             const char *name,
                             const graphion_struct_field_value *fields,
                             size_t field_count) {
  graphion_struct_type_value *type_value;
  size_t i;

  if (value == NULL || name == NULL || fields == NULL || field_count == 0U || strlen(name) >= 64U) {
    return GVM_ERR_INVALID_ARG;
  }
  type_value = vm_struct_type_create();
  if (type_value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  memcpy(type_value->name, name, strlen(name) + 1U);
  type_value->fields = (graphion_struct_field_value *)calloc(field_count, sizeof(*type_value->fields));
  if (type_value->fields == NULL) {
    free(type_value);
    return GVM_ERR_INVALID_ARG;
  }
  type_value->field_count = field_count;
  for (i = 0U; i < field_count; ++i) {
    if (fields[i].name[0] == '\0' || fields[i].type_name[0] == '\0') {
      vm_value_dispose_owned(&(graphion_vm_value){GVM_VALUE_STRUCT_TYPE, {0}, {.ref_value = type_value}});
      return GVM_ERR_INVALID_ARG;
    }
    memcpy(&type_value->fields[i], &fields[i], sizeof(type_value->fields[i]));
    if (fields[i].has_default) {
      int rc = vm_value_clone(&type_value->fields[i].default_value, &fields[i].default_value);
      if (rc != GVM_OK) {
        graphion_vm_value cleanup;
        vm_value_clear(&cleanup);
        cleanup.kind = GVM_VALUE_STRUCT_TYPE;
        cleanup.as.ref_value = type_value;
        vm_value_dispose_owned(&cleanup);
        return rc;
      }
    }
  }
  vm_value_dispose_owned(value);
  value->kind = GVM_VALUE_STRUCT_TYPE;
  value->as.ref_value = type_value;
  return GVM_OK;
}

int vm_value_instantiate_struct(graphion_vm_value *out,
                                const graphion_vm_value *type_value,
                                const graphion_vm_value *overrides) {
  const graphion_struct_type_value *type;
  graphion_struct_instance_value *instance;
  graphion_vm_value fields_dict;
  graphion_vm_dict *override_dict;
  size_t i;
  int rc;

  if (out == NULL || type_value == NULL || overrides == NULL || type_value->kind != GVM_VALUE_STRUCT_TYPE ||
      overrides->kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  type = (const graphion_struct_type_value *)type_value->as.ref_value;
  if (type == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  override_dict = (graphion_vm_dict *)overrides->as.ref_value;
  if (override_dict != NULL) {
    for (i = 0U; i < override_dict->count; ++i) {
      const char *key = override_dict->entries[i].key != NULL ? override_dict->entries[i].key : "";
      if (vm_struct_field_index(type, key) < 0) {
        return GVM_ERR_MISSING_KEY;
      }
    }
  }

  vm_value_clear(&fields_dict);
  rc = vm_value_set_empty_dict_value(&fields_dict);
  if (rc != GVM_OK) {
    return rc;
  }
  for (i = 0U; i < type->field_count; ++i) {
    graphion_vm_value field_value;
    const graphion_struct_field_value *field = &type->fields[i];
    vm_value_clear(&field_value);
    rc = vm_value_dict_get_clone(overrides, field->name, &field_value);
    if (rc == GVM_ERR_MISSING_KEY && field->has_default) {
      rc = vm_value_clone(&field_value, &field->default_value);
    }
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&fields_dict);
      return rc;
    }
    if (!vm_struct_value_matches_type(&field_value, field->type_name)) {
      vm_value_dispose_owned(&field_value);
      vm_value_dispose_owned(&fields_dict);
      return GVM_ERR_TYPE_MISMATCH;
    }
    rc = vm_value_dict_set_clone(&fields_dict, field->name, &field_value);
    vm_value_dispose_owned(&field_value);
    if (rc != GVM_OK) {
      vm_value_dispose_owned(&fields_dict);
      return rc;
    }
  }

  instance = vm_struct_instance_create();
  if (instance == NULL) {
    vm_value_dispose_owned(&fields_dict);
    return GVM_ERR_INVALID_ARG;
  }
  memcpy(instance->type_name, type->name, sizeof(instance->type_name));
  instance->fields = fields_dict;
  vm_value_dispose_owned(out);
  out->kind = GVM_VALUE_STRUCT;
  out->as.ref_value = instance;
  return GVM_OK;
}

int vm_values_deep_equal(const graphion_vm_value *lhs,
                         const graphion_vm_value *rhs,
                         int *compatible_out,
                         int *equal_out) {
  size_t i;
  graphion_vm_list *lhs_list;
  graphion_vm_list *rhs_list;
  graphion_vm_dict *lhs_dict;
  graphion_vm_dict *rhs_dict;
  graphion_struct_instance_value *lhs_struct;
  graphion_struct_instance_value *rhs_struct;

  if (lhs == NULL || rhs == NULL || compatible_out == NULL || equal_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  *compatible_out = 1;
  *equal_out = 0;

  if ((lhs->kind == GVM_VALUE_INT || lhs->kind == GVM_VALUE_FLOAT) &&
      (rhs->kind == GVM_VALUE_INT || rhs->kind == GVM_VALUE_FLOAT)) {
    int64_t lhs_i = 0;
    int64_t rhs_i = 0;
    double lhs_f = 0.0;
    double rhs_f = 0.0;
    int lhs_is_float = 0;
    int rhs_is_float = 0;
    if (!vm_value_get_numeric(lhs, &lhs_i, &lhs_f, &lhs_is_float) ||
        !vm_value_get_numeric(rhs, &rhs_i, &rhs_f, &rhs_is_float)) {
      *compatible_out = 0;
      return GVM_OK;
    }
    *equal_out = lhs_f == rhs_f;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_INT) {
    if (rhs->as.int_value != 0 && rhs->as.int_value != 1) {
      *compatible_out = 0;
      return GVM_OK;
    }
    *equal_out = rhs->as.int_value == (int64_t)lhs->as.bool_value;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_INT && rhs->kind == GVM_VALUE_BOOL) {
    if (lhs->as.int_value != 0 && lhs->as.int_value != 1) {
      *compatible_out = 0;
      return GVM_OK;
    }
    *equal_out = lhs->as.int_value == (int64_t)rhs->as.bool_value;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_BOOL && rhs->kind == GVM_VALUE_BOOL) {
    *equal_out = lhs->as.bool_value == rhs->as.bool_value;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_BITS && rhs->kind == GVM_VALUE_BITS) {
    *equal_out = vm_value_get_bits_payload(lhs) == vm_value_get_bits_payload(rhs);
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_STRING && rhs->kind == GVM_VALUE_STRING) {
    const char *lhs_text = lhs->as.string_value != NULL ? lhs->as.string_value : "";
    const char *rhs_text = rhs->as.string_value != NULL ? rhs->as.string_value : "";
    *equal_out = strcmp(lhs_text, rhs_text) == 0;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_DICT && rhs->kind == GVM_VALUE_DICT) {
    lhs_dict = (graphion_vm_dict *)lhs->as.ref_value;
    rhs_dict = (graphion_vm_dict *)rhs->as.ref_value;
    if ((lhs_dict != NULL ? lhs_dict->count : 0U) != (rhs_dict != NULL ? rhs_dict->count : 0U)) {
      *equal_out = 0;
      return GVM_OK;
    }
    for (i = 0U; i < (lhs_dict != NULL ? lhs_dict->count : 0U); ++i) {
      int nested_compatible = 0;
      int nested_equal = 0;
      int found = 0;
      size_t j;
      for (j = 0U; j < (rhs_dict != NULL ? rhs_dict->count : 0U); ++j) {
        const char *lhs_key = lhs_dict->entries[i].key != NULL ? lhs_dict->entries[i].key : "";
        const char *rhs_key = rhs_dict->entries[j].key != NULL ? rhs_dict->entries[j].key : "";
        if (strcmp(lhs_key, rhs_key) != 0) {
          continue;
        }
        found = 1;
        if (vm_values_deep_equal(&lhs_dict->entries[i].value,
                                 &rhs_dict->entries[j].value,
                                 &nested_compatible,
                                 &nested_equal) != GVM_OK) {
          return GVM_ERR_INVALID_ARG;
        }
        if (!nested_compatible) {
          *compatible_out = 0;
          return GVM_OK;
        }
        if (!nested_equal) {
          *equal_out = 0;
          return GVM_OK;
        }
        break;
      }
      if (!found) {
        *equal_out = 0;
        return GVM_OK;
      }
    }
    *equal_out = 1;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_SET && rhs->kind == GVM_VALUE_SET) {
    lhs_list = (graphion_vm_list *)lhs->as.ref_value;
    rhs_list = (graphion_vm_list *)rhs->as.ref_value;
    if ((lhs_list != NULL ? lhs_list->count : 0U) != (rhs_list != NULL ? rhs_list->count : 0U)) {
      *equal_out = 0;
      return GVM_OK;
    }
    for (i = 0U; i < (lhs_list != NULL ? lhs_list->count : 0U); ++i) {
      int found = 0;
      size_t j;
      for (j = 0U; j < (rhs_list != NULL ? rhs_list->count : 0U); ++j) {
        int nested_compatible = 0;
        int nested_equal = 0;
        const int rc = vm_values_deep_equal(&lhs_list->items[i],
                                            &rhs_list->items[j],
                                            &nested_compatible,
                                            &nested_equal);
        if (rc != GVM_OK) {
          return rc;
        }
        if (nested_compatible && nested_equal) {
          found = 1;
          break;
        }
      }
      if (!found) {
        *equal_out = 0;
        return GVM_OK;
      }
    }
    *equal_out = 1;
    return GVM_OK;
  }
  if (lhs->kind == GVM_VALUE_STRUCT && rhs->kind == GVM_VALUE_STRUCT) {
    int nested_compatible = 0;
    int nested_equal = 0;
    lhs_struct = (graphion_struct_instance_value *)lhs->as.ref_value;
    rhs_struct = (graphion_struct_instance_value *)rhs->as.ref_value;
    if (lhs_struct == NULL || rhs_struct == NULL || strcmp(lhs_struct->type_name, rhs_struct->type_name) != 0) {
      *equal_out = 0;
      return GVM_OK;
    }
    if (vm_values_deep_equal(&lhs_struct->fields,
                             &rhs_struct->fields,
                             &nested_compatible,
                             &nested_equal) != GVM_OK ||
        !nested_compatible) {
      *compatible_out = 0;
      return GVM_OK;
    }
    *equal_out = nested_equal;
    return GVM_OK;
  }
  if (!vm_value_is_sequence_kind(lhs->kind) || !vm_value_is_sequence_kind(rhs->kind) || lhs->kind != rhs->kind) {
    *compatible_out = 0;
    return GVM_OK;
  }

  lhs_list = (graphion_vm_list *)lhs->as.ref_value;
  rhs_list = (graphion_vm_list *)rhs->as.ref_value;
  if ((lhs_list != NULL ? lhs_list->count : 0U) != (rhs_list != NULL ? rhs_list->count : 0U)) {
    *equal_out = 0;
    return GVM_OK;
  }
  for (i = 0U; i < (lhs_list != NULL ? lhs_list->count : 0U); ++i) {
    int nested_compatible = 0;
    int nested_equal = 0;
    const int rc = vm_values_deep_equal(&lhs_list->items[i], &rhs_list->items[i], &nested_compatible, &nested_equal);
    if (rc != GVM_OK) {
      return rc;
    }
    if (!nested_compatible) {
      *compatible_out = 0;
      return GVM_OK;
    }
    if (!nested_equal) {
      *equal_out = 0;
      return GVM_OK;
    }
  }
  *equal_out = 1;
  return GVM_OK;
}

uint8_t vm_value_get_bits_width(const graphion_vm_value *value) {
  if (value == NULL || value->kind != GVM_VALUE_BITS) {
    return 0U;
  }
  return value->reserved[0];
}

uint64_t vm_value_get_bits_payload(const graphion_vm_value *value) {
  if (value == NULL || value->kind != GVM_VALUE_BITS) {
    return 0U;
  }
  return (uint64_t)value->as.int_value;
}

size_t vm_write_bits_text(char *buffer, size_t buffer_size, const graphion_vm_value *value, int include_newline) {
  const uint8_t width = vm_value_get_bits_width(value);
  const uint64_t payload = vm_value_get_bits_payload(value);
  size_t i;
  size_t pos = 0U;

  if (buffer == NULL || buffer_size < (size_t)width + (include_newline ? 4U : 3U) || width == 0U) {
    return 0U;
  }
  buffer[pos++] = '0';
  buffer[pos++] = 'b';
  for (i = 0U; i < (size_t)width; ++i) {
    const size_t bit_index = (size_t)width - 1U - i;
    buffer[pos++] = ((payload >> bit_index) & 1U) != 0U ? '1' : '0';
  }
  if (include_newline) {
    buffer[pos++] = '\n';
  }
  return pos;
}

static void vm_release_reg_compound(graphion_vm *vm, uint8_t reg) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  if (vm_value_is_compound_kind(vm->regs[reg].kind)) {
    vm_value_dispose_owned(&vm->regs[reg]);
  } else {
    vm_value_clear(&vm->regs[reg]);
  }
}

void vm_free_owned_reg_string(graphion_vm *vm, uint8_t reg) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  if (vm->owned_reg_strings[reg] != NULL) {
    free(vm->owned_reg_strings[reg]);
    vm->owned_reg_strings[reg] = NULL;
    vm_value_clear(&vm->regs[reg]);
    return;
  }
  vm_release_reg_compound(vm, reg);
}

void vm_release_all_reg_strings(graphion_vm *vm) {
  size_t i;
  if (vm == NULL) {
    return;
  }
  for (i = 0U; i < 16U; ++i) {
    vm_free_owned_reg_string(vm, (uint8_t)i);
  }
}

void vm_release_global_value(graphion_vm *vm, size_t index) {
  if (vm == NULL || vm->globals == NULL || index >= vm->global_count) {
    return;
  }
  if (vm->global_string_owners != NULL && vm->global_string_owners[index] != NULL) {
    free(vm->global_string_owners[index]);
    vm->global_string_owners[index] = NULL;
    vm_value_clear(&vm->globals[index]);
    return;
  }
  if (vm_value_is_compound_kind(vm->globals[index].kind)) {
    vm_value_dispose_owned(&vm->globals[index]);
  } else {
    vm_value_clear(&vm->globals[index]);
  }
}

int vm_reg_set_string_copy(graphion_vm *vm, uint8_t reg, const char *text) {
  char *copy;
  if (vm == NULL || !is_valid_reg(reg) || text == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  copy = vm_strdup_text(text);
  if (copy == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->owned_reg_strings[reg] = copy;
  vm->regs[reg].kind = GVM_VALUE_STRING;
  vm->regs[reg].as.string_value = copy;
  return GVM_OK;
}

int vm_global_set_string_copy(graphion_vm *vm, size_t index, const char *text) {
  char *copy;
  if (vm == NULL || text == NULL || index >= vm->global_count) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm->global_string_owners == NULL) {
    vm_release_global_value(vm, index);
    vm->globals[index].kind = GVM_VALUE_STRING;
    vm->globals[index].as.string_value = text;
    return GVM_OK;
  }
  copy = vm_strdup_text(text);
  if (copy == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_release_global_value(vm, index);
  vm->global_string_owners[index] = copy;
  vm->globals[index].kind = GVM_VALUE_STRING;
  vm->globals[index].as.string_value = copy;
  return GVM_OK;
}

int vm_reg_set_empty_list(graphion_vm *vm, uint8_t reg) {
  graphion_vm_list *list;
  if (vm == NULL || !is_valid_reg(reg)) {
    return GVM_ERR_INVALID_ARG;
  }
  list = vm_list_create();
  if (list == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->regs[reg].kind = GVM_VALUE_LIST;
  vm->regs[reg].as.ref_value = list;
  return GVM_OK;
}

int vm_reg_set_empty_tuple(graphion_vm *vm, uint8_t reg) {
  graphion_vm_list *tuple;
  if (vm == NULL || !is_valid_reg(reg)) {
    return GVM_ERR_INVALID_ARG;
  }
  tuple = vm_list_create();
  if (tuple == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->regs[reg].kind = GVM_VALUE_TUPLE;
  vm->regs[reg].as.ref_value = tuple;
  return GVM_OK;
}

int vm_reg_set_empty_set(graphion_vm *vm, uint8_t reg) {
  graphion_vm_list *set;
  if (vm == NULL || !is_valid_reg(reg)) {
    return GVM_ERR_INVALID_ARG;
  }
  set = vm_list_create();
  if (set == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->regs[reg].kind = GVM_VALUE_SET;
  vm->regs[reg].as.ref_value = set;
  return GVM_OK;
}

static int vm_list_append_value(graphion_vm_list *list, const graphion_vm_value *src) {
  graphion_vm_value cloned;

  if (list == NULL || src == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (list->count == list->capacity) {
    size_t new_capacity = list->capacity == 0U ? 4U : list->capacity * 2U;
    graphion_vm_value *new_items =
        (graphion_vm_value *)realloc(list->items, new_capacity * sizeof(*new_items));
    if (new_items == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    list->items = new_items;
    list->capacity = new_capacity;
  }
  vm_value_clear(&cloned);
  if (vm_value_clone(&cloned, src) != GVM_OK) {
    return GVM_ERR_INVALID_ARG;
  }
  list->items[list->count++] = cloned;
  return GVM_OK;
}

int vm_value_list_append_clone(graphion_vm_value *list_value, const graphion_vm_value *item) {
  graphion_vm_list *list;

  if (list_value == NULL || item == NULL || list_value->kind != GVM_VALUE_LIST) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  list = (graphion_vm_list *)list_value->as.ref_value;
  if (list == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  return vm_list_append_value(list, item);
}

int vm_list_append_reg(graphion_vm *vm, uint8_t list_reg, uint8_t value_reg) {
  graphion_vm_list *list;
  if (vm == NULL || !is_valid_reg(list_reg) || !is_valid_reg(value_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_value_is_sequence_kind(vm->regs[list_reg].kind)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  list = (graphion_vm_list *)vm->regs[list_reg].as.ref_value;
  return vm_list_append_value(list, &vm->regs[value_reg]);
}

int vm_list_append_int(graphion_vm *vm, uint8_t list_reg, int64_t value) {
  graphion_vm_value item;
  graphion_vm_list *list;

  if (vm == NULL || !is_valid_reg(list_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[list_reg].kind != GVM_VALUE_LIST) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  list = (graphion_vm_list *)vm->regs[list_reg].as.ref_value;
  vm_value_set_int(&item, value);
  return vm_list_append_value(list, &item);
}

int vm_tuple_append_reg(graphion_vm *vm, uint8_t tuple_reg, uint8_t value_reg) {
  graphion_vm_list *tuple;
  if (vm == NULL || !is_valid_reg(tuple_reg) || !is_valid_reg(value_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[tuple_reg].kind != GVM_VALUE_TUPLE) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  tuple = (graphion_vm_list *)vm->regs[tuple_reg].as.ref_value;
  return vm_list_append_value(tuple, &vm->regs[value_reg]);
}

static int vm_list_storage_contains_value(const graphion_vm_list *list,
                                          const graphion_vm_value *value,
                                          int *contains_out) {
  size_t i;

  if (list == NULL || value == NULL || contains_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  *contains_out = 0;
  for (i = 0U; i < list->count; ++i) {
    int compatible = 0;
    int equal = 0;
    int rc = vm_values_deep_equal(&list->items[i], value, &compatible, &equal);
    if (rc != GVM_OK) {
      return rc;
    }
    if (compatible && equal) {
      *contains_out = 1;
      return GVM_OK;
    }
  }
  return GVM_OK;
}

int vm_set_add_reg(graphion_vm *vm, uint8_t set_reg, uint8_t value_reg) {
  graphion_vm_list *set;
  int contains = 0;
  int rc;

  if (vm == NULL || !is_valid_reg(set_reg) || !is_valid_reg(value_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[set_reg].kind != GVM_VALUE_SET) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  set = (graphion_vm_list *)vm->regs[set_reg].as.ref_value;
  if (set == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  rc = vm_list_storage_contains_value(set, &vm->regs[value_reg], &contains);
  if (rc != GVM_OK || contains) {
    return rc;
  }
  return vm_list_append_value(set, &vm->regs[value_reg]);
}

int vm_collection_contains_reg(graphion_vm *vm, uint8_t collection_reg, uint8_t value_reg) {
  graphion_vm_list *collection;
  int contains = 0;
  int rc;

  if (vm == NULL || !is_valid_reg(collection_reg) || !is_valid_reg(value_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[collection_reg].kind != GVM_VALUE_SET && vm->regs[collection_reg].kind != GVM_VALUE_LIST &&
      vm->regs[collection_reg].kind != GVM_VALUE_TUPLE) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  collection = (graphion_vm_list *)vm->regs[collection_reg].as.ref_value;
  if (collection == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  rc = vm_list_storage_contains_value(collection, &vm->regs[value_reg], &contains);
  if (rc != GVM_OK) {
    return rc;
  }
  vm_free_owned_reg_string(vm, collection_reg);
  vm_value_set_bool(&vm->regs[collection_reg], contains);
  return GVM_OK;
}

int vm_reg_set_empty_graph(graphion_vm *vm, uint8_t reg) {
  return vm_reg_set_graph_node_count(vm, reg, 0U);
}

int vm_reg_set_graph_node_count(graphion_vm *vm, uint8_t reg, size_t node_count) {
  graphion_csr_graph *graph;
  if (vm == NULL || !is_valid_reg(reg)) {
    return GVM_ERR_INVALID_ARG;
  }
  if (node_count > (size_t)UINT32_MAX) {
    return GVM_ERR_INVALID_ARG;
  }
  graph = vm_graph_create_nodes(node_count);
  if (graph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->regs[reg].kind = GVM_VALUE_GRAPH_REF;
  vm->regs[reg].reserved[1] = (uint8_t)(node_count & 0xFFU);
  vm->regs[reg].reserved[2] = (uint8_t)((node_count >> 8U) & 0xFFU);
  vm->regs[reg].as.ref_value = graph;
  return GVM_OK;
}

int vm_reg_set_empty_hypergraph(graphion_vm *vm, uint8_t reg) {
  graphion_hypergraph *hypergraph;
  if (vm == NULL || !is_valid_reg(reg)) {
    return GVM_ERR_INVALID_ARG;
  }
  hypergraph = vm_hypergraph_create_empty();
  if (hypergraph == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->regs[reg].kind = GVM_VALUE_HYPERGRAPH_REF;
  vm->regs[reg].as.ref_value = hypergraph;
  return GVM_OK;
}

int vm_reg_set_empty_dict(graphion_vm *vm, uint8_t reg) {
  graphion_vm_dict *dict;
  if (vm == NULL || !is_valid_reg(reg)) {
    return GVM_ERR_INVALID_ARG;
  }
  dict = vm_dict_create();
  if (dict == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, reg);
  vm->regs[reg].kind = GVM_VALUE_DICT;
  vm->regs[reg].as.ref_value = dict;
  return GVM_OK;
}

static size_t vm_dict_find_index(const graphion_vm_dict *dict, const char *key) {
  size_t i;
  const char *lookup = key != NULL ? key : "";
  if (dict == NULL) {
    return (size_t)-1;
  }
  for (i = 0U; i < dict->count; ++i) {
    const char *entry_key = dict->entries[i].key != NULL ? dict->entries[i].key : "";
    if (strcmp(entry_key, lookup) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

int vm_dict_set_reg(graphion_vm *vm, uint8_t dict_reg, const char *key, uint8_t value_reg) {
  graphion_vm_dict *dict;
  size_t index;
  graphion_vm_value cloned;
  int has_existing = 0;

  if (vm == NULL || !is_valid_reg(dict_reg) || !is_valid_reg(value_reg) || key == NULL) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[dict_reg].kind != GVM_VALUE_DICT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  dict = (graphion_vm_dict *)vm->regs[dict_reg].as.ref_value;
  if (dict == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_value_clear(&cloned);
  if (vm_value_clone(&cloned, &vm->regs[value_reg]) != GVM_OK) {
    return GVM_ERR_INVALID_ARG;
  }
  index = vm_dict_find_index(dict, key);
  if (index == (size_t)-1) {
    if (dict->count == dict->capacity) {
      size_t new_capacity = dict->capacity == 0U ? 4U : dict->capacity * 2U;
      graphion_vm_dict_entry *new_entries =
          (graphion_vm_dict_entry *)realloc(dict->entries, new_capacity * sizeof(*new_entries));
      if (new_entries == NULL) {
        return GVM_ERR_INVALID_ARG;
      }
      dict->entries = new_entries;
      dict->capacity = new_capacity;
    }
    index = dict->count++;
    dict->entries[index].key = vm_strdup_text(key);
    if (dict->entries[index].key == NULL) {
      dict->count--;
      vm_value_dispose_owned(&cloned);
      return GVM_ERR_INVALID_ARG;
    }
    vm_value_clear(&dict->entries[index].value);
  } else {
    has_existing = 1;
    vm_value_dispose_owned(&dict->entries[index].value);
  }
  dict->entries[index].value = cloned;
  if (has_existing) {
    return GVM_OK;
  }
  return GVM_OK;
}

int vm_dict_set_element(graphion_vm *vm, uint8_t dict_reg, uint8_t key_reg, uint8_t value_reg) {
  const char *key;

  if (vm == NULL || !is_valid_reg(dict_reg) || !is_valid_reg(key_reg) || !is_valid_reg(value_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[key_reg].kind != GVM_VALUE_STRING) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  key = vm->regs[key_reg].as.string_value != NULL ? vm->regs[key_reg].as.string_value : "";
  return vm_dict_set_reg(vm, dict_reg, key, value_reg);
}

int vm_list_get_element(graphion_vm *vm, uint8_t list_reg, uint8_t index_reg) {
  const graphion_vm_value *item;
  graphion_vm_list *list;
  int64_t index_value;
  graphion_vm_value cloned;

  if (vm == NULL || !is_valid_reg(list_reg) || !is_valid_reg(index_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[list_reg].kind == GVM_VALUE_STRUCT) {
    graphion_struct_instance_value *instance = (graphion_struct_instance_value *)vm->regs[list_reg].as.ref_value;
    graphion_vm_dict *fields;
    const graphion_vm_value *field_item;
    size_t index;
    graphion_vm_value field_clone;
    const char *key;
    if (instance == NULL) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    if (vm->regs[index_reg].kind != GVM_VALUE_STRING) {
      return GVM_ERR_TYPE_MISMATCH;
    }
    fields = (graphion_vm_dict *)instance->fields.as.ref_value;
    if (fields == NULL) {
      return GVM_ERR_MISSING_KEY;
    }
    key = vm->regs[index_reg].as.string_value != NULL ? vm->regs[index_reg].as.string_value : "";
    index = vm_dict_find_index(fields, key);
    if (index == (size_t)-1) {
      return GVM_ERR_MISSING_KEY;
    }
    field_item = &fields->entries[index].value;
    if (field_item->kind == GVM_VALUE_STRING) {
      return vm_reg_set_string_copy(vm,
                                    list_reg,
                                    field_item->as.string_value != NULL ? field_item->as.string_value : "");
    }
    vm_value_clear(&field_clone);
    if (vm_value_clone(&field_clone, field_item) != GVM_OK) {
      return GVM_ERR_INVALID_ARG;
    }
    vm_free_owned_reg_string(vm, list_reg);
    vm->regs[list_reg] = field_clone;
    return GVM_OK;
  }
  if (vm->regs[list_reg].kind == GVM_VALUE_DICT) {
    return vm_dict_get_element(vm, list_reg, index_reg);
  }
  if (!vm_value_is_sequence_kind(vm->regs[list_reg].kind)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (!vm_value_get_int(&vm->regs[index_reg], &index_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  list = (graphion_vm_list *)vm->regs[list_reg].as.ref_value;
  if (index_value < 0 || list == NULL || (size_t)index_value >= list->count) {
    return GVM_ERR_INDEX_OUT_OF_RANGE;
  }
  item = &list->items[(size_t)index_value];
  if (item->kind == GVM_VALUE_STRING) {
    return vm_reg_set_string_copy(vm, list_reg, item->as.string_value != NULL ? item->as.string_value : "");
  }
  vm_value_clear(&cloned);
  if (vm_value_clone(&cloned, item) != GVM_OK) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, list_reg);
  vm->regs[list_reg] = cloned;
  return GVM_OK;
}

int vm_dict_get_element(graphion_vm *vm, uint8_t dict_reg, uint8_t key_reg) {
  graphion_vm_dict *dict;
  const graphion_vm_value *item;
  size_t index;
  graphion_vm_value cloned;
  const char *key;

  if (vm == NULL || !is_valid_reg(dict_reg) || !is_valid_reg(key_reg)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->regs[dict_reg].kind != GVM_VALUE_DICT || vm->regs[key_reg].kind != GVM_VALUE_STRING) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  dict = (graphion_vm_dict *)vm->regs[dict_reg].as.ref_value;
  key = vm->regs[key_reg].as.string_value != NULL ? vm->regs[key_reg].as.string_value : "";
  index = vm_dict_find_index(dict, key);
  if (index == (size_t)-1) {
    return GVM_ERR_MISSING_KEY;
  }
  item = &dict->entries[index].value;
  if (item->kind == GVM_VALUE_STRING) {
    return vm_reg_set_string_copy(vm, dict_reg, item->as.string_value != NULL ? item->as.string_value : "");
  }
  vm_value_clear(&cloned);
  if (vm_value_clone(&cloned, item) != GVM_OK) {
    return GVM_ERR_INVALID_ARG;
  }
  vm_free_owned_reg_string(vm, dict_reg);
  vm->regs[dict_reg] = cloned;
  return GVM_OK;
}

int vm_write_bytes(FILE *output, const char *bytes, size_t len) {
  if (output == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (bytes == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (len == 0U) {
    return GVM_OK;
  }
  return fwrite(bytes, 1U, len, output) == len ? GVM_OK : GVM_ERR_OUTPUT_UNBOUND;
}

int vm_write_bytes_sink(const graphion_output_sink *sink, const char *bytes, size_t len) {
  if (sink == NULL || sink->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  return sink->write(sink->ctx, bytes, len);
}

int vm_file_output_write(void *ctx, const char *bytes, size_t len) {
  return vm_write_bytes((FILE *)ctx, bytes, len);
}

int vm_count_output_write(void *ctx, const char *bytes, size_t len) {
  uint64_t *byte_count = (uint64_t *)ctx;
  (void)bytes;
  if (byte_count == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  *byte_count += (uint64_t)len;
  return GVM_OK;
}

int vm_sink_is_counter(const graphion_output_sink *sink) {
  return sink != NULL && sink->write == vm_count_output_write;
}

size_t vm_write_i64_text(char *buffer, int64_t value) {
  uint64_t magnitude;
  size_t digits = 0U;
  size_t i = 0U;

  if (value < 0) {
    buffer[i++] = '-';
    magnitude = (uint64_t)(-(value + 1)) + 1U;
  } else {
    magnitude = (uint64_t)value;
  }

  do {
    buffer[i + digits] = (char)('0' + (magnitude % 10U));
    magnitude /= 10U;
    digits += 1U;
  } while (magnitude != 0U);

  {
    size_t start = i;
    size_t end = i + digits - 1U;
    while (start < end) {
      char tmp = buffer[start];
      buffer[start] = buffer[end];
      buffer[end] = tmp;
      start += 1U;
      end -= 1U;
    }
  }
  return i + digits;
}

static int vm_write_value_sink_inline_ex(const graphion_output_sink *output,
                                         const graphion_vm_value *value,
                                         int string_as_list_item);

int vm_value_text_len(const graphion_vm_value *value, size_t *len_out) {
  char buffer[128];
  int written;
  size_t len;
  size_t total;
  size_t i;
  graphion_vm_list *list;
  graphion_vm_dict *dict;
  graphion_struct_type_value *struct_type;
  graphion_struct_instance_value *struct_instance;

  if (value == NULL || len_out == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  switch (value->kind) {
    case GVM_VALUE_NONE:
      *len_out = 5U;
      return GVM_OK;
    case GVM_VALUE_INT:
      len = vm_write_i64_text(buffer, value->as.int_value);
      *len_out = len + 1U;
      return GVM_OK;
    case GVM_VALUE_FLOAT:
      written = snprintf(buffer, sizeof(buffer), "%g\n", value->as.float_value);
      if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      *len_out = (size_t)written;
      return GVM_OK;
    case GVM_VALUE_BOOL:
      *len_out = value->as.bool_value != 0 ? 5U : 6U;
      return GVM_OK;
    case GVM_VALUE_STRING:
      *len_out = value->as.string_value != NULL ? strlen(value->as.string_value) + 1U : 1U;
      return GVM_OK;
    case GVM_VALUE_BITS:
      *len_out = (size_t)vm_value_get_bits_width(value) + 3U;
      return GVM_OK;
    case GVM_VALUE_GRAPH_REF:
      {
        const graphion_csr_graph *graph = (const graphion_csr_graph *)value->as.ref_value;
        const size_t visible_nodes = vm_graph_visible_node_count(value, graph);
        const size_t visible_node_attr_keys = vm_graph_visible_node_attr_key_count(value);
        const size_t visible_edge_attr_keys = vm_graph_visible_edge_attr_key_count(value);
        if (graph != NULL && visible_nodes > 0U) {
          const size_t visible_edges = vm_graph_visible_edge_count(value, graph);
          if (visible_edges > 0U && visible_node_attr_keys > 0U && visible_edge_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu, node_attrs=%zu, edge_attrs=%zu)\n",
                               visible_nodes,
                               visible_edges,
                               visible_node_attr_keys,
                               visible_edge_attr_keys);
          } else if (visible_edges > 0U && visible_node_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu, node_attrs=%zu)\n",
                               visible_nodes,
                               visible_edges,
                               visible_node_attr_keys);
          } else if (visible_edges > 0U && visible_edge_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu, edge_attrs=%zu)\n",
                               visible_nodes,
                               visible_edges,
                               visible_edge_attr_keys);
          } else if (visible_edges > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu)\n",
                               visible_nodes,
                               visible_edges);
          } else if (visible_node_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, node_attrs=%zu)\n",
                               visible_nodes,
                               visible_node_attr_keys);
          } else {
            written = snprintf(buffer, sizeof(buffer), "graph(nodes=%zu)\n", visible_nodes);
          }
          if (written < 0 || (size_t)written >= sizeof(buffer)) {
            return GVM_ERR_OUTPUT_UNBOUND;
          }
          *len_out = (size_t)written;
          return GVM_OK;
        }
        *len_out = 8U;
      }
      return GVM_OK;
    case GVM_VALUE_HYPERGRAPH_REF:
      {
        const graphion_hypergraph *hypergraph = (const graphion_hypergraph *)value->as.ref_value;
        const size_t visible_vertices = (size_t)value->reserved[1] | ((size_t)value->reserved[2] << 8U);
        const size_t active_hyperedges = vm_hypergraph_active_hyperedge_count(hypergraph);
        const size_t visible_vertex_attr_keys = vm_hypergraph_visible_vertex_attr_key_count(value);
        const size_t visible_hyperedge_attr_keys = vm_hypergraph_visible_hyperedge_attr_key_count(value);
        if (hypergraph != NULL && (hypergraph->node_count > 0U || active_hyperedges > 0U)) {
          if (active_hyperedges > 0U) {
            if (visible_vertex_attr_keys > 0U && visible_hyperedge_attr_keys > 0U) {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu, vertex_attrs=%zu, hyperedge_attrs=%zu)\n",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges,
                                 visible_vertex_attr_keys,
                                 visible_hyperedge_attr_keys);
            } else if (visible_vertex_attr_keys > 0U) {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu, vertex_attrs=%zu)\n",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges,
                                 visible_vertex_attr_keys);
            } else if (visible_hyperedge_attr_keys > 0U) {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu, hyperedge_attrs=%zu)\n",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges,
                                 visible_hyperedge_attr_keys);
            } else {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu)\n",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges);
            }
          } else if (visible_vertex_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "hypergraph(vertices=%zu, vertex_attrs=%zu)\n",
                               visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                               visible_vertex_attr_keys);
          } else {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "hypergraph(vertices=%zu)\n",
                               visible_vertices != 0U ? visible_vertices : hypergraph->node_count);
          }
          if (written < 0 || (size_t)written >= sizeof(buffer)) {
            return GVM_ERR_OUTPUT_UNBOUND;
          }
          *len_out = (size_t)written;
          return GVM_OK;
        }
        *len_out = 13U;
      }
      return GVM_OK;
    case GVM_VALUE_LIST:
    case GVM_VALUE_TUPLE:
      total = 3U;
      list = (graphion_vm_list *)value->as.ref_value;
      if (list == NULL || list->count == 0U) {
        *len_out = total;
        return GVM_OK;
      }
      for (i = 0U; i < list->count; ++i) {
        size_t item_len = 0U;
        int rc;
        if (i > 0U) {
          total += 2U;
        }
        rc = vm_value_text_len(&list->items[i], &item_len);
        if (rc != GVM_OK) {
          return rc;
        }
        if (list->items[i].kind == GVM_VALUE_STRING && item_len > 0U) {
          item_len += 2U;
        }
        total += item_len - 1U;
      }
      *len_out = total;
      return GVM_OK;
    case GVM_VALUE_SET:
      total = 6U;
      list = (graphion_vm_list *)value->as.ref_value;
      if (list == NULL || list->count == 0U) {
        *len_out = total;
        return GVM_OK;
      }
      for (i = 0U; i < list->count; ++i) {
        size_t item_len = 0U;
        int rc;
        if (i > 0U) {
          total += 2U;
        }
        rc = vm_value_text_len(&list->items[i], &item_len);
        if (rc != GVM_OK) {
          return rc;
        }
        if (list->items[i].kind == GVM_VALUE_STRING && item_len > 0U) {
          item_len += 2U;
        }
        total += item_len - 1U;
      }
      *len_out = total;
      return GVM_OK;
    case GVM_VALUE_DICT:
      total = 3U;
      dict = (graphion_vm_dict *)value->as.ref_value;
      if (dict == NULL || dict->count == 0U) {
        *len_out = total;
        return GVM_OK;
      }
      for (i = 0U; i < dict->count; ++i) {
        size_t item_len = 0U;
        int rc;
        const char *key = dict->entries[i].key != NULL ? dict->entries[i].key : "";
        if (i > 0U) {
          total += 2U;
        }
        total += strlen(key) + 4U;
        rc = vm_value_text_len(&dict->entries[i].value, &item_len);
        if (rc != GVM_OK) {
          return rc;
        }
        if (dict->entries[i].value.kind == GVM_VALUE_STRING && item_len > 0U) {
          item_len += 2U;
        }
        total += item_len - 1U;
      }
      *len_out = total;
      return GVM_OK;
    case GVM_VALUE_STRUCT_TYPE:
      struct_type = (graphion_struct_type_value *)value->as.ref_value;
      written = snprintf(buffer,
                         sizeof(buffer),
                         "struct %s(fields=%zu)\n",
                         struct_type != NULL ? struct_type->name : "",
                         struct_type != NULL ? struct_type->field_count : 0U);
      if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      *len_out = (size_t)written;
      return GVM_OK;
    case GVM_VALUE_STRUCT:
      struct_instance = (graphion_struct_instance_value *)value->as.ref_value;
      if (struct_instance == NULL) {
        *len_out = 9U;
        return GVM_OK;
      }
      if (vm_value_text_len(&struct_instance->fields, &len) != GVM_OK) {
        return GVM_ERR_TYPE_MISMATCH;
      }
      *len_out = strlen(struct_instance->type_name) + len;
      return GVM_OK;
    default:
      return GVM_ERR_TYPE_MISMATCH;
  }
}

static int vm_write_list_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  size_t i;
  graphion_vm_list *list;
  int rc;

  if (vm_write_bytes_sink(output, "[", 1U) != GVM_OK) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  list = (graphion_vm_list *)value->as.ref_value;
  if (list != NULL) {
    for (i = 0U; i < list->count; ++i) {
      if (i > 0U && vm_write_bytes_sink(output, ", ", 2U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      rc = vm_write_value_sink_inline_ex(output, &list->items[i], 1);
      if (rc != GVM_OK) {
        return rc;
      }
    }
  }
  return vm_write_bytes_sink(output, "]", 1U);
}

static int vm_write_tuple_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  size_t i;
  graphion_vm_list *tuple;
  int rc;

  if (vm_write_bytes_sink(output, "(", 1U) != GVM_OK) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  tuple = (graphion_vm_list *)value->as.ref_value;
  if (tuple != NULL) {
    for (i = 0U; i < tuple->count; ++i) {
      if (i > 0U && vm_write_bytes_sink(output, ", ", 2U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      rc = vm_write_value_sink_inline_ex(output, &tuple->items[i], 1);
      if (rc != GVM_OK) {
        return rc;
      }
    }
  }
  return vm_write_bytes_sink(output, ")", 1U);
}

static int vm_write_set_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  size_t i;
  graphion_vm_list *set;
  int rc;

  if (vm_write_bytes_sink(output, "set(", 4U) != GVM_OK) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  set = (graphion_vm_list *)value->as.ref_value;
  if (set != NULL) {
    for (i = 0U; i < set->count; ++i) {
      if (i > 0U && vm_write_bytes_sink(output, ", ", 2U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      rc = vm_write_value_sink_inline_ex(output, &set->items[i], 1);
      if (rc != GVM_OK) {
        return rc;
      }
    }
  }
  return vm_write_bytes_sink(output, ")", 1U);
}

static int vm_write_dict_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  size_t i;
  graphion_vm_dict *dict;
  int rc;

  if (vm_write_bytes_sink(output, "{", 1U) != GVM_OK) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  dict = (graphion_vm_dict *)value->as.ref_value;
  if (dict != NULL) {
    for (i = 0U; i < dict->count; ++i) {
      const char *key = dict->entries[i].key != NULL ? dict->entries[i].key : "";
      if (i > 0U && vm_write_bytes_sink(output, ", ", 2U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      if (vm_write_bytes_sink(output, "\"", 1U) != GVM_OK ||
          vm_write_bytes_sink(output, key, strlen(key)) != GVM_OK ||
          vm_write_bytes_sink(output, "\": ", 3U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      rc = vm_write_value_sink_inline_ex(output, &dict->entries[i].value, 1);
      if (rc != GVM_OK) {
        return rc;
      }
    }
  }
  return vm_write_bytes_sink(output, "}", 1U);
}

static int vm_write_struct_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  const graphion_struct_instance_value *instance;

  if (value == NULL || value->kind != GVM_VALUE_STRUCT) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  instance = (const graphion_struct_instance_value *)value->as.ref_value;
  if (instance == NULL) {
    return vm_write_bytes_sink(output, "struct{}", 8U);
  }
  if (vm_write_bytes_sink(output, instance->type_name, strlen(instance->type_name)) != GVM_OK) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  return vm_write_dict_inline(output, &instance->fields);
}

static int vm_write_value_sink_inline_ex(const graphion_output_sink *output,
                                         const graphion_vm_value *value,
                                         int string_as_list_item) {
  char buffer[128];
  int written;
  size_t len;

  if (output == NULL || output->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  switch (value->kind) {
    case GVM_VALUE_NONE:
      return vm_write_bytes_sink(output, "none", 4U);
    case GVM_VALUE_INT:
      len = vm_write_i64_text(buffer, value->as.int_value);
      return vm_write_bytes_sink(output, buffer, len);
    case GVM_VALUE_FLOAT:
      written = snprintf(buffer, sizeof(buffer), "%g", value->as.float_value);
      if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, buffer, (size_t)written);
    case GVM_VALUE_BOOL:
      return value->as.bool_value != 0 ? vm_write_bytes_sink(output, "true", 4U)
                                       : vm_write_bytes_sink(output, "false", 5U);
    case GVM_VALUE_STRING:
      if (string_as_list_item && vm_write_bytes_sink(output, "\"", 1U) != GVM_OK) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      if (value->as.string_value != NULL) {
        len = strlen(value->as.string_value);
        if (vm_write_bytes_sink(output, value->as.string_value, len) != GVM_OK) {
          return GVM_ERR_OUTPUT_UNBOUND;
        }
      }
      if (string_as_list_item) {
        return vm_write_bytes_sink(output, "\"", 1U);
      }
      return GVM_OK;
    case GVM_VALUE_BITS:
      len = vm_write_bits_text(buffer, sizeof(buffer), value, 0);
      if (len == 0U) {
        return GVM_ERR_OUTPUT_UNBOUND;
      }
      return vm_write_bytes_sink(output, buffer, len);
    case GVM_VALUE_LIST:
      return vm_write_list_inline(output, value);
    case GVM_VALUE_TUPLE:
      return vm_write_tuple_inline(output, value);
    case GVM_VALUE_SET:
      return vm_write_set_inline(output, value);
    case GVM_VALUE_STRUCT_TYPE:
      {
        const graphion_struct_type_value *type_value = (const graphion_struct_type_value *)value->as.ref_value;
        written = snprintf(buffer,
                           sizeof(buffer),
                           "struct %s(fields=%zu)",
                           type_value != NULL ? type_value->name : "",
                           type_value != NULL ? type_value->field_count : 0U);
        if (written < 0 || (size_t)written >= sizeof(buffer)) {
          return GVM_ERR_OUTPUT_UNBOUND;
        }
        return vm_write_bytes_sink(output, buffer, (size_t)written);
      }
    case GVM_VALUE_STRUCT:
      return vm_write_struct_inline(output, value);
    case GVM_VALUE_GRAPH_REF:
      {
        const graphion_csr_graph *graph = (const graphion_csr_graph *)value->as.ref_value;
        const size_t visible_nodes = vm_graph_visible_node_count(value, graph);
        const size_t visible_node_attr_keys = vm_graph_visible_node_attr_key_count(value);
        const size_t visible_edge_attr_keys = vm_graph_visible_edge_attr_key_count(value);
        if (graph != NULL && visible_nodes > 0U) {
          const size_t visible_edges = vm_graph_visible_edge_count(value, graph);
          if (visible_edges > 0U && visible_node_attr_keys > 0U && visible_edge_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu, node_attrs=%zu, edge_attrs=%zu)",
                               visible_nodes,
                               visible_edges,
                               visible_node_attr_keys,
                               visible_edge_attr_keys);
          } else if (visible_edges > 0U && visible_node_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu, node_attrs=%zu)",
                               visible_nodes,
                               visible_edges,
                               visible_node_attr_keys);
          } else if (visible_edges > 0U && visible_edge_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu, edge_attrs=%zu)",
                               visible_nodes,
                               visible_edges,
                               visible_edge_attr_keys);
          } else if (visible_edges > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, edges=%zu)",
                               visible_nodes,
                               visible_edges);
          } else if (visible_node_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "graph(nodes=%zu, node_attrs=%zu)",
                               visible_nodes,
                               visible_node_attr_keys);
          } else {
            written = snprintf(buffer, sizeof(buffer), "graph(nodes=%zu)", visible_nodes);
          }
          if (written < 0 || (size_t)written >= sizeof(buffer)) {
            return GVM_ERR_OUTPUT_UNBOUND;
          }
          return vm_write_bytes_sink(output, buffer, (size_t)written);
        }
        return vm_write_bytes_sink(output, "graph()", 7U);
      }
    case GVM_VALUE_HYPERGRAPH_REF:
      {
        const graphion_hypergraph *hypergraph = (const graphion_hypergraph *)value->as.ref_value;
        const size_t visible_vertices = (size_t)value->reserved[1] | ((size_t)value->reserved[2] << 8U);
        const size_t active_hyperedges = vm_hypergraph_active_hyperedge_count(hypergraph);
        const size_t visible_vertex_attr_keys = vm_hypergraph_visible_vertex_attr_key_count(value);
        const size_t visible_hyperedge_attr_keys = vm_hypergraph_visible_hyperedge_attr_key_count(value);
        if (hypergraph != NULL && (hypergraph->node_count > 0U || active_hyperedges > 0U)) {
          if (active_hyperedges > 0U) {
            if (visible_vertex_attr_keys > 0U && visible_hyperedge_attr_keys > 0U) {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu, vertex_attrs=%zu, hyperedge_attrs=%zu)",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges,
                                 visible_vertex_attr_keys,
                                 visible_hyperedge_attr_keys);
            } else if (visible_vertex_attr_keys > 0U) {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu, vertex_attrs=%zu)",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges,
                                 visible_vertex_attr_keys);
            } else if (visible_hyperedge_attr_keys > 0U) {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu, hyperedge_attrs=%zu)",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges,
                                 visible_hyperedge_attr_keys);
            } else {
              written = snprintf(buffer,
                                 sizeof(buffer),
                                 "hypergraph(vertices=%zu, hyperedges=%zu)",
                                 visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                                 active_hyperedges);
            }
          } else if (visible_vertex_attr_keys > 0U) {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "hypergraph(vertices=%zu, vertex_attrs=%zu)",
                               visible_vertices != 0U ? visible_vertices : hypergraph->node_count,
                               visible_vertex_attr_keys);
          } else {
            written = snprintf(buffer,
                               sizeof(buffer),
                               "hypergraph(vertices=%zu)",
                               visible_vertices != 0U ? visible_vertices : hypergraph->node_count);
          }
          if (written < 0 || (size_t)written >= sizeof(buffer)) {
            return GVM_ERR_OUTPUT_UNBOUND;
          }
          return vm_write_bytes_sink(output, buffer, (size_t)written);
        }
        return vm_write_bytes_sink(output, "hypergraph()", 12U);
      }
    case GVM_VALUE_DICT:
      return vm_write_dict_inline(output, value);
    default:
      return GVM_ERR_TYPE_MISMATCH;
  }
}

int vm_write_value_sink(const graphion_output_sink *output, const graphion_vm_value *value) {
  int rc;
  size_t len;
  if (output == NULL || output->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm_sink_is_counter(output)) {
    uint64_t *byte_count = (uint64_t *)output->ctx;
    rc = vm_value_text_len(value, &len);
    if (rc != GVM_OK) {
      return rc;
    }
    if (byte_count == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    *byte_count += (uint64_t)len;
    return GVM_OK;
  }
  rc = vm_write_value_sink_inline_ex(output, value, 0);
  if (rc != GVM_OK) {
    return rc;
  }
  return vm_write_bytes_sink(output, "\n", 1U);
}

int vm_write_value_sink_inline(const graphion_output_sink *output, const graphion_vm_value *value) {
  size_t len;
  int rc;
  if (output == NULL || output->write == NULL) {
    return GVM_ERR_OUTPUT_UNBOUND;
  }
  if (value == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm_sink_is_counter(output)) {
    uint64_t *byte_count = (uint64_t *)output->ctx;
    rc = vm_value_text_len(value, &len);
    if (rc != GVM_OK) {
      return rc;
    }
    if (byte_count == NULL || len == 0U) {
      return GVM_ERR_INVALID_ARG;
    }
    *byte_count += (uint64_t)(len - 1U);
    return GVM_OK;
  }
  return vm_write_value_sink_inline_ex(output, value, 0);
}

int vm_reg_get_int(const graphion_vm *vm, uint8_t reg, int64_t *out_value) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return 0;
  }
  return vm_value_get_int(&vm->regs[reg], out_value);
}

void vm_reg_set_int(graphion_vm *vm, uint8_t reg, int64_t value) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  vm_value_set_int(&vm->regs[reg], value);
}

int vm_copy_regs_to_raw_i64(const graphion_vm *vm, int64_t raw_regs[16]) {
  size_t i;
  if (vm == NULL || raw_regs == NULL) {
    return 0;
  }
  for (i = 0U; i < 16U; ++i) {
    if (!vm_value_get_int(&vm->regs[i], &raw_regs[i])) {
      return 0;
    }
  }
  return 1;
}

void vm_copy_raw_i64_to_regs(graphion_vm *vm, const int64_t raw_regs[16]) {
  size_t i;
  if (vm == NULL || raw_regs == NULL) {
    return;
  }
  for (i = 0U; i < 16U; ++i) {
    vm_reg_set_int(vm, (uint8_t)i, raw_regs[i]);
  }
}
