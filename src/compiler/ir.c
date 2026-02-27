/* SPDX-License-Identifier: MIT */

#include "compiler/ir.h"

#include <stddef.h>
#include <stdint.h>

static int valid_ir_opcode(uint8_t op) {
  switch (op) {
    case GIR_OP_NOP:
    case GIR_OP_HALT:
    case GIR_OP_MOV_IMM:
    case GIR_OP_ADD:
    case GIR_OP_BFS_LEVELS:
    case GIR_OP_INCIDENT_COUNT:
    case GIR_OP_HYPEREDGE_SIZE:
    case GIR_OP_INCIDENT_SUM:
    case GIR_OP_HYPEREDGE_NODE_SUM:
      return 1;
    default:
      return 0;
  }
}

int graphion_ir_lower_to_bytecode(const graphion_ir_insn *ir_program,
                                  size_t ir_count,
                                  graphion_insn *out_program,
                                  size_t out_capacity,
                                  size_t *out_count) {
  size_t i;
  if (ir_program == NULL || out_program == NULL || out_count == NULL) {
    return GIR_ERR_INVALID_ARG;
  }
  if (ir_count > out_capacity) {
    return GIR_ERR_CAPACITY;
  }

  for (i = 0U; i < ir_count; ++i) {
    if (!valid_ir_opcode(ir_program[i].op)) {
      return GIR_ERR_INVALID_OPCODE;
    }
    out_program[i].op = ir_program[i].op;
    out_program[i].a = ir_program[i].a;
    out_program[i].b = ir_program[i].b;
    out_program[i].imm = ir_program[i].imm;
  }

  *out_count = ir_count;
  return GIR_OK;
}
