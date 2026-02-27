# Performance Snapshot (x100)

This snapshot records average wall-clock benchmark results over 100 runs
(`500000` iterations per run), measured on February 27, 2026.

Format requested: `s | mteps | mips | ns_per_X`.

## vm_dispatch (`ns_per_instruction`)

| Platform | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|
| Graphion Windows | 0.007837 | - | 1159.835 | 0.871 |
| Graphion Linux | 0.006325 | - | 1434.415 | 0.703 |
| Rust | 0.006592 | - | 1379.886 | 0.732 |

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
| Graphion Windows | 0.014010 | - | 357.266 | 2.802 |
| Graphion Linux | 0.017920 | - | 280.128 | 3.584 |
| Rust | 0.041512 | - | 143.850 | 8.302 |

Notes:

- Linux measurements were taken in Docker (`GRAPHION_ENABLE_ASM=ON`).
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
