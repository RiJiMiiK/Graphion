# Performance Snapshot (x100)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-27 18:41:18 UTC.

Benchmark runs use x100 averages with benchmark-specific default iteration counts committed in the bench sources.

Format requested: `s | mteps | mips | ns_per_X`.

For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).

## Environment Metadata

| Lane | Compiler | ASM | CPU | Machine | Git | Runs |
|---|---|---|---|---|---|---:|
| Graphion Windows | msvc | off | Intel64 Family 6 Model 106 Stepping 6, GenuineIntel | AMD64 | c8d9e6e44fc2 | 100 |
| Graphion Linux | gcc | on | AMD EPYC 7763 64-Core Processor | x86_64 | c8d9e6e44fc2 | 100 |
| Rust Windows | rustc | off | AMD64 Family 25 Model 1 Stepping 1, AuthenticAMD | AMD64 | c8d9e6e44fc2 | 100 |


## frontier_primitives (`ns_per_frontier_item`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.046220 | - | 38.961 | 1.204 |
| Graphion Linux | 0.033767 | - | 53.309 | 0.879 |
| Rust Windows | 0.104219 | - | 17.331 | 2.714 |

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.010202 | - | 882.510 | 1.134 |
| Graphion Linux | 0.008505 | - | 1058.410 | 0.945 |
| Rust Windows | 0.008224 | - | 1098.145 | 0.914 |

## vm_dispatch dispatch variants (`ns_per_instruction`, x100)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (switch) | 0.010254 | - | 878.141 | 1.139 |
| Graphion Windows (jumptable) | 0.009647 | - | 933.737 | 1.072 |
| Graphion Linux (switch) | 0.008520 | - | 1056.558 | 0.947 |
| Graphion Linux (jumptable) | 0.008569 | - | 1050.656 | 0.952 |
| Graphion Linux (computed-goto) | 0.008243 | - | 1092.079 | 0.916 |

## bfs_levels (`ns_per_edge`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.004249 | 895.322 | - | 1.118 |
| Graphion Linux | 0.007114 | 534.184 | - | 1.872 |
| Rust Windows | 0.019171 | 198.568 | - | 5.045 |

## neighbor_iteration (`ns_per_neighbor`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001361 | 1763.984 | - | 0.567 |
| Graphion Linux | 0.001646 | 1506.192 | - | 0.686 |
| Rust Windows | 0.003610 | 665.157 | - | 1.504 |

Frontier mode notes:

- Graphion Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Graphion Linux: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Rust Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8

## weighted_neighbor_sums (`ns_per_edge_data`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.239678 | 10259.446 | - | 0.098 |
| Graphion Linux | 0.601602 | 4085.146 | - | 0.245 |
| Rust Windows | 0.343749 | 7149.991 | - | 0.140 |

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001735 | - | 3462.252 | 0.289 |
| Graphion Linux | 0.001147 | - | 5244.301 | 0.191 |
| Rust Windows | 0.002379 | - | 2526.214 | 0.397 |

## hypergraph_traversal (`ns_per_membership`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005632 | 1281.710 | - | 0.782 |
| Graphion Linux | 0.004582 | 1571.373 | - | 0.636 |
| Rust Windows | 0.007926 | 908.573 | - | 1.101 |

## hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005859 | - | 426.864 | 2.343 |
| Graphion Linux | 0.003944 | - | 633.949 | 1.578 |
| Rust Windows | 0.007738 | - | 323.637 | 3.095 |

## hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.004842 | - | 413.339 | 2.421 |
| Graphion Linux | 0.003882 | - | 516.774 | 1.941 |
| Rust Windows | 0.007287 | - | 274.834 | 3.644 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.011861 | - | 252.978 | 3.954 |
| Graphion Linux | 0.014325 | - | 209.440 | 4.775 |
| Rust Windows | 0.025131 | - | 119.406 | 8.377 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the versioned `graphion_rust` benchmark lane when present in the checkout.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
