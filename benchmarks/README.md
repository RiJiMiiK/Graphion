# Benchmarks

This directory holds benchmark sources plus local result artifacts.

## Source layout

- benchmark source files live directly under `benchmarks/`
- result artifacts live under `benchmarks/results/`

## Result directories

- `benchmarks/results/performance/`
  - rolling performance snapshots and lane JSON
- `benchmarks/results/optimization/`
  - PGO and optimization comparison artifacts
- `benchmarks/results/cross-compiler/`
  - cross-toolchain comparison artifacts
- `benchmarks/results/asm/`
  - asm-vs-C comparison and parity artifacts
- `benchmarks/results/release/`
  - release-oriented artifacts
- `benchmarks/results/smoke/`
  - temporary smoke outputs

`benchmarks/results/` is intentionally gitignored.

## Current benchmark reading

The important split today is:

- VM-oriented lanes
- source-level `.gion` lanes

The current scalar-language rebuild especially relies on:

- `vm_dispatch`
- `scalar_values_print`

Those are the lanes to watch when we compare:

- backend quality
- source-level overhead
- Rust parity

## Reference docs

- `docs/performance/guides/BENCHMARKS.md`
- `docs/performance/guides/PGO.md`
- `docs/performance/reports/PERFORMANCE_RESULTS.md`
