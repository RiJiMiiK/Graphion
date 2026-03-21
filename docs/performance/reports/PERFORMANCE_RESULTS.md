# Performance Snapshot (x100)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-21 15:59:55 UTC.

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
| Graphion Windows | 0.045531 | - | 39.562 | 1.186 |
| Graphion Linux | 0.036758 | - | 49.010 | 0.957 |
| Rust Windows | 0.105455 | - | 17.090 | 2.746 |

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.009796 | - | 918.792 | 1.088 |
| Graphion Linux | 0.008548 | - | 1053.898 | 0.950 |
| Rust Windows | 0.008882 | - | 1017.508 | 0.987 |

## vm_dispatch dispatch variants (`ns_per_instruction`, x100)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (switch) | 0.009879 | - | 911.402 | 1.098 |
| Graphion Windows (jumptable) | 0.010345 | - | 871.230 | 1.149 |
| Graphion Linux (switch) | 0.008506 | - | 1058.091 | 0.945 |
| Graphion Linux (jumptable) | 0.008227 | - | 1094.075 | 0.914 |
| Graphion Linux (computed-goto) | 0.008143 | - | 1105.258 | 0.905 |

## bfs_levels (`ns_per_edge`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.007265 | 523.075 | - | 1.912 |
| Graphion Linux | 0.007149 | 531.732 | - | 1.881 |
| Rust Windows | 0.019071 | 199.319 | - | 5.019 |

## neighbor_iteration (`ns_per_neighbor`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001432 | 1677.789 | - | 0.596 |
| Graphion Linux | 0.001576 | 1556.523 | - | 0.657 |
| Rust Windows | 0.000000 | 20600000.000 | - | 0.000 |

Frontier mode notes:

- Graphion Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Graphion Linux: mode=`dense` frontier_len=3 frontier_neighbor_work=8
- Rust Windows: mode=`dense` frontier_len=3 frontier_neighbor_work=8

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001597 | - | 3759.220 | 0.266 |
| Graphion Linux | 0.001149 | - | 5239.419 | 0.191 |
| Rust Windows | 0.002570 | - | 2351.839 | 0.428 |

## hypergraph_traversal (`ns_per_membership`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005516 | 1315.267 | - | 0.766 |
| Graphion Linux | 0.004631 | 1555.177 | - | 0.643 |
| Rust Windows | 0.000000 | 61920000.000 | - | 0.000 |

## hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.006320 | - | 395.660 | 2.528 |
| Graphion Linux | 0.003937 | - | 635.096 | 1.575 |
| Rust Windows | 0.007004 | - | 357.833 | 2.802 |

## hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.005066 | - | 394.853 | 2.533 |
| Graphion Linux | 0.003875 | - | 517.425 | 1.937 |
| Rust Windows | 0.007238 | - | 276.362 | 3.619 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.014234 | - | 210.839 | 4.745 |
| Graphion Linux | 0.014158 | - | 211.903 | 4.720 |
| Rust Windows | 0.025432 | - | 118.013 | 8.477 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the versioned `graphion_rust` benchmark lane when present in the checkout.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
