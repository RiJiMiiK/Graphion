# Roadmap

## Milestone 0.1 (Interpreter Core)

- Stable VM ISA v0 and bytecode decoder.
- Deterministic benchmark harness.
- Baseline safety/security CI.

## Milestone 0.2 (Graph Primitives)

- [x] Add graph-centric opcodes and core kernels (`BFS_LEVELS`, incidence/size ops).
- [ ] Add parser front-end skeleton and IR bridge.
- [x] Add benchmark scenarios for graph/hypergraph kernels.

## Milestone 0.3 (Optimization Pass)

- [x] Hotpath profiling pipeline (repeatable x100 benchmark snapshots).
- [ ] Super-instruction experiments.
- [x] Assembly integration behind measured gates (Linux x86_64 SysV path).

## Milestone 1.0 (Language MVP)

- End-to-end source -> bytecode pipeline.
- Stable runtime + documentation + release process.
