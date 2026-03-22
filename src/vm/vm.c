/* SPDX-License-Identifier: MIT */

#include "vm/vm.h"

#include <stdarg.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#if defined(_M_X64) || defined(__x86_64__) || defined(__SSE2__)
#include <immintrin.h>
#define GRAPHION_SSE2_SUMS 1
#endif

#if defined(__AVX2__)
#define GRAPHION_AVX2_SUMS 1
#endif

typedef struct {
  const graphion_insn *program;
  size_t program_len;
  size_t program_fingerprint;
  bool arith_only_fastpath;
  bool arith_only_halt_terminated;
  bool weighted_sum_fastpath;
  bool value_move_fastpath;
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
                              bool *value_move_fastpath) {
  const size_t slot = shape_cache_slot(program, program_len);
  const graphion_vm_shape_cache_entry e = g_shape_cache[slot];
  const size_t fingerprint = shape_cache_fingerprint(program, program_len);
  if (e.program != program || e.program_len != program_len || e.program_fingerprint != fingerprint) {
    return 0;
  }
  *arith_only_fastpath = e.arith_only_fastpath;
  *arith_only_halt_terminated = e.arith_only_halt_terminated;
  *weighted_sum_fastpath = e.weighted_sum_fastpath;
  *value_move_fastpath = e.value_move_fastpath;
  return 1;
}

static void shape_cache_store(const graphion_insn *program,
                              size_t program_len,
                              bool arith_only_fastpath,
                              bool arith_only_halt_terminated,
                              bool weighted_sum_fastpath,
                              bool value_move_fastpath) {
  const size_t slot = shape_cache_slot(program, program_len);
  g_shape_cache[slot].program = program;
  g_shape_cache[slot].program_len = program_len;
  g_shape_cache[slot].program_fingerprint = shape_cache_fingerprint(program, program_len);
  g_shape_cache[slot].arith_only_fastpath = arith_only_fastpath;
  g_shape_cache[slot].arith_only_halt_terminated = arith_only_halt_terminated;
  g_shape_cache[slot].weighted_sum_fastpath = weighted_sum_fastpath;
  g_shape_cache[slot].value_move_fastpath = value_move_fastpath;
}

static int is_valid_reg(uint8_t reg) { return reg < 16U ? 1 : 0; }

static int64_t wrap_add_i64(int64_t lhs, int64_t rhs) {
  const uint64_t ulhs = (uint64_t)lhs;
  const uint64_t urhs = (uint64_t)rhs;
  return (int64_t)(ulhs + urhs);
}

static void vm_value_set_int(graphion_vm_value *value, int64_t int_value) {
  if (value == NULL) {
    return;
  }
  value->kind = GVM_VALUE_INT;
  value->as.int_value = int_value;
}

static void vm_value_copy(graphion_vm_value *dst, const graphion_vm_value *src) {
  if (dst == NULL || src == NULL) {
    return;
  }
  *dst = *src;
}

static int vm_value_get_int(const graphion_vm_value *value, int64_t *out_value) {
  if (value == NULL || out_value == NULL || value->kind != GVM_VALUE_INT) {
    return 0;
  }
  *out_value = value->as.int_value;
  return 1;
}

static int vm_reg_get_int(const graphion_vm *vm, uint8_t reg, int64_t *out_value) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return 0;
  }
  return vm_value_get_int(&vm->regs[reg], out_value);
}

static void vm_reg_set_int(graphion_vm *vm, uint8_t reg, int64_t value) {
  if (vm == NULL || !is_valid_reg(reg)) {
    return;
  }
  vm_value_set_int(&vm->regs[reg], value);
}

