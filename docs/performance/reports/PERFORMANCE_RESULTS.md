# Performance Snapshot (x100)

This snapshot is generated from the latest local benchmark artifacts on 2026-03-27 19:19:47 UTC.

Benchmark runs use x100 averages with benchmark-specific default iteration counts committed in the bench sources.

Format requested: `s | mteps | mips | ns_per_X`.

For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).

## Environment Metadata

| Lane | Compiler | ASM | CPU | Machine | Git | Runs |
|---|---|---|---|---|---|---:|
| VM Windows / .gion Windows | msvc | off | AMD64 Family 25 Model 68 Stepping 1, AuthenticAMD | AMD64 | c0a6dbd275c9 | 100 |
| VM Linux / .gion Linux | gcc | on | AMD Ryzen 7 7735HS with Radeon Graphics | x86_64 | c0a6dbd275c9 | 100 |
| Rust Windows | rustc | off | AMD64 Family 25 Model 68 Stepping 1, AuthenticAMD | AMD64 | c0a6dbd275c9 | 100 |
| Rust Linux | rustc | off | AMD Ryzen 7 7735HS with Radeon Graphics | x86_64 | c0a6dbd275c9 | 100 |


## frontier_primitives (`ns_per_frontier_item`, iterations=10000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 3.595% | 0.693582 | - | 86.614 | 0.542 |
| VM Linux | 3.086% | 0.894251 | - | 67.155 | 0.699 |
| Rust Windows | 6.725% | 0.885367 | - | 68.053 | 0.692 |
| Rust Linux | 4.116% | 0.895224 | - | 67.128 | 0.699 |

## vm_dispatch (`ns_per_instruction`, iterations=5000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 5.362% | 0.244401 | - | 328.139 | 3.055 |
| VM Linux | 5.175% | 0.252231 | - | 317.912 | 3.153 |
| Rust Windows | 3.625% | 0.343088 | - | 233.460 | 4.289 |
| Rust Linux | 4.037% | 0.359315 | - | 222.988 | 4.491 |

## vm_dispatch dispatch variants (`ns_per_instruction`, iterations=5000000, x100)

| Platform | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| Graphion Windows (switch) | 5.981% | 0.247622 | - | 324.048 | 3.095 |
| Graphion Windows (jumptable) | 6.384% | 0.268091 | - | 299.506 | 3.351 |
| Graphion Linux (switch) | 6.865% | 0.261600 | - | 307.094 | 3.270 |
| Graphion Linux (jumptable) | 5.624% | 0.257122 | - | 312.030 | 3.214 |
| Graphion Linux (computed-goto) | 7.417% | 0.274766 | - | 292.717 | 3.435 |

## bfs_levels (`ns_per_edge`, iterations=5000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 2.752% | 0.251153 | 378.525 | - | 2.644 |
| VM Linux | 4.423% | 0.254997 | 373.231 | - | 2.684 |
| Rust Windows | 4.464% | 0.282696 | 336.661 | - | 2.976 |
| Rust Linux | 6.998% | 0.303209 | 314.449 | - | 3.192 |

## neighbor_iteration (`ns_per_neighbor`, iterations=10000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 3.044% | 2.671923 | 958.111 | - | 1.044 |
| VM Linux | 2.293% | 2.358400 | 1085.482 | - | 0.921 |
| Rust Windows | 4.423% | 4.004608 | 639.264 | - | 1.564 |
| Rust Linux | 2.705% | 3.562752 | 718.546 | - | 1.392 |

Frontier mode notes:

- VM Windows: mode=`dense` frontier_len=96 frontier_neighbor_work=256
- VM Linux: mode=`dense` frontier_len=96 frontier_neighbor_work=256
- Rust Windows: mode=`dense` frontier_len=96 frontier_neighbor_work=256
- Rust Linux: mode=`dense` frontier_len=96 frontier_neighbor_work=256

## weighted_neighbor_sums (`ns_per_edge_data`, iterations=300000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 4.450% | 0.450519 | 5465.468 | - | 0.183 |
| VM Linux | 3.947% | 0.342843 | 7178.380 | - | 0.140 |
| Rust Windows | 5.676% | 0.411005 | 5997.080 | - | 0.167 |
| Rust Linux | 10.147% | 0.446314 | 5555.647 | - | 0.182 |

