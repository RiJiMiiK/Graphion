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
