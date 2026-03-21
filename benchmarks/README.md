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
