# Bench Scripts

`scripts/bench/` is organized by role:

- `run/`: simple benchmark entrypoints and one-shot local runners.
- `collect/`: raw JSON collection from Graphion and Rust benchmark binaries.
- `compare/`: direct comparisons, parity checks, and regression gates.
- `render/`: markdown and merged-JSON renderers.
- `refresh/`: orchestration commands that regenerate higher-level benchmark reports.
- `pgo/`: PGO pipeline, corpus, thresholds, profile artifacts, and optimization reporting.
- root modules:
  - `bench_paths.py`: canonical benchmark artifact paths.
  - `report_metadata.py`: shared metadata collection and validation.

Use this directory as the local API surface for benchmark tooling; add new scripts to the narrowest matching subdirectory instead of dropping more entrypoints at the root.
