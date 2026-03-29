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

#define TEST_VM_REG_I(vm_, idx_) ((vm_).regs[(idx_)].as.int_value)

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
  if (!vm.halted || TEST_VM_REG_I(vm, 0) != 42) {
    return 5;
  }
  return 0;
}

int test_frontend_reference_graph_execution_examples(void) {
  typedef struct {
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
    if (TEST_VM_REG_I(vm, 0) != examples[i].expected_regs[0] ||
        TEST_VM_REG_I(vm, 1) != examples[i].expected_regs[1] ||
        TEST_VM_REG_I(vm, 2) != examples[i].expected_regs[2] ||
        TEST_VM_REG_I(vm, 3) != examples[i].expected_regs[3]) {
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

static int test_read_file_text(const char *path, char *buffer, size_t capacity) {
  FILE *fp = NULL;
  size_t read_len;
  if (path == NULL || buffer == NULL || capacity == 0U) {
    return 0;
  }
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "rb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "rb");
#endif
  if (fp == NULL) {
    return 0;
  }
  read_len = fread(buffer, 1U, capacity - 1U, fp);
  fclose(fp);
  buffer[read_len] = '\0';
  return 1;
}

int test_gion_scalar_assignments_and_prints(void) {
  const char *source =
      "count = 42\n"
      "ratio = 3.5\n"
      "name = \"graphion\"\n"
      "ready = true\n"
      "copy = count\n"
      "print(7)\n"
      "print(\"raw\")\n"
      "print(false)\n"
      "print(count)\n"
      "print(ratio)\n"
      "print(name)\n"
      "print(ready)\n"
      "print(copy)\n";
  const char *path = "gion_scalar_assignments_and_prints.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *count;
  const graphion_runtime_value *ratio;
  const graphion_runtime_value *name;
  const graphion_runtime_value *ready;
  const graphion_runtime_value *copy;
  FILE *fp = NULL;
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
  count = graphion_runtime_scope_find(&scope, "count");
  ratio = graphion_runtime_scope_find(&scope, "ratio");
  name = graphion_runtime_scope_find(&scope, "name");
  ready = graphion_runtime_scope_find(&scope, "ready");
  copy = graphion_runtime_scope_find(&scope, "copy");
  if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
    remove(path);
    return 3;
  }
  if (ratio == NULL || ratio->kind != GVM_VALUE_FLOAT || ratio->as.float_value != 3.5) {
    remove(path);
    return 4;
  }
  if (name == NULL || name->kind != GVM_VALUE_STRING || strcmp(name->as.string_value, "graphion") != 0) {
    remove(path);
    return 5;
  }
  if (ready == NULL || ready->kind != GVM_VALUE_BOOL || ready->as.bool_value != 1) {
    remove(path);
    return 6;
  }
  if (copy == NULL || copy->kind != GVM_VALUE_INT || copy->as.int_value != 42) {
    remove(path);
    return 7;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 8;
  }
  remove(path);
  if (strcmp(output, "7\nraw\nfalse\n42\n3.5\ngraphion\ntrue\n42\n") != 0) {
    return 9;
  }
  return 0;
}

int test_gion_scalar_feature_varied_names(void) {
  const char *source =
      "alpha_1 = \"ok\"\n"
      "z_value = false\n"
      "n2 = -7\n"
      "copied_name = alpha_1\n"
      "shadow_0 = n2\n"
      "print(copied_name)\n"
      "print(z_value)\n"
      "print(shadow_0)\n";
  const char *path = "gion_scalar_feature_varied_names.txt";
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *copied_name;
  const graphion_runtime_value *shadow_0;
  const graphion_runtime_value *z_value;
  FILE *fp = NULL;
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
  copied_name = graphion_runtime_scope_find(&scope, "copied_name");
  shadow_0 = graphion_runtime_scope_find(&scope, "shadow_0");
  z_value = graphion_runtime_scope_find(&scope, "z_value");
  if (copied_name == NULL || copied_name->kind != GVM_VALUE_STRING ||
      strcmp(copied_name->as.string_value, "ok") != 0) {
    remove(path);
    return 3;
  }
  if (shadow_0 == NULL || shadow_0->kind != GVM_VALUE_INT || shadow_0->as.int_value != -7) {
    remove(path);
    return 4;
  }
  if (z_value == NULL || z_value->kind != GVM_VALUE_BOOL || z_value->as.bool_value != 0) {
    remove(path);
    return 5;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 6;
  }
  remove(path);
  if (strcmp(output, "ok\nfalse\n-7\n") != 0) {
    return 7;
  }
  return 0;
}

int test_gion_unknown_variable_errors(void) {
  const char *source = "copy = missing\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source(source, &scope, &diagnostic);
  if (rc != GINT_ERR_UNKNOWN_VARIABLE) {
    return 1;
  }
  if (diagnostic.message == NULL) {
    return 2;
  }
  return strcmp(diagnostic.message, "unknown variable") == 0 ? 0 : 3;
}

int test_gion_partial_execution_stops_at_first_unsupported_line(void) {
  const char *source =
      "count = 42\n"
      "print(count)\n"
      "graph G:\n"
      "print(count)\n";
  const char *path = "gion_partial_execution.txt";
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
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
  if (rc != GINT_ERR_PARSE) {
    remove(path);
    return 2;
  }
  if (diagnostic.line != 3U) {
    remove(path);
    return 3;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 4;
  }
  remove(path);
  if (strcmp(output, "42\n") != 0) {
    return 5;
  }
  return 0;
}

int test_gion_reserved_name_errors(void) {
  static const struct {
    const char *source;
    const char *message;
    const char *path;
  } cases[] = {
      {"true = 1\n", "reserved name cannot be assigned", "gion_reserved_true.gion"},
      {"false = 0\n", "reserved name cannot be assigned", "gion_reserved_false.gion"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    FILE *fp = NULL;
    int rc;

    graphion_runtime_scope_init(&scope);
#if defined(_MSC_VER)
    if (fopen_s(&fp, cases[i].path, "wb") != 0) {
      fp = NULL;
    }
#else
    fp = fopen(cases[i].path, "wb");
#endif
    if (fp == NULL) {
      return (int)(1 + i * 10U);
    }
    fputs(cases[i].source, fp);
    fclose(fp);
    rc = graphion_run_gion_path(cases[i].path, &scope, &diagnostic);
    remove(cases[i].path);
    if (rc != GENTRY_ERR_PARSE) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.line != 1U || diagnostic.column != 1U) {
      return (int)(3 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(4 + i * 10U);
    }
  }
  return 0;
}

int test_gion_assignment_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"count 42\n", GINT_ERR_PARSE, "expected '='"},
      {"= 42\n", GINT_ERR_PARSE, "expected identifier"},
      {"count =\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 42 +\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = nope\n", GINT_ERR_UNKNOWN_VARIABLE, "unknown variable"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.line != 1U) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(3 + i * 10U);
    }
  }
  return 0;
}

