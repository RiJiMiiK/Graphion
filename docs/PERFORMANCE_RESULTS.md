# Performance Snapshot (x100)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-06 21:21:40 UTC.

Benchmark runs use x100 averages with benchmark-specific default iteration counts committed in the bench sources.

Format requested: `s | mteps | mips | ns_per_X`.

For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.007162 | - | 1259.400 | 0.796 |
| Graphion Linux | 0.006772 | - | 1338.038 | 0.752 |
| Rust Windows | 0.006341 | - | 1422.667 | 0.704 |

## vm_dispatch dispatch variants (`ns_per_instruction`, x100)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (switch) | 0.007092 | - | 1269.941 | 0.788 |
| Graphion Windows (jumptable) | 0.007314 | - | 1231.609 | 0.813 |
| Graphion Linux (switch) | 0.006589 | - | 1368.491 | 0.732 |
| Graphion Linux (jumptable) | 0.005987 | - | 1504.671 | 0.665 |
| Graphion Linux (computed-goto) | 0.006221 | - | 1452.911 | 0.691 |

## bfs_levels (`ns_per_edge`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005192 | 732.664 | - | 1.366 |
| Graphion Linux | 0.005290 | 718.959 | - | 1.392 |
| Rust Windows | 0.018781 | 236.357 | - | 4.942 |

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001166 | - | 5185.339 | 0.194 |
| Graphion Linux | 0.000942 | - | 6585.419 | 0.157 |
| Rust Windows | 0.001759 | - | 3449.891 | 0.293 |

## hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.004504 | - | 555.591 | 1.801 |
| Graphion Linux | 0.002975 | - | 842.726 | 1.190 |
| Rust Windows | 0.006227 | - | 402.106 | 2.491 |

## hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.003649 | - | 549.588 | 1.825 |
| Graphion Linux | 0.002862 | - | 701.030 | 1.431 |
| Rust Windows | 0.004793 | - | 417.718 | 2.396 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.009848 | - | 304.924 | 3.283 |
| Graphion Linux | 0.010623 | - | 282.671 | 3.541 |
| Rust Windows | 0.026911 | - | 126.701 | 8.970 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the local `graphion_rust` sandbox when present; that sandbox stays gitignored.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
