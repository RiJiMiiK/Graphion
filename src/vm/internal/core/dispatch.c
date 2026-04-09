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
      case GVM_OP_DEGREES:
        rc = op_degrees_builtin(vm, &in);
        break;
      case GVM_OP_RADIANS:
        rc = op_radians_builtin(vm, &in);
        break;
      case GVM_OP_ISNAN:
        rc = op_isnan_builtin(vm, &in);
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

#if defined(GRAPHION_VM_DISPATCH_JUMPTABLE)
static int run_dispatch_jumptable(graphion_vm *vm) {
  typedef int (*handler_fn)(graphion_vm *, const graphion_insn *);
  static const handler_fn table[256] = {
      [GVM_OP_NOP] = op_nop,
      [GVM_OP_HALT] = op_halt,
      [GVM_OP_MOV_IMM] = op_mov_imm,
      [GVM_OP_ADD] = op_add,
      [GVM_OP_SUB] = op_sub,
      [GVM_OP_MUL] = op_mul,
      [GVM_OP_DIV] = op_div,
      [GVM_OP_MOD] = op_mod,
      [GVM_OP_POW] = op_pow,
      [GVM_OP_FLOOR_DIV] = op_floor_div,
      [GVM_OP_EQ] = op_eq_cmp,
      [GVM_OP_NE] = op_ne_cmp,
      [GVM_OP_LT] = op_lt_cmp,
      [GVM_OP_LE] = op_le_cmp,
      [GVM_OP_GT] = op_gt_cmp,
      [GVM_OP_GE] = op_ge_cmp,
      [GVM_OP_BIT_AND] = op_bit_and_cmp,
      [GVM_OP_BIT_OR] = op_bit_or_cmp,
      [GVM_OP_BIT_XOR] = op_bit_xor_cmp,
      [GVM_OP_BIT_NOT] = op_bit_not_cmp,
      [GVM_OP_BIT_SHL] = op_bit_shl_cmp,
      [GVM_OP_BIT_SHR] = op_bit_shr_cmp,
      [GVM_OP_AND] = op_and_cmp,
      [GVM_OP_OR] = op_or_cmp,
      [GVM_OP_NOT] = op_not_cmp,
      [GVM_OP_NAND] = op_nand_cmp,
      [GVM_OP_NOR] = op_nor_cmp,
      [GVM_OP_JUMP] = op_jump,
      [GVM_OP_JUMP_IF_TRUE] = op_jump_if_true,
      [GVM_OP_JUMP_IF_FALSE] = op_jump_if_false,
      [GVM_OP_ABS] = op_abs,
      [GVM_OP_MIN] = op_min,
      [GVM_OP_MAX] = op_max,
      [GVM_OP_CLAMP] = op_clamp,
      [GVM_OP_SQRT] = op_sqrt,
      [GVM_OP_CBRT] = op_cbrt_builtin,
      [GVM_OP_SIN] = op_sin_builtin,
      [GVM_OP_SINH] = op_sinh_builtin,
      [GVM_OP_ASINH] = op_asinh_builtin,
      [GVM_OP_ACOSH] = op_acosh_builtin,
      [GVM_OP_COSH] = op_cosh_builtin,
      [GVM_OP_TANH] = op_tanh_builtin,
      [GVM_OP_ATANH] = op_atanh_builtin,
      [GVM_OP_COS] = op_cos_builtin,
      [GVM_OP_TAN] = op_tan_builtin,
      [GVM_OP_ASIN] = op_asin_builtin,
      [GVM_OP_ACOS] = op_acos_builtin,
      [GVM_OP_ATAN] = op_atan_builtin,
      [GVM_OP_ATAN2] = op_atan2_builtin,
      [GVM_OP_HYPOT] = op_hypot_builtin,
      [GVM_OP_EXP] = op_exp,
      [GVM_OP_LN] = op_ln,
      [GVM_OP_LOG] = op_log,
      [GVM_OP_FLOOR] = op_floor_builtin,
      [GVM_OP_CEIL] = op_ceil_builtin,
      [GVM_OP_ROUND] = op_round_builtin,
      [GVM_OP_TRUNC] = op_trunc_builtin,
      [GVM_OP_SIGN] = op_sign_builtin,
      [GVM_OP_LEN] = op_len,
      [GVM_OP_FACTORIAL] = op_factorial,
      [GVM_OP_MOV] = op_mov,
      [GVM_OP_LOAD_CONST] = op_load_const,
      [GVM_OP_LOAD_GLOBAL] = op_load_global,
      [GVM_OP_STORE_GLOBAL] = op_store_global,
      [GVM_OP_STORE_CONST_GLOBAL] = op_store_const_global,
      [GVM_OP_COPY_GLOBAL] = op_copy_global,
      [GVM_OP_PRINT_CONST] = op_print_const,
      [GVM_OP_PRINT_GLOBAL] = op_print_global,
      [GVM_OP_PRINT_REG] = op_print_reg,
      [GVM_OP_PRINT_CONST_PART] = op_print_const_part,
      [GVM_OP_PRINT_GLOBAL_PART] = op_print_global_part,
      [GVM_OP_PRINT_REG_PART] = op_print_reg_part,
      [GVM_OP_PRINT_NEWLINE] = op_print_newline,
      [GVM_OP_FRONTIER_CLEAR] = op_frontier_clear,
      [GVM_OP_FRONTIER_PUSH] = op_frontier_push,
      [GVM_OP_FRONTIER_FILTER_LT_IMM] = op_frontier_filter_lt_imm,
      [GVM_OP_FRONTIER_MAP_ADD_IMM] = op_frontier_map_add_imm,
      [GVM_OP_FRONTIER_REDUCE_SUM] = op_frontier_reduce_sum,
      [GVM_OP_FRONTIER_SWAP] = op_frontier_swap,
      [GVM_OP_NEIGHBORS_OF] = op_neighbors_of,
      [GVM_OP_NEIGHBORS_EXPAND] = op_neighbors_expand,
      [GVM_OP_INCIDENT_OF] = op_incident_of,
      [GVM_OP_HYPEREDGE_NODES_OF] = op_hyperedge_nodes_of,
      [GVM_OP_NEIGHBOR_WEIGHT_SUM] = op_neighbor_weight_sum,
      [GVM_OP_NEIGHBOR_ATTR_SUM] = op_neighbor_attr_sum,
      [GVM_OP_BFS_LEVELS] = op_bfs_levels,
      [GVM_OP_BFS_LEVEL_COUNT] = op_bfs_level_count,
      [GVM_OP_BFS_ORDER] = op_bfs_order,
      [GVM_OP_INCIDENT_COUNT] = op_incident_count,
      [GVM_OP_HYPEREDGE_SIZE] = op_hyperedge_size,
      [GVM_OP_INCIDENT_SUM] = op_incident_sum,
      [GVM_OP_HYPEREDGE_NODE_SUM] = op_hyperedge_node_sum,
  };

  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc++];
    const handler_fn fn = table[in.op];
    int rc;
    if (fn == NULL) {
      return GVM_ERR_UNKNOWN_OPCODE;
    }
    rc = fn(vm, &in);
    if (rc != 0) {
      return rc;
    }
  }
  return 0;
}
#endif

