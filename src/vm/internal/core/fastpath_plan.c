/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/fastpath.h"
#include "vm/internal/core/value.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>

typedef struct {
  const graphion_insn *program;
  size_t program_len;
  size_t program_fingerprint;
  bool arith_only_fastpath;
  bool arith_only_halt_terminated;
  bool weighted_sum_fastpath;
  bool frontier_fastpath;
  bool value_move_fastpath;
  bool global_materialize_fastpath;
  bool global_print_fastpath;
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
                              bool *frontier_fastpath,
                              bool *value_move_fastpath,
                              bool *global_materialize_fastpath,
                              bool *global_print_fastpath) {
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
  *value_move_fastpath = e.value_move_fastpath;
  *global_materialize_fastpath = e.global_materialize_fastpath;
  *global_print_fastpath = e.global_print_fastpath;
  return 1;
}

static void shape_cache_store(const graphion_insn *program,
                              size_t program_len,
                              bool arith_only_fastpath,
                              bool arith_only_halt_terminated,
                              bool weighted_sum_fastpath,
                              bool frontier_fastpath,
                              bool value_move_fastpath,
                              bool global_materialize_fastpath,
                              bool global_print_fastpath) {
  const size_t slot = shape_cache_slot(program, program_len);
  g_shape_cache[slot].program = program;
  g_shape_cache[slot].program_len = program_len;
  g_shape_cache[slot].program_fingerprint = shape_cache_fingerprint(program, program_len);
  g_shape_cache[slot].arith_only_fastpath = arith_only_fastpath;
  g_shape_cache[slot].arith_only_halt_terminated = arith_only_halt_terminated;
  g_shape_cache[slot].weighted_sum_fastpath = weighted_sum_fastpath;
  g_shape_cache[slot].frontier_fastpath = frontier_fastpath;
  g_shape_cache[slot].value_move_fastpath = value_move_fastpath;
  g_shape_cache[slot].global_materialize_fastpath = global_materialize_fastpath;
  g_shape_cache[slot].global_print_fastpath = global_print_fastpath;
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

static int is_value_move_fastpath_candidate(const graphion_insn *program, size_t program_len) {
  size_t i;
  int has_value_ops = 0;
  for (i = 0U; i < program_len; ++i) {
    const graphion_insn in = program[i];
    switch (in.op) {
      case GVM_OP_NOP:
      case GVM_OP_HALT:
        break;
      case GVM_OP_ADD:
      case GVM_OP_SUB:
      case GVM_OP_MUL:
      case GVM_OP_DIV:
      case GVM_OP_MOD:
      case GVM_OP_POW:
      case GVM_OP_FLOOR_DIV:
        return 0;
      case GVM_OP_MOV:
        has_value_ops = 1;
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return 0;
        }
        break;
      case GVM_OP_LOAD_CONST:
      case GVM_OP_LOAD_GLOBAL:
      case GVM_OP_STORE_GLOBAL:
      case GVM_OP_PRINT_REG:
        has_value_ops = 1;
        if (!is_valid_reg(in.a)) {
          return 0;
        }
        break;
      case GVM_OP_STORE_CONST_GLOBAL:
      case GVM_OP_COPY_GLOBAL:
        has_value_ops = 1;
        break;
      case GVM_OP_PRINT_CONST:
      case GVM_OP_PRINT_GLOBAL:
        return 0;
      default:
        return 0;
    }
  }
  return has_value_ops;
}

static int is_global_materialize_fastpath_candidate(const graphion_insn *program, size_t program_len) {
  size_t i;
  int has_global_ops = 0;
  for (i = 0U; i < program_len; ++i) {
    const graphion_insn in = program[i];
    switch (in.op) {
      case GVM_OP_NOP:
      case GVM_OP_HALT:
        break;
      case GVM_OP_STORE_CONST_GLOBAL:
      case GVM_OP_COPY_GLOBAL:
        has_global_ops = 1;
        break;
      default:
        return 0;
    }
  }
  return has_global_ops;
}

