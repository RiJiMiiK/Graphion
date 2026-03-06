# Performance Snapshot (x100)

This snapshot records average wall-clock benchmark results over 100 runs
(`500000` iterations per run), measured on February 27, 2026.

Format requested: `s | mteps | mips | ns_per_X`.

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.006540 | - | 1377.279 | 0.727 |
| Graphion Linux | 0.006585 | - | 1371.265 | 0.732 |
| Rust Windows | 0.006275 | - | 1435.327 | 0.697 |
| Rust Linux | 0.006425 | - | 1403.331 | 0.714 |

## vm_dispatch dispatch variants (`ns_per_instruction`, x100)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (switch) | 0.007001 | - | 1286.439 | 0.778 |
| Graphion Windows (jumptable) | 0.007407 | - | 1216.154 | 0.823 |
| Graphion Linux (switch) | 0.007306 | - | 1235.826 | 0.812 |
| Graphion Linux (jumptable) | 0.006093 | - | 1480.431 | 0.677 |
| Graphion Linux (computed-goto) | 0.005980 | - | 1510.238 | 0.665 |
| Rust Windows (baseline) | 0.006469 | - | 1396.877 | 0.719 |

## fastpath shape cache checkpoint (`vm_dispatch`, x100)

The bytecode-shape cache targets `graphion_vm_load`, not the steady-state dispatch loop in
`graphion_vm_run`. The benchmark below is therefore used as a regression check: the optimization
should stay performance-neutral on `vm_dispatch`.

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows (shape cache enabled) | 0.006172 | - | 1460.188 | 0.686 |
| Graphion Linux (shape cache-neutral reference) | 0.006585 | - | 1371.265 | 0.732 |
| Rust Windows (reference) | 0.006592 | - | 1379.886 | 0.733 |

## bfs_levels (`ns_per_edge`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.012864 | 738.672 | - | 1.354 |
| Graphion Linux | 0.012969 | 733.779 | - | 1.365 |
| Rust | 0.044667 | 247.537 | - | 4.702 |

## hypergraph_incidence (`ns_per_incidence`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.001116 | - | 5387.278 | 0.186 |
| Graphion Linux | 0.000874 | - | 6970.889 | 0.146 |
| Rust | 0.001447 | - | 4151.426 | 0.241 |

## vm_graph_ops (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.008430 | - | 356.186 | 2.810 |
| Graphion Linux | 0.018138 | - | 276.201 | 3.628 |
| Rust | 0.049152 | - | 76.209 | 16.384 |

Notes:

- Linux measurements were taken in Docker (`GRAPHION_ENABLE_ASM=ON`).
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.

## Additional Snapshot (x100, API-equivalent)

This additional checkpoint records averages over 100 runs, focused on API-equivalent
comparisons for the new hypergraph sum operations.

### hypergraph_incident_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.004977 | - | 508.110 | 1.991 |
| Graphion Linux | 0.003284 | - | 793.314 | 1.314 |
| Rust | 0.004848 | - | 532.562 | 1.939 |

### hypergraph_hyperedge_node_sum (`ns_per_call`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.003867 | - | 523.752 | 1.934 |
| Graphion Linux | 0.003005 | - | 672.239 | 1.503 |
| Rust | 0.005489 | - | 366.782 | 2.745 |