#if defined(GRAPHION_VM_DISPATCH_COMPUTED_GOTO) && !defined(_MSC_VER)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
static int run_dispatch_computed_goto(graphion_vm *vm) {
  static void *dispatch[256] = {
      [GVM_OP_NOP] = &&L_nop,
      [GVM_OP_HALT] = &&L_halt,
      [GVM_OP_MOV_IMM] = &&L_mov_imm,
      [GVM_OP_ADD] = &&L_add,
      [GVM_OP_SUB] = &&L_sub,
      [GVM_OP_MUL] = &&L_mul,
      [GVM_OP_DIV] = &&L_div,
      [GVM_OP_MOD] = &&L_mod,
      [GVM_OP_POW] = &&L_pow,
      [GVM_OP_FLOOR_DIV] = &&L_floor_div,
      [GVM_OP_EQ] = &&L_eq,
      [GVM_OP_NE] = &&L_ne,
      [GVM_OP_LT] = &&L_lt,
      [GVM_OP_LE] = &&L_le,
      [GVM_OP_GT] = &&L_gt,
      [GVM_OP_GE] = &&L_ge,
      [GVM_OP_BIT_AND] = &&L_bit_and,
      [GVM_OP_BIT_OR] = &&L_bit_or,
      [GVM_OP_BIT_XOR] = &&L_bit_xor,
      [GVM_OP_BIT_NOT] = &&L_bit_not,
      [GVM_OP_BIT_SHL] = &&L_bit_shl,
      [GVM_OP_BIT_SHR] = &&L_bit_shr,
      [GVM_OP_AND] = &&L_and,
      [GVM_OP_OR] = &&L_or,
      [GVM_OP_NOT] = &&L_not,
      [GVM_OP_NAND] = &&L_nand,
      [GVM_OP_NOR] = &&L_nor,
      [GVM_OP_JUMP] = &&L_jump,
      [GVM_OP_JUMP_IF_TRUE] = &&L_jump_if_true,
      [GVM_OP_JUMP_IF_FALSE] = &&L_jump_if_false,
      [GVM_OP_ABS] = &&L_abs,
      [GVM_OP_MIN] = &&L_min,
      [GVM_OP_MAX] = &&L_max,
      [GVM_OP_CLAMP] = &&L_clamp,
      [GVM_OP_SQRT] = &&L_sqrt,
      [GVM_OP_CBRT] = &&L_cbrt_builtin,
      [GVM_OP_SIN] = &&L_sin_builtin,
      [GVM_OP_SINH] = &&L_sinh_builtin,
      [GVM_OP_ASINH] = &&L_asinh_builtin,
      [GVM_OP_ACOSH] = &&L_acosh_builtin,
      [GVM_OP_COSH] = &&L_cosh_builtin,
      [GVM_OP_TANH] = &&L_tanh_builtin,
      [GVM_OP_ATANH] = &&L_atanh_builtin,
      [GVM_OP_COS] = &&L_cos_builtin,
      [GVM_OP_TAN] = &&L_tan_builtin,
      [GVM_OP_ASIN] = &&L_asin_builtin,
      [GVM_OP_ACOS] = &&L_acos_builtin,
      [GVM_OP_ATAN] = &&L_atan_builtin,
      [GVM_OP_ATAN2] = &&L_atan2_builtin,
      [GVM_OP_HYPOT] = &&L_hypot_builtin,
      [GVM_OP_EXP] = &&L_exp,
      [GVM_OP_LN] = &&L_ln,
      [GVM_OP_LOG] = &&L_log,
      [GVM_OP_FLOOR] = &&L_floor_builtin,
      [GVM_OP_CEIL] = &&L_ceil_builtin,
      [GVM_OP_ROUND] = &&L_round_builtin,
      [GVM_OP_TRUNC] = &&L_trunc_builtin,
      [GVM_OP_SIGN] = &&L_sign_builtin,
      [GVM_OP_LEN] = &&L_len,
      [GVM_OP_FACTORIAL] = &&L_factorial,
      [GVM_OP_MOV] = &&L_mov,
      [GVM_OP_LOAD_CONST] = &&L_load_const,
      [GVM_OP_LOAD_GLOBAL] = &&L_load_global,
      [GVM_OP_STORE_GLOBAL] = &&L_store_global,
      [GVM_OP_STORE_CONST_GLOBAL] = &&L_store_const_global,
      [GVM_OP_COPY_GLOBAL] = &&L_copy_global,
      [GVM_OP_PRINT_CONST] = &&L_print_const,
      [GVM_OP_PRINT_GLOBAL] = &&L_print_global,
      [GVM_OP_PRINT_REG] = &&L_print_reg,
      [GVM_OP_PRINT_CONST_PART] = &&L_print_const_part,
      [GVM_OP_PRINT_GLOBAL_PART] = &&L_print_global_part,
      [GVM_OP_PRINT_REG_PART] = &&L_print_reg_part,
      [GVM_OP_PRINT_NEWLINE] = &&L_print_newline,
      [GVM_OP_FRONTIER_CLEAR] = &&L_frontier_clear,
      [GVM_OP_FRONTIER_PUSH] = &&L_frontier_push,
      [GVM_OP_FRONTIER_FILTER_LT_IMM] = &&L_frontier_filter_lt_imm,
      [GVM_OP_FRONTIER_MAP_ADD_IMM] = &&L_frontier_map_add_imm,
      [GVM_OP_FRONTIER_REDUCE_SUM] = &&L_frontier_reduce_sum,
      [GVM_OP_FRONTIER_SWAP] = &&L_frontier_swap,
      [GVM_OP_NEIGHBORS_OF] = &&L_neighbors_of,
      [GVM_OP_NEIGHBORS_EXPAND] = &&L_neighbors_expand,
      [GVM_OP_INCIDENT_OF] = &&L_incident_of,
      [GVM_OP_HYPEREDGE_NODES_OF] = &&L_hyperedge_nodes_of,
      [GVM_OP_NEIGHBOR_WEIGHT_SUM] = &&L_neighbor_weight_sum,
      [GVM_OP_NEIGHBOR_ATTR_SUM] = &&L_neighbor_attr_sum,
      [GVM_OP_BFS_LEVELS] = &&L_bfs_levels,
      [GVM_OP_BFS_LEVEL_COUNT] = &&L_bfs_level_count,
      [GVM_OP_BFS_ORDER] = &&L_bfs_order,
      [GVM_OP_INCIDENT_COUNT] = &&L_incident_count,
      [GVM_OP_HYPEREDGE_SIZE] = &&L_hyperedge_size,
      [GVM_OP_INCIDENT_SUM] = &&L_incident_sum,
      [GVM_OP_HYPEREDGE_NODE_SUM] = &&L_hyperedge_node_sum,
  };

  while (!vm->halted && vm->pc < vm->program_len) {
    const graphion_insn in = vm->program[vm->pc++];
    int rc;
    if (dispatch[in.op] == NULL) {
      return GVM_ERR_UNKNOWN_OPCODE;
    }
    goto *dispatch[in.op];
L_nop:
    rc = op_nop(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_halt:
    rc = op_halt(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_mov_imm:
    rc = op_mov_imm(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_add:
    rc = op_add(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_sub:
    rc = op_sub(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_mul:
    rc = op_mul(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_div:
    rc = op_div(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_mod:
    rc = op_mod(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_pow:
    rc = op_pow(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_floor_div:
    rc = op_floor_div(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_eq:
    rc = op_eq_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_ne:
    rc = op_ne_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_lt:
    rc = op_lt_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_le:
    rc = op_le_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_gt:
    rc = op_gt_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_ge:
    rc = op_ge_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bit_and:
    rc = op_bit_and_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bit_or:
    rc = op_bit_or_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bit_xor:
    rc = op_bit_xor_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bit_not:
    rc = op_bit_not_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bit_shl:
    rc = op_bit_shl_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bit_shr:
    rc = op_bit_shr_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_and:
    rc = op_and_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_or:
    rc = op_or_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_not:
    rc = op_not_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_nand:
    rc = op_nand_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_nor:
    rc = op_nor_cmp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_jump:
    rc = op_jump(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_jump_if_true:
    rc = op_jump_if_true(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_jump_if_false:
    rc = op_jump_if_false(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_abs:
    rc = op_abs(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_min:
    rc = op_min(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_max:
    rc = op_max(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_clamp:
    rc = op_clamp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_sqrt:
    rc = op_sqrt(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_cbrt_builtin:
    rc = op_cbrt_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_sin_builtin:
    rc = op_sin_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_sinh_builtin:
    rc = op_sinh_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_asinh_builtin:
    rc = op_asinh_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_acosh_builtin:
    rc = op_acosh_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_cosh_builtin:
    rc = op_cosh_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_tanh_builtin:
    rc = op_tanh_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_atanh_builtin:
    rc = op_atanh_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_cos_builtin:
    rc = op_cos_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_tan_builtin:
    rc = op_tan_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_asin_builtin:
    rc = op_asin_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_acos_builtin:
    rc = op_acos_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_atan_builtin:
    rc = op_atan_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_atan2_builtin:
    rc = op_atan2_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_hypot_builtin:
    rc = op_hypot_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_exp:
    rc = op_exp(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_ln:
    rc = op_ln(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_log:
    rc = op_log(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_floor_builtin:
    rc = op_floor_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_ceil_builtin:
    rc = op_ceil_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_round_builtin:
    rc = op_round_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_trunc_builtin:
    rc = op_trunc_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_sign_builtin:
    rc = op_sign_builtin(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  L_len:
    rc = op_len(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_factorial:
    rc = op_factorial(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_mov:
    rc = op_mov(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_load_const:
    rc = op_load_const(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_load_global:
    rc = op_load_global(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_store_global:
    rc = op_store_global(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_store_const_global:
    rc = op_store_const_global(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_copy_global:
    rc = op_copy_global(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_print_const:
    rc = op_print_const(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_print_global:
    rc = op_print_global(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_print_reg:
    rc = op_print_reg(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_print_const_part:
    rc = op_print_const_part(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_print_global_part:
    rc = op_print_global_part(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_print_reg_part:
    rc = op_print_reg_part(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_print_newline:
    rc = op_print_newline(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_frontier_clear:
    rc = op_frontier_clear(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_frontier_push:
    rc = op_frontier_push(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_frontier_filter_lt_imm:
    rc = op_frontier_filter_lt_imm(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_frontier_map_add_imm:
    rc = op_frontier_map_add_imm(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_frontier_reduce_sum:
    rc = op_frontier_reduce_sum(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_frontier_swap:
    rc = op_frontier_swap(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_neighbors_of:
    rc = op_neighbors_of(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_neighbors_expand:
    rc = op_neighbors_expand(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_incident_of:
    rc = op_incident_of(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_hyperedge_nodes_of:
    rc = op_hyperedge_nodes_of(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_neighbor_weight_sum:
    rc = op_neighbor_weight_sum(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_neighbor_attr_sum:
    rc = op_neighbor_attr_sum(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bfs_levels:
    rc = op_bfs_levels(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bfs_level_count:
    rc = op_bfs_level_count(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_bfs_order:
    rc = op_bfs_order(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_incident_count:
    rc = op_incident_count(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_hyperedge_size:
    rc = op_hyperedge_size(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_incident_sum:
    rc = op_incident_sum(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
L_hyperedge_node_sum:
    rc = op_hyperedge_node_sum(vm, &in);
    if (rc != 0) {
      return rc;
    }
    continue;
  }
  return 0;
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

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

#if defined(GRAPHION_VM_DISPATCH_COMPUTED_GOTO) && !defined(_MSC_VER)
  return run_dispatch_computed_goto(vm);
#elif defined(GRAPHION_VM_DISPATCH_JUMPTABLE)
  return run_dispatch_jumptable(vm);
#else
  return run_dispatch_switch(vm);
#endif
}
