# Architecture

## Scope

Graphion is a graph/hypergraph-focused language project. Current implementation targets an efficient interpreter core.

## Runtime layers

- `src/runtime/arena.*`: bump allocator for predictable low-overhead temporary allocations.
- `src/vm/vm.*`: register-based VM scaffold with fixed-size register file.
- `src/vm/hotpaths.s`: assembly hotpath entry point (disabled by default).

## VM model (current)

- Register VM with 16 integer registers.
- Instruction format:
  - `op`: opcode
  - `a`, `b`: register operands
  - `imm`: immediate
- Implemented opcodes:
  - `GVM_OP_NOP`
  - `GVM_OP_HALT`
  - `GVM_OP_MOV_IMM`
  - `GVM_OP_ADD`
  - `GVM_OP_BFS_LEVELS`
  - `GVM_OP_INCIDENT_COUNT`
  - `GVM_OP_HYPEREDGE_SIZE`
- Bytecode parser:
  - `src/parser/bytecode.*` decodes fixed 7-byte instruction encoding.

## Hotpath acceleration

- `graphion_vm_run` selects a fast arithmetic path when the loaded program only contains:
  - `NOP`, `HALT`, `MOV_IMM`, `ADD`
- Fast path backends:
  - Portable C fallback (always available).
  - x86_64 assembly backend (`src/vm/hotpaths.s`) when `GRAPHION_ENABLE_ASM=ON` with GCC/Clang.
- Register/ABI details for assembly are documented in `docs/ASM_REGISTERS.md`.

## Safety constraints

- Assembly path disabled by default.
- CI blocks privileged/high-risk instructions in asm files.
- Sanitizer and static-analysis pipeline available.

## Near-term roadmap

- Add graph-centric opcodes and memory layouts.
- Add parser/bytecode loader and structured error model.
- Establish stable benchmark suite for Rust parity tracking.
