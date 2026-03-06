# Official Optimization Report

Generated: 2026-03-06T21:08:55.208954+00:00

## Metadata

- Platform: Windows-11-10.0.26200-SP0
- Compiler: msvc
- Config: Release
- Iterations per benchmark run: 500000
- Averaging runs: 100
- PGO training scale: 1.0
- Main suite dispatch: switch
- Dispatch variants checked: switch, jumptable, computed-goto

## Summary

- PGO improved 3/6 benchmark families on this snapshot.
- The comparison set covers `vm_dispatch`, `bfs_levels`, `vm_graph_ops`, and all current `hypergraph_*` benches.

## Baseline vs PGO

| Benchmark | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline thr | PGO thr | Speedup x |
|---|---:|---:|---:|---:|---:|---:|---:|
| vm_dispatch | 0.007323 | 0.005672 | 0.814 | 0.630 | 1233.392 | 1596.464 | 1.292 |
| bfs_levels | 0.013156 | 0.013900 | 1.385 | 1.463 | 722.831 | 685.537 | 0.947 |
| hypergraph_incidence | 0.001194 | 0.001203 | 0.199 | 0.201 | 5088.241 | 5052.606 | 0.990 |
| hypergraph_incident_sum | 0.004710 | 0.002962 | 1.884 | 1.185 | 536.347 | 860.539 | 1.590 |
| hypergraph_hyperedge_node_sum | 0.003780 | 0.002145 | 1.890 | 1.073 | 535.634 | 958.963 | 1.761 |
| vm_graph_ops | 0.017037 | 0.017566 | 3.407 | 3.513 | 295.929 | 285.322 | 0.970 |

## vm_dispatch By Dispatch Variant

| Variant | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline mips | PGO mips | Speedup x | Status |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| switch | 0.007402 | 0.005686 | 0.822 | 0.632 | 1220.857 | 1585.896 | 1.301 | ok |
| jumptable | 0.007606 | 0.005712 | 0.845 | 0.635 | 1188.649 | 1581.476 | 1.331 | ok |
| computed-goto | - | - | - | - | - | - | - | computed-goto is not supported with MSVC/Windows |

## Notes

- Latency columns (`ns_per_X`) are the primary comparison metric; lower is better.
- Throughput columns (`mips` / `mteps`) are secondary confirmation metrics; higher is better.
- PGO training uses the committed Graphion benchmark set before rebuilding in `USE` mode.
