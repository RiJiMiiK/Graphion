# Official Optimization Report

Generated: 2026-03-06T22:18:09.471287+00:00

## Metadata

- Platform label: Windows-11-10.0.26200-SP0
- Platform: Windows-11-10.0.26200-SP0
- Compiler: msvc
- Config: Release
- Iterations per benchmark run: 2000
- Averaging runs: 1
- PGO training scale: 0.01
- PGO corpus profile: representative
- PGO corpus coverage classes: vm, csr, hypergraph, vm-graph
- PGO training targets: graphion_bench, graphion_bench_bfs, graphion_bench_hypergraph, graphion_bench_hypergraph_incident_sum, graphion_bench_hypergraph_hyperedge_node_sum, graphion_bench_vm_graph
- Main suite dispatch: switch
- Dispatch variants checked: switch, jumptable, computed-goto

## Summary

- PGO improved 3/6 benchmark families on this snapshot.
- Threshold coverage: 3/6 met minimum and 3/6 met target.
- The comparison set covers `vm_dispatch`, `bfs_levels`, `vm_graph_ops`, and all current `hypergraph_*` benches.

## PGO Effectiveness Thresholds

| Benchmark | Family | Minimum x | Target x | Rationale |
|---|---|---:|---:|---|
| bfs_levels | csr_bfs | 0.980 | 1.050 | PGO should not regress traversal-heavy kernels; gains are welcome but not mandatory. |
| hypergraph_hyperedge_node_sum | hypergraph_reducer | 1.050 | 1.150 | Reducer-style hot loops are expected to benefit clearly from PGO. |
| hypergraph_incidence | hypergraph_traversal | 1.030 | 1.100 | Traversal-heavy hypergraph kernels should see at least modest improvement. |
| hypergraph_incident_sum | hypergraph_reducer | 1.050 | 1.150 | Reducer-style hot loops are expected to benefit clearly from PGO. |
| vm_dispatch | vm_dispatch | 1.050 | 1.150 | PGO should materially improve interpreter dispatch hotpaths. |
| vm_graph_ops | vm_graph_ops | 1.000 | 1.080 | Graph-specific opcode dispatch should at least break even under PGO. |

## Baseline vs PGO

| Benchmark | Family | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline thr | PGO thr | Speedup x | Min x | Target x | Status |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| vm_dispatch | vm_dispatch | 0.000030 | 0.000022 | 0.828 | 0.603 | 1207.960 | 1659.285 | 1.373 | 1.050 | 1.150 | meets-target |
| bfs_levels | csr_bfs | 0.000051 | 0.000055 | 1.355 | 1.449 | 737.887 | 689.972 | 0.935 | 0.980 | 1.050 | below-minimum |
| hypergraph_incidence | hypergraph_traversal | 0.000005 | 0.000005 | 0.189 | 0.209 | 5298.068 | 4793.490 | 0.904 | 1.030 | 1.100 | below-minimum |
| hypergraph_incident_sum | hypergraph_reducer | 0.000027 | 0.000010 | 2.718 | 0.978 | 367.921 | 1023.001 | 2.779 | 1.050 | 1.150 | meets-target |
| hypergraph_hyperedge_node_sum | hypergraph_reducer | 0.000014 | 0.000008 | 1.788 | 0.954 | 559.241 | 1048.576 | 1.874 | 1.050 | 1.150 | meets-target |
| vm_graph_ops | vm_graph_ops | 0.000065 | 0.000101 | 3.254 | 5.066 | 307.275 | 197.379 | 0.642 | 1.000 | 1.080 | below-minimum |

## vm_dispatch By Dispatch Variant

| Variant | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline mips | PGO mips | Speedup x | Status |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| switch | 0.000027 | 0.000021 | 0.755 | 0.583 | 1324.517 | 1715.852 | 1.295 | ok |
| jumptable | 0.000029 | 0.000021 | 0.815 | 0.596 | 1227.601 | 1677.722 | 1.367 | ok |
| computed-goto | - | - | - | - | - | - | - | computed-goto is not supported with MSVC/Windows |

## Notes

- Latency columns (`ns_per_X`) are the primary comparison metric; lower is better.
- Throughput columns (`mips` / `mteps`) are secondary confirmation metrics; higher is better.
- PGO training uses the committed Graphion representative-workload policy before rebuilding in `USE` mode.
- Threshold status is advisory governance for optimization review; it is not yet a release blocker by itself.