int test_gion_arithmetic_expressions(void) {
  const char *source =
      "base = 8\n"
      "sum = 40 + 2\n"
      "mixed = 1 + 2 * 3\n"
      "grouped = (1 + 2) * 3\n"
      "delta = base - 3\n"
      "ratio = 7 / 2\n"
      "scaled = ratio * 2\n"
      "total = base + ratio * 2\n"
      "print(sum)\n"
      "print(mixed)\n"
      "print(grouped)\n"
      "print(delta)\n"
      "print(ratio)\n"
      "print(total)\n"
      "print(3 + 4 * 2)\n"
      "print((3 + 4) * 2)\n";
  const char *path = "gion_arithmetic_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *sum;
  const graphion_runtime_value *mixed;
  const graphion_runtime_value *grouped;
  const graphion_runtime_value *delta;
  const graphion_runtime_value *ratio;
  const graphion_runtime_value *scaled;
  const graphion_runtime_value *total;
  FILE *fp = NULL;
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

  sum = graphion_runtime_scope_find(&scope, "sum");
  mixed = graphion_runtime_scope_find(&scope, "mixed");
  grouped = graphion_runtime_scope_find(&scope, "grouped");
  delta = graphion_runtime_scope_find(&scope, "delta");
  ratio = graphion_runtime_scope_find(&scope, "ratio");
  scaled = graphion_runtime_scope_find(&scope, "scaled");
  total = graphion_runtime_scope_find(&scope, "total");
  if (sum == NULL || sum->kind != GVM_VALUE_INT || sum->as.int_value != 42) {
    remove(path);
    return 3;
  }
  if (mixed == NULL || mixed->kind != GVM_VALUE_INT || mixed->as.int_value != 7) {
    remove(path);
    return 4;
  }
  if (grouped == NULL || grouped->kind != GVM_VALUE_INT || grouped->as.int_value != 9) {
    remove(path);
    return 5;
  }
  if (delta == NULL || delta->kind != GVM_VALUE_INT || delta->as.int_value != 5) {
    remove(path);
    return 6;
  }
  if (ratio == NULL || ratio->kind != GVM_VALUE_FLOAT || ratio->as.float_value != 3.5) {
    remove(path);
    return 7;
  }
  if (scaled == NULL || scaled->kind != GVM_VALUE_FLOAT || scaled->as.float_value != 7.0) {
    remove(path);
    return 8;
  }
  if (total == NULL || total->kind != GVM_VALUE_FLOAT || total->as.float_value != 15.0) {
    remove(path);
    return 9;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 10;
  }
  remove(path);
  if (strcmp(output, "42\n7\n9\n5\n3.5\n15\n11\n14\n") != 0) {
    return 11;
  }
  return 0;
}

