# Scripts Layout

This directory is organized by purpose:

- `scripts/bench/`: benchmark runners and comparison tools.
- `scripts/dev/`: local developer bootstrap/build/hooks helpers.
- `scripts/quality/`: local quality and safety checks.
- `scripts/repo/`: repository maintenance automation.

Quick pointers:

- Bench run: `python scripts/bench/run_bench.py --build-dir build-bench --iterations 500000`
- PGO run: `python scripts/bench/run_pgo_pipeline.py --build-dir build-pgo`
- ASM safety: `python scripts/quality/check_asm_safety.py`
- Local gate: `scripts/quality/quality_gate.sh` or `scripts/quality/quality_gate.ps1`
