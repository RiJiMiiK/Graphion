/* SPDX-License-Identifier: MIT */

#include "vm/internal/core/fastpath.h"
#include "vm/internal/core/value.h"
#include "vm/vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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
    vm->owned_reg_strings[i] = NULL;
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
  vm->frontier_filter_map_reduce_fastpath = false;
  vm->frontier_fastpath = false;
  vm->graph_ops_fastpath = false;
  vm->value_move_fastpath = false;
  vm->global_materialize_fastpath = false;
  vm->global_print_fastpath = false;
  vm->value_move_indices_valid = false;
  vm->value_move_int_add_safe = false;
  vm->global_print_indices_valid = false;
  vm->const_pool = NULL;
  vm->const_count = 0U;
  vm->globals = NULL;
  vm->global_string_owners = NULL;
  vm->global_count = 0U;
  vm->output.write = NULL;
  vm->output.ctx = NULL;
  memset(vm->global_print_const_lens, 0, sizeof(vm->global_print_const_lens));
  memset(vm->global_print_global_lens, 0, sizeof(vm->global_print_global_lens));
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

void graphion_vm_dispose(graphion_vm *vm) {
  if (vm == NULL) {
    return;
  }
  vm_release_all_reg_strings(vm);
}

void graphion_vm_reset_execution(graphion_vm *vm) {
  size_t i;
  if (vm == NULL) {
    return;
  }
  vm_release_all_reg_strings(vm);
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
  refresh_global_print_validation(vm);
}

void graphion_output_sink_from_file(graphion_output_sink *sink, FILE *output) {
  if (sink == NULL) {
    return;
  }
  sink->write = vm_file_output_write;
  sink->ctx = output;
}

void graphion_output_sink_from_counter(graphion_output_sink *sink, uint64_t *byte_count) {
  if (sink == NULL) {
    return;
  }
  sink->write = vm_count_output_write;
  sink->ctx = byte_count;
}

void graphion_vm_bind_globals(graphion_vm *vm, graphion_vm_value *globals, size_t global_count) {
  if (vm == NULL) {
    return;
  }
  vm->globals = globals;
  vm->global_count = global_count;
  refresh_value_move_validation(vm);
  refresh_global_print_validation(vm);
}

void graphion_vm_bind_global_string_owners(graphion_vm *vm, char **owners, size_t owner_count) {
  if (vm == NULL) {
    return;
  }
  if (owners == NULL || owner_count < vm->global_count) {
    vm->global_string_owners = NULL;
    return;
  }
  vm->global_string_owners = owners;
}

void graphion_vm_bind_output_sink(graphion_vm *vm, const graphion_output_sink *output) {
  if (vm == NULL) {
    return;
  }
  if (output == NULL) {
    vm->output.write = NULL;
    vm->output.ctx = NULL;
    return;
  }
  vm->output = *output;
}

void graphion_vm_bind_output(graphion_vm *vm, FILE *output) {
  graphion_output_sink sink;
  graphion_output_sink_from_file(&sink, output);
  graphion_vm_bind_output_sink(vm, &sink);
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
  offset = appendf(buffer, buffer_size, offset, "graph_ops_fastpath=%d\n",
                   vm->graph_ops_fastpath ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "value_move_fastpath=%d\n",
                   vm->value_move_fastpath ? 1 : 0);
  offset = appendf(buffer, buffer_size, offset, "global_materialize_fastpath=%d\n",
                   vm->global_materialize_fastpath ? 1 : 0);
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




