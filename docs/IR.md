# IR v0 (Parser Front-End Bridge)

This document defines the minimal IR used between the source parser and VM bytecode in Milestone 0.2.x.

## Scope

- IR is currently instruction-oriented and linear (no blocks/CFG yet).
- IR is intentionally close to VM bytecode to keep lowering simple and deterministic.
- Purpose: stabilize the parser/bridge contract before optimization work in Milestone 0.3.

## Instruction Model

Each IR instruction is represented by:

- `op` (`uint8_t`): operation code.
- `a` (`uint8_t`): first operand/register slot.
- `b` (`uint8_t`): second operand/register slot.
- `imm` (`int32_t`): immediate literal for `MOV_IMM` style ops.

Reference type: `graphion_ir_insn` in `src/compiler/ir.h`.

## Supported IR Opcodes (v0)

- `GIR_OP_NOP`
- `GIR_OP_HALT`
- `GIR_OP_MOV_IMM`
- `GIR_OP_ADD`
- `GIR_OP_BFS_LEVELS`
- `GIR_OP_INCIDENT_COUNT`
- `GIR_OP_HYPEREDGE_SIZE`
- `GIR_OP_INCIDENT_SUM`
- `GIR_OP_HYPEREDGE_NODE_SUM`

These map 1:1 to current VM opcodes in `src/vm/vm.h`.

## Front-End Source Syntax (Current Skeleton)

Accepted forms:

- `nop`
- `halt`
- `mov rX, IMM`
- `add rA, rB`
- `bfs_levels rA, rB`
- `incident_count rA, rB`
- `hyperedge_size rA, rB`
- `incident_sum rA, rB`
- `hyperedge_node_sum rA, rB`

Notes:

- Case-insensitive mnemonics.
- Registers are written as `r0`..`r255` at parse stage.
- Comments supported with `#` and `//`.
- Parser only validates syntax; VM validates runtime register constraints.

## Lowering Contract (IR -> Bytecode)

Function: `graphion_ir_lower_to_bytecode(...)` in `src/compiler/ir.c`.

Guarantees:

- Order-preserving copy.
- Opcode validity check against IR v0 set.
- Exact field transfer (`op`, `a`, `b`, `imm`) on success.

Errors:

- `GIR_ERR_INVALID_ARG`
- `GIR_ERR_CAPACITY`
- `GIR_ERR_INVALID_OPCODE`

## Stability

IR v0 is a bridge format and may evolve.
Any incompatible change should update this file and `docs/ISA.md` together.