static int is_global_print_fastpath_candidate(const graphion_insn *program, size_t program_len) {
  size_t i;
  int has_print_ops = 0;
  for (i = 0U; i < program_len; ++i) {
    const graphion_insn in = program[i];
    switch (in.op) {
      case GVM_OP_NOP:
      case GVM_OP_HALT:
        break;
      case GVM_OP_STORE_CONST_GLOBAL:
      case GVM_OP_COPY_GLOBAL:
      case GVM_OP_PRINT_CONST:
      case GVM_OP_PRINT_GLOBAL:
        has_print_ops = 1;
        break;
      default:
        return 0;
    }
  }
  return has_print_ops;
}

static int validate_value_move_program_indices(const graphion_vm *vm) {
  size_t i;
  if (vm == NULL || vm->program == NULL) {
    return 0;
  }
  for (i = 0U; i < vm->program_len; ++i) {
    const graphion_insn in = vm->program[i];
    switch (in.op) {
      case GVM_OP_LOAD_CONST:
        if (vm->const_pool == NULL || in.imm < 0 || (size_t)in.imm >= vm->const_count) {
          return 0;
        }
        break;
      case GVM_OP_STORE_CONST_GLOBAL:
        if ((vm->const_pool == NULL || in.imm < 0 || (size_t)in.imm >= vm->const_count) ||
            (vm->globals == NULL || (size_t)in.b >= vm->global_count)) {
          return 0;
        }
        break;
      case GVM_OP_PRINT_CONST:
        if (vm->const_pool == NULL || in.imm < 0 || (size_t)in.imm >= vm->const_count) {
          return 0;
        }
        break;
      case GVM_OP_PRINT_REG:
        if (!is_valid_reg(in.a)) {
          return 0;
        }
        break;
      case GVM_OP_LOAD_GLOBAL:
      case GVM_OP_STORE_GLOBAL:
        if (vm->globals == NULL || in.imm < 0 || (size_t)in.imm >= vm->global_count) {
          return 0;
        }
        break;
      case GVM_OP_COPY_GLOBAL:
      case GVM_OP_PRINT_GLOBAL:
        if (vm->globals == NULL || in.imm < 0 || (size_t)in.imm >= vm->global_count ||
            (in.op == GVM_OP_COPY_GLOBAL && (size_t)in.b >= vm->global_count)) {
          return 0;
        }
        break;
      default:
        break;
    }
  }
  return 1;
}

