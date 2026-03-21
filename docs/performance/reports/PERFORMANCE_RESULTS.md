# Performance Snapshot (x100)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-21 16:36:55 UTC.

Benchmark runs use x100 averages with benchmark-specific default iteration counts committed in the bench sources.

Format requested: `s | mteps | mips | ns_per_X`.

For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).

## Environment Metadata

| Lane | Compiler | ASM | CPU | Machine | Git | Runs |
|---|---|---|---|---|---|---:|
| Graphion Windows | msvc | off | AMD64 Family 25 Model 1 Stepping 1, AuthenticAMD | AMD64 | 028e48f1647d | 100 |
| Graphion Linux | gcc | on | AMD EPYC 7763 64-Core Processor | x86_64 | 028e48f1647d | 100 |
| Rust Windows | rustc | off | AMD64 Family 25 Model 1 Stepping 1, AuthenticAMD | AMD64 | 028e48f1647d | 100 |


## frontier_primitives (`ns_per_frontier_item`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.045265 | - | 39.769 | 1.179 |
| Graphion Linux | 0.036451 | - | 49.383 | 0.949 |
| Rust Windows | 0.105425 | - | 17.090 | 2.745 |

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.009772 | - | 921.039 | 1.086 |
| Graphion Linux | 0.008528 | - | 1055.440 | 0.948 |
| Rust Windows | 0.008686 | - | 1036.353 | 0.965 |

## vm_dispatch dispatch variants (`ns_per_instruction`, x100)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (switch) | 0.009885 | - | 913.738 | 1.098 |
| Graphion Windows (jumptable) | 0.010224 | - | 880.495 | 1.136 |
| Graphion Linux (switch) | 0.008517 | - | 1056.742 | 0.946 |
| Graphion Linux (jumptable) | 0.008279 | - | 1087.275 | 0.920 |
| Graphion Linux (computed-goto) | 0.008139 | - | 1105.858 | 0.904 |

## bfs_levels (`ns_per_edge`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.007249 | 524.446 | - | 1.908 |
| Graphion Linux | 0.007131 | 532.874 | - | 1.877 |
| Rust Windows | 0.019059 | 199.461 | - | 5.016 |

## neighbor_iteration (`ns_per_neighbor`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001421 | 1689.793 | - | 0.592 |
| Graphion Linux | 0.001614 | 1535.638 | - | 0.672 |
| Rust Windows | 0.000000 | 21220000.000 | - | 0.000 |

Frontier mode notes:

- Graphion Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Graphion Linux: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Rust Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001585 | - | 3786.687 | 0.264 |
| Graphion Linux | 0.001158 | - | 5201.890 | 0.193 |
| Rust Windows | 0.002532 | - | 2370.310 | 0.422 |

## hypergraph_traversal (`ns_per_membership`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005395 | 1334.886 | - | 0.749 |
| Graphion Linux | 0.004623 | 1558.263 | - | 0.642 |
| Rust Windows | 0.000000 | 62280000.000 | - | 0.000 |

## hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.006302 | - | 396.750 | 2.521 |
| Graphion Linux | 0.003926 | - | 636.899 | 1.570 |
| Rust Windows | 0.006921 | - | 361.270 | 2.768 |

## hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005033 | - | 397.373 | 2.517 |
| Graphion Linux | 0.003927 | - | 511.657 | 1.963 |
| Rust Windows | 0.007266 | - | 275.443 | 3.633 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.014126 | - | 212.394 | 4.709 |
| Graphion Linux | 0.014187 | - | 211.478 | 4.729 |
| Rust Windows | 0.025429 | - | 118.031 | 8.476 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the versioned `graphion_rust` benchmark lane when present in the checkout.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
