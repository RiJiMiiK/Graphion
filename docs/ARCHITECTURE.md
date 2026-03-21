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
- ISA versioning and compatibility policy:
  - `docs/ISA_VERSIONING.md` defines `v0.x` vs `v1.0` expectations.
- Structured error model:
  - `docs/VM_ERRORS.md` defines subsystem-local error-code interpretation.
- Public VM runtime error codes:
  - `src/vm/vm.h` exposes named `graphion_vm_result` values for load/run failures.
- VM state snapshot format:
  - `graphion_vm_write_snapshot(...)` emits a versioned text dump for deterministic repro.
- Deterministic repro workflow:
  - `docs/VM_REPRO.md` defines how to capture fixture + snapshot + environment.
- Arithmetic overflow policy:
  - `ADD` uses explicit two's-complement wraparound semantics in the VM.

## Hotpath acceleration

- `graphion_vm_run` selects a fast arithmetic path when the loaded program only contains:
  - `NOP`, `HALT`, `MOV_IMM`, `ADD`
- Fast path backends:
  - Portable C fallback (always available).
  - x86_64 assembly backend (`src/vm/hotpaths.s`) when `GRAPHION_ENABLE_ASM=ON` with GCC/Clang.
- Register/ABI details for assembly are documented in `docs/ASM_REGISTERS.md`.
- Assembly-vs-C parity/performance policy is documented in `docs/ASM_FALLBACK_POLICY.md`.
- `graphion_vm_set_deterministic(vm, true)` forces the portable switch-dispatch
  path and bypasses fast arithmetic specialization for reproducible debugging.

## Safety constraints

- Assembly path disabled by default.
- CI blocks privileged/high-risk instructions in asm files.
- Sanitizer and static-analysis pipeline available.

## Near-term roadmap

- Add graph-centric opcodes and memory layouts.
- Add parser/bytecode loader and structured error model.
- Establish stable benchmark suite for Rust parity tracking.
