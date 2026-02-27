# Performance Snapshot (x100)

This snapshot records average wall-clock benchmark results over 100 runs
(`500000` iterations per run), measured on February 27, 2026.

`factor = Graphion / Rust` on `seconds` (below `1.0` means Graphion faster).

| Benchmark | Graphion Windows (s) | Graphion Linux (s) | Rust (s) | Win/Rust | Linux/Rust |
|---|---:|---:|---:|---:|---:|
| `vm_dispatch` | 0.007837 | 0.006325 | 0.006592 | 1.189 | 0.959 |
| `bfs_levels` | 0.012864 | 0.012969 | 0.044667 | 0.288 | 0.290 |
| `hypergraph_incidence` | 0.001116 | 0.000874 | 0.001447 | 0.771 | 0.604 |
| `vm_graph_ops` | 0.014010 | 0.017920 | 0.041512 | 0.337 | 0.432 |

Notes:

- Linux measurements were taken in Docker (`GRAPHION_ENABLE_ASM=ON`).
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