int test_gion_string_concatenation(void) {
  const char *source =
      "label = \"debut\" + \"fin\"\n"
      "full = label + \"!\"\n"
      "print(label)\n"
      "print(full)\n";
  const char *path = "gion_string_concatenation.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *label;
  const graphion_runtime_value *full;
  FILE *fp = NULL;
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
  label = graphion_runtime_scope_find(&scope, "label");
  full = graphion_runtime_scope_find(&scope, "full");
  if (label == NULL || label->kind != GVM_VALUE_STRING || strcmp(label->as.string_value, "debutfin") != 0) {
    remove(path);
    return 3;
  }
  if (full == NULL || full->kind != GVM_VALUE_STRING || strcmp(full->as.string_value, "debutfin!") != 0) {
    remove(path);
    return 4;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 5;
  }
  remove(path);
  if (strcmp(output, "debutfin\ndebutfin!\n") != 0) {
    return 6;
  }
  return 0;
}

int test_gion_compound_assignments(void) {
  const char *source =
      "count = 10\n"
      "count += 5\n"
      "count -= 3\n"
      "count *= 4\n"
      "count /= 3\n"
      "text = \"debut\"\n"
      "text += \"fin\"\n"
      "print(count)\n"
      "print(text)\n";
  const char *path = "gion_compound_assignments.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *count;
  const graphion_runtime_value *text;
  FILE *fp = NULL;
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
  count = graphion_runtime_scope_find(&scope, "count");
  text = graphion_runtime_scope_find(&scope, "text");
  if (count == NULL || count->kind != GVM_VALUE_FLOAT || count->as.float_value != 16.0) {
    remove(path);
    return 3;
  }
  if (text == NULL || text->kind != GVM_VALUE_STRING || strcmp(text->as.string_value, "debutfin") != 0) {
    remove(path);
    return 4;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 5;
  }
  remove(path);
  if (strcmp(output, "16\ndebutfin\n") != 0) {
    return 6;
  }
  return 0;
}

int test_gion_compound_assignment_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"count += 1\n", GINT_ERR_UNKNOWN_VARIABLE, "unknown variable"},
      {"count = 1\ncount +=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount -=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount *=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount /=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount /= 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 1\ncount += \"x\"\n", GINT_ERR_RUN, "arithmetic requires numeric operands"},
      {"text = \"x\"\ntext -= \"y\"\n", GINT_ERR_RUN, "arithmetic requires numeric operands"},
      {"text = \"x\"\ntext *= 2\n", GINT_ERR_RUN, "arithmetic requires numeric operands"},
      {"text = \"x\"\ntext /= 2\n", GINT_ERR_RUN, "arithmetic requires numeric operands"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(2 + i * 10U);
    }
  }
  return 0;
}

int test_gion_arithmetic_precedence_and_associativity(void) {
  const char *source =
      "a = 20 - 5 - 3\n"
      "b = 20 / 5 / 2\n"
      "c = 2 * 3 + 4 * 5\n"
      "d = 2 + 3 * 4 - 5\n"
      "e = 10 - 2 * 3\n"
      "f = 2 * 3 / 4\n"
      "g = -7 + 2\n"
      "h = 2 + -3 * 4\n"
      "i = (2 + 3) * 4\n"
      "j = 2 * (3 + 4)\n"
      "k = (20 - 5) - 3\n"
      "l = 20 - (5 - 3)\n"
      "print(a)\n"
      "print(b)\n"
      "print(c)\n"
      "print(d)\n"
      "print(e)\n"
      "print(f)\n"
      "print(g)\n"
      "print(h)\n"
      "print(i)\n"
      "print(j)\n"
      "print(k)\n"
      "print(l)\n";
  const char *path = "gion_arithmetic_precedence.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  FILE *fp = NULL;
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
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 3;
  }
  remove(path);
  if (strcmp(output, "12\n2\n26\n9\n4\n1.5\n-5\n-10\n20\n14\n12\n18\n") != 0) {
    return 4;
  }
  return 0;
}

