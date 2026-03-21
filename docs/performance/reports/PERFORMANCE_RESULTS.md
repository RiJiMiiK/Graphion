# Performance Snapshot (x100)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-21 16:11:36 UTC.

Benchmark runs use x100 averages with benchmark-specific default iteration counts committed in the bench sources.

Format requested: `s | mteps | mips | ns_per_X`.

For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).

## Environment Metadata

| Lane | Compiler | ASM | CPU | Machine | Git | Runs |
|---|---|---|---|---|---|---:|
| Graphion Windows | msvc | off | AMD64 Family 25 Model 1 Stepping 1, AuthenticAMD | AMD64 | 40017d20d314 | 100 |
| Graphion Linux | gcc | on | AMD EPYC 7763 64-Core Processor | x86_64 | 40017d20d314 | 100 |
| Rust Windows | rustc | off | Intel64 Family 6 Model 106 Stepping 6, GenuineIntel | AMD64 | 40017d20d314 | 100 |


## frontier_primitives (`ns_per_frontier_item`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.045319 | - | 39.721 | 1.180 |
| Graphion Linux | 0.036685 | - | 49.122 | 0.955 |
| Rust Windows | 0.109796 | - | 16.405 | 2.859 |

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.009778 | - | 920.543 | 1.086 |
| Graphion Linux | 0.008548 | - | 1053.070 | 0.950 |
| Rust Windows | 0.007936 | - | 1135.603 | 0.882 |

## vm_dispatch dispatch variants (`ns_per_instruction`, x100)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (switch) | 0.012707 | - | 751.781 | 1.412 |
| Graphion Windows (jumptable) | 0.013436 | - | 702.768 | 1.493 |
| Graphion Linux (switch) | 0.008506 | - | 1058.185 | 0.945 |
| Graphion Linux (jumptable) | 0.008275 | - | 1087.783 | 0.919 |
| Graphion Linux (computed-goto) | 0.008123 | - | 1108.045 | 0.903 |

## bfs_levels (`ns_per_edge`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.007231 | 525.558 | - | 1.903 |
| Graphion Linux | 0.007322 | 524.034 | - | 1.927 |
| Rust Windows | 0.018172 | 209.683 | - | 4.782 |

## neighbor_iteration (`ns_per_neighbor`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001433 | 1682.141 | - | 0.597 |
| Graphion Linux | 0.001537 | 1583.669 | - | 0.640 |
| Rust Windows | 0.000000 | 14136000.000 | - | 0.000 |

Frontier mode notes:

- Graphion Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Graphion Linux: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Rust Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001580 | - | 3798.345 | 0.263 |
| Graphion Linux | 0.001150 | - | 5228.396 | 0.192 |
| Rust Windows | 0.002761 | - | 2174.218 | 0.460 |

## hypergraph_traversal (`ns_per_membership`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005430 | 1330.467 | - | 0.754 |
| Graphion Linux | 0.004625 | 1556.931 | - | 0.642 |
| Rust Windows | 0.000000 | 47160000.000 | - | 0.000 |

## hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.006381 | - | 394.151 | 2.552 |
| Graphion Linux | 0.003958 | - | 632.100 | 1.583 |
| Rust Windows | 0.007936 | - | 315.227 | 3.174 |

## hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005241 | - | 386.169 | 2.620 |
| Graphion Linux | 0.003868 | - | 517.966 | 1.934 |
| Rust Windows | 0.007971 | - | 250.928 | 3.985 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.016768 | - | 187.409 | 5.590 |
| Graphion Linux | 0.014190 | - | 211.489 | 4.730 |
| Rust Windows | 0.023597 | - | 127.150 | 7.866 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the versioned `graphion_rust` benchmark lane when present in the checkout.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