## hypergraph_incidence (`ns_per_incidence`, iterations=10000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 5.209% | 0.453836 | - | 2120.756 | 0.473 |
| VM Linux | 6.513% | 0.326842 | - | 2937.203 | 0.340 |
| Rust Windows | 4.369% | 0.652090 | - | 1472.190 | 0.679 |
| Rust Linux | 4.240% | 0.603072 | - | 1591.850 | 0.628 |

## hypergraph_traversal (`ns_per_membership`, iterations=10000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 7.437% | 0.273294 | 882.751 | - | 1.139 |
| VM Linux | 4.943% | 0.235041 | 1023.340 | - | 0.979 |
| Rust Windows | 4.063% | 0.398395 | 603.338 | - | 1.660 |
| Rust Linux | 7.056% | 0.465398 | 518.206 | - | 1.939 |

## hypergraph_incident_sum (`ns_per_call`, iterations=10000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 4.421% | 4.163456 | - | 192.148 | 5.204 |
| VM Linux | 2.796% | 1.903504 | - | 420.278 | 2.379 |
| Rust Windows | 7.623% | 4.485552 | - | 178.350 | 5.607 |
| Rust Linux | 2.901% | 3.944840 | - | 202.797 | 4.931 |

## hypergraph_hyperedge_node_sum (`ns_per_call`, iterations=10000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 8.836% | 0.151737 | - | 265.464 | 3.793 |
| VM Linux | 15.499% | 0.121527 | - | 335.499 | 3.038 |
| Rust Windows | 5.242% | 0.169045 | - | 237.257 | 4.226 |
| Rust Linux | 9.356% | 0.239190 | - | 168.702 | 5.980 |

## vm_graph_ops (`ns_per_instruction`, iterations=10000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 5.067% | 0.459278 | - | 261.904 | 3.827 |
| VM Linux | 6.202% | 0.510733 | - | 235.969 | 4.256 |
| Rust Windows | 2.992% | 1.129885 | - | 106.295 | 9.416 |
| Rust Linux | 3.446% | 1.240276 | - | 96.866 | 10.336 |

## gion_source (`ns_per_iteration`, iterations=5000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 5.362% | 0.244401 | - | 328.139 | 50.959 |
| VM Linux | 5.175% | 0.252231 | - | 317.912 | 50.046 |
| .gion Windows | 5.382% | 0.440552 | - | 136.577 | 78.040 |
| .gion Linux | 3.018% | 0.419138 | - | 143.272 | 83.958 |
| Rust Windows | 3.625% | 0.343088 | - | 233.460 | 67.888 |
| Rust Linux | 4.037% | 0.359315 | - | 222.988 | 69.663 |

## vm_print_dispatch (`ns_per_iteration`, iterations=5000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 4.870% | 0.702384 | - | 128.414 | 140.477 |
| VM Linux | 3.261% | 0.441099 | - | 204.238 | 88.220 |
| Rust Windows | 4.932% | 0.903032 | - | 55.499 | 180.606 |
| Rust Linux | 6.262% | 1.009009 | - | 49.726 | 201.802 |

## gion_print_source (`ns_per_iteration`, iterations=5000000)

| Lane | var_% | s | mteps | mips | ns_per_X |
|---|---:|---:|---:|---:|---:|
| VM Windows | 4.870% | 0.702384 | - | 128.414 | 140.477 |
| VM Linux | 3.261% | 0.441099 | - | 204.238 | 88.220 |
| .gion Windows | 2.729% | 0.704129 | - | 120.805 | 140.826 |
| .gion Linux | 2.371% | 0.529982 | - | 160.470 | 105.996 |
| Rust Windows | 4.932% | 0.903032 | - | 55.499 | 180.606 |
| Rust Linux | 6.262% | 1.009009 | - | 49.726 | 201.802 |

Notes:

- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).
- `computed-goto` is expected only on Linux/GCC/Clang paths.
- Rust comparison uses the versioned `graphion_rust` benchmark lane when present in the checkout.
- Numbers vary by CPU governor, thermal state, and host load.
- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.