static int vm_copy_regs_to_raw_i64(const graphion_vm *vm, int64_t raw_regs[16]) {
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

static void vm_copy_raw_i64_to_regs(graphion_vm *vm, const int64_t raw_regs[16]) {
  size_t i;
  if (vm == NULL || raw_regs == NULL) {
    return;
  }
  for (i = 0U; i < 16U; ++i) {
    vm_reg_set_int(vm, (uint8_t)i, raw_regs[i]);
  }
}

#define REG_I(vm_, reg_) ((vm_)->regs[(reg_)].as.int_value)
#define SET_REG_I(vm_, reg_, value_) vm_reg_set_int((vm_), (reg_), (value_))

static uint64_t sum_weight_slice_wrap(const int64_t *values, size_t count) {
#if defined(GRAPHION_AVX2_SUMS)
  __m256i acc0 = _mm256_setzero_si256();
  __m256i acc1 = _mm256_setzero_si256();
  uint64_t lanes[4] = {0U, 0U, 0U, 0U};
  size_t i = 0U;

  for (; i + 8U <= count; i += 8U) {
    const __m256i chunk0 = _mm256_loadu_si256((const __m256i *)(const void *)(values + i));
    const __m256i chunk1 = _mm256_loadu_si256((const __m256i *)(const void *)(values + i + 4U));
    acc0 = _mm256_add_epi64(acc0, chunk0);
    acc1 = _mm256_add_epi64(acc1, chunk1);
  }
  acc0 = _mm256_add_epi64(acc0, acc1);
  for (; i + 4U <= count; i += 4U) {
    const __m256i chunk = _mm256_loadu_si256((const __m256i *)(const void *)(values + i));
    acc0 = _mm256_add_epi64(acc0, chunk);
  }
  _mm256_storeu_si256((__m256i *)(void *)lanes, acc0);
  {
    uint64_t sum = (lanes[0] + lanes[1]) + (lanes[2] + lanes[3]);
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#elif defined(GRAPHION_SSE2_SUMS)
  __m128i acc = _mm_setzero_si128();
  uint64_t lanes[2] = {0U, 0U};
  size_t i = 0U;

  for (; i + 2U <= count; i += 2U) {
    const __m128i chunk = _mm_loadu_si128((const __m128i *)(const void *)(values + i));
    acc = _mm_add_epi64(acc, chunk);
  }
  _mm_storeu_si128((__m128i *)(void *)lanes, acc);
  {
    uint64_t sum = lanes[0] + lanes[1];
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#else
  uint64_t sum0 = 0U;
  uint64_t sum1 = 0U;
  uint64_t sum2 = 0U;
  uint64_t sum3 = 0U;
  size_t i = 0U;

  for (; i + 4U <= count; i += 4U) {
    sum0 += (uint64_t)values[i];
    sum1 += (uint64_t)values[i + 1U];
    sum2 += (uint64_t)values[i + 2U];
    sum3 += (uint64_t)values[i + 3U];
  }
  for (; i < count; ++i) {
    sum0 += (uint64_t)values[i];
  }
  return (sum0 + sum1) + (sum2 + sum3);
#endif
}

static uint64_t sum_attr_slice_wrap(const uint32_t *values, size_t count) {
#if defined(GRAPHION_AVX2_SUMS)
  __m256i acc0 = _mm256_setzero_si256();
  __m256i acc1 = _mm256_setzero_si256();
  uint64_t lanes[4] = {0U, 0U, 0U, 0U};
  size_t i = 0U;

  for (; i + 8U <= count; i += 8U) {
    const __m256i chunk = _mm256_loadu_si256((const __m256i *)(const void *)(values + i));
    const __m128i lo = _mm256_castsi256_si128(chunk);
    const __m128i hi = _mm256_extracti128_si256(chunk, 1);
    acc0 = _mm256_add_epi64(acc0, _mm256_cvtepu32_epi64(lo));
    acc1 = _mm256_add_epi64(acc1, _mm256_cvtepu32_epi64(hi));
  }
  acc0 = _mm256_add_epi64(acc0, acc1);
  for (; i + 4U <= count; i += 4U) {
    const __m128i chunk = _mm_loadu_si128((const __m128i *)(const void *)(values + i));
    acc0 = _mm256_add_epi64(acc0, _mm256_cvtepu32_epi64(chunk));
  }
  _mm256_storeu_si256((__m256i *)(void *)lanes, acc0);
  {
    uint64_t sum = (lanes[0] + lanes[1]) + (lanes[2] + lanes[3]);
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#elif defined(GRAPHION_SSE2_SUMS)
  const __m128i zero = _mm_setzero_si128();
  __m128i acc_lo = _mm_setzero_si128();
  __m128i acc_hi = _mm_setzero_si128();
  uint64_t lanes_lo[2] = {0U, 0U};
  uint64_t lanes_hi[2] = {0U, 0U};
  size_t i = 0U;

  for (; i + 4U <= count; i += 4U) {
    const __m128i chunk = _mm_loadu_si128((const __m128i *)(const void *)(values + i));
    acc_lo = _mm_add_epi64(acc_lo, _mm_unpacklo_epi32(chunk, zero));
    acc_hi = _mm_add_epi64(acc_hi, _mm_unpackhi_epi32(chunk, zero));
  }
  _mm_storeu_si128((__m128i *)(void *)lanes_lo, acc_lo);
  _mm_storeu_si128((__m128i *)(void *)lanes_hi, acc_hi);
  {
    uint64_t sum = (lanes_lo[0] + lanes_lo[1]) + (lanes_hi[0] + lanes_hi[1]);
    for (; i < count; ++i) {
      sum += (uint64_t)values[i];
    }
    return sum;
  }
#else
  uint64_t sum0 = 0U;
  uint64_t sum1 = 0U;
  uint64_t sum2 = 0U;
  uint64_t sum3 = 0U;
  size_t i = 0U;

  for (; i + 4U <= count; i += 4U) {
    sum0 += (uint64_t)values[i];
    sum1 += (uint64_t)values[i + 1U];
    sum2 += (uint64_t)values[i + 2U];
    sum3 += (uint64_t)values[i + 3U];
  }
  for (; i < count; ++i) {
    sum0 += (uint64_t)values[i];
  }
  return (sum0 + sum1) + (sum2 + sum3);
#endif
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
      case GVM_OP_MOV:
      case GVM_OP_LOAD_CONST:
      case GVM_OP_LOAD_GLOBAL:
      case GVM_OP_STORE_GLOBAL:
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
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return 0;
        }
        break;
      case GVM_OP_MOV:
        has_value_ops = 1;
        if (!is_valid_reg(in.a) || !is_valid_reg(in.b)) {
          return 0;
        }
        break;
      case GVM_OP_LOAD_CONST:
      case GVM_OP_LOAD_GLOBAL:
      case GVM_OP_STORE_GLOBAL:
        has_value_ops = 1;
        if (!is_valid_reg(in.a)) {
          return 0;
        }
        break;
      default:
        return 0;
    }
  }
  return has_value_ops;
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
      case GVM_OP_LOAD_GLOBAL:
      case GVM_OP_STORE_GLOBAL:
        if (vm->globals == NULL || in.imm < 0 || (size_t)in.imm >= vm->global_count) {
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
      case GVM_OP_ADD:
        if (reg_kinds[in.a] != GVM_VALUE_INT || reg_kinds[in.b] != GVM_VALUE_INT) {
          return 0;
        }
        reg_kinds[in.a] = GVM_VALUE_INT;
        break;
      default:
        return 0;
    }
  }
  return 1;
}

