/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/fastpath.h"
#include "vm/internal/core/value.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  const graphion_insn *program;
  size_t program_len;
  size_t program_fingerprint;
  bool arith_only_fastpath;
  bool arith_only_halt_terminated;
  bool weighted_sum_fastpath;
  bool frontier_fastpath;
} graphion_vm_shape_cache_entry;

enum { GRAPHION_VM_SHAPE_CACHE_SIZE = 64 };

static graphion_vm_shape_cache_entry g_shape_cache[GRAPHION_VM_SHAPE_CACHE_SIZE];

static size_t shape_cache_fingerprint(const graphion_insn *program, size_t program_len) {
  const unsigned char *bytes = (const unsigned char *)program;
  size_t hash = (size_t)1469598103934665603ULL;
  size_t i;
  for (i = 0U; i < program_len * sizeof(graphion_insn); ++i) {
    hash ^= (size_t)bytes[i];
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}
static size_t shape_cache_slot(const graphion_insn *program, size_t program_len) {
  uintptr_t p = (uintptr_t)program;
  return (size_t)((p ^ (p >> 7U) ^ (uintptr_t)(program_len * 1315423911U)) &
                  (GRAPHION_VM_SHAPE_CACHE_SIZE - 1U));
}

static int shape_cache_lookup(const graphion_insn *program,
                              size_t program_len,
                              bool *arith_only_fastpath,
                              bool *arith_only_halt_terminated,
                              bool *weighted_sum_fastpath,
                              bool *frontier_fastpath) {
  const size_t slot = shape_cache_slot(program, program_len);
  const graphion_vm_shape_cache_entry e = g_shape_cache[slot];
  const size_t fingerprint = shape_cache_fingerprint(program, program_len);
  if (e.program != program || e.program_len != program_len || e.program_fingerprint != fingerprint) {
    return 0;
  }
  *arith_only_fastpath = e.arith_only_fastpath;
  *arith_only_halt_terminated = e.arith_only_halt_terminated;
  *weighted_sum_fastpath = e.weighted_sum_fastpath;
  *frontier_fastpath = e.frontier_fastpath;
  return 1;
}

static void shape_cache_store(const graphion_insn *program,
                              size_t program_len,
                              bool arith_only_fastpath,
                              bool arith_only_halt_terminated,
                              bool weighted_sum_fastpath,
                              bool frontier_fastpath) {
  const size_t slot = shape_cache_slot(program, program_len);
  g_shape_cache[slot].program = program;
  g_shape_cache[slot].program_len = program_len;
  g_shape_cache[slot].program_fingerprint = shape_cache_fingerprint(program, program_len);
  g_shape_cache[slot].arith_only_fastpath = arith_only_fastpath;
  g_shape_cache[slot].arith_only_halt_terminated = arith_only_halt_terminated;
  g_shape_cache[slot].weighted_sum_fastpath = weighted_sum_fastpath;
  g_shape_cache[slot].frontier_fastpath = frontier_fastpath;
}


static int is_arith_only_fastpath_candidate(const graphion_insn *program,
                                            size_t program_len,
                                            bool *halt_terminated) {
  size_t i;
  bool has_halt = false;
  for (i = 0U; i < program_len; ++i) {
    const graphion_insn in = program[i];
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        has_halt = true;
        break;
      case GVM_OP_MOV_IMM:
        if (!is_valid_reg(in.a)) {
          return 0;
        }
        break;
      case GVM_OP_ADD:
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return 0;
        }
        break;
      case GVM_OP_SUB:
      case GVM_OP_MUL:
      case GVM_OP_DIV:
      case GVM_OP_MOD:
      case GVM_OP_POW:
      case GVM_OP_FLOOR_DIV:
      case GVM_OP_EQ:
      case GVM_OP_NE:
      case GVM_OP_LT:
      case GVM_OP_LE:
      case GVM_OP_GT:
      case GVM_OP_GE:
      case GVM_OP_AND:
      case GVM_OP_OR:
      case GVM_OP_NOT:
      case GVM_OP_NAND:
      case GVM_OP_NOR:
      case GVM_OP_JUMP:
      case GVM_OP_JUMP_IF_TRUE:
      case GVM_OP_JUMP_IF_FALSE:
        return 0;
      case GVM_OP_MOV:
      case GVM_OP_LOAD_CONST:
      case GVM_OP_LOAD_GLOBAL:
      case GVM_OP_STORE_GLOBAL:
      case GVM_OP_STORE_CONST_GLOBAL:
      case GVM_OP_COPY_GLOBAL:
      case GVM_OP_PRINT_CONST:
      case GVM_OP_PRINT_GLOBAL:
      case GVM_OP_FRONTIER_CLEAR:
      case GVM_OP_FRONTIER_FILTER_LT_IMM:
      case GVM_OP_FRONTIER_MAP_ADD_IMM:
      case GVM_OP_FRONTIER_REDUCE_SUM:
      case GVM_OP_FRONTIER_SWAP:
      case GVM_OP_INCIDENT_OF:
      case GVM_OP_HYPEREDGE_NODES_OF:
      case GVM_OP_NEIGHBOR_WEIGHT_SUM:
      case GVM_OP_NEIGHBOR_ATTR_SUM:
      case GVM_OP_BFS_LEVELS:
      case GVM_OP_BFS_LEVEL_COUNT:
      case GVM_OP_BFS_ORDER:
      case GVM_OP_INCIDENT_COUNT:
      case GVM_OP_HYPEREDGE_SIZE:
      case GVM_OP_INCIDENT_SUM:
      case GVM_OP_HYPEREDGE_NODE_SUM:
      case GVM_OP_FRONTIER_PUSH:
        return 0;
      default:
        return 0;
    }
  }
  if (halt_terminated != NULL) {
    *halt_terminated = has_halt;
  }
  return 1;
}

