# Cross-Compiler Optimization Policy

## Goal

Compare optimization effectiveness across supported toolchain lanes without mixing in assembly-only wins or mismatched benchmark settings.

## Canonical Comparison Lane

Cross-compiler comparison is valid only when all lanes use the same portable configuration:

- `GRAPHION_ENABLE_ASM=OFF`
- `GRAPHION_VM_DISPATCH=switch`
- identical benchmark iterations and run count
- identical `baseline` vs `PGO` workflow
- identical PGO corpus profile

Current governance lanes:

- `Windows MSVC portable`
- `Linux GCC portable`
- `Linux Clang portable`

This is a release-governance view, not a pure compiler microbenchmark. `MSVC` remains a Windows lane, while `GCC` and `Clang` run inside the Linux Docker environment.

## Decision Rules

- compare `speedup_x` from official `baseline` vs `PGO` reports, not raw wall-clock time across operating systems
- classify each benchmark family with `worst_speedup_x / best_speedup_x`
- `aligned`: ratio `>= 0.90`
- `review`: ratio `>= 0.80` and `< 0.90`
- `investigate`: ratio `< 0.80`

## Commands

Refresh the cross-compiler governance report:

```powershell
python scripts/bench/refresh_cross_compiler_report.py --runs 20 --iterations 500000
```

This generates:

- `docs/CROSS_COMPILER_REPORT.md`
- `benchmarks/results/cross_compiler_report_latest.json`

## Review Guidance

- do not compare asm-enabled Linux snapshots against portable compiler lanes
- if a benchmark family is `investigate`, rerun at higher sample counts before treating it as a release blocker
- if only one lane falls behind repeatedly, inspect toolchain-specific flags and code generation before changing the shared runtime
- keep GCC and Clang Docker runs in the same container baseline when updating the report
