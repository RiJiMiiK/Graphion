# Benchmarks

## Goal

Track interpreter performance over time with reproducible measurements.

## Run

Build benchmark binary:

```bash
cmake -S . -B build-bench -G Ninja -DGRAPHION_ENABLE_BENCHMARKS=ON
cmake --build build-bench
```

Run and store JSON:

```bash
python3 scripts/bench/run_bench.py --build-dir build-bench --iterations 500000
```

Refresh the rolling performance snapshot doc from local Windows, optional Rust sandbox,
and Docker Linux measurements:

```powershell
python scripts/bench/refresh_performance_results.py
```

Run hypergraph sum benches directly:

```bash
./build-bench/graphion_bench_hypergraph_incident_sum 500000
./build-bench/graphion_bench_hypergraph_hyperedge_node_sum 500000
```

Dispatch variant study (switch vs jumptable vs computed-goto when supported):

```bash
python3 scripts/bench/compare_dispatch_variants.py --iterations 500000 --runs 20
```

Render `docs/PERFORMANCE_RESULTS.md` from collected JSON artifacts only:

```bash
python3 scripts/bench/render_performance_results.py \
  --windows-json benchmarks/results/windows_100x_latest.json \
  --linux-json benchmarks/results/linux_100x_latest.json \
  --rust-json benchmarks/results/rust_100x_latest.json \
  --dispatch-windows-json benchmarks/results/dispatch_variants_windows.json \
  --dispatch-linux-json benchmarks/results/dispatch_variants.json
```

PGO training + optimized rebuild:

```bash
python3 scripts/bench/run_pgo_pipeline.py --build-dir build-pgo -- -G Ninja -DCMAKE_C_COMPILER=clang
```

MSVC:

```powershell
python scripts/bench/run_pgo_pipeline.py --build-dir build-pgo
```

The default PGO corpus is `representative`. For CI-style smoke runs:

```bash
python3 scripts/bench/run_pgo_pipeline.py --build-dir build-pgo --corpus-profile ci --iterations-scale 0.10 -- -G Ninja -DCMAKE_C_COMPILER=clang
```

Official baseline vs PGO report:

```bash
python3 scripts/bench/generate_optimization_report.py \
  --build-root build-opt-report \
  --output-json benchmarks/results/optimization_report_latest.json \
  --output-md docs/OPTIMIZATION_REPORTS.md \
  -- -G Ninja -DCMAKE_C_COMPILER=clang
```

MSVC:

```powershell
python scripts/bench/generate_optimization_report.py --build-root build-opt-report
```

Refresh the unified optimization report with local Windows plus Docker Linux:

```powershell
python scripts/bench/refresh_optimization_reports.py --runs 100
```

Optional local Rust comparison (for private/local sandbox projects):

```bash
python3 scripts/bench/bench_compare_with_rust.py \
  --vm-json benchmarks/results/latest.json \
  --rust-cmd "cargo run --release --manifest-path /absolute/path/to/rust_bench/Cargo.toml"
```

Or with a prepared Rust JSON result:

```bash
python3 scripts/bench/bench_compare_with_rust.py \
  --vm-json benchmarks/results/latest.json \
  --rust-json /absolute/path/to/rust_result.json
```

Output example:

```json
{
  "benchmark": "vm_dispatch",
  "iterations": 500000,
  "instructions_per_iteration": 18,
  "seconds": 0.123456,
  "mips": 72.941,
  "ns_per_instruction": 13.717,
  "timestamp_utc": "..."
}
```

Interpretation order:
- `seconds`: primary metric (wall-clock speed on the measured workload).
- `ns_per_*`: primary normalized latency metric (`ns_per_instruction`, `ns_per_edge`, `ns_per_incidence`).
- `mips` / `mteps`: throughput indicator, useful for engine efficiency tracking.

## Policy

- Keep benchmark input deterministic.
- Run on a stable machine profile when comparing commits.
- Record compiler, flags, and CPU model in benchmark reports.
- Compare against baseline with `scripts/bench/compare_bench.py` in CI.
- Keep allowed regression threshold explicit in workflow config.
- Keep Rust comparisons local/optional; do not commit Rust sandbox projects.
- Keep periodic summarized snapshots in `docs/PERFORMANCE_RESULTS.md`.
- Keep official `baseline` vs `PGO` reports in `docs/OPTIMIZATION_REPORTS.md` and the paired JSON artifact in `benchmarks/results/`.