static void refresh_value_move_validation(graphion_vm *vm) {
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

static void run_arith_fastpath_c_halt_terminated(graphion_vm *vm) {
  const graphion_insn *p = vm->program + vm->pc;
  const graphion_insn *const end = vm->program + vm->program_len;
  int64_t regs[16];

  if (!vm_copy_regs_to_raw_i64(vm, regs)) {
    return;
  }

  for (;;) {
    const graphion_insn in = *p++;
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        vm_copy_raw_i64_to_regs(vm, regs);
        return;
      case GVM_OP_MOV_IMM:
        regs[in.a] = (int64_t)in.imm;
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.b == in.a) {
            regs[next.a] = wrap_add_i64(regs[next.a], regs[in.a]);
            p++;
          }
        }
        break;
      case GVM_OP_ADD:
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.a == in.a && in.b != in.a && next.b != in.a) {
            regs[in.a] = wrap_add_i64(regs[in.a], wrap_add_i64(regs[in.b], regs[next.b]));
            p++;
            break;
          }
        }
        regs[in.a] = wrap_add_i64(regs[in.a], regs[in.b]);
        break;
      default:
        vm->pc = (size_t)(p - vm->program);
        vm_copy_raw_i64_to_regs(vm, regs);
        return;
    }
  }
}

#if !(defined(GRAPHION_ENABLE_ASM) && !defined(_MSC_VER))
static void run_arith_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p = vm->program + vm->pc;
  const graphion_insn *const end = vm->program + vm->program_len;
  int64_t regs[16];

  if (!vm_copy_regs_to_raw_i64(vm, regs)) {
    return;
  }

  while (p < end) {
    const graphion_insn in = *p++;
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        vm_copy_raw_i64_to_regs(vm, regs);
        return;
      case GVM_OP_MOV_IMM:
        regs[in.a] = (int64_t)in.imm;
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.b == in.a) {
            regs[next.a] = wrap_add_i64(regs[next.a], regs[in.a]);
            p++;
          }
        }
        break;
      case GVM_OP_ADD:
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.a == in.a && in.b != in.a && next.b != in.a) {
            regs[in.a] = wrap_add_i64(regs[in.a], wrap_add_i64(regs[in.b], regs[next.b]));
            p++;
            break;
          }
        }
        regs[in.a] = wrap_add_i64(regs[in.a], regs[in.b]);
        break;
      default:
        vm->pc = (size_t)(p - vm->program);
        vm_copy_raw_i64_to_regs(vm, regs);
        return;
    }
  }
  vm->pc = vm->program_len;
  vm_copy_raw_i64_to_regs(vm, regs);
}
#endif

static int run_weighted_sum_fastpath_c(graphion_vm *vm) {
  const graphion_csr_graph *graph = vm->csr_graph;
  const graphion_insn *p = vm->program + vm->pc;
  const graphion_insn *const end = vm->program + vm->program_len;
  int64_t regs[16];

  if (graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_copy_regs_to_raw_i64(vm, regs)) {
    return GVM_ERR_TYPE_MISMATCH;
  }

  while (p < end) {
    const graphion_insn in = *p++;
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        vm_copy_raw_i64_to_regs(vm, regs);
        return GVM_OK;
      case GVM_OP_MOV_IMM:
        if (p < end && p->a == in.a &&
            (p->op == GVM_OP_NEIGHBOR_WEIGHT_SUM || p->op == GVM_OP_NEIGHBOR_ATTR_SUM)) {
          const uint32_t node = (uint32_t)in.imm;
          const size_t begin = (size_t)graph->offsets[node];
          const size_t finish = (size_t)graph->offsets[node + 1U];
          const size_t count = finish - begin;
          const graphion_insn next = *p++;
          if (in.imm < 0 || (size_t)node >= graph->node_count) {
            vm->pc = (size_t)((p - vm->program) - 2U);
            return GVM_ERR_INVALID_NODE_ID;
          }
          if (next.op == GVM_OP_NEIGHBOR_WEIGHT_SUM) {
            if (graph->weights == NULL) {
              vm->pc = (size_t)((p - vm->program) - 1U);
              return GVM_ERR_CSR_WEIGHTS_UNBOUND;
            }
            regs[next.b] = (int64_t)sum_weight_slice_wrap(graph->weights + begin, count);
          } else {
            if (graph->edge_attrs == NULL) {
              vm->pc = (size_t)((p - vm->program) - 1U);
              return GVM_ERR_CSR_EDGE_ATTRS_UNBOUND;
            }
            regs[next.b] = (int64_t)sum_attr_slice_wrap(graph->edge_attrs + begin, count);
          }
          break;
        }
        regs[in.a] = (int64_t)in.imm;
        break;
      case GVM_OP_NEIGHBOR_WEIGHT_SUM: {
        uint32_t node;
        size_t begin;
        size_t finish;
        uint64_t sum;
        if (graph->weights == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_CSR_WEIGHTS_UNBOUND;
        }
        if (regs[in.a] < 0) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_NODE_ID;
        }
        node = (uint32_t)regs[in.a];
        if ((size_t)node >= graph->node_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_NODE_ID;
        }
        begin = (size_t)graph->offsets[node];
        finish = (size_t)graph->offsets[node + 1U];
        sum = sum_weight_slice_wrap(graph->weights + begin, finish - begin);
        regs[in.b] = (int64_t)sum;
      } break;
      case GVM_OP_NEIGHBOR_ATTR_SUM: {
        uint32_t node;
        size_t begin;
        size_t finish;
        uint64_t sum;
        if (graph->edge_attrs == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_CSR_EDGE_ATTRS_UNBOUND;
        }
        if (regs[in.a] < 0) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_NODE_ID;
        }
        node = (uint32_t)regs[in.a];
        if ((size_t)node >= graph->node_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_NODE_ID;
        }
        begin = (size_t)graph->offsets[node];
        finish = (size_t)graph->offsets[node + 1U];
        sum = sum_attr_slice_wrap(graph->edge_attrs + begin, finish - begin);
        regs[in.b] = (int64_t)sum;
      } break;
      default:
        vm->pc = (size_t)(p - vm->program);
        vm_copy_raw_i64_to_regs(vm, regs);
        return GVM_ERR_UNKNOWN_OPCODE;
    }
  }

  vm->pc = vm->program_len;
  vm_copy_raw_i64_to_regs(vm, regs);
  return GVM_OK;
}

