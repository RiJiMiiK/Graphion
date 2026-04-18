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

static int run_global_materialize_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p;
  const graphion_insn *end;
  const graphion_vm_value *const_pool;
  graphion_vm_value *globals;
  size_t const_count;
  size_t global_count;

  if (vm == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm->const_pool == NULL) {
    return GVM_ERR_CONST_UNBOUND;
  }
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }

  end = vm->program + vm->program_len;
  const_pool = vm->const_pool;
  globals = vm->globals;
  const_count = vm->const_count;
  global_count = vm->global_count;
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
      case GVM_OP_STORE_CONST_GLOBAL:
        if (in.imm < 0 || (size_t)in.imm >= const_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_CONST_INDEX;
        }
        if ((size_t)in.b >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        globals[in.b] = const_pool[(size_t)in.imm];
        break;
      case GVM_OP_COPY_GLOBAL:
        if (in.imm < 0 || (size_t)in.imm >= global_count || (size_t)in.b >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        globals[in.b] = globals[(size_t)in.imm];
        break;
      case GVM_OP_PRINT_CONST:
        if (in.imm < 0 || (size_t)in.imm >= const_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_CONST_INDEX;
        }
        {
          const int rc = vm_write_value_sink(&vm->output, &const_pool[(size_t)in.imm]);
          if (rc != GVM_OK) {
            vm->pc = (size_t)((p - vm->program) - 1U);
            return rc;
          }
        }
        break;
      case GVM_OP_PRINT_GLOBAL:
        if (in.imm < 0 || (size_t)in.imm >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        {
          const int rc = vm_write_value_sink(&vm->output, &globals[(size_t)in.imm]);
          if (rc != GVM_OK) {
            vm->pc = (size_t)((p - vm->program) - 1U);
            return rc;
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

static int run_global_print_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p;
  const graphion_insn *end;
  const graphion_vm_value *const_pool;
  graphion_vm_value *globals;
  size_t const_count;
  size_t global_count;
  int counter_sink;
  uint64_t *byte_count = NULL;

  if (vm == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm->const_pool == NULL) {
    return GVM_ERR_CONST_UNBOUND;
  }
  if (vm->globals == NULL) {
    return GVM_ERR_GLOBALS_UNBOUND;
  }

  end = vm->program + vm->program_len;
  const_pool = vm->const_pool;
  globals = vm->globals;
  const_count = vm->const_count;
  global_count = vm->global_count;
  counter_sink = vm_sink_is_counter(&vm->output);
  if (counter_sink != 0) {
    byte_count = (uint64_t *)vm->output.ctx;
    if (byte_count == NULL) {
      return GVM_ERR_INVALID_ARG;
    }
    if (!vm->global_print_indices_valid) {
      counter_sink = 0;
    } else {
      memset(vm->global_print_global_lens, 0, vm->global_count * sizeof(vm->global_print_global_lens[0]));
    }
  }
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
      case GVM_OP_STORE_CONST_GLOBAL:
        if (in.imm < 0 || (size_t)in.imm >= const_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_CONST_INDEX;
        }
        if ((size_t)in.b >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        globals[in.b] = const_pool[(size_t)in.imm];
        if (counter_sink != 0) {
          vm->global_print_global_lens[in.b] = vm->global_print_const_lens[(size_t)in.imm];
        }
        break;
      case GVM_OP_COPY_GLOBAL:
        if (in.imm < 0 || (size_t)in.imm >= global_count || (size_t)in.b >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        globals[in.b] = globals[(size_t)in.imm];
        if (counter_sink != 0) {
          vm->global_print_global_lens[in.b] = vm->global_print_global_lens[(size_t)in.imm];
        }
        break;
      case GVM_OP_PRINT_CONST: {
        if (in.imm < 0 || (size_t)in.imm >= const_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_CONST_INDEX;
        }
        if (counter_sink != 0) {
          *byte_count += (uint64_t)vm->global_print_const_lens[(size_t)in.imm];
        } else {
          const int rc = vm_write_value_sink(&vm->output, &const_pool[(size_t)in.imm]);
          if (rc != GVM_OK) {
            vm->pc = (size_t)((p - vm->program) - 1U);
            return rc;
          }
        }
      } break;
      case GVM_OP_PRINT_GLOBAL: {
        if (in.imm < 0 || (size_t)in.imm >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        if (counter_sink != 0) {
          *byte_count += (uint64_t)vm->global_print_global_lens[(size_t)in.imm];
        } else {
          const int rc = vm_write_value_sink(&vm->output, &globals[(size_t)in.imm]);
          if (rc != GVM_OK) {
            vm->pc = (size_t)((p - vm->program) - 1U);
            return rc;
          }
        }
      } break;
      default:
        vm->pc = (size_t)((p - vm->program) - 1U);
        return GVM_ERR_UNKNOWN_OPCODE;
    }
  }
  vm->pc = vm->program_len;
  return GVM_OK;
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

static int run_value_move_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p;
  const graphion_insn *end;
  graphion_vm_value *regs;
  const graphion_vm_value *const_pool;
  graphion_vm_value *globals;
  size_t const_count;
  size_t global_count;

  if (vm == NULL) {
    return GVM_ERR_INVALID_ARG;
  }

  end = vm->program + vm->program_len;
  regs = vm->regs;
  const_pool = vm->const_pool;
  globals = vm->globals;
  const_count = vm->const_count;
  global_count = vm->global_count;
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
      case GVM_OP_STORE_CONST_GLOBAL:
        if (const_pool == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_CONST_UNBOUND;
        }
        if (globals == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_GLOBALS_UNBOUND;
        }
        if (in.imm < 0 || (size_t)in.imm >= const_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_CONST_INDEX;
        }
        if ((size_t)in.b >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        globals[in.b] = const_pool[(size_t)in.imm];
        break;
      case GVM_OP_COPY_GLOBAL:
        if (globals == NULL) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_GLOBALS_UNBOUND;
        }
        if (in.imm < 0 || (size_t)in.imm >= global_count || (size_t)in.b >= global_count) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return GVM_ERR_INVALID_GLOBAL_INDEX;
        }
        globals[in.b] = globals[(size_t)in.imm];
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
  const graphion_insn *end;
  graphion_vm_value *regs;
  const graphion_vm_value *const_pool;
  graphion_vm_value *globals;

  if (vm == NULL || vm->const_pool == NULL) {
    return vm == NULL ? GVM_ERR_INVALID_ARG : GVM_ERR_CONST_UNBOUND;
  }

  end = vm->program + vm->program_len;
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
      case GVM_OP_STORE_CONST_GLOBAL:
        globals[in.b] = const_pool[(size_t)in.imm];
        break;
      case GVM_OP_COPY_GLOBAL:
        globals[in.b] = globals[(size_t)in.imm];
        break;
      case GVM_OP_PRINT_CONST: {
        const int rc = vm_write_value_sink(&vm->output, &const_pool[(size_t)in.imm]);
        if (rc != GVM_OK) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return rc;
        }
      } break;
      case GVM_OP_PRINT_GLOBAL: {
        const int rc = vm_write_value_sink(&vm->output, &globals[(size_t)in.imm]);
        if (rc != GVM_OK) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return rc;
        }
      } break;
      case GVM_OP_PRINT_REG: {
        const int rc = vm_write_value_sink(&vm->output, &regs[in.a]);
        if (rc != GVM_OK) {
          vm->pc = (size_t)((p - vm->program) - 1U);
          return rc;
        }
      } break;
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
  if (vm->global_materialize_fastpath) {
    *handled = 1;
    return run_global_materialize_fastpath_c(vm);
  }
  if (vm->global_print_fastpath) {
    *handled = 1;
    return run_global_print_fastpath_c(vm);
  }
  if (vm->value_move_fastpath) {
    *handled = 1;
    if (vm->value_move_indices_valid) {
      return run_value_move_fastpath_verified_c(vm);
    }
    return run_value_move_fastpath_c(vm);
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


