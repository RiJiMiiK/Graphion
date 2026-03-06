# Official Optimization Report

Generated: 2026-03-06T21:32:54.415832+00:00

## Scope

- Platforms covered: Graphion Windows (MSVC), Graphion Linux (Docker GCC)
- This report merges local per-platform `baseline` vs `PGO` runs.
- Linux sections are intended to include Docker-based GCC/Clang workflows, including `computed-goto` when supported.

## Baseline vs PGO

| Platform | Benchmark | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline thr | PGO thr | Speedup x |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| Graphion Windows (MSVC) | vm_dispatch | 0.007776 | 0.005897 | 0.864 | 0.655 | 1172.151 | 1530.191 | 1.319 |
| Graphion Windows (MSVC) | bfs_levels | 0.013523 | 0.013590 | 1.423 | 1.430 | 702.921 | 699.233 | 0.995 |
| Graphion Windows (MSVC) | hypergraph_incidence | 0.001443 | 0.001157 | 0.240 | 0.193 | 4345.000 | 5195.364 | 1.244 |
| Graphion Windows (MSVC) | hypergraph_incident_sum | 0.005265 | 0.003097 | 2.106 | 1.239 | 486.238 | 836.402 | 1.700 |
| Graphion Windows (MSVC) | hypergraph_hyperedge_node_sum | 0.003799 | 0.002143 | 1.900 | 1.071 | 528.110 | 945.762 | 1.774 |
| Graphion Windows (MSVC) | vm_graph_ops | 0.018777 | 0.019458 | 3.755 | 3.891 | 270.437 | 259.214 | 0.965 |
| Graphion Linux (Docker GCC) | vm_dispatch | 0.019972 | 0.006289 | 2.219 | 0.699 | 450.658 | 1433.580 | 3.175 |
| Graphion Linux (Docker GCC) | bfs_levels | 0.054625 | 0.014715 | 5.750 | 1.549 | 173.945 | 645.926 | 3.712 |
| Graphion Linux (Docker GCC) | hypergraph_incidence | 0.003371 | 0.000591 | 0.562 | 0.099 | 1781.423 | 10263.058 | 5.677 |
| Graphion Linux (Docker GCC) | hypergraph_incident_sum | 0.017503 | 0.004956 | 7.001 | 1.982 | 142.858 | 525.611 | 3.532 |
| Graphion Linux (Docker GCC) | hypergraph_hyperedge_node_sum | 0.014147 | 0.003692 | 7.074 | 1.846 | 141.389 | 546.739 | 3.832 |
| Graphion Linux (Docker GCC) | vm_graph_ops | 0.064257 | 0.017615 | 12.851 | 3.523 | 77.841 | 283.911 | 3.648 |

## vm_dispatch By Dispatch Variant

### Graphion Windows (MSVC)

| Variant | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline mips | PGO mips | Speedup x | Status |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| switch | 0.007095 | 0.005811 | 0.788 | 0.646 | 1268.648 | 1554.120 | 1.220 | ok |
| jumptable | 0.007400 | 0.006844 | 0.822 | 0.761 | 1216.596 | 1372.681 | 1.080 | ok |
| computed-goto | - | - | - | - | - | - | - | computed-goto is not supported with MSVC/Windows |
### Graphion Linux (Docker GCC)

| Variant | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline mips | PGO mips | Speedup x | Status |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| switch | 0.006806 | 0.006312 | 0.756 | 0.701 | 1323.305 | 1428.880 | 1.078 | ok |
| jumptable | 0.007077 | 0.006499 | 0.786 | 0.722 | 1273.370 | 1386.376 | 1.089 | ok |
| computed-goto | 0.007350 | 0.006623 | 0.817 | 0.736 | 1228.850 | 1363.249 | 1.110 | ok |

## Notes

- Latency columns (`ns_per_X`) are the primary comparison metric; lower is better.
- Throughput columns (`mips` / `mteps`) are secondary confirmation metrics; higher is better.
- `computed-goto` is expected only on non-MSVC paths.
