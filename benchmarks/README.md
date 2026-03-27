# Benchmarks

`benchmarks/` contains the executable benchmark sources, fixed baselines, and local generated result artifacts.

Layout:
- `benchmarks/baselines/`: checked-in reference thresholds used by quality checks.
- `benchmarks/results/performance/`: raw benchmark snapshots and dispatch comparison JSON outputs.
- `benchmarks/results/optimization/`: baseline-vs-PGO JSON reports.
- `benchmarks/results/cross-compiler/`: MSVC/GCC/Clang comparison JSON and temporary markdown outputs.
- `benchmarks/results/asm/`: assembly fallback and hardening parity artifacts.
- `benchmarks/results/release/`: release-dry-run artifacts.
- `benchmarks/results/smoke/`: metadata smoke outputs and temporary merged-report scratch files.

`benchmarks/results/` is intentionally gitignored. The structure is still documented here so scripts and local runs converge on the same paths.

Benchmark scope is intentionally split:
- some binaries measure raw kernels directly (`bench_bfs.c`, `bench_hypergraph*.c`)
- some binaries measure VM execution (`bench_vm.c`, `bench_vm_graph.c`)

`bench_vm.c` now tracks the first typed-value VM baseline:
- it exercises `GVM_OP_LOAD_CONST`
- it exercises `GVM_OP_STORE_GLOBAL`
- it exercises `GVM_OP_LOAD_GLOBAL`
- it exercises `GVM_OP_MOV`
- it keeps one integer `ADD` in the mix so typed arithmetic still stays measured

`bench_vm_graph.c` is the coherence check for the current `.gion -> VM` backend path:
- it exercises `GVM_OP_BFS_LEVEL_COUNT`
- it exercises `GVM_OP_BFS_ORDER`
- it exercises `GVM_OP_INCIDENT_COUNT`
- it exercises `GVM_OP_INCIDENT_SUM`

`bench_gion.c` is the checked-in `.gion` source lane:
- it interprets the checked-in workload in `benchmarks/workloads/values.gion`
- it measures the end-to-end `.gion` cost separately from the direct VM lane
- it is collected into the standard performance snapshot/reporting pipeline

`bench_vm_print.c` is the direct VM print lane for the current top-level scalar `print(...)` subset:
- it exercises `GVM_OP_STORE_CONST_GLOBAL`
- it exercises `GVM_OP_PRINT_GLOBAL`
- it exercises `GVM_OP_PRINT_CONST`
- it writes to a null sink so the lane measures the VM print path rather than terminal rendering

`bench_gion_print.c` is the checked-in `.gion` print lane:
- it interprets the checked-in workload in `benchmarks/workloads/print_values.gion`
- it measures the prepared `.gion -> bytecode -> VM` path for top-level scalar `print(...)`
- it is collected into the standard performance snapshot/reporting pipeline as `gion_print_source`

`bench_vm_expr.c` is the direct VM lane for the current top-level integer expression subset:
- it loads the prepared VM program generated from `benchmarks/workloads/expr_values.gion`
- it measures the direct VM cost of top-level integer additions plus simple `print(...)`
- it is collected into the standard performance snapshot/reporting pipeline as `vm_expr_dispatch`

`bench_gion_expr.c` is the checked-in `.gion` expression lane:
- it interprets the checked-in workload in `benchmarks/workloads/expr_values.gion`
- it measures the prepared `.gion -> bytecode -> VM` path for top-level integer additions
- it is collected into the standard performance snapshot/reporting pipeline as `gion_expr_source`
