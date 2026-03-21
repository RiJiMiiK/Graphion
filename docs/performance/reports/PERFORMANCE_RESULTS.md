# Performance Snapshot (x1)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-21 14:57:03 UTC.

Benchmark runs use x1 averages with benchmark-specific default iteration counts committed in the bench sources.

Format requested: `s | mteps | mips | ns_per_X`.

For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).

## Environment Metadata

| Lane | Compiler | ASM | CPU | Machine | Git | Runs |
|---|---|---|---|---|---|---:|
| Graphion Windows | msvc | off | AMD64 Family 25 Model 68 Stepping 1, AuthenticAMD | AMD64 | fe4e39425a13 | 1 |
| Graphion Linux (preview) | gcc | off | AMD64 Family 25 Model 68 Stepping 1, AuthenticAMD | AMD64 | fe4e39425a13 | 1 |
| Rust Windows | rustc | off | AMD64 Family 25 Model 68 Stepping 1, AuthenticAMD | AMD64 | fe4e39425a13 | 1 |


## frontier_primitives (`ns_per_frontier_item`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.065240 | - | 27.591 | 1.699 |
| Graphion Linux (preview) | 0.065240 | - | 27.591 | 1.699 |
| Rust Windows | 0.177894 | - | 10.118 | 4.633 |

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.014658 | - | 613.980 | 1.629 |
| Graphion Linux (preview) | 0.014658 | - | 613.980 | 1.629 |
| Rust Windows | 0.016855 | - | 533.973 | 1.873 |

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
| Graphion Windows | 0.010927 | 347.764 | - | 2.876 |
| Graphion Linux (preview) | 0.010927 | 347.764 | - | 2.876 |
| Rust Windows | 0.032338 | 117.510 | - | 8.510 |

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.003922 | - | 1529.651 | 0.654 |
| Graphion Linux (preview) | 0.003922 | - | 1529.651 | 0.654 |
| Rust Windows | 0.003835 | - | 1564.537 | 0.639 |

## hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.012170 | - | 205.422 | 4.868 |
| Graphion Linux (preview) | 0.012170 | - | 205.422 | 4.868 |
| Rust Windows | 0.012135 | - | 206.017 | 4.854 |

## hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.006680 | - | 299.422 | 3.340 |
| Graphion Linux (preview) | 0.006680 | - | 299.422 | 3.340 |
| Rust Windows | 0.009671 | - | 206.808 | 4.835 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.019402 | - | 154.623 | 6.467 |
| Graphion Linux (preview) | 0.019402 | - | 154.623 | 6.467 |
| Rust Windows | 0.057007 | - | 52.625 | 19.002 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the local `graphion_rust` sandbox when present; that sandbox stays gitignored.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
