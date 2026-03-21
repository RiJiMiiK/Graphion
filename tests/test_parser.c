/* SPDX-License-Identifier: MIT */

#include "parser/bytecode.h"
#include "parser/frontend.h"
#include "compiler/ir.h"
#include "graph/csr_graph.h"
#include "graph/hypergraph.h"
#include "runtime/entry.h"
#include "runtime/interpreter.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int test_parser_decode_valid_program(void) {
  const uint8_t bytes[] = {
      GVM_OP_MOV_IMM, 0, 0, 7, 0, 0, 0,   GVM_OP_MOV_IMM, 1, 0, 35, 0, 0, 0,
      GVM_OP_ADD,     0, 1, 0, 0, 0, 0,   GVM_OP_HALT,    0, 0, 0,  0, 0, 0,
  };
  graphion_insn program[8];
  size_t count = 0U;
  int rc;

  rc = graphion_decode_bytecode(bytes, sizeof(bytes), program, 8U, &count);
  if (rc != 0) {
    return 1;
  }
  if (count != 4U) {
    return 2;
  }
  if (program[1].imm != 35 || program[2].op != GVM_OP_ADD) {
    return 3;
  }
  return 0;
}

int test_parser_rejects_truncated_input(void) {
  const uint8_t bytes[] = {GVM_OP_HALT, 0, 0, 0, 0, 0};
  graphion_insn program[2];
  size_t count = 0U;
  int rc;

  rc = graphion_decode_bytecode(bytes, sizeof(bytes), program, 2U, &count);
  if (rc != GBC_ERR_TRUNCATED) {
    return 1;
  }
  return 0;
}

int test_frontend_parse_and_ir_lowering(void) {
  const char *source = "mov r0, 7\n"
                       "mov r1, 35\n"
                       "add r0, r1\n"
                       "incident_sum r0, r2\n"
                       "halt\n";
  graphion_ir_insn ir[8];
  graphion_insn program[8];
  size_t ir_count = 0U;
  size_t program_count = 0U;
  int rc;

  rc = graphion_parse_source_to_ir(source, ir, 8U, &ir_count);
  if (rc != GFE_OK) {
    return 1;
  }
  if (ir_count != 5U) {
    return 2;
  }
  if (ir[0].op != GIR_OP_MOV_IMM || ir[0].imm != 7 || ir[3].op != GIR_OP_INCIDENT_SUM) {
    return 3;
  }

  rc = graphion_ir_lower_to_bytecode(ir, ir_count, program, 8U, &program_count);
  if (rc != GIR_OK) {
    return 4;
  }
  if (program_count != ir_count) {
    return 5;
  }
  if (program[2].op != GVM_OP_ADD || program[4].op != GVM_OP_HALT) {
    return 6;
  }
  return 0;
}

int test_frontend_rejects_invalid_source(void) {
  const char *source = "mov r0, nope\n";
  graphion_ir_insn ir[4];
  size_t count = 0U;
  int rc = graphion_parse_source_to_ir(source, ir, 4U, &count);
  if (rc != GFE_ERR_PARSE) {
    return 1;
  }
  return 0;
}

int test_frontend_source_to_vm_execution(void) {
  const char *source = "mov r0, 7\n"
                       "mov r1, 35\n"
                       "add r0, r1\n"
                       "halt\n";
  graphion_ir_insn ir[8];
  graphion_insn program[8];
  size_t ir_count = 0U;
  size_t program_count = 0U;
  graphion_vm vm;
  int rc;

  rc = graphion_parse_source_to_ir(source, ir, 8U, &ir_count);
  if (rc != GFE_OK) {
    return 1;
  }
  rc = graphion_ir_lower_to_bytecode(ir, ir_count, program, 8U, &program_count);
  if (rc != GIR_OK) {
    return 2;
  }

  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, program, program_count);
  if (rc != 0) {
    return 3;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 4;
  }
  if (!vm.halted || vm.regs[0] != 42) {
    return 5;
  }
  return 0;
}

