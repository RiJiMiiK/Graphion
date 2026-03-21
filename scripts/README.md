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
- PGO corpus policy: `docs/performance/policies/PGO_CORPUS_POLICY.md`
- Cross-compiler policy: `docs/performance/policies/CROSS_COMPILER_POLICY.md`
- PGO artifact manifest: `<build-dir>/pgo-data/profile_manifest.json`
- PGO thresholds: `scripts/bench/pgo_thresholds.py`
- PGO release alerts: `python scripts/bench/check_pgo_alerts.py --report-json <path>`
- ASM fallback compare: `python scripts/bench/compare_asm_fallback.py --build-root build-asm-fallback --runs 20 --iterations 500000 -- -G Ninja -DCMAKE_C_COMPILER=clang`
- ASM hardening parity: `python scripts/quality/test_asm_hardening_parity.py --build-root build-asm-hardening -- -G Ninja -DCMAKE_C_COMPILER=gcc`
- Optimization report: `python scripts/bench/generate_optimization_report.py --build-root build-opt-report`
- Unified optimization report: `python scripts/bench/refresh_optimization_reports.py --runs 100`
- Cross-compiler report: `python scripts/bench/refresh_cross_compiler_report.py --runs 20 --iterations 500000`
- Report metadata is enforced by the bench JSON schema; renderers now reject missing `metadata` blocks.
- Dispatch parity: `python scripts/quality/test_dispatch_variants.py --build-root build-dispatch-tests`
  This includes deterministic-mode VM tests on each supported dispatch variant.
- ASM safety: `python scripts/quality/check_asm_safety.py`
- Local gate: `scripts/quality/quality_gate.sh` or `scripts/quality/quality_gate.ps1`
