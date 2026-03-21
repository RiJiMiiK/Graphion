# Architecture

## Scope

Graphion is a graph/hypergraph-focused language project. Current implementation targets an efficient interpreter core.

Current source-program entry flow uses the `.gion` extension.

## Runtime layers

- `src/runtime/arena.*`: bump allocator for predictable low-overhead temporary allocations.
- `src/runtime/interpreter.*`: minimal interpreted-language runtime for dynamic scalar values and assignment.
- `src/vm/vm.*`: register-based VM scaffold with fixed-size register file.
- `src/vm/hotpaths.s`: assembly hotpath entry point (disabled by default).
- `src/graph/csr_graph.*`: CSR graph runtime with optional per-edge weights and edge attributes.

## Interpreted source model (current)

- `.gion` programs currently execute through `src/runtime/entry.*`.
- The current interpreted surface is intentionally minimal:
  - dynamic variable assignment only
  - builtin `print(...)`
  - top-level user-defined functions via `def name(...):`
  - `return` inside function bodies
  - top-level `graph Name:` declarations
  - top-level `hypergraph Name:` declarations
  - no user-declared types
  - supported scalar values:
    - `int`
    - `float`
    - `bool`
    - `string`
- assignment expressions may reference an already-bound variable:
  - `answer = 42`
  - `copy = answer`
- graph declarations currently support integer node ids only:
  - `graph G:`
  - `  1 -> 2`
  - `  2 -> 3`
- hypergraph declarations currently support explicit hyperedge ids and integer node lists:
  - `hypergraph H:`
  - `  e1: [1, 2, 3]`
  - `  e2: [2, 4]`
- function calls may appear in assignment expressions:
  - `answer = echo(42)`
- `print(...)` writes scalar runtime values to the configured interpreter output stream.
- printable graph-oriented runtime values are part of the intended user-facing model:
  - `print(graph)` should show graph name, node count, and edge count
  - `print(node)` should show node id/name and neighbor count for graph nodes
  - `print(edge)` should show source, target, reserved `weight` when present, and other attributes
  - `print(hypergraph)` should show hypergraph name, node count, and hyperedge count
  - `print(node)` should show node id/name and incident hyperedge count for hypergraph nodes
  - `print(hyperedge)` should show hyperedge id and member node count
- reserved names such as `def`, `return`, `print`, `graph`, and `hypergraph` are rejected as variable names.
- the current function model is intentionally narrow:
  - top-level definitions only
  - no nested `def`
  - local function scope with fallback reads from the global scope
- the current graph declaration model is also intentionally narrow:
  - top-level declarations only
  - integer node ids only
  - no graph attributes yet
  - only summary printing is implemented today:
    - `print(graph)` -> graph name, node count, edge count
    - `print(hypergraph)` -> hypergraph name, node count, hyperedge count
  - `node`, `edge`, and `hyperedge` printable values will land with the future user-facing graph API

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
- Source entry flow:
  - `src/runtime/entry.*` validates `.gion` source files, reads source text, and executes
    the current interpreted runtime.
- Legacy VM-oriented parser flow:
  - `src/parser/frontend.*` still exists for mnemonic/IR bridge coverage and internal VM tests.
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
  - reference source programs for these flows live in `docs/runtime/core/GRAPH_EXECUTION_EXAMPLES.md`

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
- The VM now exposes aggregated weighted execution opcodes over the same layout:
  - `GVM_OP_NEIGHBOR_WEIGHT_SUM`
  - `GVM_OP_NEIGHBOR_ATTR_SUM`

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
