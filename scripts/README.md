# Scripts Layout

This directory is organized by purpose:

- `scripts/bench/`: benchmark runners and comparison tools.
- `scripts/dev/`: local developer bootstrap/build/hooks helpers.
- `scripts/quality/`: local quality and safety checks.
- `scripts/repo/`: repository maintenance automation.

Quick pointers:

- Bench run: `python scripts/bench/run_bench.py --build-dir build-bench --iterations 500000`
- Perf snapshot: `python scripts/bench/refresh_performance_results.py`
- PGO run: `python scripts/bench/run_pgo_pipeline.py --build-dir build-pgo`
- PGO corpus policy: `docs/PGO_CORPUS_POLICY.md`
- PGO thresholds: `scripts/bench/pgo_thresholds.py`
- PGO release alerts: `python scripts/bench/check_pgo_alerts.py --report-json <path>`
- Optimization report: `python scripts/bench/generate_optimization_report.py --build-root build-opt-report`
- Unified optimization report: `python scripts/bench/refresh_optimization_reports.py --runs 100`
- Dispatch parity: `python scripts/quality/test_dispatch_variants.py --build-root build-dispatch-tests`
- ASM safety: `python scripts/quality/check_asm_safety.py`
- Local gate: `scripts/quality/quality_gate.sh` or `scripts/quality/quality_gate.ps1`
