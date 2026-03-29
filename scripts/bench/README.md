# Bench Scripts

`scripts/bench/` is the local API surface for benchmark tooling.

## Layout

- `run/`
  - direct benchmark entrypoints and one-shot local runners
- `collect/`
  - raw JSON collection from Graphion and Rust benchmark binaries
- `compare/`
  - direct comparisons, parity checks, and regression gates
- `render/`
  - Markdown and merged-JSON renderers
- `refresh/`
  - higher-level orchestration commands that regenerate benchmark reports
- `pgo/`
  - PGO pipeline, thresholds, and optimization-report tooling

Root helpers:

- `bench_paths.py`
  - canonical benchmark artifact paths
- `report_metadata.py`
  - shared metadata collection and validation

## Intent

Use this directory for benchmark automation only.

When adding new tooling:

- prefer the narrowest existing subdirectory
- keep artifact paths aligned with `benchmarks/results/`
- keep generated report behavior consistent with the docs in `docs/performance/`