int test_gion_arithmetic_runtime_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"value = 1 / 0\n", GINT_ERR_RUN, "division by zero"},
      {"value = \"x\" + 1\n", GINT_ERR_RUN, "arithmetic requires numeric operands"},
      {"value = true + 1\n", GINT_ERR_RUN, "arithmetic requires numeric operands"},
      {"print(\"x\" / 2)\n", GINT_ERR_RUN, "arithmetic requires numeric operands"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(2 + i * 10U);
    }
  }
  return 0;
}

int test_gion_arithmetic_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"value = 1 + * 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 / / 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 ** 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 + 2 3\n", GINT_ERR_PARSE, "unsupported assignment expression"},
      {"value = 1\nvalue +=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value + 2\n", GINT_ERR_PARSE, "expected '='"},
      {"value = (1 + 2\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = 1 + (2 * 3\n", GINT_ERR_PARSE, "expected ')' after expression"},
      {"value = ()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print(1 + )\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print((1 + 2)\n", GINT_ERR_PARSE, "expected ')' after print argument"},
      {"print(()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print(1 + 2\n", GINT_ERR_PARSE, "expected ')' after print argument"},
      {"print(1 + 2 3)\n", GINT_ERR_PARSE, "expected ')' after print argument"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(2 + i * 10U);
    }
  }
  return 0;
}

int test_gion_print_syntax_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    const char *message;
  } cases[] = {
      {"print = 42\n", GINT_ERR_PARSE, "expected '(' after print"},
      {"print\n", GINT_ERR_PARSE, "expected '(' after print"},
      {"print(\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"print(count\n", GINT_ERR_UNKNOWN_VARIABLE, "unknown variable"},
      {"print(count) extra\n", GINT_ERR_UNKNOWN_VARIABLE, "unknown variable"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.line != 1U) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(3 + i * 10U);
    }
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("count = 42\nprint(count\n", &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return 100;
    }
    if (diagnostic.line != 2U) {
      return 101;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "expected ')' after print argument") != 0) {
      return 102;
    }
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("count = 42\nprint(count) extra\n", &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return 110;
    }
    if (diagnostic.line != 2U) {
      return 111;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "unexpected trailing tokens after print") != 0) {
      return 112;
    }
  }

  return 0;
}

int test_gion_unterminated_string_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"name = \"graphion\n", "unterminated string literal"},
      {"print(\"x\n", "unterminated string literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.line != 1U) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(3 + i * 10U);
    }
  }
  return 0;
}

int test_gion_invalid_identifier_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"1count = 42\n", "expected identifier"},
      {"-name = 42\n", "expected identifier"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.line != 1U) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(3 + i * 10U);
    }
  }
  return 0;
}

int test_gion_trailing_token_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"count = 42 extra\n", "unsupported assignment expression"},
      {"name = \"x\" extra\n", "unsupported assignment expression"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.line != 1U) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return (int)(3 + i * 10U);
    }
  }
  return 0;
}

int test_gion_reference_before_definition_errors(void) {
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source("copy = count\ncount = 42\n", &scope, &diagnostic);
  if (rc != GINT_ERR_UNKNOWN_VARIABLE) {
    return 1;
  }
  if (diagnostic.line != 1U || diagnostic.column != 1U) {
    return 2;
  }
  if (diagnostic.message == NULL || strcmp(diagnostic.message, "unknown variable") != 0) {
    return 3;
  }
  return 0;
}

int test_gion_reassignment_and_type_change(void) {
  const char *source =
      "value = 1\n"
      "value = 2\n"
      "value = \"ok\"\n"
      "flag = true\n"
      "flag = false\n"
      "print(value)\n"
      "print(flag)\n";
  const char *path = "gion_reassignment_and_type_change.txt";
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *value;
  const graphion_runtime_value *flag;
  FILE *fp = NULL;
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
  value = graphion_runtime_scope_find(&scope, "value");
  flag = graphion_runtime_scope_find(&scope, "flag");
  if (value == NULL || value->kind != GVM_VALUE_STRING || strcmp(value->as.string_value, "ok") != 0) {
    remove(path);
    return 3;
  }
  if (flag == NULL || flag->kind != GVM_VALUE_BOOL || flag->as.bool_value != 0) {
    remove(path);
    return 4;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 5;
  }
  remove(path);
  if (strcmp(output, "ok\nfalse\n") != 0) {
    return 6;
  }
  return 0;
}

int test_gion_copy_chains_and_blank_lines(void) {
  const char *source =
      "\n"
      "a = 1\n"
      "\n"
      "b = a\n"
      "c = b\n"
      "\n"
      "print(a)\n"
      "print(b)\n"
      "print(c)\n";
  const char *path = "gion_copy_chains_and_blank_lines.txt";
  char output[64];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *c;
  FILE *fp = NULL;
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
  c = graphion_runtime_scope_find(&scope, "c");
  if (c == NULL || c->kind != GVM_VALUE_INT || c->as.int_value != 1) {
    remove(path);
    return 3;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 4;
  }
  remove(path);
  if (strcmp(output, "1\n1\n1\n") != 0) {
    return 5;
  }
  return 0;
}

