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
python3 scripts/run_bench.py --build-dir build-bench --iterations 500000
```

Optional local Rust comparison (for private/local sandbox projects):

```bash
python3 scripts/bench_compare_with_rust.py \
  --vm-json benchmarks/results/latest.json \
  --rust-cmd "cargo run --release --manifest-path /absolute/path/to/rust_bench/Cargo.toml"
```

Or with a prepared Rust JSON result:

```bash
python3 scripts/bench_compare_with_rust.py \
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
  "timestamp_utc": "..."
}
```

## Policy

- Keep benchmark input deterministic.
- Run on a stable machine profile when comparing commits.
- Record compiler, flags, and CPU model in benchmark reports.
- Compare against baseline with `scripts/compare_bench.py` in CI.
- Keep allowed regression threshold explicit in workflow config.
- Keep Rust comparisons local/optional; do not commit Rust sandbox projects.
