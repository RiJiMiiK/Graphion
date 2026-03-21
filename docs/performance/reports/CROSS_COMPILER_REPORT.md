# Cross-Compiler Optimization Report

Generated: 2026-03-06T22:57:55.352612+00:00

This report reflects the latest local portable-lane comparison run. It is a governance checkpoint, not an automatic release baseline.

## Policy

| Rule | Decision |
|---|---|
| Canonical lane | Portable configuration only: `GRAPHION_ENABLE_ASM=OFF`, `dispatch=switch`, same corpus profile, same run count, same benchmark iterations. |
| Primary metric | Compare `speedup_x` from official `baseline` vs `PGO` reports; use latency-derived `speedup_x` as the source of truth. |
| Toolchain set | `MSVC` on Windows, `GCC` in Linux Docker, `Clang` in Linux Docker. |
| Status thresholds | `aligned` when worst/best `speedup_x >= 0.90`; `review` when `>= 0.80`; `investigate` otherwise. |
| Interpretation | `MSVC` is a Windows release lane; `GCC` and `Clang` are Linux portable lanes. Treat it as governance, not a pure compiler-only microbenchmark. |

## Lanes

| Lane | Compiler | Platform | ASM | Dispatch | Corpus | Runs | Iterations |
|---|---|---|---|---|---|---:|---:|
| Windows MSVC portable | msvc | Windows-11-10.0.26200-SP0 | off | switch | representative | 1 | 2000 |
| Linux GCC portable | gcc | Linux-6.6.87.2-microsoft-standard-WSL2-x86_64-with-glibc2.39 | off | switch | representative | 1 | 2000 |
| Linux Clang portable | clang | Linux-6.6.87.2-microsoft-standard-WSL2-x86_64-with-glibc2.39 | off | switch | representative | 1 | 2000 |

## Speedup Comparison

| Benchmark | Windows MSVC portable | Linux GCC portable | Linux Clang portable | Best lane | Worst lane | Status |
|---|---|---|---|---|---|---|
| vm_dispatch | 1.870 | 1.118 | 1.450 | Windows MSVC portable (1.870) | Linux GCC portable (1.118) | investigate |
| bfs_levels | 0.899 | 0.674 | 0.520 | Windows MSVC portable (0.899) | Linux Clang portable (0.520) | investigate |
| hypergraph_incidence | 0.543 | 1.606 | 1.505 | Linux GCC portable (1.606) | Windows MSVC portable (0.543) | investigate |
| hypergraph_incident_sum | 1.738 | 1.889 | 1.333 | Linux GCC portable (1.889) | Linux Clang portable (1.333) | investigate |
| hypergraph_hyperedge_node_sum | 1.788 | 1.339 | 1.220 | Windows MSVC portable (1.788) | Linux Clang portable (1.220) | investigate |
| vm_graph_ops | 1.504 | 0.741 | 0.366 | Windows MSVC portable (1.504) | Linux Clang portable (0.366) | investigate |

## Notes

- This report compares optimization effectiveness, not absolute wall-clock ranking between operating systems.
- `speedup_x` is derived from the official `baseline` vs `PGO` latency metric for each workload family.
- If a row is `investigate`, refresh the lane snapshots at higher run counts before making a release decision.