int test_gion_late_line_error_diagnostics(void) {
  const char *source =
      "count = 42\n"
      "ratio = 3.5\n"
      "print(count)\n"
      "print(ratio)\n"
      "name =\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source(source, &scope, &diagnostic);
  if (rc != GINT_ERR_PARSE) {
    return 1;
  }
  if (diagnostic.line != 5U || diagnostic.column != 1U) {
    return 2;
  }
  if (diagnostic.message == NULL || strcmp(diagnostic.message, "expected scalar literal") != 0) {
    return 3;
  }
  return 0;
}

int test_gion_unexpected_indentation_errors(void) {
  static const struct {
    const char *source;
  } invalid_cases[] = {
      {"  count = 42\n"},
      {"\tprint(1)\n"},
  };
  size_t i;

  for (i = 0U; i < sizeof(invalid_cases) / sizeof(invalid_cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(invalid_cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return (int)(1 + i * 10U);
    }
    if (diagnostic.line != 1U || diagnostic.column != 1U) {
      return (int)(2 + i * 10U);
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "unexpected indentation") != 0) {
      return (int)(3 + i * 10U);
    }
  }

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    const graphion_runtime_value *count;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("\t  \ncount = 42\nprint(count)\n", &scope, &diagnostic);
    if (rc != GINT_OK) {
      return 100;
    }
    count = graphion_runtime_scope_find(&scope, "count");
    if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
      return 101;
    }
  }

  return 0;
}

int test_gion_mixed_scalar_values(void) {
  const char *source =
      "i = -7\n"
      "f = -3.25\n"
      "b = false\n"
      "s = \"hello\"\n"
      "copy_i = i\n"
      "print(i)\n"
      "print(f)\n"
      "print(b)\n"
      "print(s)\n"
      "print(copy_i)\n";
  const char *path = "gion_mixed_scalar_values.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *copy_i;
  FILE *fp = NULL;
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
  copy_i = graphion_runtime_scope_find(&scope, "copy_i");
  if (copy_i == NULL || copy_i->kind != GVM_VALUE_INT || copy_i->as.int_value != -7) {
    remove(path);
    return 3;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 4;
  }
  remove(path);
  if (strcmp(output, "-7\n-3.25\nfalse\nhello\n-7\n") != 0) {
    return 5;
  }
  return 0;
}

int test_gion_capacity_errors(void) {
  {
    char source[GRAPHION_RUNTIME_NAME_MAX + 32U];
    memset(source, 'a', GRAPHION_RUNTIME_NAME_MAX);
    source[GRAPHION_RUNTIME_NAME_MAX] = '\0';
    memcpy(source + GRAPHION_RUNTIME_NAME_MAX, " = 1\n", 6U);

    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(source, &scope, &diagnostic);
    if (rc != GINT_ERR_CAPACITY) {
      return 1;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "identifier too long") != 0) {
      return 2;
    }
  }

  {
    char source[640];
    memset(source, 'a', sizeof(source));
    source[0] = 'n';
    source[1] = 'a';
    source[2] = 'm';
    source[3] = 'e';
    source[4] = ' ';
    source[5] = '=';
    source[6] = ' ';
    source[7] = '"';
    memset(source + 8, 'x', 620U);
    source[628] = '"';
    source[629] = '\n';
    source[630] = '\0';

    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(source, &scope, &diagnostic);
    if (rc != GINT_ERR_CAPACITY) {
      return 10;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "source line too long") != 0) {
      return 11;
    }
  }

  {
    char source[2048];
    size_t offset = 0U;
    unsigned int i;
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    source[0] = '\0';
    for (i = 0U; i <= GRAPHION_RUNTIME_BINDING_MAX; ++i) {
      int written = snprintf(source + offset, sizeof(source) - offset, "v%u = %u\n", i, i);
      if (written <= 0) {
        return 20;
      }
      offset += (size_t)written;
      if (offset >= sizeof(source)) {
        return 21;
      }
    }

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(source, &scope, &diagnostic);
    if (rc != GINT_ERR_CAPACITY) {
      return 22;
    }
    if (diagnostic.line != (unsigned int)(GRAPHION_RUNTIME_BINDING_MAX + 1U)) {
      return 23;
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, "too many globals") != 0) {
      return 24;
    }
  }

  return 0;
}