static int validate_value_move_program_int_add_safety(const graphion_vm *vm) {
  uint8_t reg_kinds[16];
  uint8_t global_kinds[256];
  size_t global_count;
  size_t i;

  if (vm == NULL || vm->program == NULL || vm->program_len == 0U) {
    return 0;
  }
  if (vm->globals == NULL || vm->const_pool == NULL) {
    return 0;
  }
  global_count = vm->global_count;
  if (global_count > (sizeof(global_kinds) / sizeof(global_kinds[0]))) {
    return 0;
  }

  for (i = 0U; i < 16U; ++i) {
    reg_kinds[i] = GVM_VALUE_INT;
  }
  for (i = 0U; i < global_count; ++i) {
    global_kinds[i] = vm->globals[i].kind;
  }

  for (i = 0U; i < vm->program_len; ++i) {
    const graphion_insn in = vm->program[i];
    switch (in.op) {
      case GVM_OP_NOP:
      case GVM_OP_HALT:
      case GVM_OP_JUMP:
      case GVM_OP_JUMP_IF_TRUE:
      case GVM_OP_JUMP_IF_FALSE:
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
      case GVM_OP_BIT_AND:
      case GVM_OP_BIT_OR:
      case GVM_OP_BIT_XOR:
      case GVM_OP_BIT_NOT:
      case GVM_OP_BIT_SHL:
      case GVM_OP_BIT_SHR:
        if (in.op == GVM_OP_NOT) {
          if (reg_kinds[in.a] != GVM_VALUE_INT && reg_kinds[in.a] != GVM_VALUE_FLOAT &&
              reg_kinds[in.a] != GVM_VALUE_BOOL) {
            return 0;
          }
        } else if (in.op == GVM_OP_BIT_NOT) {
          if (reg_kinds[in.a] != GVM_VALUE_BITS) {
            return 0;
          }
          reg_kinds[in.a] = GVM_VALUE_BITS;
          break;
        } else if (in.op == GVM_OP_BIT_AND) {
          if (reg_kinds[in.a] != GVM_VALUE_BITS || reg_kinds[in.b] != GVM_VALUE_BITS) {
            return 0;
          }
          reg_kinds[in.a] = GVM_VALUE_BITS;
          break;
        } else if (in.op == GVM_OP_BIT_OR) {
          if (reg_kinds[in.a] != GVM_VALUE_BITS || reg_kinds[in.b] != GVM_VALUE_BITS) {
            return 0;
          }
          reg_kinds[in.a] = GVM_VALUE_BITS;
          break;
        } else if (in.op == GVM_OP_BIT_XOR) {
          if (reg_kinds[in.a] != GVM_VALUE_BITS || reg_kinds[in.b] != GVM_VALUE_BITS) {
            return 0;
          }
          reg_kinds[in.a] = GVM_VALUE_BITS;
          break;
        } else if (in.op == GVM_OP_BIT_SHL) {
          if (reg_kinds[in.a] != GVM_VALUE_BITS || reg_kinds[in.b] != GVM_VALUE_INT) {
            return 0;
          }
          reg_kinds[in.a] = GVM_VALUE_BITS;
          break;
        } else if (in.op == GVM_OP_BIT_SHR) {
          if (reg_kinds[in.a] != GVM_VALUE_BITS || reg_kinds[in.b] != GVM_VALUE_INT) {
            return 0;
          }
          reg_kinds[in.a] = GVM_VALUE_BITS;
          break;
        } else {
          if ((reg_kinds[in.a] != GVM_VALUE_INT && reg_kinds[in.a] != GVM_VALUE_FLOAT &&
               reg_kinds[in.a] != GVM_VALUE_BOOL) ||
              (reg_kinds[in.b] != GVM_VALUE_INT && reg_kinds[in.b] != GVM_VALUE_FLOAT &&
               reg_kinds[in.b] != GVM_VALUE_BOOL)) {
            return 0;
          }
        }
        reg_kinds[in.a] = GVM_VALUE_BOOL;
        break;
      case GVM_OP_MOV:
        reg_kinds[in.a] = reg_kinds[in.b];
        break;
      case GVM_OP_LOAD_CONST:
        reg_kinds[in.a] = vm->const_pool[(size_t)in.imm].kind;
        break;
      case GVM_OP_LOAD_GLOBAL:
        reg_kinds[in.a] = global_kinds[(size_t)in.imm];
        break;
      case GVM_OP_STORE_GLOBAL:
        global_kinds[(size_t)in.imm] = reg_kinds[in.a];
        break;
      case GVM_OP_STORE_CONST_GLOBAL:
        global_kinds[(size_t)in.b] = vm->const_pool[(size_t)in.imm].kind;
        break;
      case GVM_OP_COPY_GLOBAL:
        global_kinds[(size_t)in.b] = global_kinds[(size_t)in.imm];
        break;
      case GVM_OP_PRINT_REG:
      case GVM_OP_PRINT_CONST:
      case GVM_OP_PRINT_GLOBAL:
        break;
      case GVM_OP_ADD:
        if ((reg_kinds[in.a] != GVM_VALUE_INT && reg_kinds[in.a] != GVM_VALUE_FLOAT) ||
            (reg_kinds[in.b] != GVM_VALUE_INT && reg_kinds[in.b] != GVM_VALUE_FLOAT)) {
          return 0;
        }
        reg_kinds[in.a] =
            reg_kinds[in.a] == GVM_VALUE_FLOAT || reg_kinds[in.b] == GVM_VALUE_FLOAT ? GVM_VALUE_FLOAT : GVM_VALUE_INT;
        break;
      default:
        return 0;
    }
  }
  return 1;
}

