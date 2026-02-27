/* SPDX-License-Identifier: MIT */

#include "parser/bytecode.h"
#include "parser/frontend.h"
#include "compiler/ir.h"

#include <stddef.h>
#include <stdint.h>

int test_parser_decode_valid_program(void) {
  const uint8_t bytes[] = {
      GVM_OP_MOV_IMM, 0, 0, 7, 0, 0, 0,   GVM_OP_MOV_IMM, 1, 0, 35, 0, 0, 0,
      GVM_OP_ADD,     0, 1, 0, 0, 0, 0,   GVM_OP_HALT,    0, 0, 0,  0, 0, 0,
  };
  graphion_insn program[8];
  size_t count = 0U;
  int rc;

  rc = graphion_decode_bytecode(bytes, sizeof(bytes), program, 8U, &count);
  if (rc != 0) {
    return 1;
  }
  if (count != 4U) {
    return 2;
  }
  if (program[1].imm != 35 || program[2].op != GVM_OP_ADD) {
    return 3;
  }
  return 0;
}

int test_parser_rejects_truncated_input(void) {
  const uint8_t bytes[] = {GVM_OP_HALT, 0, 0, 0, 0, 0};
  graphion_insn program[2];
  size_t count = 0U;
  int rc;

  rc = graphion_decode_bytecode(bytes, sizeof(bytes), program, 2U, &count);
  if (rc != GBC_ERR_TRUNCATED) {
    return 1;
  }
  return 0;
}

int test_frontend_parse_and_ir_lowering(void) {
  const char *source = "mov r0, 7\n"
                       "mov r1, 35\n"
                       "add r0, r1\n"
                       "incident_sum r0, r2\n"
                       "halt\n";
  graphion_ir_insn ir[8];
  graphion_insn program[8];
  size_t ir_count = 0U;
  size_t program_count = 0U;
  int rc;

  rc = graphion_parse_source_to_ir(source, ir, 8U, &ir_count);
  if (rc != GFE_OK) {
    return 1;
  }
  if (ir_count != 5U) {
    return 2;
  }
  if (ir[0].op != GIR_OP_MOV_IMM || ir[0].imm != 7 || ir[3].op != GIR_OP_INCIDENT_SUM) {
    return 3;
  }

  rc = graphion_ir_lower_to_bytecode(ir, ir_count, program, 8U, &program_count);
  if (rc != GIR_OK) {
    return 4;
  }
  if (program_count != ir_count) {
    return 5;
  }
  if (program[2].op != GVM_OP_ADD || program[4].op != GVM_OP_HALT) {
    return 6;
  }
  return 0;
}

int test_frontend_rejects_invalid_source(void) {
  const char *source = "mov r0, nope\n";
  graphion_ir_insn ir[4];
  size_t count = 0U;
  int rc = graphion_parse_source_to_ir(source, ir, 4U, &count);
  if (rc != GFE_ERR_PARSE) {
    return 1;
  }
  return 0;
}

int test_frontend_source_to_vm_execution(void) {
  const char *source = "mov r0, 7\n"
                       "mov r1, 35\n"
                       "add r0, r1\n"
                       "halt\n";
  graphion_ir_insn ir[8];
  graphion_insn program[8];
  size_t ir_count = 0U;
  size_t program_count = 0U;
  graphion_vm vm;
  int rc;

  rc = graphion_parse_source_to_ir(source, ir, 8U, &ir_count);
  if (rc != GFE_OK) {
    return 1;
  }
  rc = graphion_ir_lower_to_bytecode(ir, ir_count, program, 8U, &program_count);
  if (rc != GIR_OK) {
    return 2;
  }

  graphion_vm_init(&vm);
  rc = graphion_vm_load(&vm, program, program_count);
  if (rc != 0) {
    return 3;
  }
  rc = graphion_vm_run(&vm);
  if (rc != 0) {
    return 4;
  }
  if (!vm.halted || vm.regs[0] != 42) {
    return 5;
  }
  return 0;
}
