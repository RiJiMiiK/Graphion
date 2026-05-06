/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/fastpath.h"
#include "vm/internal/opcodes/op_frontier.h"
#include "vm/internal/opcodes/op_graph.h"
#include "vm/internal/opcodes/op_state.h"
#include "vm/internal/core/value.h"

#include <limits.h>
#include <stdint.h>

static int frontier_is_bound(const graphion_vm *vm) {
  return vm->frontier_input != NULL && vm->frontier_output != NULL &&
         vm->frontier_input_len <= vm->frontier_capacity;
}

static void finish_arith_fastpath(graphion_vm *vm, const graphion_insn *p, const int64_t regs[16], int halted) {
  vm->halted = halted != 0;
  vm->pc = (size_t)(p - vm->program);
  vm_copy_raw_i64_to_regs(vm, regs);
}

static void run_arith_fastpath_c(graphion_vm *vm, int halt_terminated) {
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
        finish_arith_fastpath(vm, p, regs, 1);
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
        finish_arith_fastpath(vm, p, regs, 0);
        return;
    }
    if (halt_terminated != 0 && vm->halted) {
      return;
    }
  }
  finish_arith_fastpath(vm, p, regs, 0);
}

static int fastpath_fail_at(graphion_vm *vm, const graphion_insn *p, int rc) {
  vm->pc = (size_t)((p - vm->program) - 1U);
  return rc;
}

static int run_weighted_sum_fastpath_c(graphion_vm *vm) {
  const graphion_insn *p = vm->program + vm->pc;
  const graphion_insn *const end = vm->program + vm->program_len;

  if (vm == NULL) {
    return GVM_ERR_INVALID_ARG;
  }
  if (vm->csr_graph == NULL) {
    return GVM_ERR_CSR_UNBOUND;
  }

  while (p < end) {
    const graphion_insn in = *p++;
    int rc = GVM_OK;

    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return GVM_OK;
      case GVM_OP_MOV_IMM:
        rc = op_mov_imm(vm, &in);
        if (rc != GVM_OK) {
          return fastpath_fail_at(vm, p, rc);
        }
        if (p < end && p->a == in.a &&
            (p->op == GVM_OP_NEIGHBOR_WEIGHT_SUM || p->op == GVM_OP_NEIGHBOR_ATTR_SUM)) {
          const graphion_insn next = *p++;
          rc = next.op == GVM_OP_NEIGHBOR_WEIGHT_SUM ? op_neighbor_weight_sum(vm, &next) : op_neighbor_attr_sum(vm, &next);
          if (rc != GVM_OK) {
            return fastpath_fail_at(vm, p, rc);
          }
        }
        break;
      case GVM_OP_NEIGHBOR_WEIGHT_SUM:
        rc = op_neighbor_weight_sum(vm, &in);
        if (rc != GVM_OK) {
          return fastpath_fail_at(vm, p, rc);
        }
        break;
      case GVM_OP_NEIGHBOR_ATTR_SUM:
        rc = op_neighbor_attr_sum(vm, &in);
        if (rc != GVM_OK) {
          return fastpath_fail_at(vm, p, rc);
        }
        break;
      default:
        vm->pc = (size_t)(p - vm->program);
        return GVM_ERR_UNKNOWN_OPCODE;
    }
  }

  vm->pc = vm->program_len;
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
    int rc = GVM_OK;

    switch (in.op) {
      case GVM_OP_NOP:
        break;
      case GVM_OP_HALT:
        vm->halted = true;
        vm->pc = (size_t)(p - vm->program);
        return GVM_OK;
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
      default:
        return fastpath_fail_at(vm, p, GVM_ERR_UNKNOWN_OPCODE);
    }

    if (rc != GVM_OK) {
      return fastpath_fail_at(vm, p, rc);
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
    run_arith_fastpath_c(vm, vm->arith_only_halt_terminated ? 1 : 0);
    return GVM_OK;
  }

  return GVM_OK;
}