int test_frontend_reference_graph_execution_examples(void) {
  typedef struct {
    const char *name;
    const char *source;
    int bind_csr;
    int bind_hypergraph;
    int bind_frontier;
    int64_t expected_regs[4];
    size_t expected_frontier_input_len;
    uint32_t expected_frontier_input[8];
    size_t expected_frontier_output_len;
    uint32_t expected_frontier_output[8];
  } example_case;
  static const example_case examples[] = {
      {
          "frontier_pipeline",
          "frontier_clear r0, 0\n"
          "frontier_filter_lt_imm r1, 7\n"
          "frontier_swap r2, 0\n"
          "frontier_map_add_imm r3, 1\n"
          "frontier_swap r4, 0\n"
          "frontier_reduce_sum r5, 0\n"
          "halt\n",
          0,
          0,
          1,
          {0, 2, 2, 2},
          2U,
          {2U, 5U, 0U, 0U, 0U, 0U, 0U, 0U},
          0U,
          {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "neighbor_traversal",
          "mov r0, 2\n"
          "neighbors_of r0, 0\n"
          "frontier_swap r1, 0\n"
          "neighbors_expand r2, 0\n"
          "halt\n",
          1,
          0,
          1,
          {2, 2, 3, 0},
          2U,
          {0U, 3U, 0U, 0U, 0U, 0U, 0U, 0U},
          3U,
          {1U, 2U, 1U, 0U, 0U, 0U, 0U, 0U},
      },
      {
          "hyperedge_traversal",
          "mov r0, 1\n"
          "incident_of r0, 0\n"
          "frontier_swap r1, 0\n"
          "mov r2, 1\n"
          "hyperedge_nodes_of r2, 0\n"
          "halt\n",
          0,
          1,
          1,
          {1, 2, 1, 0},
          2U,
          {0U, 1U, 0U, 0U, 0U, 0U, 0U, 0U},
          2U,
          {1U, 3U, 0U, 0U, 0U, 0U, 0U, 0U},
      },
  };
  const uint32_t csr_offsets[] = {0U, 2U, 3U, 5U, 6U};
  const uint32_t csr_neighbors[] = {1U, 2U, 3U, 0U, 3U, 1U};
  const uint32_t hg_node_offsets[] = {0U, 1U, 3U, 5U, 7U};
  const uint32_t hg_node_hyperedges[] = {0U, 0U, 1U, 0U, 2U, 1U, 2U};
  const uint32_t hg_hyperedge_offsets[] = {0U, 3U, 5U, 7U};
  const uint32_t hg_hyperedge_nodes[] = {0U, 1U, 2U, 1U, 3U, 2U, 3U};
  graphion_csr_graph csr_graph;
  graphion_hypergraph hypergraph;
  int32_t levels[4];
  uint32_t queue[4];
  uint32_t frontier_a[8];
  uint32_t frontier_b[8];
  size_t i;
  int rc;

  rc = graphion_csr_graph_init(&csr_graph, 4U, 6U, csr_offsets, csr_neighbors);
  if (rc != 0) {
    return 10;
  }
  rc = graphion_hypergraph_init(&hypergraph,
                                4U,
                                3U,
                                7U,
                                hg_node_offsets,
                                hg_node_hyperedges,
                                hg_hyperedge_offsets,
                                hg_hyperedge_nodes);
  if (rc != 0) {
    return 11;
  }

  for (i = 0U; i < sizeof(examples) / sizeof(examples[0]); ++i) {
    graphion_ir_insn ir[16];
    graphion_insn program[16];
    size_t ir_count = 0U;
    size_t program_count = 0U;
    graphion_vm vm;

    rc = graphion_parse_source_to_ir(examples[i].source, ir, 16U, &ir_count);
    if (rc != GFE_OK) {
      return (int)(20 + i);
    }
    rc = graphion_ir_lower_to_bytecode(ir, ir_count, program, 16U, &program_count);
    if (rc != GIR_OK) {
      return (int)(30 + i);
    }

    graphion_vm_init(&vm);
    if (examples[i].bind_csr) {
      memset(levels, 0, sizeof(levels));
      memset(queue, 0, sizeof(queue));
      graphion_vm_bind_csr(&vm, &csr_graph, levels, queue, 4U);
    }
    if (examples[i].bind_hypergraph) {
      graphion_vm_bind_hypergraph(&vm, &hypergraph);
    }
    if (examples[i].bind_frontier) {
      memset(frontier_a, 0, sizeof(frontier_a));
      memset(frontier_b, 0, sizeof(frontier_b));
      frontier_a[0] = 1U;
      frontier_a[1] = 4U;
      frontier_a[2] = 7U;
      frontier_a[3] = 10U;
      graphion_vm_bind_frontier(&vm, frontier_a, 4U, frontier_b, 8U);
    }
    rc = graphion_vm_load(&vm, program, program_count);
    if (rc != 0) {
      return (int)(40 + i);
    }
    rc = graphion_vm_run(&vm);
    if (rc != 0 || !vm.halted) {
      return (int)(50 + i);
    }
    if (vm.regs[0] != examples[i].expected_regs[0] || vm.regs[1] != examples[i].expected_regs[1] ||
        vm.regs[2] != examples[i].expected_regs[2] || vm.regs[3] != examples[i].expected_regs[3]) {
      return (int)(60 + i);
    }
    if (vm.frontier_input_len != examples[i].expected_frontier_input_len) {
      return (int)(70 + i);
    }
    if (vm.frontier_output_len != examples[i].expected_frontier_output_len) {
      return (int)(90 + i);
    }
    if (memcmp(vm.frontier_input,
               examples[i].expected_frontier_input,
               vm.frontier_input_len * sizeof(uint32_t)) != 0) {
      return (int)(110 + i);
    }
    if (memcmp(vm.frontier_output,
               examples[i].expected_frontier_output,
               vm.frontier_output_len * sizeof(uint32_t)) != 0) {
      return (int)(130 + i);
    }
  }

  return 0;
}

int test_gion_source_path_detection(void) {
  if (!graphion_source_path_is_gion("main.gion")) {
    return 1;
  }
  if (!graphion_source_path_is_gion("C:\\tmp\\demo.gion")) {
    return 2;
  }
  if (graphion_source_path_is_gion("demo.gio")) {
    return 3;
  }
  if (graphion_source_path_is_gion("demo.gion.txt")) {
    return 4;
  }
  return 0;
}

static int expect_int_binding(const graphion_runtime_scope *scope, const char *name, int64_t expected) {
  const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
  if (value == NULL || value->kind != GRAPHION_VALUE_INT) {
    return 0;
  }
  return value->int_value == expected;
}

static int expect_float_binding(const graphion_runtime_scope *scope, const char *name, double expected) {
  const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
  if (value == NULL || value->kind != GRAPHION_VALUE_FLOAT) {
    return 0;
  }
  return value->float_value == expected;
}

static int expect_bool_binding(const graphion_runtime_scope *scope, const char *name, int expected) {
  const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
  if (value == NULL || value->kind != GRAPHION_VALUE_BOOL) {
    return 0;
  }
  return value->bool_value == expected;
}

static int expect_string_binding(const graphion_runtime_scope *scope, const char *name, const char *expected) {
  const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
  if (value == NULL || value->kind != GRAPHION_VALUE_STRING) {
    return 0;
  }
  return strcmp(value->string_value, expected) == 0;
}

static int expect_graph_binding(const graphion_runtime_scope *scope,
                                const char *name,
                                size_t expected_edge_count,
                                size_t expected_node_count,
                                int64_t first_source,
                                int64_t first_target,
                                int64_t second_source,
                                int64_t second_target) {
  const graphion_runtime_value *value = graphion_runtime_scope_find(scope, name);
  if (value == NULL || value->kind != GRAPHION_VALUE_GRAPH) {
    return 0;
  }
  if (value->graph_edge_count != expected_edge_count) {
    return 0;
  }
  if (value->graph_node_count != expected_node_count) {
    return 0;
  }
  if (expected_edge_count >= 1U &&
      (value->graph_edges[0].source != first_source || value->graph_edges[0].target != first_target)) {
    return 0;
  }
  if (expected_edge_count >= 2U &&
      (value->graph_edges[1].source != second_source || value->graph_edges[1].target != second_target)) {
    return 0;
  }
  return 1;
}

int test_interpreter_dynamic_assignments(void) {
  const char *source = "count = 7\n"
                       "ratio = 3.5\n"
                       "name = \"graphion\"\n"
                       "ready = true\n"
                       "copy = count\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source(source, &scope, &diagnostic);
  if (rc != GINT_OK) {
    return 1;
  }
  if (!expect_int_binding(&scope, "count", 7) || !expect_int_binding(&scope, "copy", 7)) {
    return 2;
  }
  if (!expect_float_binding(&scope, "ratio", 3.5)) {
    return 3;
  }
  if (!expect_string_binding(&scope, "name", "graphion")) {
    return 4;
  }
  if (!expect_bool_binding(&scope, "ready", 1)) {
    return 5;
  }
  return 0;
}

int test_interpreter_rejects_declared_type_syntax(void) {
  static const char *bad_sources[] = {
      "int count = 7\n",
      "let value = 3.5\n",
  };
  size_t i;
  for (i = 0U; i < sizeof(bad_sources) / sizeof(bad_sources[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;
    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(bad_sources[i], &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return (int)(1 + i);
    }
    if (diagnostic.line != 1U) {
      return (int)(10 + i);
    }
  }
  return 0;
}

int test_interpreter_graph_declaration(void) {
  const char *source = "graph G:\n"
                       "  1 -> 2\n"
                       "  2 -> 3\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source(source, &scope, &diagnostic);
  if (rc != GINT_OK) {
    return 1;
  }
  if (!expect_graph_binding(&scope, "G", 2U, 3U, 1, 2, 2, 3)) {
    return 2;
  }
  return 0;
}

int test_interpreter_rejects_non_integer_graph_nodes(void) {
  const char *source = "graph G:\n"
                       "  alpha -> 2\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source(source, &scope, &diagnostic);
  if (rc != GINT_ERR_PARSE) {
    return 1;
  }
  if (diagnostic.line != 2U) {
    return 2;
  }
  return 0;
}

int test_interpreter_print_and_function_return(void) {
  const char *path = "interpreter_print_output.txt";
  const char *source = "def echo(value):\n"
                       "  print(value)\n"
                       "  return value\n"
                       "answer = echo(42)\n"
                       "print(answer)\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  char output[64];
  FILE *fp = NULL;
  size_t read_len;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return 1;
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return 2;
  }
  if (!expect_int_binding(&scope, "answer", 42)) {
    remove(path);
    return 3;
  }
  fp = NULL;
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    remove(path);
    return 4;
  }
  read_len = fread(output, 1U, sizeof(output) - 1U, fp);
  fclose(fp);
  remove(path);
  output[read_len] = '\0';
  if (strcmp(output, "42\n42\n") != 0) {
    return 5;
  }
  return 0;
}

int test_interpreter_print_graph_summary(void) {
  const char *path = "interpreter_graph_output.txt";
  const char *source = "graph G:\n"
                       "  1 -> 2\n"
                       "  2 -> 3\n"
                       "print(G)\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  char output[128];
  FILE *fp = NULL;
  size_t read_len;
  int rc;

  graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return 1;
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return 2;
  }
  fp = NULL;
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    remove(path);
    return 3;
  }
  read_len = fread(output, 1U, sizeof(output) - 1U, fp);
  fclose(fp);
  remove(path);
  output[read_len] = '\0';
  if (strcmp(output, "<graph name=G nodes=3 edges=2>\n") != 0) {
    return 4;
  }
  return 0;
}

int test_gion_entry_flow_execution(void) {
  const char *path = "entry_flow_sample.gion";
  graphion_runtime_scope scope;
  const graphion_runtime_value *answer;
  const graphion_runtime_value *copy;
  FILE *fp = fopen(path, "wb");
  int rc;
  if (fp == NULL) {
    return 1;
  }
  if (fputs("answer = 42\ncopy = answer\n", fp) < 0) {
    fclose(fp);
    remove(path);
    return 2;
  }
  fclose(fp);

  rc = graphion_run_gion_path(path, &scope);
  remove(path);
  if (rc != GENTRY_OK) {
    return 3;
  }
  answer = graphion_runtime_scope_find(&scope, "answer");
  copy = graphion_runtime_scope_find(&scope, "copy");
  if (answer == NULL || answer->kind != GRAPHION_VALUE_INT || answer->int_value != 42) {
    return 4;
  }
  if (copy == NULL || copy->kind != GRAPHION_VALUE_INT || copy->int_value != 42) {
    return 5;
  }
  rc = graphion_run_gion_path("entry_flow_sample.txt", &scope);
  if (rc != GENTRY_ERR_EXTENSION) {
    return 6;
  }
  return 0;
}
