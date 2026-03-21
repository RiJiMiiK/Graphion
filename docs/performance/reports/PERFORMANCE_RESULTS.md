# Performance Snapshot (x100)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-21 15:49:29 UTC.

Benchmark runs use x100 averages with benchmark-specific default iteration counts committed in the bench sources.

Format requested: `s | mteps | mips | ns_per_X`.

For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).

## Environment Metadata

| Lane | Compiler | ASM | CPU | Machine | Git | Runs |
|---|---|---|---|---|---|---:|
| Graphion Windows | msvc | off | AMD64 Family 25 Model 1 Stepping 1, AuthenticAMD | AMD64 | afd7bb746737 | 100 |
| Graphion Linux | gcc | on | AMD EPYC 7763 64-Core Processor | x86_64 | afd7bb746737 | 100 |
| Rust Windows | rustc | off | AMD64 Family 25 Model 1 Stepping 1, AuthenticAMD | AMD64 | afd7bb746737 | 100 |


## frontier_primitives (`ns_per_frontier_item`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.045785 | - | 39.361 | 1.192 |
| Graphion Linux | 0.037098 | - | 48.674 | 0.966 |
| Rust Windows | 0.105484 | - | 17.084 | 2.747 |

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.009773 | - | 920.976 | 1.086 |
| Graphion Linux | 0.008521 | - | 1056.254 | 0.947 |
| Rust Windows | 0.008773 | - | 1027.666 | 0.975 |

## vm_dispatch dispatch variants (`ns_per_instruction`, x100)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (switch) | 0.009816 | - | 917.694 | 1.091 |
| Graphion Windows (jumptable) | 0.010282 | - | 875.599 | 1.142 |
| Graphion Linux (switch) | 0.008516 | - | 1056.850 | 0.946 |
| Graphion Linux (jumptable) | 0.008273 | - | 1087.981 | 0.919 |
| Graphion Linux (computed-goto) | 0.008136 | - | 1106.309 | 0.904 |

## bfs_levels (`ns_per_edge`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.007242 | 524.922 | - | 1.906 |
| Graphion Linux | 0.007108 | 534.619 | - | 1.870 |
| Rust Windows | 0.019493 | 196.246 | - | 5.130 |

## neighbor_iteration (`ns_per_neighbor`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001424 | 1686.862 | - | 0.593 |
| Graphion Linux | 0.001577 | 1555.029 | - | 0.657 |
| Rust Windows | 0.000000 | 19040000.000 | - | 0.000 |

Frontier mode notes:

- Graphion Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Graphion Linux: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Rust Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001593 | - | 3783.039 | 0.265 |
| Graphion Linux | 0.001147 | - | 5245.070 | 0.191 |
| Rust Windows | 0.002529 | - | 2372.554 | 0.422 |

## hypergraph_traversal (`ns_per_membership`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005399 | 1334.266 | - | 0.750 |
| Graphion Linux | 0.004608 | 1562.956 | - | 0.640 |
| Rust Windows | 0.000000 | 55500000.000 | - | 0.000 |

## hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.006305 | - | 396.611 | 2.522 |
| Graphion Linux | 0.004028 | - | 626.552 | 1.611 |
| Rust Windows | 0.006958 | - | 359.406 | 2.783 |

## hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005108 | - | 393.329 | 2.554 |
| Graphion Linux | 0.003918 | - | 512.446 | 1.959 |
| Rust Windows | 0.007257 | - | 275.645 | 3.628 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.014181 | - | 211.610 | 4.727 |
| Graphion Linux | 0.014352 | - | 210.264 | 4.784 |
| Rust Windows | 0.025523 | - | 117.599 | 8.508 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the versioned `graphion_rust` benchmark lane when present in the checkout.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
