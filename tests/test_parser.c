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

static int finish_scope_test(graphion_runtime_scope *scope, int code) {
  graphion_runtime_scope_dispose(scope);
  return code;
}

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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  count = graphion_runtime_scope_find(&scope, "count");
  ratio = graphion_runtime_scope_find(&scope, "ratio");
  name = graphion_runtime_scope_find(&scope, "name");
  ready = graphion_runtime_scope_find(&scope, "ready");
  copy = graphion_runtime_scope_find(&scope, "copy");
  if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (ratio == NULL || ratio->kind != GVM_VALUE_FLOAT || ratio->as.float_value != 3.5) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (name == NULL || name->kind != GVM_VALUE_STRING || strcmp(name->as.string_value, "graphion") != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (ready == NULL || ready->kind != GVM_VALUE_BOOL || ready->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (copy == NULL || copy->kind != GVM_VALUE_INT || copy->as.int_value != 42) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  remove(path);
  if (strcmp(output, "7\nraw\nfalse\n42\n3.5\ngraphion\ntrue\n42\n") != 0) {
    return finish_scope_test(&scope, 9);
  }
  return finish_scope_test(&scope, 0);
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  copied_name = graphion_runtime_scope_find(&scope, "copied_name");
  shadow_0 = graphion_runtime_scope_find(&scope, "shadow_0");
  z_value = graphion_runtime_scope_find(&scope, "z_value");
  if (copied_name == NULL || copied_name->kind != GVM_VALUE_STRING ||
      strcmp(copied_name->as.string_value, "ok") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (shadow_0 == NULL || shadow_0->kind != GVM_VALUE_INT || shadow_0->as.int_value != -7) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (z_value == NULL || z_value->kind != GVM_VALUE_BOOL || z_value->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  remove(path);
  if (strcmp(output, "ok\nfalse\n-7\n") != 0) {
    return finish_scope_test(&scope, 7);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_unknown_variable_errors(void) {
  const char *source = "copy = missing\n";
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_interpret_source(source, &scope, &diagnostic);
  if (rc != GINT_ERR_UNKNOWN_OPERAND) {
    return 1;
  }
  if (diagnostic.message == NULL) {
    return 2;
  }
  return strcmp(diagnostic.message, "unknown operand") == 0 ? 0 : 3;
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
      {"abs = 1\n", "reserved name cannot be assigned", "gion_reserved_abs.gion"},
      {"if = true\n", "reserved name cannot be assigned", "gion_reserved_if.gion"},
      {"elif = false\n", "reserved name cannot be assigned", "gion_reserved_elif.gion"},
      {"else = true\n", "reserved name cannot be assigned", "gion_reserved_else.gion"},
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
      {"count = nope\n", GINT_ERR_UNKNOWN_OPERAND, "unknown operand"},
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
      "negative_add = -5 + 2\n"
      "negative_sub = 5 - -2\n"
      "negative_mul = -3 * 4\n"
      "negative_div = -7 / 2\n"
      "floor_half = 7 // 2\n"
      "negative_floor = -7 // 2\n"
      "float_floor = 7.5 // 2\n"
      "power = 2 ** 3\n"
      "negative_power = (-2) ** 3\n"
      "negative_exponent = 2 ** -1\n"
      "right_assoc = 2 ** 3 ** 2\n"
      "powered_group = (1 + 2) ** 2\n"
      "abs_int = abs(-42)\n"
      "abs_float = abs(-3.5)\n"
      "abs_expr = abs(-5 + 2)\n"
      "total = base + ratio * 2\n"
      "remainder = 10 % 4\n"
      "negative_remainder = -10 % 4\n"
      "float_remainder = 7.5 % 2\n"
      "print(sum)\n"
      "print(mixed)\n"
      "print(grouped)\n"
      "print(delta)\n"
      "print(ratio)\n"
      "print(total)\n"
      "print(negative_add)\n"
      "print(negative_sub)\n"
      "print(negative_mul)\n"
      "print(negative_div)\n"
      "print(floor_half)\n"
      "print(negative_floor)\n"
      "print(float_floor)\n"
      "print(power)\n"
      "print(negative_power)\n"
      "print(negative_exponent)\n"
      "print(right_assoc)\n"
      "print(powered_group)\n"
      "print(abs_int)\n"
      "print(abs_float)\n"
      "print(abs_expr)\n"
      "print(remainder)\n"
      "print(negative_remainder)\n"
      "print(float_remainder)\n"
      "print(3 + 4 * 2)\n"
      "print((3 + 4) * 2)\n"
      "print(10 % 4)\n";
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
  const graphion_runtime_value *negative_add;
  const graphion_runtime_value *negative_sub;
  const graphion_runtime_value *negative_mul;
  const graphion_runtime_value *negative_div;
  const graphion_runtime_value *floor_half;
  const graphion_runtime_value *negative_floor;
  const graphion_runtime_value *float_floor;
  const graphion_runtime_value *power;
  const graphion_runtime_value *negative_power;
  const graphion_runtime_value *negative_exponent;
  const graphion_runtime_value *right_assoc;
  const graphion_runtime_value *powered_group;
  const graphion_runtime_value *abs_int;
  const graphion_runtime_value *abs_float;
  const graphion_runtime_value *abs_expr;
  const graphion_runtime_value *total;
  const graphion_runtime_value *remainder;
  const graphion_runtime_value *negative_remainder;
  const graphion_runtime_value *float_remainder;
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
  negative_add = graphion_runtime_scope_find(&scope, "negative_add");
  negative_sub = graphion_runtime_scope_find(&scope, "negative_sub");
  negative_mul = graphion_runtime_scope_find(&scope, "negative_mul");
  negative_div = graphion_runtime_scope_find(&scope, "negative_div");
  floor_half = graphion_runtime_scope_find(&scope, "floor_half");
  negative_floor = graphion_runtime_scope_find(&scope, "negative_floor");
  float_floor = graphion_runtime_scope_find(&scope, "float_floor");
  power = graphion_runtime_scope_find(&scope, "power");
  negative_power = graphion_runtime_scope_find(&scope, "negative_power");
  negative_exponent = graphion_runtime_scope_find(&scope, "negative_exponent");
  right_assoc = graphion_runtime_scope_find(&scope, "right_assoc");
  powered_group = graphion_runtime_scope_find(&scope, "powered_group");
  abs_int = graphion_runtime_scope_find(&scope, "abs_int");
  abs_float = graphion_runtime_scope_find(&scope, "abs_float");
  abs_expr = graphion_runtime_scope_find(&scope, "abs_expr");
  total = graphion_runtime_scope_find(&scope, "total");
  remainder = graphion_runtime_scope_find(&scope, "remainder");
  negative_remainder = graphion_runtime_scope_find(&scope, "negative_remainder");
  float_remainder = graphion_runtime_scope_find(&scope, "float_remainder");
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
  if (negative_add == NULL || negative_add->kind != GVM_VALUE_INT || negative_add->as.int_value != -3) {
    remove(path);
    return 9;
  }
  if (negative_sub == NULL || negative_sub->kind != GVM_VALUE_INT || negative_sub->as.int_value != 7) {
    remove(path);
    return 10;
  }
  if (negative_mul == NULL || negative_mul->kind != GVM_VALUE_INT || negative_mul->as.int_value != -12) {
    remove(path);
    return 11;
  }
  if (negative_div == NULL || negative_div->kind != GVM_VALUE_FLOAT || negative_div->as.float_value != -3.5) {
    remove(path);
    return 12;
  }
  if (floor_half == NULL || floor_half->kind != GVM_VALUE_INT || floor_half->as.int_value != 3) {
    remove(path);
    return 13;
  }
  if (negative_floor == NULL || negative_floor->kind != GVM_VALUE_INT || negative_floor->as.int_value != -4) {
    remove(path);
    return 14;
  }
  if (float_floor == NULL || float_floor->kind != GVM_VALUE_FLOAT || float_floor->as.float_value != 3.0) {
    remove(path);
    return 15;
  }
  if (power == NULL || power->kind != GVM_VALUE_FLOAT || power->as.float_value != 8.0) {
    remove(path);
    return 16;
  }
  if (negative_power == NULL || negative_power->kind != GVM_VALUE_FLOAT || negative_power->as.float_value != -8.0) {
    remove(path);
    return 17;
  }
  if (negative_exponent == NULL || negative_exponent->kind != GVM_VALUE_FLOAT || negative_exponent->as.float_value != 0.5) {
    remove(path);
    return 18;
  }
  if (right_assoc == NULL || right_assoc->kind != GVM_VALUE_FLOAT || right_assoc->as.float_value != 512.0) {
    remove(path);
    return 19;
  }
  if (powered_group == NULL || powered_group->kind != GVM_VALUE_FLOAT || powered_group->as.float_value != 9.0) {
    remove(path);
    return 20;
  }
  if (abs_int == NULL || abs_int->kind != GVM_VALUE_INT || abs_int->as.int_value != 42) {
    remove(path);
    return 21;
  }
  if (abs_float == NULL || abs_float->kind != GVM_VALUE_FLOAT || abs_float->as.float_value != 3.5) {
    remove(path);
    return 22;
  }
  if (abs_expr == NULL || abs_expr->kind != GVM_VALUE_INT || abs_expr->as.int_value != 3) {
    remove(path);
    return 23;
  }
  if (total == NULL || total->kind != GVM_VALUE_FLOAT || total->as.float_value != 15.0) {
    remove(path);
    return 24;
  }
  if (remainder == NULL || remainder->kind != GVM_VALUE_INT || remainder->as.int_value != 2) {
    remove(path);
    return 25;
  }
  if (negative_remainder == NULL || negative_remainder->kind != GVM_VALUE_INT || negative_remainder->as.int_value != -2) {
    remove(path);
    return 26;
  }
  if (float_remainder == NULL || float_remainder->kind != GVM_VALUE_FLOAT || float_remainder->as.float_value != 1.5) {
    remove(path);
    return 27;
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return 28;
  }
  remove(path);
  if (strcmp(output, "42\n7\n9\n5\n3.5\n15\n-3\n7\n-12\n-3.5\n3\n-4\n3\n8\n-8\n0.5\n512\n9\n42\n3.5\n3\n2\n-2\n1.5\n11\n14\n2\n") != 0) {
    return 29;
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  label = graphion_runtime_scope_find(&scope, "label");
  full = graphion_runtime_scope_find(&scope, "full");
  if (label == NULL || label->kind != GVM_VALUE_STRING || strcmp(label->as.string_value, "debutfin") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (full == NULL || full->kind != GVM_VALUE_STRING || strcmp(full->as.string_value, "debutfin!") != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "debutfin\ndebutfin!\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_print_string_coercion(void) {
  const char *source =
      "name = \"Test \"\n"
      "print(\"Test \" + 7)\n"
      "print(name + 7)\n"
      "print(\"value=\" + (3 + 4))\n";
  const char *path = "gion_print_string_coercion.txt";
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
  if (strcmp(output, "Test 7\nTest 7\nvalue=7\n") != 0) {
    return finish_scope_test(&scope, 4);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_compound_assignments(void) {
  const char *source =
      "count = 10\n"
      "count += 5\n"
      "count -= 3\n"
      "count *= 4\n"
      "count /= 3\n"
      "count //= 2\n"
      "count %= 7\n"
      "count **= 3\n"
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  count = graphion_runtime_scope_find(&scope, "count");
  text = graphion_runtime_scope_find(&scope, "text");
  if (count == NULL || count->kind != GVM_VALUE_FLOAT || count->as.float_value != 1.0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (text == NULL || text->kind != GVM_VALUE_STRING || strcmp(text->as.string_value, "debutfin") != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "1\ndebutfin\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
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
      {"count = 1\ncount //=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount %=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount **=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"count = 1\ncount /= 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 1\ncount //= 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 1\ncount = count // 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 1\ncount %= 0\n", GINT_ERR_RUN, "division by zero"},
      {"count = 2\ncount **= \"x\"\n", GINT_ERR_RUN, "incompatible operand types"},
      {"count = 1\ncount += \"x\"\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = \"Test \" + 7\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext -= \"y\"\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext *= 2\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext /= 2\n", GINT_ERR_RUN, "incompatible operand types"},
      {"text = \"x\"\ntext %= 2\n", GINT_ERR_RUN, "incompatible operand types"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
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
      {"value = \"x\" + 1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = true + 1\n", GINT_ERR_RUN, "incompatible operand types"},
      {"value = abs(\"x\")\n", GINT_ERR_RUN, "incompatible operand types"},
      {"print(\"x\" / 2)\n", GINT_ERR_RUN, "incompatible operand types"},
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
      {"value = 1 ** * 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 % % 2\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 2 **\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 2 //\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = 1 + 2 3\n", GINT_ERR_PARSE, "unsupported assignment expression"},
      {"value = 1\nvalue +=\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value + 2\n", GINT_ERR_PARSE, "expected '='"},
      {"value = abs()\n", GINT_ERR_PARSE, "expected scalar literal"},
      {"value = abs(1 + 2\n", GINT_ERR_PARSE, "expected ')' after abs argument"},
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
      {"print(count\n", GINT_ERR_UNKNOWN_OPERAND, "unknown operand"},
      {"print(count) extra\n", GINT_ERR_UNKNOWN_OPERAND, "unknown operand"},
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
  if (rc != GINT_ERR_UNKNOWN_OPERAND) {
    return 1;
  }
  if (diagnostic.line != 1U || diagnostic.column != 1U) {
    return 2;
  }
  if (diagnostic.message == NULL || strcmp(diagnostic.message, "unknown operand") != 0) {
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  value = graphion_runtime_scope_find(&scope, "value");
  flag = graphion_runtime_scope_find(&scope, "flag");
  if (value == NULL || value->kind != GVM_VALUE_STRING || strcmp(value->as.string_value, "ok") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (flag == NULL || flag->kind != GVM_VALUE_BOOL || flag->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "ok\nfalse\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
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

  {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    const graphion_runtime_value *count;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source("flag = true\nif flag:\n    count = 42\nprint(count)\n", &scope, &diagnostic);
    if (rc != GINT_OK) {
      return 110;
    }
    count = graphion_runtime_scope_find(&scope, "count");
    if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
      return 111;
    }
    graphion_runtime_scope_dispose(&scope);
  }

  return 0;
}

int test_gion_if_elif_else_control_flow(void) {
  const char *source =
      "flag = true\n"
      "other = false\n"
      "nested = false\n"
      "if false:\n"
      "    selected = \"bad\"\n"
      "elif flag:\n"
      "    selected = \"if branch\"\n"
      "else:\n"
      "    selected = \"fallback\"\n"
      "if other:\n"
      "    optional = \"bad\"\n"
      "else:\n"
      "    optional = \"if else without elif\"\n"
      "if false:\n"
      "    single = \"bad\"\n"
      "single = \"if without else stays optional\"\n"
      "if 1:\n"
      "    int_true_branch = \"int one acts like true\"\n"
      "if 0:\n"
      "    int_false_branch = \"bad\"\n"
      "else:\n"
      "    int_false_branch = \"int zero acts like false\"\n"
      "if 1 + 1 == 2:\n"
      "    equality_branch = \"equality condition works\"\n"
      "else:\n"
      "    equality_branch = \"bad\"\n"
      "if 2 + 2 != 5:\n"
      "    inequality_branch = \"inequality condition works\"\n"
      "else:\n"
      "    inequality_branch = \"bad\"\n"
      "if 2 < 3:\n"
      "    less_than_branch = \"less-than condition works\"\n"
      "else:\n"
      "    less_than_branch = \"bad\"\n"
      "if 3 <= 3:\n"
      "    less_equal_branch = \"less-equal condition works\"\n"
      "else:\n"
      "    less_equal_branch = \"bad\"\n"
      "if nested:\n"
      "    nested_result = \"bad\"\n"
      "elif false:\n"
      "    nested_result = \"bad2\"\n"
      "elif true:\n"
      "    if flag:\n"
      "        nested_result = \"nested if branch\"\n"
      "    else:\n"
      "        nested_result = \"nested else branch\"\n"
      "else:\n"
      "    nested_result = \"bad3\"\n"
      "print(selected)\n"
      "print(optional)\n"
      "print(single)\n"
      "print(equality_branch)\n"
      "print(inequality_branch)\n"
      "print(less_than_branch)\n"
      "print(less_equal_branch)\n"
      "print(nested_result)\n";
  const char *path = "gion_if_elif_else_control_flow.txt";
  char output[512];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *selected;
  const graphion_runtime_value *optional;
  const graphion_runtime_value *single;
  const graphion_runtime_value *int_true_branch;
  const graphion_runtime_value *int_false_branch;
  const graphion_runtime_value *equality_branch;
  const graphion_runtime_value *inequality_branch;
  const graphion_runtime_value *less_than_branch;
  const graphion_runtime_value *less_equal_branch;
  const graphion_runtime_value *nested_result;
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  selected = graphion_runtime_scope_find(&scope, "selected");
  optional = graphion_runtime_scope_find(&scope, "optional");
  single = graphion_runtime_scope_find(&scope, "single");
  int_true_branch = graphion_runtime_scope_find(&scope, "int_true_branch");
  int_false_branch = graphion_runtime_scope_find(&scope, "int_false_branch");
  equality_branch = graphion_runtime_scope_find(&scope, "equality_branch");
  inequality_branch = graphion_runtime_scope_find(&scope, "inequality_branch");
  less_than_branch = graphion_runtime_scope_find(&scope, "less_than_branch");
  less_equal_branch = graphion_runtime_scope_find(&scope, "less_equal_branch");
  nested_result = graphion_runtime_scope_find(&scope, "nested_result");
  if (selected == NULL || selected->kind != GVM_VALUE_STRING || strcmp(selected->as.string_value, "if branch") != 0) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (optional == NULL || optional->kind != GVM_VALUE_STRING ||
      strcmp(optional->as.string_value, "if else without elif") != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (single == NULL || single->kind != GVM_VALUE_STRING ||
      strcmp(single->as.string_value, "if without else stays optional") != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (int_true_branch == NULL || int_true_branch->kind != GVM_VALUE_STRING ||
      strcmp(int_true_branch->as.string_value, "int one acts like true") != 0) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (int_false_branch == NULL || int_false_branch->kind != GVM_VALUE_STRING ||
      strcmp(int_false_branch->as.string_value, "int zero acts like false") != 0) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (equality_branch == NULL || equality_branch->kind != GVM_VALUE_STRING ||
      strcmp(equality_branch->as.string_value, "equality condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (inequality_branch == NULL || inequality_branch->kind != GVM_VALUE_STRING ||
      strcmp(inequality_branch->as.string_value, "inequality condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  if (less_than_branch == NULL || less_than_branch->kind != GVM_VALUE_STRING ||
      strcmp(less_than_branch->as.string_value, "less-than condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 10);
  }
  if (less_equal_branch == NULL || less_equal_branch->kind != GVM_VALUE_STRING ||
      strcmp(less_equal_branch->as.string_value, "less-equal condition works") != 0) {
    remove(path);
    return finish_scope_test(&scope, 11);
  }
  if (nested_result == NULL || nested_result->kind != GVM_VALUE_STRING ||
      strcmp(nested_result->as.string_value, "nested if branch") != 0) {
    remove(path);
    return finish_scope_test(&scope, 12);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 13);
  }
  remove(path);
  if (strcmp(output, "if branch\nif else without elif\nif without else stays optional\nequality condition works\ninequality condition works\nless-than condition works\nless-equal condition works\nnested if branch\n") != 0) {
    return finish_scope_test(&scope, 14);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_if_elif_else_errors(void) {
  static const struct {
    const char *source;
    int expected_rc;
    unsigned int expected_line;
    const char *message;
  } cases[] = {
      {"if true\n    print(1)\n", GINT_ERR_PARSE, 1U, "expected ':' after if condition"},
      {"if :\n    print(1)\n", GINT_ERR_PARSE, 1U, "expected condition after if"},
      {"if true: extra\n    print(1)\n", GINT_ERR_PARSE, 1U, "unexpected trailing tokens after condition"},
      {"elif true:\n    print(1)\n", GINT_ERR_PARSE, 1U, "elif without matching if"},
      {"elif :\n    print(1)\n", GINT_ERR_PARSE, 1U, "elif without matching if"},
      {"else:\n    print(1)\n", GINT_ERR_PARSE, 1U, "else without matching if"},
      {"if true:\nprint(1)\n", GINT_ERR_PARSE, 1U, "expected indented block after if"},
      {"if false:\n    print(1)\nelif true:\nprint(2)\n", GINT_ERR_PARSE, 3U, "expected indented block after elif"},
      {"if false:\n    print(1)\nelse:\nprint(2)\n", GINT_ERR_PARSE, 3U, "expected indented block after else"},
      {"if true:\n    print(1)\n  print(2)\n", GINT_ERR_PARSE, 3U, "unexpected indentation"},
      {"if true:\n    if false:\n        print(1)\n      print(2)\n", GINT_ERR_PARSE, 4U, "unexpected indentation"},
      {"if true:\n    elif false:\n        print(1)\n", GINT_ERR_PARSE, 2U, "elif without matching if"},
      {"if true:\n    else:\n        print(1)\n", GINT_ERR_PARSE, 2U, "else without matching if"},
      {"if 2:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if -1:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if 0.0:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if 1.5:\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if \"x\":\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"if abs(2):\n    print(1)\n", GINT_ERR_RUN, 1U, "if condition must be boolean or 0/1"},
      {"flag = true\nif flag:\n    print(1)\nelse:\n    print(2)\nelif false:\n    print(3)\n", GINT_ERR_PARSE, 6U, "else must be last in if chain"},
      {"if false:\n    print(1)\nelse:\n    print(2)\nelse:\n    print(3)\n", GINT_ERR_PARSE, 5U, "else must be last in if chain"},
      {"if false:\n    print(1)\nelif true\n    print(2)\n", GINT_ERR_PARSE, 3U, "expected ':' after elif condition"},
      {"if false:\n    print(1)\nelif :\n    print(2)\n", GINT_ERR_PARSE, 3U, "expected condition after elif"},
      {"if false:\n    print(1)\nelif true: extra\n    print(2)\n", GINT_ERR_PARSE, 3U, "unexpected trailing tokens after condition"},
      {"if false:\n    print(1)\nelse\n    print(2)\n", GINT_ERR_PARSE, 3U, "expected ':' after else"},
      {"if false:\n    print(1)\nelse: extra\n    print(2)\n", GINT_ERR_PARSE, 3U, "unexpected trailing tokens after else"},
      {"if nope:\n    print(1)\n", GINT_ERR_UNKNOWN_OPERAND, 1U, "unknown operand"},
      {"flag = false\nif flag:\n    print(1)\nelif nope:\n    print(2)\nelse:\n    print(3)\n", GINT_ERR_UNKNOWN_OPERAND, 4U, "unknown operand"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != cases[i].expected_rc) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (cases[i].expected_rc == GINT_OK) {
      graphion_runtime_scope_dispose(&scope);
      continue;
    }
    if (diagnostic.line != cases[i].expected_line) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(3 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_equality_expressions(void) {
  const char *source =
      "same_int = 42 == 42\n"
      "mixed_numeric = 42 == 42.0\n"
      "int_true_bool = 1 == true\n"
      "int_false_bool = 0 == false\n"
      "different_numeric = 42 == 41\n"
      "same_bool = true == true\n"
      "different_bool = true == false\n"
      "same_string = \"ok\" == \"ok\"\n"
      "different_string = \"ok\" == \"no\"\n"
      "grouped = (1 + 2) == 3\n"
      "precedence = 1 + 2 == 3\n"
      "power_cmp = 2 ** 3 == 8\n"
      "print(same_int)\n"
      "print(mixed_numeric)\n"
      "print(int_true_bool)\n"
      "print(int_false_bool)\n"
      "print(different_numeric)\n"
      "print(same_bool)\n"
      "print(different_bool)\n"
      "print(same_string)\n"
      "print(different_string)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  const char *path = "gion_equality_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *same_int;
  const graphion_runtime_value *mixed_numeric;
  const graphion_runtime_value *int_true_bool;
  const graphion_runtime_value *int_false_bool;
  const graphion_runtime_value *different_numeric;
  const graphion_runtime_value *same_string;
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  same_int = graphion_runtime_scope_find(&scope, "same_int");
  mixed_numeric = graphion_runtime_scope_find(&scope, "mixed_numeric");
  int_true_bool = graphion_runtime_scope_find(&scope, "int_true_bool");
  int_false_bool = graphion_runtime_scope_find(&scope, "int_false_bool");
  different_numeric = graphion_runtime_scope_find(&scope, "different_numeric");
  same_string = graphion_runtime_scope_find(&scope, "same_string");
  if (same_int == NULL || same_int->kind != GVM_VALUE_BOOL || same_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (mixed_numeric == NULL || mixed_numeric->kind != GVM_VALUE_BOOL || mixed_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (int_true_bool == NULL || int_true_bool->kind != GVM_VALUE_BOOL || int_true_bool->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (int_false_bool == NULL || int_false_bool->kind != GVM_VALUE_BOOL || int_false_bool->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (different_numeric == NULL || different_numeric->kind != GVM_VALUE_BOOL || different_numeric->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (same_string == NULL || same_string->kind != GVM_VALUE_BOOL || same_string->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  remove(path);
  if (strcmp(output, "true\ntrue\ntrue\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 10);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_equality_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 1 == \"1\"\n", 1U},
      {"if 1 == \"1\":\n    print(1)\n", 1U},
      {"value = \"true\" == true\n", 1U},
      {"value = \"x\" == 1.5\n", 1U},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_RUN) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "incompatible operand types") != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_comment_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"/* unterminated block comment\ncount = 42\n", 1U},
      {"count = 42\n/* unterminated block comment\nprint(count)\n", 2U},
      {"message = \"/* not a comment */\"\n/* unterminated\n", 2U},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    graphion_runtime_program program;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "unterminated block comment") != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);

    graphion_runtime_program_init(&program);
    rc = graphion_prepare_source(cases[i].source, &program, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      graphion_runtime_program_dispose(&program);
      return (int)(3 + i * 10U);
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "unterminated block comment") != 0) {
      graphion_runtime_program_dispose(&program);
      return (int)(4 + i * 10U);
    }
    graphion_runtime_program_dispose(&program);
  }
  return 0;
}

int test_gion_comments(void) {
  const char *prepare_source_text =
      "# prepare_source should ignore comments too\n"
      "count = 40 # inline line comment\n"
      "/* block comment before an assignment */\n"
      "count += 2\n"
      "message = \"/* not a comment */\"\n"
      "ratio = /* inline block */ 7 / 2\n"
      "print(count)\n"
      "print(message)\n"
      "print(ratio)\n";
  const char *source =
      "# line comment before code\n"
      "count = 40 # inline line comment\n"
      "/* block comment before an assignment */\n"
      "count += 2\n"
      "message = \"/* not a comment */\"\n"
      "/*\n"
      "multi-line block comment\n"
      "that spans several lines\n"
      "*/\n"
      "if true: # comment after header\n"
      "    # comment inside block\n"
      "    label = \"ok\" /* inline block comment in block */\n"
      "else:\n"
      "    label = \"bad\"\n"
      "ratio = /* inline block */ 7 / 2\n"
      "print(count) # trailing print comment\n"
      "print(message)\n"
      "print(label)\n"
      "print(ratio)\n";
  const char *path = "gion_comments.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  graphion_runtime_program program;
  const graphion_runtime_value *count;
  const graphion_runtime_value *message;
  const graphion_runtime_value *label;
  const graphion_runtime_value *ratio;
  FILE *fp = NULL;
  int rc;

  graphion_runtime_scope_init(&scope);
  rc = graphion_prepare_source(prepare_source_text, &program, &diagnostic);
  if (rc != GINT_OK) {
    return finish_scope_test(&scope, 1);
  }
  graphion_runtime_program_dispose(&program);
#if defined(_MSC_VER)
  if (fopen_s(&fp, path, "wb") != 0) {
    fp = NULL;
  }
#else
  fp = fopen(path, "wb");
#endif
  if (fp == NULL) {
    return finish_scope_test(&scope, 2);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  count = graphion_runtime_scope_find(&scope, "count");
  message = graphion_runtime_scope_find(&scope, "message");
  label = graphion_runtime_scope_find(&scope, "label");
  ratio = graphion_runtime_scope_find(&scope, "ratio");
  if (count == NULL || count->kind != GVM_VALUE_INT || count->as.int_value != 42) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (message == NULL || message->kind != GVM_VALUE_STRING || strcmp(message->as.string_value, "/* not a comment */") != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (label == NULL || label->kind != GVM_VALUE_STRING || strcmp(label->as.string_value, "ok") != 0) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (ratio == NULL || ratio->kind != GVM_VALUE_FLOAT || ratio->as.float_value != 3.5) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  remove(path);
  if (strcmp(output, "42\n/* not a comment */\nok\n3.5\n") != 0) {
    return finish_scope_test(&scope, 9);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_equality_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 ==\n", "expected scalar literal"},
      {"value = == 1\n", "expected scalar literal"},
      {"print(1 == )\n", "expected scalar literal"},
      {"if 1 ==:\n    print(1)\n", "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_inequality_expressions(void) {
  const char *source =
      "different_int = 42 != 41\n"
      "same_numeric = 42 != 42.0\n"
      "int_true_bool = 1 != true\n"
      "int_false_bool = 0 != false\n"
      "different_numeric = 42 != 43\n"
      "different_bool = true != false\n"
      "same_string = \"ok\" != \"ok\"\n"
      "different_string = \"ok\" != \"no\"\n"
      "grouped = (1 + 2) != 4\n"
      "precedence = 1 + 2 != 4\n"
      "power_cmp = 2 ** 3 != 9\n"
      "print(different_int)\n"
      "print(same_numeric)\n"
      "print(int_true_bool)\n"
      "print(int_false_bool)\n"
      "print(different_numeric)\n"
      "print(different_bool)\n"
      "print(same_string)\n"
      "print(different_string)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  const char *path = "gion_inequality_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *different_int;
  const graphion_runtime_value *same_numeric;
  const graphion_runtime_value *int_true_bool;
  const graphion_runtime_value *int_false_bool;
  const graphion_runtime_value *different_numeric;
  const graphion_runtime_value *different_string;
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  different_int = graphion_runtime_scope_find(&scope, "different_int");
  same_numeric = graphion_runtime_scope_find(&scope, "same_numeric");
  int_true_bool = graphion_runtime_scope_find(&scope, "int_true_bool");
  int_false_bool = graphion_runtime_scope_find(&scope, "int_false_bool");
  different_numeric = graphion_runtime_scope_find(&scope, "different_numeric");
  different_string = graphion_runtime_scope_find(&scope, "different_string");
  if (different_int == NULL || different_int->kind != GVM_VALUE_BOOL || different_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (same_numeric == NULL || same_numeric->kind != GVM_VALUE_BOOL || same_numeric->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (int_true_bool == NULL || int_true_bool->kind != GVM_VALUE_BOOL || int_true_bool->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  if (int_false_bool == NULL || int_false_bool->kind != GVM_VALUE_BOOL || int_false_bool->as.bool_value != 0) {
    remove(path);
    return finish_scope_test(&scope, 6);
  }
  if (different_numeric == NULL || different_numeric->kind != GVM_VALUE_BOOL || different_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 7);
  }
  if (different_string == NULL || different_string->kind != GVM_VALUE_BOOL || different_string->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 8);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 9);
  }
  remove(path);
  if (strcmp(output, "true\nfalse\nfalse\nfalse\ntrue\ntrue\nfalse\ntrue\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 10);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_inequality_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = 1 != \"1\"\n", 1U},
      {"if 1 != \"1\":\n    print(1)\n", 1U},
      {"value = \"true\" != true\n", 1U},
      {"value = \"x\" != 1.5\n", 1U},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_RUN) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "incompatible operand types") != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_inequality_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 !=\n", "expected scalar literal"},
      {"value = != 1\n", "expected scalar literal"},
      {"print(1 != )\n", "expected scalar literal"},
      {"if 1 !=:\n    print(1)\n", "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_less_than_expressions(void) {
  const char *source =
      "smaller_int = 1 < 2\n"
      "same_numeric = 2 < 2.0\n"
      "mixed_numeric = 2 < 2.5\n"
      "reverse_numeric = 3.0 < 2\n"
      "grouped = (1 + 2) < 4\n"
      "precedence = 1 + 2 < 4\n"
      "power_cmp = 2 ** 3 < 9\n"
      "print(smaller_int)\n"
      "print(same_numeric)\n"
      "print(mixed_numeric)\n"
      "print(reverse_numeric)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  const char *path = "gion_less_than_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *smaller_int;
  const graphion_runtime_value *mixed_numeric;
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  smaller_int = graphion_runtime_scope_find(&scope, "smaller_int");
  mixed_numeric = graphion_runtime_scope_find(&scope, "mixed_numeric");
  if (smaller_int == NULL || smaller_int->kind != GVM_VALUE_BOOL || smaller_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (mixed_numeric == NULL || mixed_numeric->kind != GVM_VALUE_BOOL || mixed_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_less_than_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = true < 1\n", 1U},
      {"if true < 1:\n    print(1)\n", 1U},
      {"value = \"x\" < \"y\"\n", 1U},
      {"value = \"x\" < 1.5\n", 1U},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_RUN) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "incompatible operand types") != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_less_than_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 <\n", "expected scalar literal"},
      {"value = < 1\n", "expected scalar literal"},
      {"print(1 < )\n", "expected scalar literal"},
      {"if 1 <:\n    print(1)\n", "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_less_equal_expressions(void) {
  const char *source =
      "smaller_int = 1 <= 2\n"
      "same_numeric = 2 <= 2.0\n"
      "mixed_numeric = 2 <= 2.5\n"
      "reverse_numeric = 3.0 <= 2\n"
      "grouped = (1 + 2) <= 3\n"
      "precedence = 1 + 2 <= 3\n"
      "power_cmp = 2 ** 3 <= 8\n"
      "print(smaller_int)\n"
      "print(same_numeric)\n"
      "print(mixed_numeric)\n"
      "print(reverse_numeric)\n"
      "print(grouped)\n"
      "print(precedence)\n"
      "print(power_cmp)\n";
  const char *path = "gion_less_equal_expressions.txt";
  char output[128];
  graphion_runtime_scope scope;
  graphion_runtime_diagnostic diagnostic;
  const graphion_runtime_value *smaller_int;
  const graphion_runtime_value *same_numeric;
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  smaller_int = graphion_runtime_scope_find(&scope, "smaller_int");
  same_numeric = graphion_runtime_scope_find(&scope, "same_numeric");
  if (smaller_int == NULL || smaller_int->kind != GVM_VALUE_BOOL || smaller_int->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (same_numeric == NULL || same_numeric->kind != GVM_VALUE_BOOL || same_numeric->as.bool_value != 1) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 5);
  }
  remove(path);
  if (strcmp(output, "true\ntrue\ntrue\nfalse\ntrue\ntrue\ntrue\n") != 0) {
    return finish_scope_test(&scope, 6);
  }
  return finish_scope_test(&scope, 0);
}

int test_gion_less_equal_runtime_errors(void) {
  static const struct {
    const char *source;
    unsigned int expected_line;
  } cases[] = {
      {"value = true <= 1\n", 1U},
      {"if true <= 1:\n    print(1)\n", 1U},
      {"value = \"x\" <= \"y\"\n", 1U},
      {"value = \"x\" <= 1.5\n", 1U},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_RUN) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.line != cases[i].expected_line || diagnostic.message == NULL ||
        strcmp(diagnostic.message, "incompatible operand types") != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
  }
  return 0;
}

int test_gion_less_equal_syntax_errors(void) {
  static const struct {
    const char *source;
    const char *message;
  } cases[] = {
      {"value = 1 <=\n", "expected scalar literal"},
      {"value = <= 1\n", "expected scalar literal"},
      {"print(1 <= )\n", "expected scalar literal"},
      {"if 1 <=:\n    print(1)\n", "expected scalar literal"},
  };
  size_t i;

  for (i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    graphion_runtime_scope scope;
    graphion_runtime_diagnostic diagnostic;
    int rc;

    graphion_runtime_scope_init(&scope);
    rc = graphion_interpret_source(cases[i].source, &scope, &diagnostic);
    if (rc != GINT_ERR_PARSE) {
      return finish_scope_test(&scope, (int)(1 + i * 10U));
    }
    if (diagnostic.message == NULL || strcmp(diagnostic.message, cases[i].message) != 0) {
      return finish_scope_test(&scope, (int)(2 + i * 10U));
    }
    graphion_runtime_scope_dispose(&scope);
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
    return finish_scope_test(&scope, 1);
  }
  rc = graphion_interpret_source_with_output(source, &scope, &diagnostic, fp);
  fclose(fp);
  if (rc != GINT_OK) {
    remove(path);
    return finish_scope_test(&scope, 2);
  }
  copy_i = graphion_runtime_scope_find(&scope, "copy_i");
  if (copy_i == NULL || copy_i->kind != GVM_VALUE_INT || copy_i->as.int_value != -7) {
    remove(path);
    return finish_scope_test(&scope, 3);
  }
  if (!test_read_file_text(path, output, sizeof(output))) {
    remove(path);
    return finish_scope_test(&scope, 4);
  }
  remove(path);
  if (strcmp(output, "-7\n-3.25\nfalse\nhello\n-7\n") != 0) {
    return finish_scope_test(&scope, 5);
  }
  return finish_scope_test(&scope, 0);
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

