# Roadmap

## Milestone 0.1 (Interpreter Core) [done]

- [x] Stable VM ISA v0 and bytecode decoder.
- [x] Deterministic benchmark harness.
- [x] Baseline safety/security CI.

## Milestone 0.2 (Graph Primitives) [in progress]

- [x] Runtime CSR/BFS core (`csr_graph` + `graphion_bfs_levels`) with tests.
- [x] Graph-centric opcodes and kernels (`BFS_LEVELS`, incidence/size ops).
- [x] Parser front-end skeleton and IR bridge.
- [x] Benchmark scenarios for graph/hypergraph kernels.
- [x] Initial hypergraph traversal opcode set (beyond count/size).

## Milestone 0.3 (Optimization Pass) [in progress]

- [x] Hotpath profiling pipeline (repeatable x100 snapshots).
- [ ] Super-instruction experiments.
- [x] Assembly integration behind measured gates (Linux x86_64 SysV path).
- [ ] PGO pipeline (MSVC + GCC/Clang).
- [ ] Branch-prediction-oriented dispatch variants (computed-goto / jump-table study).
- [ ] Fastpath specialization cache by bytecode shape.

## Milestone 0.4 (VM + ISA Hardening)

- [ ] ISA version policy (`v0.x` -> `v1.0`) with compatibility matrix.
- [ ] Golden ISA conformance tests (decode + execute fixtures).
- [ ] Structured VM error model and error codes document.
- [ ] Deterministic execution mode toggle (for reproducible debugging).
- [ ] Overflow/checked arithmetic policy per opcode class.

## Milestone 0.5 (Graph/Hypergraph Execution Model)

- [ ] Frontier operations (`push/filter/map/reduce` style primitives).
- [ ] Neighbor iteration opcodes with bounded memory contracts.
- [ ] Hyperedge traversal primitives (node->edge and edge->node).
- [ ] Optional weighted graph support and edge attributes.
- [ ] Sparse/dense frontier switching heuristics.

## Milestone 0.6 (Frontend And Language Surface)

- [ ] Lexer and parser for source language prototype.
- [ ] AST + lowering to bytecode.
- [ ] Diagnostics with line/column spans and stable error messages.
- [ ] Minimal standard library for graph/hypergraph operations.
- [ ] Examples and reference programs for BFS, centrality, and incidence queries.

## Milestone 0.7 (Runtime And Memory)

- [ ] Arena metrics (peak, allocations, reset stats).
- [ ] Optional debug runtime checks (bounds/poison/guards).
- [ ] Memory lifetime audit for VM-bound graph objects.
- [ ] Configurable allocator strategy (arena/system/hybrid).
- [ ] Thread-safety plan for future parallel runtime.

## Milestone 0.8 (Assembly Program)

- [ ] Additional assembly hotpaths for proven bottlenecks only.
- [ ] Per-ABI docs (SysV + Windows x64 strategy).
- [ ] Automated asm correctness tests vs C reference path.
- [ ] Differential perf checks (asm on/off thresholds).
- [ ] Hardened asm lint rules and exception workflow.

## Milestone 0.9 (Benchmark Governance)

- [ ] Official benchmark matrix: small/medium/large inputs.
- [ ] Mandatory report metadata (CPU, governor, flags, OS, date).
- [ ] Baseline update policy and review gate.
- [ ] Trend reports committed on schedule (weekly or per release).
- [ ] Cross-platform comparison table (Windows/Linux/Rust) in docs.

## Milestone 1.0 (Language MVP)

- [ ] End-to-end source -> bytecode pipeline.
- [ ] Stable runtime + documentation + release process.
- [ ] Performance target definition per workload family.
- [ ] Security model and supported platform matrix.
- [ ] v1.0 release checklist and migration notes.

## Continuous Tracks

### Security

- [ ] Keep action pinning and supply-chain checks green.
- [ ] Quarterly dependency and workflow audit.
- [ ] Incident/postmortem templates exercised by drill.

### Quality

- [ ] Expand tests for parser, VM, graph kernels, and edge cases.
- [ ] Fuzz corpus curation and crash triage process.
- [ ] Static analysis budget (`clang-tidy`, `cppcheck`) with zero-regression rule.

### Developer Experience

- [ ] One-command local setup parity (Windows/Linux).
- [ ] Script UX consistency and structured logs.
- [ ] Contributor quickstart for perf repro.
