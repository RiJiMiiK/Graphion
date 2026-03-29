# Scripts

This directory is organized by purpose:

- `scripts/bench/`
  - benchmark runners, collectors, renderers, refresh flows, and PGO helpers
- `scripts/dev/`
  - local bootstrap, build, and hook helpers
- `scripts/quality/`
  - local quality and safety checks
- `scripts/repo/`
  - repository maintenance helpers

## Common entry points

- bench run:
  - `python scripts/bench/run/run_bench.py --build-dir build-bench --iterations 500000`
- rolling performance snapshot:
  - `python scripts/bench/refresh/refresh_performance_results.py`
- PGO pipeline:
  - `python scripts/bench/pgo/run_pgo_pipeline.py --build-dir build-pgo`
- quality gate:
  - `scripts/quality/quality_gate.sh`
  - `scripts/quality/quality_gate.ps1`

## Notes

- generated benchmark artifacts are written under `benchmarks/results/`
- benchmark metadata is validated by the shared reporting helpers
- if you add a new script, put it in the narrowest matching subdirectory instead of growing the root
