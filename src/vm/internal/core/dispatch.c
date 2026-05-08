/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/fastpath.h"
#include "vm/internal/opcodes/op_frontier.h"
#include "vm/internal/opcodes/op_graph.h"
#include "vm/internal/opcodes/op_hypergraph.h"
#include "vm/internal/opcodes/op_io.h"
#include "vm/internal/opcodes/op_scalar.h"
#include "vm/internal/opcodes/op_state.h"
#include "vm/vm.h"

static int run_dispatch_switch(graphion_vm *vm) {
  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc++];
    int rc;
    switch (in.op) {
      case GVM_OP_NOP:
        rc = op_nop(vm, &in);
        break;
      case GVM_OP_HALT:
        rc = op_halt(vm, &in);
        break;
      case GVM_OP_MOV_IMM:
        rc = op_mov_imm(vm, &in);
        break;
      case GVM_OP_ADD:
        rc = op_add(vm, &in);
        break;
      case GVM_OP_SUB:
        rc = op_sub(vm, &in);
        break;
      case GVM_OP_MUL:
        rc = op_mul(vm, &in);
        break;
      case GVM_OP_DIV:
        rc = op_div(vm, &in);
        break;
      case GVM_OP_MOD:
        rc = op_mod(vm, &in);
        break;
      case GVM_OP_POW:
        rc = op_pow(vm, &in);
        break;
      case GVM_OP_FLOOR_DIV:
        rc = op_floor_div(vm, &in);
        break;
      case GVM_OP_EQ:
        rc = op_eq_cmp(vm, &in);
        break;
      case GVM_OP_NE:
        rc = op_ne_cmp(vm, &in);
        break;
      case GVM_OP_LT:
        rc = op_lt_cmp(vm, &in);
        break;
      case GVM_OP_LE:
        rc = op_le_cmp(vm, &in);
        break;
      case GVM_OP_GT:
        rc = op_gt_cmp(vm, &in);
        break;
      case GVM_OP_GE:
        rc = op_ge_cmp(vm, &in);
        break;
      case GVM_OP_BIT_AND:
        rc = op_bit_and_cmp(vm, &in);
        break;
      case GVM_OP_BIT_OR:
        rc = op_bit_or_cmp(vm, &in);
        break;
      case GVM_OP_BIT_XOR:
        rc = op_bit_xor_cmp(vm, &in);
        break;
      case GVM_OP_BIT_NOT:
        rc = op_bit_not_cmp(vm, &in);
        break;
      case GVM_OP_BIT_SHL:
        rc = op_bit_shl_cmp(vm, &in);
        break;
      case GVM_OP_BIT_SHR:
        rc = op_bit_shr_cmp(vm, &in);
        break;
      case GVM_OP_AND:
        rc = op_and_cmp(vm, &in);
        break;
      case GVM_OP_OR:
        rc = op_or_cmp(vm, &in);
        break;
      case GVM_OP_NOT:
        rc = op_not_cmp(vm, &in);
        break;
      case GVM_OP_NAND:
        rc = op_nand_cmp(vm, &in);
        break;
      case GVM_OP_NOR:
        rc = op_nor_cmp(vm, &in);
        break;
      case GVM_OP_JUMP:
        rc = op_jump(vm, &in);
        break;
      case GVM_OP_JUMP_IF_TRUE:
        rc = op_jump_if_true(vm, &in);
        break;
      case GVM_OP_JUMP_IF_FALSE:
        rc = op_jump_if_false(vm, &in);
        break;
      case GVM_OP_ABS:
        rc = op_abs(vm, &in);
        break;
      case GVM_OP_MIN:
        rc = op_min(vm, &in);
        break;
      case GVM_OP_MAX:
        rc = op_max(vm, &in);
        break;
      case GVM_OP_CLAMP:
        rc = op_clamp(vm, &in);
        break;
      case GVM_OP_SQRT:
        rc = op_sqrt(vm, &in);
        break;
      case GVM_OP_CBRT:
        rc = op_cbrt_builtin(vm, &in);
        break;
      case GVM_OP_SIN:
        rc = op_sin_builtin(vm, &in);
        break;
      case GVM_OP_CSC:
        rc = op_csc_builtin(vm, &in);
        break;
      case GVM_OP_SEC:
        rc = op_sec_builtin(vm, &in);
        break;
      case GVM_OP_COT:
        rc = op_cot_builtin(vm, &in);
        break;
      case GVM_OP_ACSC:
        rc = op_acsc_builtin(vm, &in);
        break;
      case GVM_OP_ASEC:
        rc = op_asec_builtin(vm, &in);
        break;
      case GVM_OP_ACOT:
        rc = op_acot_builtin(vm, &in);
        break;
      case GVM_OP_SECH:
        rc = op_sech_builtin(vm, &in);
        break;
      case GVM_OP_CSCH:
        rc = op_csch_builtin(vm, &in);
        break;
      case GVM_OP_COTH:
        rc = op_coth_builtin(vm, &in);
        break;
      case GVM_OP_SINH:
        rc = op_sinh_builtin(vm, &in);
        break;
      case GVM_OP_ASINH:
        rc = op_asinh_builtin(vm, &in);
        break;
      case GVM_OP_ACOSH:
        rc = op_acosh_builtin(vm, &in);
        break;
      case GVM_OP_COSH:
        rc = op_cosh_builtin(vm, &in);
        break;
      case GVM_OP_TANH:
        rc = op_tanh_builtin(vm, &in);
        break;
      case GVM_OP_ATANH:
        rc = op_atanh_builtin(vm, &in);
        break;
      case GVM_OP_COS:
        rc = op_cos_builtin(vm, &in);
        break;
      case GVM_OP_TAN:
        rc = op_tan_builtin(vm, &in);
        break;
      case GVM_OP_ASIN:
        rc = op_asin_builtin(vm, &in);
        break;
      case GVM_OP_ACOS:
        rc = op_acos_builtin(vm, &in);
        break;
      case GVM_OP_ATAN:
        rc = op_atan_builtin(vm, &in);
        break;
      case GVM_OP_ATAN2:
        rc = op_atan2_builtin(vm, &in);
        break;
      case GVM_OP_HYPOT:
        rc = op_hypot_builtin(vm, &in);
        break;
      case GVM_OP_COPYSIGN:
        rc = op_copysign_builtin(vm, &in);
        break;
      case GVM_OP_DEGREES:
        rc = op_degrees_builtin(vm, &in);
        break;
      case GVM_OP_RADIANS:
        rc = op_radians_builtin(vm, &in);
        break;
      case GVM_OP_ISNAN:
        rc = op_isnan_builtin(vm, &in);
        break;
      case GVM_OP_ISINF:
        rc = op_isinf_builtin(vm, &in);
        break;
      case GVM_OP_ISFINITE:
        rc = op_isfinite_builtin(vm, &in);
        break;
      case GVM_OP_EXPM1:
        rc = op_expm1_builtin(vm, &in);
        break;
      case GVM_OP_EXP2:
        rc = op_exp2_builtin(vm, &in);
        break;
      case GVM_OP_LOG1P:
        rc = op_log1p_builtin(vm, &in);
        break;
      case GVM_OP_ERF:
        rc = op_erf_builtin(vm, &in);
        break;
      case GVM_OP_ERFC:
        rc = op_erfc_builtin(vm, &in);
        break;
      case GVM_OP_GAMMA:
        rc = op_gamma_builtin(vm, &in);
        break;
      case GVM_OP_LGAMMA:
        rc = op_lgamma_builtin(vm, &in);
        break;
      case GVM_OP_FMA:
        rc = op_fma_builtin(vm, &in);
        break;
      case GVM_OP_FDIM:
        rc = op_fdim_builtin(vm, &in);
        break;
      case GVM_OP_REMAINDER:
        rc = op_remainder_builtin(vm, &in);
        break;
      case GVM_OP_RINT:
        rc = op_rint_builtin(vm, &in);
        break;
      case GVM_OP_EXP:
        rc = op_exp(vm, &in);
        break;
      case GVM_OP_LN:
        rc = op_ln(vm, &in);
        break;
      case GVM_OP_LOG:
        rc = op_log(vm, &in);
        break;
      case GVM_OP_FLOOR:
        rc = op_floor_builtin(vm, &in);
        break;
      case GVM_OP_CEIL:
        rc = op_ceil_builtin(vm, &in);
        break;
      case GVM_OP_ROUND:
        rc = op_round_builtin(vm, &in);
        break;
      case GVM_OP_TRUNC:
        rc = op_trunc_builtin(vm, &in);
        break;
      case GVM_OP_FRACT:
        rc = op_fract_builtin(vm, &in);
        break;
      case GVM_OP_SIGN:
        rc = op_sign_builtin(vm, &in);
        break;
      case GVM_OP_LEN:
        rc = op_len(vm, &in);
        break;
      case GVM_OP_FACTORIAL:
        rc = op_factorial(vm, &in);
        break;
      case GVM_OP_MOV:
        rc = op_mov(vm, &in);
        break;
      case GVM_OP_LOAD_CONST:
        rc = op_load_const(vm, &in);
        break;
      case GVM_OP_LOAD_GLOBAL:
        rc = op_load_global(vm, &in);
        break;
      case GVM_OP_STORE_GLOBAL:
        rc = op_store_global(vm, &in);
        break;
      case GVM_OP_STORE_CONST_GLOBAL:
        rc = op_store_const_global(vm, &in);
        break;
      case GVM_OP_COPY_GLOBAL:
        rc = op_copy_global(vm, &in);
        break;
      case GVM_OP_LIST_NEW:
        rc = op_list_new(vm, &in);
        break;
      case GVM_OP_LIST_APPEND:
        rc = op_list_append(vm, &in);
        break;
      case GVM_OP_LIST_GET:
        rc = op_list_get(vm, &in);
        break;
      case GVM_OP_DICT_NEW:
        rc = op_dict_new(vm, &in);
        break;
      case GVM_OP_DICT_SET:
        rc = op_dict_set(vm, &in);
        break;
      case GVM_OP_DICT_GET:
        rc = op_dict_get(vm, &in);
        break;
      case GVM_OP_DICT_SET_KEY:
        rc = op_dict_set_key(vm, &in);
        break;
      case GVM_OP_TUPLE_NEW:
        rc = op_tuple_new(vm, &in);
        break;
      case GVM_OP_TUPLE_APPEND:
        rc = op_tuple_append(vm, &in);
        break;
      case GVM_OP_PRINT_CONST:
        rc = op_print_const(vm, &in);
        break;
      case GVM_OP_PRINT_GLOBAL:
        rc = op_print_global(vm, &in);
        break;
      case GVM_OP_PRINT_REG:
        rc = op_print_reg(vm, &in);
        break;
      case GVM_OP_PRINT_CONST_PART:
        rc = op_print_const_part(vm, &in);
        break;
      case GVM_OP_PRINT_GLOBAL_PART:
        rc = op_print_global_part(vm, &in);
        break;
      case GVM_OP_PRINT_REG_PART:
        rc = op_print_reg_part(vm, &in);
        break;
      case GVM_OP_PRINT_NEWLINE:
        rc = op_print_newline(vm, &in);
        break;
      case GVM_OP_FRONTIER_CLEAR:
        rc = op_frontier_clear(vm, &in);
        break;
      case GVM_OP_FRONTIER_PUSH:
        rc = op_frontier_push(vm, &in);
        break;
      case GVM_OP_FRONTIER_FILTER_LT_IMM:
        rc = op_frontier_filter_lt_imm(vm, &in);
        break;
      case GVM_OP_FRONTIER_MAP_ADD_IMM:
        rc = op_frontier_map_add_imm(vm, &in);
        break;
      case GVM_OP_FRONTIER_REDUCE_SUM:
        rc = op_frontier_reduce_sum(vm, &in);
        break;
      case GVM_OP_FRONTIER_SWAP:
        rc = op_frontier_swap(vm, &in);
        break;
      case GVM_OP_NEIGHBORS_OF:
        rc = op_neighbors_of(vm, &in);
        break;
      case GVM_OP_NEIGHBORS_EXPAND:
        rc = op_neighbors_expand(vm, &in);
        break;
      case GVM_OP_INCIDENT_OF:
        rc = op_incident_of(vm, &in);
        break;
      case GVM_OP_HYPEREDGE_NODES_OF:
        rc = op_hyperedge_nodes_of(vm, &in);
        break;
      case GVM_OP_NEIGHBOR_WEIGHT_SUM:
        rc = op_neighbor_weight_sum(vm, &in);
        break;
      case GVM_OP_NEIGHBOR_ATTR_SUM:
        rc = op_neighbor_attr_sum(vm, &in);
        break;
      case GVM_OP_BFS_LEVELS:
        rc = op_bfs_levels(vm, &in);
        break;
      case GVM_OP_BFS_LEVEL_COUNT:
        rc = op_bfs_level_count(vm, &in);
        break;
      case GVM_OP_BFS_ORDER:
        rc = op_bfs_order(vm, &in);
        break;
      case GVM_OP_INCIDENT_COUNT:
        rc = op_incident_count(vm, &in);
        break;
      case GVM_OP_HYPEREDGE_SIZE:
        rc = op_hyperedge_size(vm, &in);
        break;
      case GVM_OP_INCIDENT_SUM:
        rc = op_incident_sum(vm, &in);
        break;
      case GVM_OP_HYPEREDGE_NODE_SUM:
        rc = op_hyperedge_node_sum(vm, &in);
        break;
      default:
        return GVM_ERR_UNKNOWN_OPCODE;
    }
    if (rc != 0) {
      return rc;
    }
  }
  return 0;
}

int graphion_vm_run(graphion_vm *vm) {
  int handled = 0;
  int rc;

  if (vm == NULL || vm->program == NULL) {
    return GVM_ERR_INVALID_ARG;
  }

  rc = graphion_vm_try_run_fastpath(vm, &handled);
  if (handled != 0) {
    return rc;
  }

  if (vm->deterministic_mode) {
    return run_dispatch_switch(vm);
  }
  return run_dispatch_switch(vm);
}