void refresh_value_move_validation(graphion_vm *vm) {
  if (vm == NULL || !vm->value_move_fastpath || vm->program == NULL || vm->program_len == 0U) {
    if (vm != NULL) {
      vm->value_move_indices_valid = false;
      vm->value_move_int_add_safe = false;
    }
    return;
  }
  vm->value_move_indices_valid = validate_value_move_program_indices(vm) != 0;
  vm->value_move_int_add_safe =
      vm->value_move_indices_valid ? (validate_value_move_program_int_add_safety(vm) != 0) : false;
}

void refresh_global_print_validation(graphion_vm *vm) {
  size_t i;
  if (vm == NULL || !vm->global_print_fastpath || vm->program == NULL || vm->program_len == 0U) {
    if (vm != NULL) {
      vm->global_print_indices_valid = false;
    }
    return;
  }
  if (vm->const_pool == NULL || vm->globals == NULL || vm->const_count > 512U || vm->global_count > 256U) {
    vm->global_print_indices_valid = false;
    return;
  }
  if (!validate_value_move_program_indices(vm)) {
    vm->global_print_indices_valid = false;
    return;
  }
  for (i = 0U; i < vm->const_count; ++i) {
    if (vm_value_text_len(&vm->const_pool[i], &vm->global_print_const_lens[i]) != GVM_OK) {
      vm->global_print_indices_valid = false;
      return;
    }
  }
  for (i = 0U; i < vm->global_count; ++i) {
    vm->global_print_global_lens[i] = 0U;
  }
  vm->global_print_indices_valid = true;
}

int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len) {
  bool halt_terminated = false;
  bool arith_only_fastpath = false;
  bool weighted_sum_fastpath = false;
  bool frontier_fastpath = false;
  bool value_move_fastpath = false;
  bool global_materialize_fastpath = false;
  bool global_print_fastpath = false;
  if (vm == NULL || program == NULL || program_len == 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  vm->program = program;
  vm->program_len = program_len;
  vm->pc = 0U;
  vm->halted = false;

  if (!shape_cache_lookup(program, program_len, &arith_only_fastpath, &halt_terminated,
                          &weighted_sum_fastpath, &frontier_fastpath, &value_move_fastpath,
                          &global_materialize_fastpath, &global_print_fastpath)) {
    arith_only_fastpath = is_arith_only_fastpath_candidate(program, program_len, &halt_terminated) != 0;
    weighted_sum_fastpath = is_weighted_sum_fastpath_candidate(program, program_len) != 0;
    frontier_fastpath = is_frontier_fastpath_candidate(program, program_len) != 0;
    value_move_fastpath = is_value_move_fastpath_candidate(program, program_len) != 0;
    global_materialize_fastpath =
        is_global_materialize_fastpath_candidate(program, program_len) != 0;
    global_print_fastpath = is_global_print_fastpath_candidate(program, program_len) != 0;
    shape_cache_store(program, program_len, arith_only_fastpath, halt_terminated,
                      weighted_sum_fastpath, frontier_fastpath, value_move_fastpath,
                      global_materialize_fastpath, global_print_fastpath);
  }
  if (vm->global_string_owners != NULL) {
    value_move_fastpath = false;
    global_materialize_fastpath = false;
    global_print_fastpath = false;
  }
  vm->arith_only_fastpath = arith_only_fastpath;
  vm->arith_only_halt_terminated = halt_terminated;
  vm->weighted_sum_fastpath = weighted_sum_fastpath;
  vm->frontier_fastpath = frontier_fastpath;
  vm->value_move_fastpath = value_move_fastpath;
  vm->global_materialize_fastpath = global_materialize_fastpath;
  vm->global_print_fastpath = global_print_fastpath;
  refresh_value_move_validation(vm);
  refresh_global_print_validation(vm);
  return 0;
}


