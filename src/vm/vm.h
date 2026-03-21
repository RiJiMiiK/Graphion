/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_VM_H
#define GRAPHION_VM_VM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "graph/csr_graph.h"
#include "graph/hypergraph.h"

typedef enum {
  GVM_OP_NOP = 0,
  GVM_OP_HALT = 1,
  GVM_OP_MOV_IMM = 2,
  GVM_OP_ADD = 3,
  GVM_OP_FRONTIER_CLEAR = 32,
  GVM_OP_FRONTIER_PUSH = 33,
  GVM_OP_FRONTIER_FILTER_LT_IMM = 34,
  GVM_OP_FRONTIER_MAP_ADD_IMM = 35,
  GVM_OP_FRONTIER_REDUCE_SUM = 36,
  GVM_OP_FRONTIER_SWAP = 37,
  GVM_OP_NEIGHBORS_OF = 38,
  GVM_OP_NEIGHBORS_EXPAND = 39,
  GVM_OP_BFS_LEVELS = 16,
  GVM_OP_INCIDENT_COUNT = 17,
  GVM_OP_HYPEREDGE_SIZE = 18,
  GVM_OP_INCIDENT_SUM = 19,
  GVM_OP_HYPEREDGE_NODE_SUM = 20
} graphion_opcode;

typedef enum {
  GVM_OK = 0,
  GVM_ERR_INVALID_ARG = -1,
  GVM_ERR_INVALID_MOV_IMM_REG = -2,
  GVM_ERR_INVALID_REG = -3,
  GVM_ERR_UNKNOWN_OPCODE = -4,
  GVM_ERR_CSR_UNBOUND = -5,
  GVM_ERR_INVALID_BFS_SOURCE = -6,
  GVM_ERR_BFS_RUNTIME = -7,
  GVM_ERR_HYPERGRAPH_UNBOUND = -8,
  GVM_ERR_INVALID_NODE_ID = -9,
  GVM_ERR_INVALID_HYPEREDGE_ID = -10,
  GVM_ERR_FRONTIER_UNBOUND = -11,
  GVM_ERR_FRONTIER_OVERFLOW = -12,
  GVM_ERR_INVALID_FRONTIER_VALUE = -13
} graphion_vm_result;

typedef struct {
  uint8_t op;
  uint8_t a;
  uint8_t b;
  int32_t imm;
} graphion_insn;

typedef struct {
  int64_t regs[16];
  const graphion_insn *program;
  size_t program_len;
  size_t pc;
  bool halted;
  bool deterministic_mode;
  bool arith_only_fastpath;
  bool arith_only_halt_terminated;
  const graphion_csr_graph *csr_graph;
  int32_t *bfs_levels;
  uint32_t *bfs_queue;
  size_t bfs_capacity;
  const graphion_hypergraph *hypergraph;
  uint32_t *frontier_input;
  size_t frontier_input_len;
  uint32_t *frontier_output;
  size_t frontier_output_len;
  size_t frontier_capacity;
} graphion_vm;

void graphion_vm_init(graphion_vm *vm);
void graphion_vm_set_deterministic(graphion_vm *vm, bool enabled);
int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len);
void graphion_vm_bind_csr(graphion_vm *vm,
                          const graphion_csr_graph *graph,
                          int32_t *bfs_levels,
                          uint32_t *bfs_queue,
                          size_t bfs_capacity);
void graphion_vm_bind_hypergraph(graphion_vm *vm, const graphion_hypergraph *graph);
void graphion_vm_bind_frontier(graphion_vm *vm,
                               uint32_t *input,
                               size_t input_len,
                               uint32_t *output,
                               size_t capacity);
int graphion_vm_run(graphion_vm *vm);
size_t graphion_vm_write_snapshot(const graphion_vm *vm, char *buffer, size_t buffer_size);

#endif
