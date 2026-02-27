/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_COMPILER_IR_H
#define GRAPHION_COMPILER_IR_H

#include <stddef.h>
#include <stdint.h>

#include "vm/vm.h"

typedef enum {
  GIR_OP_NOP = 0,
  GIR_OP_HALT = 1,
  GIR_OP_MOV_IMM = 2,
  GIR_OP_ADD = 3,
  GIR_OP_BFS_LEVELS = 16,
  GIR_OP_INCIDENT_COUNT = 17,
  GIR_OP_HYPEREDGE_SIZE = 18,
  GIR_OP_INCIDENT_SUM = 19,
  GIR_OP_HYPEREDGE_NODE_SUM = 20
} graphion_ir_opcode;

typedef struct {
  uint8_t op;
  uint8_t a;
  uint8_t b;
  int32_t imm;
} graphion_ir_insn;

typedef enum {
  GIR_OK = 0,
  GIR_ERR_INVALID_ARG = -1,
  GIR_ERR_CAPACITY = -2,
  GIR_ERR_INVALID_OPCODE = -3
} graphion_ir_result;

int graphion_ir_lower_to_bytecode(const graphion_ir_insn *ir_program,
                                  size_t ir_count,
                                  graphion_insn *out_program,
                                  size_t out_capacity,
                                  size_t *out_count);

#endif
