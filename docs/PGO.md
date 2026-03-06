# PGO Pipeline

Graphion supports a two-phase profile-guided optimization workflow for:

- MSVC
- GCC
- Clang

The build is controlled with:

- `GRAPHION_PGO_MODE=OFF|GENERATE|USE`
- `GRAPHION_PGO_PROFILE_DIR=<dir>`
- `--corpus-profile representative|ci`

## Local Run

Recommended entrypoint:

```bash
python3 scripts/bench/run_pgo_pipeline.py --build-dir build-pgo -- -G Ninja -DCMAKE_C_COMPILER=clang
```

GCC:

```bash
python3 scripts/bench/run_pgo_pipeline.py --build-dir build-pgo -- -G Ninja -DCMAKE_C_COMPILER=gcc
```

MSVC:

```powershell
python scripts/bench/run_pgo_pipeline.py --build-dir build-pgo
```

The default corpus profile is `representative`.

The script does:

1. Configure with `GRAPHION_PGO_MODE=GENERATE`
2. Build `Release`
3. Train on the benchmark set
4. Merge Clang `.profraw` files into `graphion.profdata` when needed
5. Reconfigure with `GRAPHION_PGO_MODE=USE`
6. Rebuild and run `ctest`

## Training Corpus

Graphion now uses named corpus profiles instead of an implicit hardcoded list.

The default `representative` profile covers:

- arithmetic dispatch
- CSR/BFS graph traversal
- hypergraph incidence traversal
- hypergraph reducer paths
- graph-specific VM opcodes

The detailed policy is documented in [PGO_CORPUS_POLICY.md](./PGO_CORPUS_POLICY.md).

PGO effectiveness thresholds are reported per workload family in the generated optimization report.
They are review guidance for optimization quality, not a standalone release gate.

Generated PGO profiles are treated as single-run artifacts and are reset before each new `GENERATE` phase.
The profile directory records a `profile_manifest.json` so invalidation reasons are explicit.

## CI

GitHub Actions workflow:

- [`.github/workflows/pgo.yml`](../.github/workflows/pgo.yml)

It runs in three modes:

- `workflow_dispatch`
- scheduled weekly smoke
- pull requests to `main` that touch release-gating or PGO-policy files

It covers:

- `ubuntu-latest` with `gcc`
- `ubuntu-latest` with `clang`
- `windows-latest` with `msvc`

Policy by trigger:

- scheduled weekly smoke
  - corpus: `ci`
  - iteration scale: `0.10`
  - artifact retention: `7 days`
- release-gated or PGO-policy pull request
  - corpus: `ci`
  - iteration scale: `0.10`
  - artifact retention: `21 days`
- manual dispatch
  - corpus: `representative`
  - iteration scale: `0.25`
  - artifact retention: `14 days`

The `pull_request` gate is intentionally path-scoped to release and PGO workflow files so the smoke job is not attached to unrelated feature work.

For a unified engineering report that merges local Windows and Docker Linux optimization results,
use `scripts/bench/refresh_optimization_reports.py`.

Release-related pull requests also run a small clang-based release-candidate smoke report and evaluate
the PGO/non-PGO alert policy before the dry-run archive job is considered complete.

## Notes

- Clang uses source-based profiling and requires `llvm-profdata`.
- GCC uses `-fprofile-generate` / `-fprofile-use`.
- MSVC uses `/GENPROFILE` and `/USEPROFILE` on `Release`.
- MSVC writes `.pgc` / `.pgd` artifacts next to the `Release` binaries; the runner cleans stale files before a new training pass.
- The recommended mode is to reuse the same build directory across both PGO phases.