static int is_weighted_sum_fastpath_candidate(const graphion_insn *program, size_t program_len) {
  size_t i;
  int has_weighted = 0;
  for (i = 0U; i < program_len; ++i) {
    const graphion_insn in = program[i];
    switch (in.op) {
      case GVM_OP_NOP:
      case GVM_OP_HALT:
        break;
      case GVM_OP_MOV_IMM:
        if (!is_valid_reg(in.a)) {
          return 0;
        }
        break;
      case GVM_OP_NEIGHBOR_WEIGHT_SUM:
      case GVM_OP_NEIGHBOR_ATTR_SUM:
        has_weighted = 1;
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return 0;
        }
        break;
      default:
        return 0;
    }
  }
  return has_weighted;
}

static int is_frontier_fastpath_candidate(const graphion_insn *program, size_t program_len) {
  size_t i;
  int has_frontier_ops = 0;
  for (i = 0U; i < program_len; ++i) {
    const graphion_insn in = program[i];
    switch (in.op) {
      case GVM_OP_NOP:
      case GVM_OP_HALT:
        break;
      case GVM_OP_FRONTIER_CLEAR:
      case GVM_OP_FRONTIER_FILTER_LT_IMM:
      case GVM_OP_FRONTIER_MAP_ADD_IMM:
      case GVM_OP_FRONTIER_REDUCE_SUM:
      case GVM_OP_FRONTIER_SWAP:
        has_frontier_ops = 1;
        if (!is_valid_reg(in.a)) {
          return 0;
        }
        break;
      case GVM_OP_FRONTIER_PUSH:
        has_frontier_ops = 1;
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return 0;
        }
        break;
      default:
        return 0;
    }
  }
  return has_frontier_ops;
}

int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len) {
  bool halt_terminated = false;
  bool arith_only_fastpath = false;
  bool weighted_sum_fastpath = false;
  bool frontier_fastpath = false;
  if (vm == NULL || program == NULL || program_len == 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  vm->program = program;
  vm->program_len = program_len;
  vm->pc = 0U;
  vm->halted = false;

  if (!shape_cache_lookup(program, program_len, &arith_only_fastpath, &halt_terminated,
                          &weighted_sum_fastpath, &frontier_fastpath)) {
    arith_only_fastpath = is_arith_only_fastpath_candidate(program, program_len, &halt_terminated) != 0;
    weighted_sum_fastpath = is_weighted_sum_fastpath_candidate(program, program_len) != 0;
    frontier_fastpath = is_frontier_fastpath_candidate(program, program_len) != 0;
    shape_cache_store(program, program_len, arith_only_fastpath, halt_terminated,
                      weighted_sum_fastpath, frontier_fastpath);
  }
  vm->arith_only_fastpath = arith_only_fastpath;
  vm->arith_only_halt_terminated = halt_terminated;
  vm->weighted_sum_fastpath = weighted_sum_fastpath;
  vm->frontier_fastpath = frontier_fastpath;
  return 0;
}


