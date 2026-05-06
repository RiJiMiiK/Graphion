/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/fastpath.h"
#include "vm/internal/opcodes/op_frontier.h"
#include "vm/internal/opcodes/op_graph.h"
#include "vm/internal/opcodes/op_hypergraph.h"
#include "vm/internal/opcodes/op_state.h"
#include "vm/internal/core/sum.h"
#include "vm/internal/core/value.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

static int frontier_is_bound(const graphion_vm *vm) {
  return vm->frontier_input != NULL && vm->frontier_output != NULL &&
         vm->frontier_input_len <= vm->frontier_capacity;
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

static int run_frontier_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p;
  const graphion_insn *end;

  if (vm == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (!frontier_is_bound(vm)) {
    return GVM_ERR_FRONTIER_UNBOUND;
  }

  end = vm->program + vm->program_len;
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
      case GVM_OP_FRONTIER_CLEAR:
        vm->frontier_output_len = 0U;
        vm_value_set_int(&vm->regs[in.a], 0);
        break;
      case GVM_OP_FRONTIER_PUSH: {
        int64_t value;
        if (!vm_reg_get_int(vm, in.a, &value)) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_TYPE_MISMATCH;
        }
        if (value < 0 || (uint64_t)value > UINT32_MAX) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_FRONTIER_VALUE;
        }
        if (vm->frontier_output_len >= vm->frontier_capacity) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_FRONTIER_OVERFLOW;
        }
        vm->frontier_output[vm->frontier_output_len++] = (uint32_t)value;
        vm_value_set_int(&vm->regs[in.b], (int64_t)vm->frontier_output_len);
      } break;
      case GVM_OP_FRONTIER_FILTER_LT_IMM: {
        size_t i;
        const int64_t threshold = (int64_t)in.imm;
        vm->frontier_output_len = 0U;
        for (i = 0U; i < vm->frontier_input_len; ++i) {
          const uint32_t value = vm->frontier_input[i];
          if ((int64_t)value < threshold) {
            if (vm->frontier_output_len >= vm->frontier_capacity) {
              vm->frontier_output_len = 0U;
              vm->pc = (size_t)((p - vm->program) - 1U);
              return GVM_ERR_FRONTIER_OVERFLOW;
            }
            vm->frontier_output[vm->frontier_output_len++] = value;
          }
        }
        vm_value_set_int(&vm->regs[in.a], (int64_t)vm->frontier_output_len);
      } break;
      case GVM_OP_FRONTIER_MAP_ADD_IMM: {
        size_t i;
        const int64_t delta = (int64_t)in.imm;
        if (vm->frontier_input_len > vm->frontier_capacity) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_FRONTIER_OVERFLOW;
        }
        vm->frontier_output_len = vm->frontier_input_len;
        for (i = 0U; i < vm->frontier_input_len; ++i) {
          const int64_t mapped = (int64_t)vm->frontier_input[i] + delta;
          if (mapped < 0 || (uint64_t)mapped > UINT32_MAX) {
            vm->frontier_output_len = 0U;
            vm->pc = (size_t)((p - vm->program) - 1U);
            return GVM_ERR_INVALID_FRONTIER_VALUE;
          }
          vm->frontier_output[i] = (uint32_t)mapped;
        }
        vm_value_set_int(&vm->regs[in.a], (int64_t)vm->frontier_output_len);
      } break;
      case GVM_OP_FRONTIER_REDUCE_SUM: {
        size_t i;
        uint64_t sum = 0U;
        for (i = 0U; i < vm->frontier_input_len; ++i) {
          sum += (uint64_t)vm->frontier_input[i];
          if (sum > (uint64_t)INT64_MAX) {
            vm->pc = (size_t)((p - vm->program) - 1U);
            return GVM_ERR_INVALID_FRONTIER_VALUE;
          }
        }
        vm_value_set_int(&vm->regs[in.a], (int64_t)sum);
      } break;
      case GVM_OP_FRONTIER_SWAP: {
        uint32_t *tmp_values = vm->frontier_input;
        vm->frontier_input = vm->frontier_output;
        vm->frontier_input_len = vm->frontier_output_len;
        vm->frontier_output = tmp_values;
        vm->frontier_output_len = 0U;
        vm_value_set_int(&vm->regs[in.a], (int64_t)vm->frontier_input_len);
      } break;
      default:
        vm->pc = (size_t)((p - vm->program) - 1U);
        return GVM_ERR_UNKNOWN_OPCODE;
    }
  }

  vm->pc = vm->program_len;
  return GVM_OK;
}

int graphion_vm_try_run_fastpath(graphion_vm *vm, int *handled) {
  if (handled == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  *handled = 0;
  if (vm == NULL) {
    return GVM_ERR_INVALID_ARG;
  }

  if (vm->weighted_sum_fastpath) {
    *handled = 1;
    return run_weighted_sum_fastpath_c(vm);
  }
  if (vm->frontier_fastpath) {
    *handled = 1;
    return run_frontier_fastpath_c(vm);
  }
  if (vm->arith_only_fastpath) {
    *handled = 1;
    if (vm->arith_only_halt_terminated) {
      run_arith_fastpath_c_halt_terminated(vm);
    } else {
      run_arith_fastpath_c(vm);
    }
    return GVM_OK;
  }

  return GVM_OK;
}