static int run_value_move_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p;
  const graphion_insn *const end = vm->program + vm->program_len;
  graphion_vm_value *regs;
  const graphion_vm_value *const_pool;
  graphion_vm_value *globals;
  const size_t const_count = vm->const_count;
  const size_t global_count = vm->global_count;

  if (vm == NULL) {
    return GVM_ERR_INVALID_ARG;
  }

  regs = vm->regs;
  const_pool = vm->const_pool;
  globals = vm->globals;
  p = vm->program + vm->pc;
  while (p < end) {
    const graphion_insn in = *p++;
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return GVM_OK;
      case GVM_OP_MOV:
        regs[in.a] = regs[in.b];
        break;
      case GVM_OP_LOAD_CONST:
        if (const_pool == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_CONST_UNBOUND;
        }
        if (in.imm < 0 || (size_t)in.imm >= const_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_CONST_INDEX;
        }
        regs[in.a] = const_pool[(size_t)in.imm];
        break;
      case GVM_OP_LOAD_GLOBAL:
        if (globals == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_GLOBALS_UNBOUND;
        }
        if (in.imm < 0 || (size_t)in.imm >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        regs[in.a] = globals[(size_t)in.imm];
        break;
      case GVM_OP_STORE_GLOBAL:
        if (globals == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_GLOBALS_UNBOUND;
        }
        if (in.imm < 0 || (size_t)in.imm >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        globals[(size_t)in.imm] = regs[in.a];
        break;
      case GVM_OP_ADD:
        if (regs[in.a].kind != GVM_VALUE_INT || regs[in.b].kind != GVM_VALUE_INT) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_TYPE_MISMATCH;
        }
        regs[in.a].as.int_value = wrap_add_i64(regs[in.a].as.int_value, regs[in.b].as.int_value);
        break;
      default:
        vm->pc = (size_t)((p - vm->program) - 1U);
        return GVM_ERR_UNKNOWN_OPCODE;
    }
  }
  vm->pc = vm->program_len;
  return GVM_OK;
}

static int run_value_move_fastpath_verified_c(graphion_vm *vm) {
  const graphion_insn *p;
  const graphion_insn *const end = vm->program + vm->program_len;
  graphion_vm_value *regs;
  const graphion_vm_value *const_pool;
  graphion_vm_value *globals;

  if (vm == NULL || vm->const_pool == NULL || vm->globals == NULL) {
    return vm == NULL ? GVM_ERR_INVALID_ARG
                      : (vm->const_pool == NULL ? GVM_ERR_CONST_UNBOUND : GVM_ERR_GLOBALS_UNBOUND);
  }

  regs = vm->regs;
  const_pool = vm->const_pool;
  globals = vm->globals;
  p = vm->program + vm->pc;
  while (p < end) {
    const graphion_insn in = *p++;
    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return GVM_OK;
      case GVM_OP_MOV:
        if (vm->value_move_int_add_safe && p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_ADD && next.b == in.a) {
            regs[in.a] = regs[in.b];
            regs[next.a].as.int_value = wrap_add_i64(regs[next.a].as.int_value, regs[in.a].as.int_value);
            ++p;
            break;
          }
        }
        regs[in.a] = regs[in.b];
        break;
      case GVM_OP_LOAD_CONST:
        if (p < end) {
          const graphion_insn next = *p;
          const graphion_vm_value value = const_pool[(size_t)in.imm];
          if (next.op == GVM_OP_STORE_GLOBAL && next.a == in.a) {
            regs[in.a] = value;
            globals[(size_t)next.imm] = value;
            ++p;
            break;
          }
          if (vm->value_move_int_add_safe && next.op == GVM_OP_ADD && next.b == in.a) {
            regs[in.a] = value;
            regs[next.a].as.int_value = wrap_add_i64(regs[next.a].as.int_value, value.as.int_value);
            ++p;
            break;
          }
        }
        regs[in.a] = const_pool[(size_t)in.imm];
        break;
      case GVM_OP_LOAD_GLOBAL:
        if (p < end) {
          const graphion_insn next = *p;
          const graphion_vm_value value = globals[(size_t)in.imm];
          if (next.op == GVM_OP_MOV && next.b == in.a) {
            regs[in.a] = value;
            regs[next.a] = value;
            ++p;
            break;
          }
          if (vm->value_move_int_add_safe && next.op == GVM_OP_ADD && next.b == in.a) {
            regs[in.a] = value;
            regs[next.a].as.int_value = wrap_add_i64(regs[next.a].as.int_value, value.as.int_value);
            ++p;
            break;
          }
        }
        regs[in.a] = globals[(size_t)in.imm];
        break;
      case GVM_OP_STORE_GLOBAL:
        globals[(size_t)in.imm] = regs[in.a];
        break;
      case GVM_OP_ADD:
        if (!vm->value_move_int_add_safe) {
          if (regs[in.a].kind != GVM_VALUE_INT || regs[in.b].kind != GVM_VALUE_INT) {
            vm->pc = (size_t)((p - vm->program) - 1U);
            return GVM_ERR_TYPE_MISMATCH;
          }
        }
        regs[in.a].as.int_value = wrap_add_i64(regs[in.a].as.int_value, regs[in.b].as.int_value);
        if (p < end) {
          const graphion_insn next = *p;
          if (next.op == GVM_OP_STORE_GLOBAL && next.a == in.a) {
            globals[(size_t)next.imm] = regs[in.a];
            ++p;
          }
        }
        break;
      default:
        vm->pc = (size_t)((p - vm->program) - 1U);
        return GVM_ERR_UNKNOWN_OPCODE;
    }
  }
  vm->pc = vm->program_len;
  return GVM_OK;
}

#if defined(GRAPHION_ENABLE_ASM) && !defined(_MSC_VER)
extern size_t graphion_vm_run_hotpath_arith_asm(int64_t *regs,
                                                const graphion_insn *program,
                                                size_t program_len,
                                                int *halted);
