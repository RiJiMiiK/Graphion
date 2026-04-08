/* SPDX-License-Identifier: MIT */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int (*test_fn)(void);

typedef struct {
  const char *name;
  test_fn fn;
} test_case;

#include "../vm/test_vm_cases.h"
#include "../gion/test_parser_cases.h"
#include "../core/test_core_cases.h"


static int should_run_test(const char *name, int argc, char **argv) {
  int i;
  if (argc <= 1) {
    return 1;
  }
  for (i = 1; i < argc; ++i) {
    if (strcmp(name, argv[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static int unknown_requested_tests(const test_case *tests, size_t count, int argc, char **argv) {
  int i;
  for (i = 1; i < argc; ++i) {
    size_t j;
    int found = 0;
    for (j = 0; j < count; ++j) {
      if (strcmp(argv[i], tests[j].name) == 0) {
        found = 1;
        break;
      }
    }
    if (!found) {
      fprintf(stderr, "[FAIL] unknown test '%s'\n", argv[i]);
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  const test_case tests[] = {
      {"vm_addition_program", test_vm_addition_program},
      {"vm_invalid_register_fails", test_vm_invalid_register_fails},
      {"vm_typed_register_defaults", test_vm_typed_register_defaults},
      {"vm_value_movement_and_globals", test_vm_value_movement_and_globals},
      {"vm_typed_value_errors", test_vm_typed_value_errors},
      {"vm_numeric_arithmetic_opcodes", test_vm_numeric_arithmetic_opcodes},
      {"vm_numeric_arithmetic_precedence_shapes", test_vm_numeric_arithmetic_precedence_shapes},
      {"vm_divide_by_zero_fails", test_vm_divide_by_zero_fails},
      {"vm_modulo_opcode", test_vm_modulo_opcode},
      {"vm_power_opcode", test_vm_power_opcode},
      {"vm_floor_div_opcode", test_vm_floor_div_opcode},
      {"vm_abs_opcode", test_vm_abs_opcode},
      {"vm_min_opcode", test_vm_min_opcode},
      {"vm_max_opcode", test_vm_max_opcode},
      {"vm_clamp_opcode", test_vm_clamp_opcode},
      {"vm_sqrt_opcode", test_vm_sqrt_opcode},
      {"vm_cbrt_builtin_opcode", test_vm_cbrt_builtin_opcode},
      {"vm_sin_builtin_opcode", test_vm_sin_builtin_opcode},
      {"vm_cos_builtin_opcode", test_vm_cos_builtin_opcode},
        {"vm_tan_builtin_opcode", test_vm_tan_builtin_opcode},
        {"vm_asin_builtin_opcode", test_vm_asin_builtin_opcode},
        {"vm_acos_builtin_opcode", test_vm_acos_builtin_opcode},
        {"vm_exp_opcode", test_vm_exp_opcode},
      {"vm_ln_opcode", test_vm_ln_opcode},
      {"vm_log_opcode", test_vm_log_opcode},
      {"vm_floor_builtin_opcode", test_vm_floor_builtin_opcode},
      {"vm_ceil_builtin_opcode", test_vm_ceil_builtin_opcode},
      {"vm_round_builtin_opcode", test_vm_round_builtin_opcode},
      {"vm_trunc_builtin_opcode", test_vm_trunc_builtin_opcode},
      {"vm_sign_builtin_opcode", test_vm_sign_builtin_opcode},
      {"vm_len_opcode", test_vm_len_opcode},
      {"vm_factorial_opcode", test_vm_factorial_opcode},
      {"vm_eq_opcode", test_vm_eq_opcode},
      {"vm_eq_incompatible_types_fail", test_vm_eq_incompatible_types_fail},
      {"vm_eq_int_bool_out_of_range_fail", test_vm_eq_int_bool_out_of_range_fail},
      {"vm_ne_opcode", test_vm_ne_opcode},
      {"vm_ne_incompatible_types_fail", test_vm_ne_incompatible_types_fail},
      {"vm_ne_int_bool_out_of_range_fail", test_vm_ne_int_bool_out_of_range_fail},
      {"vm_lt_opcode", test_vm_lt_opcode},
      {"vm_lt_incompatible_types_fail", test_vm_lt_incompatible_types_fail},
      {"vm_le_opcode", test_vm_le_opcode},
      {"vm_le_incompatible_types_fail", test_vm_le_incompatible_types_fail},
      {"vm_gt_opcode", test_vm_gt_opcode},
      {"vm_gt_incompatible_types_fail", test_vm_gt_incompatible_types_fail},
      {"vm_ge_opcode", test_vm_ge_opcode},
      {"vm_ge_incompatible_types_fail", test_vm_ge_incompatible_types_fail},
      {"vm_bit_and_opcode", test_vm_bit_and_opcode},
      {"vm_bit_and_incompatible_types_fail", test_vm_bit_and_incompatible_types_fail},
      {"vm_bit_or_opcode", test_vm_bit_or_opcode},
      {"vm_bit_or_incompatible_types_fail", test_vm_bit_or_incompatible_types_fail},
      {"vm_bit_xor_opcode", test_vm_bit_xor_opcode},
      {"vm_bit_xor_incompatible_types_fail", test_vm_bit_xor_incompatible_types_fail},
      {"vm_bit_not_opcode", test_vm_bit_not_opcode},
      {"vm_bit_not_incompatible_types_fail", test_vm_bit_not_incompatible_types_fail},
      {"vm_bit_shl_opcode", test_vm_bit_shl_opcode},
      {"vm_bit_shl_incompatible_types_fail", test_vm_bit_shl_incompatible_types_fail},
      {"vm_bit_shr_opcode", test_vm_bit_shr_opcode},
      {"vm_bit_shr_incompatible_types_fail", test_vm_bit_shr_incompatible_types_fail},
      {"vm_and_opcode", test_vm_and_opcode},
      {"vm_and_incompatible_types_fail", test_vm_and_incompatible_types_fail},
      {"vm_or_opcode", test_vm_or_opcode},
      {"vm_or_incompatible_types_fail", test_vm_or_incompatible_types_fail},
      {"vm_not_opcode", test_vm_not_opcode},
      {"vm_not_incompatible_types_fail", test_vm_not_incompatible_types_fail},
      {"vm_nand_opcode", test_vm_nand_opcode},
      {"vm_nand_incompatible_types_fail", test_vm_nand_incompatible_types_fail},
      {"vm_nor_opcode", test_vm_nor_opcode},
      {"vm_nor_incompatible_types_fail", test_vm_nor_incompatible_types_fail},
      {"vm_jump_opcode", test_vm_jump_opcode},
      {"vm_string_addition_opcode", test_vm_string_addition_opcode},
      {"vm_print_scalar_opcodes", test_vm_print_scalar_opcodes},
      {"vm_print_reg_opcode", test_vm_print_reg_opcode},
      {"vm_bfs_levels_opcode", test_vm_bfs_levels_opcode},
      {"vm_bfs_level_count_opcode", test_vm_bfs_level_count_opcode},
      {"vm_bfs_order_opcode", test_vm_bfs_order_opcode},
      {"vm_hypergraph_opcodes", test_vm_hypergraph_opcodes},
      {"vm_superinstruction_add_pair_semantics", test_vm_superinstruction_add_pair_semantics},
      {"vm_superinstruction_movimm_add_semantics", test_vm_superinstruction_movimm_add_semantics},
      {"vm_deterministic_mode_toggle", test_vm_deterministic_mode_toggle},
      {"vm_deterministic_mode_unknown_opcode", test_vm_deterministic_mode_unknown_opcode},
      {"vm_deterministic_mode_graph_semantics", test_vm_deterministic_mode_graph_semantics},
      {"vm_add_wraparound_semantics", test_vm_add_wraparound_semantics},
      {"vm_frontier_primitives", test_vm_frontier_primitives},
      {"vm_frontier_errors", test_vm_frontier_errors},
      {"vm_neighbor_iteration_primitives", test_vm_neighbor_iteration_primitives},
      {"vm_neighbor_iteration_errors", test_vm_neighbor_iteration_errors},
      {"vm_weighted_graph_opcodes", test_vm_weighted_graph_opcodes},
      {"vm_weighted_graph_opcode_errors", test_vm_weighted_graph_opcode_errors},
      {"vm_hyperedge_traversal_primitives", test_vm_hyperedge_traversal_primitives},
      {"vm_hyperedge_traversal_errors", test_vm_hyperedge_traversal_errors},
      {"vm_snapshot_format", test_vm_snapshot_format},
      {"vm_fastpath_shape_cache_load_flags", test_vm_fastpath_shape_cache_load_flags},
      {"vm_fastpath_shape_cache_same_pointer_content_change",
       test_vm_fastpath_shape_cache_same_pointer_content_change},
      {"vm_dispatch_variant_edge_semantics", test_vm_dispatch_variant_edge_semantics},
      {"arena_alignment_and_reset", test_arena_alignment_and_reset},
      {"arena_invalid_alignment_fails", test_arena_invalid_alignment_fails},
      {"parser_decode_valid_program", test_parser_decode_valid_program},
      {"parser_rejects_truncated_input", test_parser_rejects_truncated_input},
      {"frontend_parse_and_ir_lowering", test_frontend_parse_and_ir_lowering},
      {"frontend_rejects_invalid_source", test_frontend_rejects_invalid_source},
      {"frontend_source_to_vm_execution", test_frontend_source_to_vm_execution},
      {"frontend_reference_graph_execution_examples", test_frontend_reference_graph_execution_examples},
      {"gion_source_path_detection", test_gion_source_path_detection},
      {"gion_scalar_assignments_and_prints", test_gion_scalar_assignments_and_prints},
      {"gion_scalar_feature_varied_names", test_gion_scalar_feature_varied_names},
      {"gion_unknown_variable_errors", test_gion_unknown_variable_errors},
      {"gion_partial_execution_stops_at_first_unsupported_line", test_gion_partial_execution_stops_at_first_unsupported_line},
      {"gion_reserved_name_errors", test_gion_reserved_name_errors},
      {"gion_assignment_syntax_errors", test_gion_assignment_syntax_errors},
      {"gion_arithmetic_expressions", test_gion_arithmetic_expressions},
      {"gion_string_concatenation", test_gion_string_concatenation},
      {"gion_print_string_coercion", test_gion_print_string_coercion},
      {"gion_compound_assignments", test_gion_compound_assignments},
      {"gion_compound_assignment_errors", test_gion_compound_assignment_errors},
      {"gion_arithmetic_precedence_and_associativity", test_gion_arithmetic_precedence_and_associativity},
      {"gion_arithmetic_runtime_errors", test_gion_arithmetic_runtime_errors},
      {"gion_arithmetic_syntax_errors", test_gion_arithmetic_syntax_errors},
      {"gion_bits_literals", test_gion_bits_literals},
      {"gion_bits_literal_syntax_errors", test_gion_bits_literal_syntax_errors},
      {"gion_bits_equality", test_gion_bits_equality},
      {"gion_bits_inequality", test_gion_bits_inequality},
      {"gion_bits_mixed_type_errors", test_gion_bits_mixed_type_errors},
      {"gion_bits_and", test_gion_bits_and},
      {"gion_bits_and_runtime_errors", test_gion_bits_and_runtime_errors},
      {"gion_bits_or", test_gion_bits_or},
      {"gion_bits_or_runtime_errors", test_gion_bits_or_runtime_errors},
      {"gion_bits_xor", test_gion_bits_xor},
      {"gion_bits_xor_runtime_errors", test_gion_bits_xor_runtime_errors},
      {"gion_bits_not", test_gion_bits_not},
      {"gion_bits_not_runtime_errors", test_gion_bits_not_runtime_errors},
      {"gion_bits_shl", test_gion_bits_shl},
      {"gion_bits_shl_runtime_errors", test_gion_bits_shl_runtime_errors},
      {"gion_bits_shr", test_gion_bits_shr},
      {"gion_bits_shr_runtime_errors", test_gion_bits_shr_runtime_errors},
      {"gion_print_syntax_errors", test_gion_print_syntax_errors},
      {"gion_unterminated_string_errors", test_gion_unterminated_string_errors},
      {"gion_invalid_identifier_errors", test_gion_invalid_identifier_errors},
      {"gion_trailing_token_errors", test_gion_trailing_token_errors},
      {"gion_reference_before_definition_errors", test_gion_reference_before_definition_errors},
      {"gion_reassignment_and_type_change", test_gion_reassignment_and_type_change},
      {"gion_copy_chains_and_blank_lines", test_gion_copy_chains_and_blank_lines},
      {"gion_late_line_error_diagnostics", test_gion_late_line_error_diagnostics},
      {"gion_unexpected_indentation_errors", test_gion_unexpected_indentation_errors},
      {"gion_mixed_scalar_values", test_gion_mixed_scalar_values},
      {"gion_capacity_errors", test_gion_capacity_errors},
      {"gion_if_elif_else_control_flow", test_gion_if_elif_else_control_flow},
      {"gion_if_elif_else_errors", test_gion_if_elif_else_errors},
      {"gion_match_control_flow", test_gion_match_control_flow},
      {"gion_match_errors", test_gion_match_errors},
      {"gion_comments", test_gion_comments},
      {"gion_comment_errors", test_gion_comment_errors},
      {"gion_equality_expressions", test_gion_equality_expressions},
      {"gion_equality_runtime_errors", test_gion_equality_runtime_errors},
      {"gion_equality_syntax_errors", test_gion_equality_syntax_errors},
      {"gion_inequality_expressions", test_gion_inequality_expressions},
      {"gion_inequality_runtime_errors", test_gion_inequality_runtime_errors},
      {"gion_inequality_syntax_errors", test_gion_inequality_syntax_errors},
      {"gion_less_than_expressions", test_gion_less_than_expressions},
      {"gion_less_than_runtime_errors", test_gion_less_than_runtime_errors},
      {"gion_less_than_syntax_errors", test_gion_less_than_syntax_errors},
      {"gion_less_equal_expressions", test_gion_less_equal_expressions},
      {"gion_less_equal_runtime_errors", test_gion_less_equal_runtime_errors},
      {"gion_less_equal_syntax_errors", test_gion_less_equal_syntax_errors},
      {"gion_greater_than_expressions", test_gion_greater_than_expressions},
      {"gion_greater_than_runtime_errors", test_gion_greater_than_runtime_errors},
      {"gion_greater_than_syntax_errors", test_gion_greater_than_syntax_errors},
      {"gion_greater_equal_expressions", test_gion_greater_equal_expressions},
      {"gion_greater_equal_runtime_errors", test_gion_greater_equal_runtime_errors},
      {"gion_greater_equal_syntax_errors", test_gion_greater_equal_syntax_errors},
    {"gion_and_expressions", test_gion_and_expressions},
    {"gion_and_runtime_errors", test_gion_and_runtime_errors},
    {"gion_and_syntax_errors", test_gion_and_syntax_errors},
    {"gion_or_expressions", test_gion_or_expressions},
    {"gion_or_runtime_errors", test_gion_or_runtime_errors},
    {"gion_or_syntax_errors", test_gion_or_syntax_errors},
    {"gion_not_expressions", test_gion_not_expressions},
    {"gion_not_runtime_errors", test_gion_not_runtime_errors},
    {"gion_not_syntax_errors", test_gion_not_syntax_errors},
    {"gion_nand_expressions", test_gion_nand_expressions},
    {"gion_nand_runtime_errors", test_gion_nand_runtime_errors},
    {"gion_nand_syntax_errors", test_gion_nand_syntax_errors},
    {"gion_nor_expressions", test_gion_nor_expressions},
    {"gion_nor_runtime_errors", test_gion_nor_runtime_errors},
    {"gion_nor_syntax_errors", test_gion_nor_syntax_errors},
    {"gion_boolean_short_circuit", test_gion_boolean_short_circuit},
    {"gion_boolean_short_circuit_runtime_errors", test_gion_boolean_short_circuit_runtime_errors},
      {"gion_ternary_expressions", test_gion_ternary_expressions},
      {"gion_ternary_runtime_errors", test_gion_ternary_runtime_errors},
      {"gion_ternary_syntax_errors", test_gion_ternary_syntax_errors},
      {"gion_warning_directives", test_gion_warning_directives},
      {"gion_warning_directives_from_path", test_gion_warning_directives_from_path},
      {"isa_decode_golden_fixtures", test_isa_decode_golden_fixtures},
      {"isa_execute_golden_fixtures", test_isa_execute_golden_fixtures},
      {"graph_init_and_neighbors", test_graph_init_and_neighbors},
      {"graph_bfs_levels", test_graph_bfs_levels},
      {"graph_optional_edge_data", test_graph_optional_edge_data},
      {"graph_frontier_mode_heuristics", test_graph_frontier_mode_heuristics},
      {"hypergraph_init_and_queries", test_hypergraph_init_and_queries},
  };
  const size_t count = sizeof(tests) / sizeof(tests[0]);
  size_t i;
  size_t executed = 0;

  if (unknown_requested_tests(tests, count, argc, argv) != 0) {
    return EXIT_FAILURE;
  }

  for (i = 0; i < count; ++i) {
    if (!should_run_test(tests[i].name, argc, argv)) {
      continue;
    }
    {
      const int rc = tests[i].fn();
      if (rc != 0) {
        fprintf(stderr, "[FAIL] %s (rc=%d)\n", tests[i].name, rc);
        return EXIT_FAILURE;
      }
    }
    fprintf(stdout, "[OK] %s\n", tests[i].name);
    executed++;
  }

  fprintf(stdout, "All tests passed (%zu)\n", executed);
  return EXIT_SUCCESS;
}
