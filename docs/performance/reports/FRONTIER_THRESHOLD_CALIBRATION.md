# Frontier Threshold Calibration

This report records the current benchmark-backed calibration of Graphion's
frontier mode recommendation thresholds.

## Result

- `GRAPHION_FRONTIER_DENSE_NODE_PERCENT = 15`
- `GRAPHION_FRONTIER_DENSE_EDGE_PERCENT = 28`

These values replace the earlier heuristic pair:

- node threshold: `20%`
- frontier neighbor work threshold: `35%`

## Method

Calibration uses `graphion_bench_frontier_thresholds`, a synthetic mixed-degree
CSR benchmark that measures:

- sparse push cost over the active frontier
- dense node-scan proxy cost for low-degree frontiers
- dense edge-scan proxy cost for high-degree frontiers

The benchmark does not claim that a future dense kernel already outperforms the
current sparse path. Instead, it identifies the point where dense-style proxy
overhead stops being catastrophically mismatched to the active frontier shape.

## Bench Summary

The current calibration run uses:

- `node_count = 1024`
- `edge_count = 13312`
- `iterations = 10000`

Observed knees:

- low-degree frontier proxy overhead reaches its best measured node-scan knee around `15%` of nodes
- high-degree frontier proxy overhead compresses materially around `28%` of total
  edge work

Those knees were adopted as the new runtime recommendation thresholds.

## Interpretation

- below these thresholds, Graphion should continue to prefer `sparse`
- at or above these thresholds, Graphion should recommend `dense`
- this remains a recommendation only; Graphion does not yet ship a second
  frontier backend inside the VM

## Reproduce

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGRAPHION_ENABLE_BENCHMARKS=ON
cmake --build build --config Release --target graphion_bench_frontier_thresholds
.\build\Release\graphion_bench_frontier_thresholds.exe
```