#endif

static int64_t count_visited_levels(const int32_t *levels, size_t count) {
  size_t i;
  int64_t total = 0;
  for (i = 0; i < count; ++i) {
    if (levels[i] >= 0) {
      total++;
    }
  }
  return total;
}

static int64_t count_bfs_level_count(const int32_t *levels, size_t count) {
  size_t i;
  int32_t max_level = -1;
  for (i = 0U; i < count; ++i) {
    if (levels[i] > max_level) {
      max_level = levels[i];
    }
  }
  return max_level < 0 ? 0 : (int64_t)max_level + 1;
}

static size_t appendf(char *buffer, size_t buffer_size, size_t offset, const char *fmt, ...) {
  va_list args;
  int written;
  char *dst = NULL;
  size_t remaining = 0U;

  if (offset < buffer_size) {
    dst = buffer + offset;
    remaining = buffer_size - offset;
  }

  va_start(args, fmt);
  written = vsnprintf(dst, remaining, fmt, args);
  va_end(args);

  if (written < 0) {
    return offset;
  }
  return offset + (size_t)written;
}

void graphion_vm_init(graphion_vm *vm) {
  size_t i;
  if (vm == NULL) {
    return;
  }
  for (i = 0; i < 16U; ++i) {
    vm_value_set_int(&vm->regs[i], 0);
  }
  vm->program = NULL;
  vm->program_len = 0U;
  vm->pc = 0U;
  vm->halted = false;
  vm->deterministic_mode = false;
  vm->arith_only_fastpath = false;
  vm->arith_only_halt_terminated = false;
  vm->weighted_sum_fastpath = false;
  vm->value_move_fastpath = false;
  vm->value_move_indices_valid = false;
  vm->value_move_int_add_safe = false;
  vm->const_pool = NULL;
  vm->const_count = 0U;
  vm->globals = NULL;
  vm->global_count = 0U;
  vm->csr_graph = NULL;
  vm->bfs_levels = NULL;
  vm->bfs_queue = NULL;
  vm->bfs_capacity = 0U;
  vm->hypergraph = NULL;
  vm->frontier_input = NULL;
  vm->frontier_input_len = 0U;
  vm->frontier_output = NULL;
  vm->frontier_output_len = 0U;
  vm->frontier_capacity = 0U;
}

void graphion_vm_reset_execution(graphion_vm *vm) {
  size_t i;
  if (vm == NULL) {
    return;
  }
  for (i = 0; i < 16U; ++i) {
    vm_value_set_int(&vm->regs[i], 0);
  }
  vm->pc = 0U;
  vm->halted = false;
}

void graphion_vm_set_deterministic(graphion_vm *vm, bool enabled) {
  if (vm == NULL) {
    return;
  }
  vm->deterministic_mode = enabled;
}

int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len) {
  bool halt_terminated = false;
  bool arith_only_fastpath = false;
  bool weighted_sum_fastpath = false;
  bool value_move_fastpath = false;
  if (vm == NULL || program == NULL || program_len == 0U) {
    return GVM_ERR_INVALID_ARG;
  }
  vm->program = program;
  vm->program_len = program_len;
  vm->pc = 0U;
  vm->halted = false;

  if (!shape_cache_lookup(program, program_len, &arith_only_fastpath, &halt_terminated,
                          &weighted_sum_fastpath, &value_move_fastpath)) {
    arith_only_fastpath = is_arith_only_fastpath_candidate(program, program_len, &halt_terminated) != 0;
    weighted_sum_fastpath = is_weighted_sum_fastpath_candidate(program, program_len) != 0;
    value_move_fastpath = is_value_move_fastpath_candidate(program, program_len) != 0;
    shape_cache_store(program, program_len, arith_only_fastpath, halt_terminated, weighted_sum_fastpath,
                      value_move_fastpath);
  }
  vm->arith_only_fastpath = arith_only_fastpath;
  vm->arith_only_halt_terminated = halt_terminated;
  vm->weighted_sum_fastpath = weighted_sum_fastpath;
  vm->value_move_fastpath = value_move_fastpath;
  refresh_value_move_validation(vm);
  return 0;
}

void graphion_vm_bind_csr(graphion_vm *vm,
                          const graphion_csr_graph *graph,
                          int32_t *bfs_levels,
                          uint32_t *bfs_queue,
                          size_t bfs_capacity) {
  if (vm == NULL) {
    return;
  }
  vm->csr_graph = graph;
  vm->bfs_levels = bfs_levels;
  vm->bfs_queue = bfs_queue;
  vm->bfs_capacity = bfs_capacity;
}

void graphion_vm_bind_constants(graphion_vm *vm, const graphion_vm_value *const_pool, size_t const_count) {
  if (vm == NULL) {
    return;
  }
  vm->const_pool = const_pool;
  vm->const_count = const_count;
  refresh_value_move_validation(vm);
}

void graphion_vm_bind_globals(graphion_vm *vm, graphion_vm_value *globals, size_t global_count) {
  if (vm == NULL) {
    return;
  }
  vm->globals = globals;
  vm->global_count = global_count;
  refresh_value_move_validation(vm);
}

void graphion_vm_bind_hypergraph(graphion_vm *vm, const graphion_hypergraph *graph) {
  if (vm == NULL) {
    return;
  }
  vm->hypergraph = graph;
}

void graphion_vm_bind_frontier(graphion_vm *vm,
                               uint32_t *input,
                               size_t input_len,
                               uint32_t *output,
                               size_t capacity) {
  if (vm == NULL) {
    return;
  }
  if (input == NULL || output == NULL || input_len > capacity) {
    vm->frontier_input = NULL;
    vm->frontier_input_len = 0U;
    vm->frontier_output = NULL;
    vm->frontier_output_len = 0U;
    vm->frontier_capacity = 0U;
    return;
  }
  vm->frontier_input = input;
  vm->frontier_input_len = input_len;
  vm->frontier_output = output;
  vm->frontier_output_len = 0U;
  vm->frontier_capacity = capacity;
}

