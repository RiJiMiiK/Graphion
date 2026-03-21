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
python3 scripts/bench/run/run_bench.py --build-dir build-bench --iterations 500000
```

Refresh the rolling performance snapshot doc from local Windows, optional Rust sandbox,
and Docker Linux measurements:

```powershell
python scripts/bench/refresh/refresh_performance_results.py
```

Run hypergraph sum benches directly:

```bash
./build-bench/graphion_bench_hypergraph_incident_sum 500000
./build-bench/graphion_bench_hypergraph_hyperedge_node_sum 500000
```

Dispatch variant study (switch vs jumptable vs computed-goto when supported):

```bash
python3 scripts/bench/compare/compare_dispatch_variants.py --iterations 500000 --runs 20
```

Render `docs/performance/reports/PERFORMANCE_RESULTS.md` from collected JSON artifacts only:

```bash
python3 scripts/bench/render/render_performance_results.py \
  --windows-json benchmarks/results/performance/windows_100x_latest.json \
  --linux-json benchmarks/results/performance/linux_100x_latest.json \
  --rust-json benchmarks/results/performance/rust_100x_latest.json \
  --dispatch-windows-json benchmarks/results/performance/dispatch_variants_windows.json \
  --dispatch-linux-json benchmarks/results/performance/dispatch_variants.json
```

PGO training + optimized rebuild:

```bash
python3 scripts/bench/pgo/run_pgo_pipeline.py --build-dir build-pgo -- -G Ninja -DCMAKE_C_COMPILER=clang
```

MSVC:

```powershell
python scripts/bench/pgo/run_pgo_pipeline.py --build-dir build-pgo
```

The default PGO corpus is `representative`. For CI-style smoke runs:

```bash
python3 scripts/bench/pgo/run_pgo_pipeline.py --build-dir build-pgo --corpus-profile ci --iterations-scale 0.10 -- -G Ninja -DCMAKE_C_COMPILER=clang
```

Official baseline vs PGO report:

```bash
python3 scripts/bench/pgo/generate_optimization_report.py \
  --build-root build-opt-report \
  --output-json benchmarks/results/optimization/optimization_report_latest.json \
  --output-md docs/performance/reports/OPTIMIZATION_REPORTS.md \
  -- -G Ninja -DCMAKE_C_COMPILER=clang
```

MSVC:

```powershell
python scripts/bench/pgo/generate_optimization_report.py --build-root build-opt-report
```

Refresh the unified optimization report with local Windows plus Docker Linux:

```powershell
python scripts/bench/refresh/refresh_optimization_reports.py --runs 100
```

Refresh the portable cross-compiler governance report:

```powershell
python scripts/bench/refresh/refresh_cross_compiler_report.py --runs 20 --iterations 500000
```

Compare the asm hotpath against the C fallback (Linux / Docker):

```bash
python3 scripts/bench/compare/compare_asm_fallback.py \
  --build-root build-asm-fallback \
  --runs 20 \
  --iterations 500000 \
  -- -G Ninja -DCMAKE_C_COMPILER=clang
```

Optional local Rust comparison (for private/local sandbox projects):

```bash
python3 scripts/bench/compare/bench_compare_with_rust.py \
  --vm-json benchmarks/results/performance/latest.json \
  --rust-cmd "cargo run --release --manifest-path /absolute/path/to/rust_bench/Cargo.toml"
```

Or with a prepared Rust JSON result:

```bash
python3 scripts/bench/compare/bench_compare_with_rust.py \
  --vm-json benchmarks/results/performance/latest.json \
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
- Generated reports must include enforced environment metadata (`platform_label`, `platform`, `machine`, `cpu_model`, `hostname`, `python`, `git_rev`, `runs`).
- Toolchain-oriented reports must also include enforced lane metadata such as `compiler_kind`, `asm_enabled`, and relevant build/report parameters.
- Compare against baseline with `scripts/bench/compare/compare_bench.py` in CI.
- Keep allowed regression threshold explicit in workflow config.
- Keep Rust comparisons local/optional; do not commit Rust sandbox projects.
- Keep periodic summarized snapshots in `docs/performance/reports/PERFORMANCE_RESULTS.md`.
- Keep official `baseline` vs `PGO` reports in `docs/performance/reports/OPTIMIZATION_REPORTS.md` and the paired JSON artifact in `benchmarks/results/optimization/`.
- Keep cross-compiler governance snapshots in `docs/performance/reports/CROSS_COMPILER_REPORT.md` using the portable lane only.
