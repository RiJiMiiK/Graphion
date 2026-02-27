/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_VM_H
#define GRAPHION_VM_VM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  GVM_OP_NOP = 0,
  GVM_OP_HALT = 1,
  GVM_OP_MOV_IMM = 2,
  GVM_OP_ADD = 3
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
} graphion_vm;

void graphion_vm_init(graphion_vm *vm);
int graphion_vm_load(graphion_vm *vm, const graphion_insn *program, size_t program_len);
int graphion_vm_run(graphion_vm *vm);

#endif
