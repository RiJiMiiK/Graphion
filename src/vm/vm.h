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
  GVM_OP_BFS_LEVELS = 16,
  GVM_OP_INCIDENT_COUNT = 17,
  GVM_OP_HYPEREDGE_SIZE = 18
} graphion_opcode;

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
  const graphion_csr_graph *csr_graph;
  int32_t *bfs_levels;
  uint32_t *bfs_queue;
  size_t bfs_capacity;
  const graphion_hypergraph *hypergraph;
} graphion_vm;

void graphion_vm_init(graphion_vm *vm);
int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len);
void graphion_vm_bind_csr(graphion_vm *vm,
                          const graphion_csr_graph *graph,
                          int32_t *bfs_levels,
                          uint32_t *bfs_queue,
                          size_t bfs_capacity);
void graphion_vm_bind_hypergraph(graphion_vm *vm, const graphion_hypergraph *graph);
int graphion_vm_run(graphion_vm *vm);

#endif
