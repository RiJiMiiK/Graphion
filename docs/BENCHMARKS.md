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

Run hypergraph sum benches directly:

```bash
./build-bench/graphion_bench_hypergraph_incident_sum 500000
./build-bench/graphion_bench_hypergraph_hyperedge_node_sum 500000
```

Dispatch variant study (switch vs jumptable vs computed-goto when supported):

```bash
python3 scripts/bench/compare_dispatch_variants.py --iterations 500000 --runs 20
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