static int frontier_is_bound(const graphion_vm *vm) {
  return vm->frontier_input != NULL && vm->frontier_output != NULL && vm->frontier_input_len <= vm->frontier_capacity;
}

static int op_frontier_clear(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  vm->frontier_output_len = 0U;
  SET_REG_I(vm, in->a, 0);
  return GVM_OK;
}

static int op_frontier_push(graphion_vm *vm, const graphion_insn *in) {
  int64_t value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (value < 0 || (uint64_t)value > UINT32_MAX) {
    return GVM_ERR_INVALID_FRONTIER_VALUE;
  }
  if (vm->frontier_output_len >= vm->frontier_capacity) {
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output[vm->frontier_output_len++] = (uint32_t)value;
  SET_REG_I(vm, in->b, (int64_t)vm->frontier_output_len);
  return GVM_OK;
}

static int op_frontier_filter_lt_imm(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  int64_t threshold;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  threshold = (int64_t)in->imm;
  vm->frontier_output_len = 0U;
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    const uint32_t value = vm->frontier_input[i];
    if ((int64_t)value < threshold) {
      if (vm->frontier_output_len >= vm->frontier_capacity) {
        vm->frontier_output_len = 0U;
        return GVM_ERR_FRONTIER_OVERFLOW;
      }
      vm->frontier_output[vm->frontier_output_len++] = value;
    }
  }
  SET_REG_I(vm, in->a, (int64_t)vm->frontier_output_len);
  return GVM_OK;
}

static int op_frontier_map_add_imm(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  int64_t delta;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (vm->frontier_input_len > vm->frontier_capacity) {
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  delta = (int64_t)in->imm;
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    const int64_t mapped = (int64_t)vm->frontier_input[i] + delta;
    if (mapped < 0 || (uint64_t)mapped > UINT32_MAX) {
      vm->frontier_output_len = 0U;
      return GVM_ERR_INVALID_FRONTIER_VALUE;
    }
  }
  vm->frontier_output_len = vm->frontier_input_len;
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    vm->frontier_output[i] = (uint32_t)((int64_t)vm->frontier_input[i] + delta);
  }
  SET_REG_I(vm, in->a, (int64_t)vm->frontier_output_len);
  return GVM_OK;
}

static int op_frontier_reduce_sum(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  uint64_t sum = 0U;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    sum += (uint64_t)vm->frontier_input[i];
    if (sum > (uint64_t)INT64_MAX) {
      return GVM_ERR_INVALID_FRONTIER_VALUE;
    }
  }
  SET_REG_I(vm, in->a, (int64_t)sum);
  return GVM_OK;
}

static int op_frontier_swap(graphion_vm *vm, const graphion_insn *in) {
  uint32_t *tmp_values;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  tmp_values = vm->frontier_input;
  vm->frontier_input = vm->frontier_output;
  vm->frontier_input_len = vm->frontier_output_len;
  vm->frontier_output = tmp_values;
  vm->frontier_output_len = 0U;
  SET_REG_I(vm, in->a, (int64_t)vm->frontier_input_len);
  return GVM_OK;
}

