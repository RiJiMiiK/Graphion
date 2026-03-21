# Roadmap

## Milestone 0.1 (Interpreter Core) [done]

- [x] Stable VM ISA v0 and bytecode decoder.
- [x] Deterministic benchmark harness.
- [x] Baseline safety/security CI.

## Milestone 0.2 (Graph Primitives) [done]

- [x] Runtime CSR/BFS core (`csr_graph` + `graphion_bfs_levels`) with tests.
- [x] Graph-centric opcodes and kernels (`BFS_LEVELS`, incidence/size ops).
- [x] Parser front-end skeleton and IR bridge.
- [x] Benchmark scenarios for graph/hypergraph kernels.
- [x] Initial hypergraph traversal opcode set (beyond count/size).

## Milestone 0.2.1 (Stabilization) [done]

- [x] End-to-end parser integration test (`source -> IR -> bytecode -> VM run`).
- [x] IR v0 bridge contract document (`docs/runtime/core/IR.md`).

## Milestone 0.3 (Optimization Pass) [done]
- [x] Hotpath profiling pipeline (repeatable x100 snapshots).
- [x] Super-instruction experiments (initial `ADD+ADD` fusion in arithmetic fastpath).
- [x] Assembly integration behind measured gates (Linux x86_64 SysV path).
- [x] PGO pipeline (MSVC + GCC/Clang).
- [x] Branch-prediction-oriented dispatch variants (computed-goto / jump-table study, with portable selection gate).
- [x] Fastpath specialization cache by bytecode shape.

## Milestone 0.3.1 (Optimization Stabilization) [done]

- [x] PGO training corpus review and representative-workload policy.
- [x] Official before/after optimization reports (`baseline` vs `PGO`, per dispatch strategy where applicable).
- [x] Optimization parity tests for dispatch variants and fastpath cache edge cases.
- [x] Scheduled or release-gated PGO smoke execution policy with artifact retention rules.

## Milestone 0.3.2 (Optimization Governance) [done]

- [x] Official PGO effectiveness thresholds per workload family.
- [x] Cross-compiler optimization comparison policy (`MSVC` vs `GCC` vs `Clang`).
- [x] Benchmark environment metadata enforcement in generated reports.
- [x] PGO / non-PGO regression alert policy for release candidates.
- [x] Assembly-vs-C fallback parity and performance reporting policy.
- [x] Profile artifact hygiene and cache invalidation rules.

## Milestone 0.4 (VM + ISA Hardening) [done]

- [x] ISA version policy (`v0.x` -> `v1.0`) with compatibility matrix.
- [x] Golden ISA conformance tests (decode + execute fixtures).
- [x] Structured VM error model and error codes document.
- [x] Deterministic execution mode toggle (for reproducible debugging).
- [x] Overflow/checked arithmetic policy per opcode class.

## Milestone 0.4.1 (VM/ISA Stabilization) [done]

- [x] Public named VM runtime error codes in `src/vm/vm.h`.
- [x] ISA fixture format documentation and fixture expansion policy.
- [x] Deterministic-mode coverage across all dispatch variants.
- [x] ASM parity coverage for hardening-sensitive ISA cases.
- [x] VM state snapshot/debug dump format for deterministic repro.
- [x] Opcode-by-opcode semantic tables (inputs, outputs, failure cases).

## Milestone 0.4.2 (VM/ISA Repro And Governance)

- [x] Deterministic repro workflow documentation (snapshot + fixture + environment capture).
- [x] Named repro artifact policy for bug reports and CI failures.
- [x] VM/ISA compatibility checklist for adding or changing opcodes.
- [x] Decode/load/execute failure classification table for debugging and tests.

## Milestone 0.5 (Graph/Hypergraph Execution Model)

- [x] Frontier operations (`push/filter/map/reduce` style primitives).
- [x] Neighbor iteration opcodes with bounded memory contracts.
- [x] Hyperedge traversal primitives (node->edge and edge->node).
- [x] Optional weighted graph support and edge attributes.
- [x] Sparse/dense frontier switching heuristics.

## Milestone 0.5.1 (Graph/Frontier Benchmark Stabilization) [done]

- [x] Official benchmarks for frontier primitives.
- [x] Official benchmarks for CSR neighbor iteration primitives.
- [x] Official benchmarks for hypergraph traversal primitives.
- [x] Benchmark-backed calibration of sparse/dense frontier thresholds.
- [x] Frontier-mode reporting in benchmark outputs and docs.

## Milestone 0.5.2 (Graph Execution Stabilization)

- [x] Weighted graph execution opcodes and VM coverage.
- [x] Frontier golden fixtures for graph and hypergraph traversal primitives.
- [x] Performance regression gates for frontier and traversal workloads.
- [x] Reference graph execution examples for frontier, neighbor, and hyperedge traversal flows.

