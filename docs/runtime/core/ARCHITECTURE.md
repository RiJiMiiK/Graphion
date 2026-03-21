# Architecture

## Scope

Graphion is a graph/hypergraph-focused language project. Current implementation targets an efficient interpreter core.

## Runtime layers

- `src/runtime/arena.*`: bump allocator for predictable low-overhead temporary allocations.
- `src/vm/vm.*`: register-based VM scaffold with fixed-size register file.
- `src/vm/hotpaths.s`: assembly hotpath entry point (disabled by default).
- `src/graph/csr_graph.*`: CSR graph runtime with optional per-edge weights and edge attributes.

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
  - `docs/runtime/contracts/ISA_VERSIONING.md` defines `v0.x` vs `v1.0` expectations.
- Structured error model:
  - `docs/runtime/debugging/VM_ERRORS.md` defines subsystem-local error-code interpretation.
- Public VM runtime error codes:
  - `src/vm/vm.h` exposes named `graphion_vm_result` values for load/run failures.
- VM state snapshot format:
  - `graphion_vm_write_snapshot(...)` emits a versioned text dump for deterministic repro.
  - snapshot output now includes frontier binding state and frontier lengths.
- Deterministic repro workflow:
  - `docs/runtime/debugging/VM_REPRO.md` defines how to capture fixture + snapshot + environment.
- Arithmetic overflow policy:
  - `ADD` uses explicit two's-complement wraparound semantics in the VM.
- Frontier execution model:
  - host binds input/output frontier buffers directly to the VM
  - frontier capacity is explicit and fixed by the binding call
  - `clear`, `push`, `filter_lt_imm`, `map_add_imm`, `reduce_sum`, and `swap`
    operate without dynamic allocation
  - neighbor iteration opcodes already reuse this bounded model for CSR adjacency expansion
  - hyperedge traversal opcodes reuse the same bounded contract for `node->edge` and `edge->node` materialization

## Graph storage model (current)

- CSR graphs keep mandatory topology in:
  - `offsets`
  - `neighbors`
- CSR graphs may also expose optional per-edge side data:
  - `weights` (`int64_t`)
  - `edge_attrs` (`uint32_t`)
- These side arrays are aligned by edge index with `neighbors`.
- Existing graph algorithms remain valid when the optional arrays are absent.
- Weighted/attribute-aware execution can build on the same CSR layout without a second graph format.

## Frontier mode heuristics

- Graphion now exposes a runtime recommendation for frontier execution mode:
  - `GRAPHION_FRONTIER_MODE_SPARSE`
  - `GRAPHION_FRONTIER_MODE_DENSE`
- The current heuristic is benchmark-backed, simple, and deterministic:
  - prefer `dense` when `frontier_len >= 15%` of `node_count`
  - prefer `dense` when estimated frontier neighbor work reaches `28%` of total edge count
  - otherwise prefer `sparse`
- This does not introduce a second frontier representation inside the VM yet.
- It is a planning/runtime hint for future frontier kernels and benchmark work.
- Calibration report:
  - `docs/performance/reports/FRONTIER_THRESHOLD_CALIBRATION.md`

## Hotpath acceleration

- `graphion_vm_run` selects a fast arithmetic path when the loaded program only contains:
  - `NOP`, `HALT`, `MOV_IMM`, `ADD`
- Fast path backends:
  - Portable C fallback (always available).
  - x86_64 assembly backend (`src/vm/hotpaths.s`) when `GRAPHION_ENABLE_ASM=ON` with GCC/Clang.
- Register/ABI details for assembly are documented in `docs/runtime/asm/ASM_REGISTERS.md`.
- Assembly-vs-C parity/performance policy is documented in `docs/performance/policies/ASM_FALLBACK_POLICY.md`.
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