static int op_neighbors_of(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  const uint32_t *neighbors;
  size_t count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  neighbors = graphion_csr_graph_neighbors(vm->csr_graph, node);
  count = graphion_csr_graph_neighbor_count(vm->csr_graph, node);
  if (count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = count;
  for (i = 0U; i < count; ++i) {
    vm->frontier_output[i] = neighbors[i];
  }
  return GVM_OK;
}

static int op_neighbors_expand(graphion_vm *vm, const graphion_insn *in) {
  size_t i;
  size_t out_len = 0U;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  for (i = 0U; i < vm->frontier_input_len; ++i) {
    const uint32_t node = vm->frontier_input[i];
    const uint32_t *neighbors;
    size_t count;
    size_t j;
    if ((size_t)node >= vm->csr_graph->node_count) {
      vm->frontier_output_len = 0U;
      return GVM_ERR_INVALID_NODE_ID;
    }
    neighbors = graphion_csr_graph_neighbors(vm->csr_graph, node);
    count = graphion_csr_graph_neighbor_count(vm->csr_graph, node);
    if (out_len + count > vm->frontier_capacity) {
      vm->frontier_output_len = 0U;
      return GVM_ERR_FRONTIER_OVERFLOW;
    }
    for (j = 0U; j < count; ++j) {
      vm->frontier_output[out_len++] = neighbors[j];
    }
  }
  vm->frontier_output_len = out_len;
  SET_REG_I(vm, in->a, (int64_t)out_len);
  return GVM_OK;
}

static int op_neighbor_weight_sum(graphion_vm *vm, const graphion_insn *in) {
  const graphion_csr_graph *graph;
  uint32_t node;
  size_t begin;
  size_t end;
  uint64_t sum;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  graph = vm->csr_graph;
  if (graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (graph->weights == NULL) {
    return GVM_ERR_CSR_WEIGHTS_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= graph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  begin = (size_t)graph->offsets[node];
  end = (size_t)graph->offsets[node + 1U];
  sum = sum_weight_slice_wrap(graph->weights + begin, end - begin);
  SET_REG_I(vm, in->b, (int64_t)sum);
  return GVM_OK;
}

static int op_neighbor_attr_sum(graphion_vm *vm, const graphion_insn *in) {
  const graphion_csr_graph *graph;
  uint32_t node;
  size_t begin;
  size_t end;
  uint64_t sum;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  graph = vm->csr_graph;
  if (graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (graph->edge_attrs == NULL) {
    return GVM_ERR_CSR_EDGE_ATTRS_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= graph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  begin = (size_t)graph->offsets[node];
  end = (size_t)graph->offsets[node + 1U];
  sum = sum_attr_slice_wrap(graph->edge_attrs + begin, end - begin);
  SET_REG_I(vm, in->b, (int64_t)sum);
  return GVM_OK;
}

static int op_incident_of(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  const uint32_t *hyperedges;
  size_t count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->hypergraph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  hyperedges = graphion_hypergraph_incident(vm->hypergraph, node);
  count = graphion_hypergraph_incident_count(vm->hypergraph, node);
  if (count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = count;
  for (i = 0U; i < count; ++i) {
    vm->frontier_output[i] = hyperedges[i];
  }
  return GVM_OK;
}

static int op_hyperedge_nodes_of(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  const uint32_t *nodes;
  size_t count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  hyperedge = (uint32_t)reg_value;
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  nodes = graphion_hypergraph_hyperedge_nodes(vm->hypergraph, hyperedge);
  count = graphion_hypergraph_hyperedge_size(vm->hypergraph, hyperedge);
  if (count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = count;
  for (i = 0U; i < count; ++i) {
    vm->frontier_output[i] = nodes[i];
  }
  return GVM_OK;
}

static int op_nop(graphion_vm *vm, const graphion_insn *in) {
  (void)vm;
  (void)in;
  return 0;
}

static int op_halt(graphion_vm *vm, const graphion_insn *in) {
  (void)in;
  vm->halted = true;
  return 0;
}

static int op_mov_imm(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_MOV_IMM_REG;
  }
  SET_REG_I(vm, in->a, (int64_t)in->imm);
  return 0;
}

static int op_add(graphion_vm *vm, const graphion_insn *in) {
  int64_t lhs;
  int64_t rhs;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (!vm_reg_get_int(vm, in->a, &lhs) || !vm_reg_get_int(vm, in->b, &rhs)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  SET_REG_I(vm, in->a, wrap_add_i64(lhs, rhs));
  return 0;
}

static int op_mov(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  vm_value_copy(&vm->regs[in->a], &vm->regs[in->b]);
  return 0;
}

static int op_load_const(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->const_pool == NULL) {
    return GVM_ERR_CONST_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->const_count) {
    return GVM_ERR_INVALID_CONST_INDEX;
  }
  vm_value_copy(&vm->regs[in->a], &vm->const_pool[(size_t)in->imm]);
  return 0;
}

static int op_load_global(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  vm_value_copy(&vm->regs[in->a], &vm->globals[(size_t)in->imm]);
  return 0;
}

static int op_store_global(graphion_vm *vm, const graphion_insn *in) {
  if (!is_valid_reg(in->a)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }
  if (in->imm < 0 || (size_t)in->imm >= vm->global_count) {
    return GVM_ERR_INVALID_GLOBAL_INDEX;
  }
  vm_value_copy(&vm->globals[(size_t)in->imm], &vm->regs[in->a]);
  return 0;
}

static int op_bfs_levels(graphion_vm *vm, const graphion_insn *in) {
  uint32_t source;
  int rc;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  source = (uint32_t)reg_value;
  if ((size_t)source >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
  if (rc != 0) {
    return GVM_ERR_BFS_RUNTIME;
  }
  SET_REG_I(vm, in->b, count_visited_levels(vm->bfs_levels, vm->csr_graph->node_count));
  return 0;
}

static int op_bfs_level_count(graphion_vm *vm, const graphion_insn *in) {
  uint32_t source;
  int rc;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  source = (uint32_t)reg_value;
  if ((size_t)source >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
  if (rc != 0) {
    return GVM_ERR_BFS_RUNTIME;
  }
  SET_REG_I(vm, in->b, count_bfs_level_count(vm->bfs_levels, vm->csr_graph->node_count));
  return 0;
}

static int op_bfs_order(graphion_vm *vm, const graphion_insn *in) {
  uint32_t source;
  int rc;
  int64_t visited_count;
  size_t i;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->csr_graph == NULL || vm->bfs_levels == NULL || vm->bfs_queue == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  source = (uint32_t)reg_value;
  if ((size_t)source >= vm->csr_graph->node_count) {
    return GVM_ERR_INVALID_BFS_SOURCE;
  }
  rc = graphion_bfs_levels(vm->csr_graph, source, vm->bfs_levels, vm->bfs_queue, vm->bfs_capacity);
  if (rc != 0) {
    return GVM_ERR_BFS_RUNTIME;
  }
  visited_count = count_visited_levels(vm->bfs_levels, vm->csr_graph->node_count);
  if ((size_t)visited_count > vm->frontier_capacity) {
    vm->frontier_output_len = 0U;
    return GVM_ERR_FRONTIER_OVERFLOW;
  }
  vm->frontier_output_len = (size_t)visited_count;
  for (i = 0U; i < vm->frontier_output_len; ++i) {
    vm->frontier_output[i] = vm->bfs_queue[i];
  }
  SET_REG_I(vm, in->b, visited_count);
  return 0;
}

static int op_incident_count(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->hypergraph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  SET_REG_I(vm, in->b, (int64_t)(vm->hypergraph->node_offsets[node + 1U] - vm->hypergraph->node_offsets[node]));
  return 0;
}

static int op_hyperedge_size(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  hyperedge = (uint32_t)reg_value;
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  SET_REG_I(vm,
            in->b,
            (int64_t)(vm->hypergraph->hyperedge_offsets[hyperedge + 1U] - vm->hypergraph->hyperedge_offsets[hyperedge]));
  return 0;
}

static int op_incident_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t node;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  node = (uint32_t)reg_value;
  if ((size_t)node >= vm->hypergraph->node_count) {
    return GVM_ERR_INVALID_NODE_ID;
  }
  SET_REG_I(vm, in->b, (int64_t)graphion_hypergraph_incident_sum(vm->hypergraph, node));
  return 0;
}

static int op_hyperedge_node_sum(graphion_vm *vm, const graphion_insn *in) {
  uint32_t hyperedge;
  int64_t reg_value;
  if (!is_valid_reg(in->a) || !is_valid_reg(in->b)) {
    return GVM_ERR_INVALID_REG;
  }
  if (vm->hypergraph == NULL) {
    return GVM_ERR_HYPERGRAPH_UNBOUND;
  }
  if (!vm_reg_get_int(vm, in->a, &reg_value)) {
    return GVM_ERR_TYPE_MISMATCH;
  }
  if (reg_value < 0) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  hyperedge = (uint32_t)reg_value;
  if ((size_t)hyperedge >= vm->hypergraph->hyperedge_count) {
    return GVM_ERR_INVALID_HYPEREDGE_ID;
  }
  SET_REG_I(vm, in->b, (int64_t)graphion_hypergraph_hyperedge_node_sum(vm->hypergraph, hyperedge));
  return 0;
}

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
      [GVM_OP_MOV] = op_mov,
      [GVM_OP_LOAD_CONST] = op_load_const,
      [GVM_OP_LOAD_GLOBAL] = op_load_global,
      [GVM_OP_STORE_GLOBAL] = op_store_global,
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
      [GVM_OP_MOV] = &&L_mov,
      [GVM_OP_LOAD_CONST] = &&L_load_const,
      [GVM_OP_LOAD_GLOBAL] = &&L_load_global,
      [GVM_OP_STORE_GLOBAL] = &&L_store_global,
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
  if (vm == NULL || vm->program == NULL) {
    return GVM_ERR_INVALID_ARG;
  }

  if (vm->weighted_sum_fastpath) {
    return run_weighted_sum_fastpath_c(vm);
  }

  if (vm->value_move_fastpath) {
    if (vm->value_move_indices_valid) {
      return run_value_move_fastpath_verified_c(vm);
    }
    return run_value_move_fastpath_c(vm);
  }

  if (vm->deterministic_mode) {
    return run_dispatch_switch(vm);
  }

  if (vm->arith_only_fastpath) {
    int64_t raw_regs[16];
    if (!vm_copy_regs_to_raw_i64(vm, raw_regs)) {
      return GVM_ERR_TYPE_MISMATCH;
    }
#if defined(GRAPHION_ENABLE_ASM) && !defined(_MSC_VER)
    if (vm->arith_only_halt_terminated) {
      run_arith_fastpath_c_halt_terminated(vm);
    } else {
      int halted = 0;
      vm->pc = graphion_vm_run_hotpath_arith_asm(raw_regs, vm->program, vm->program_len, &halted);
      vm_copy_raw_i64_to_regs(vm, raw_regs);
      vm->halted = halted != 0;
    }
#else
    if (vm->arith_only_halt_terminated) {
      run_arith_fastpath_c_halt_terminated(vm);
    } else {
      run_arith_fastpath_c(vm);
    }
#endif
    return 0;
  }

#if defined(GRAPHION_VM_DISPATCH_COMPUTED_GOTO) && !defined(_MSC_VER)
  return run_dispatch_computed_goto(vm);
#elif defined(GRAPHION_VM_DISPATCH_JUMPTABLE)
  return run_dispatch_jumptable(vm);
#else
  return run_dispatch_switch(vm);
#endif
}

size_t graphion_vm_write_snapshot(const graphion_vm *vm, char *buffer, size_t buffer_size) {
  size_t offset = 0U;
  size_t i;

  if (vm == NULL) {
    return 0U;
  }

  offset = appendf(buffer, buffer_size, offset, "GRAPHION_VM_SNAPSHOT_V1\n");
  offset = appendf(buffer, buffer_size, offset, "pc=%zu\n", vm->pc);
  offset = appendf(buffer, buffer_size, offset, "program_bound=%d\n", vm->program != NULL ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "program_len=%zu\n", vm->program_len);
  offset = appendf(buffer, buffer_size, offset, "halted=%d\n", vm->halted ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "deterministic_mode=%d\n", vm->deterministic_mode ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "arith_only_fastpath=%d\n", vm->arith_only_fastpath ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "arith_only_halt_terminated=%d\n",
                   vm->arith_only_halt_terminated ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "weighted_sum_fastpath=%d\n",
                   vm->weighted_sum_fastpath ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "value_move_fastpath=%d\n",
                   vm->value_move_fastpath ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "const_bound=%d\n", vm->const_pool != NULL ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "const_count=%zu\n", vm->const_count);
  offset = appendf(buffer, buffer_size, offset, "globals_bound=%d\n", vm->globals != NULL ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "global_count=%zu\n", vm->global_count);
  offset = appendf(buffer, buffer_size, offset, "csr_bound=%d\n", vm->csr_graph != NULL ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "hypergraph_bound=%d\n", vm->hypergraph != NULL ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "frontier_bound=%d\n", frontier_is_bound(vm) ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "frontier_input_len=%zu\n", vm->frontier_input_len);
  offset = appendf(buffer, buffer_size, offset, "frontier_output_len=%zu\n", vm->frontier_output_len);
  offset = appendf(buffer, buffer_size, offset, "frontier_capacity=%zu\n", vm->frontier_capacity);
  offset = appendf(buffer, buffer_size, offset, "regs=[");
  for (i = 0U; i < 16U; ++i) {
    if (vm->regs[i].kind == GVM_VALUE_INT) {
      offset = appendf(buffer, buffer_size, offset, "%s%lld", i == 0U ? "" : ",", (long long)vm->regs[i].as.int_value);
    } else {
      offset = appendf(buffer, buffer_size, offset, "%s<k=%u>", i == 0U ? "" : ",", (unsigned)vm->regs[i].kind);
    }
  }
  offset = appendf(buffer, buffer_size, offset, "]\n");
  return offset;
}