## Milestone 0.6 (Frontend And Language Surface)

- [x] `.gion` source-file extension and interpreter entry flow.
- [x] High-level interpreted syntax for dynamic variables and assignment, with no user-declared types.
- [x] Builtin `print(...)` plus user-defined functions via `def ...` and `return`.
- [x] User-facing graph declarations with integer node ids and `a -> b` edge syntax.
- [x] User-facing hypergraph declarations with auto-indexed hyperedges and integer node lists.
- [x] Scalar attribute parsing for graph/hypergraph declarations (`int`, `float`, `string`, `bool`), with reserved `weight` normalized to float.
- [x] Builtin graph/hypergraph functions with user-facing semantics:
  - `bfs(...)` returns visited node ids in BFS encounter order
  - `bfs_level(...)` returns only the number of BFS levels
  - incidence query builtins align with the future user-facing graph/hypergraph API
- [x] User-facing printable graph values for `graph`, `node`, and `edge`:
  - `print(graph)` shows graph name, node count, and edge count
  - `print(G.node[id])` shows node id/name and neighbor count for graph nodes
  - `print(G.edge[id])` shows source, target, reserved `weight` when present, and other attributes
- [x] User-facing printable hypergraph values for `hypergraph` and `hyperedge`:
  - `print(hypergraph)` shows hypergraph name, node count, and hyperedge count
  - `print(H.vertex[id])` shows vertex id/name and incident hyperedge count for hypergraph nodes
  - `print(H.hyperedge[id])` shows hyperedge id and member node count
- [ ] Legacy VM-facing naming review for user-facing builtin alignment (notably `bfs_levels`).

## Milestone 0.6.1 (Language Surface Follow-Up)

- [ ] Composite attribute values: `list`, `enum`, `dict`, and `struct`.
- [ ] Non-integer node identifiers for graphs and hypergraphs.
- [ ] Richer user-facing graph/hypergraph examples using post-0.6 value types.

## Milestone 0.7 (Runtime And Memory)

- [ ] Runtime value model for dynamic variables, scalars, graphs, hypergraphs, and function returns.
- [ ] Scope/environment model for globals, locals, builtin functions, and user-defined functions.
- [ ] Memory lifetime audit for interpreted values and VM-bound graph objects.
- [ ] Optional debug runtime checks for interpreted execution and runtime value invariants.
- [ ] Configurable allocator strategy (arena/system/hybrid) for interpreter-owned values.
- [ ] Arena/runtime metrics (peak, allocations, reset stats) for interpreted workloads.
- [ ] Thread-safety plan for future parallel runtime.

## Milestone 0.8 (Assembly Program)

- [ ] Additional assembly hotpaths for proven interpreted-runtime bottlenecks only.
- [ ] Per-ABI docs (SysV + Windows x64 strategy).
- [ ] Automated asm correctness tests vs C reference path.
- [ ] Differential perf checks (asm on/off thresholds) for language-visible workloads.
- [ ] Hardened asm lint rules and exception workflow.

## Milestone 0.9 (Benchmark Governance)

- [ ] Official benchmark matrix split between VM/internal kernels and user-facing interpreted programs.
- [ ] Mandatory report metadata (CPU, governor, flags, OS, date).
- [ ] Baseline update policy and review gate for both VM and language-surface workloads.
- [ ] Trend reports committed on schedule (weekly or per release).
- [ ] Cross-platform comparison tables for interpreted-language workloads (Windows/Linux/Rust where applicable).
- [ ] Benchmark policy for startup cost vs steady-state interpreter cost.

## Milestone 1.0 (Language MVP)

- [ ] End-to-end interpreted source execution for `.gion` programs.
- [ ] Stable user-facing syntax for graphs, hypergraphs, functions, builtins, and scalar attributes.
- [ ] Stable runtime + documentation + release process.
- [ ] Performance target definition for interpreted-language workloads and internal kernels.
- [ ] Security model and supported platform matrix.
- [ ] v1.0 release checklist and migration notes.

## Continuous Tracks

### Security

- [ ] Keep action pinning and supply-chain checks green.
- [ ] Quarterly dependency and workflow audit.
- [ ] Incident/postmortem templates exercised by drill.

### Quality

- [ ] Expand tests for parser, interpreter runtime, VM kernels, and edge cases.
- [ ] Fuzz corpus curation and crash triage process.
- [ ] Static analysis budget (`clang-tidy`, `cppcheck`) with zero-regression rule.

### Developer Experience

- [ ] One-command local setup parity (Windows/Linux).
- [ ] Script UX consistency and structured logs.
- [ ] Contributor quickstart for interpreted-language repro and perf repro.
