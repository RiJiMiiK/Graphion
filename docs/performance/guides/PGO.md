# PGO

## What this page is for

This page describes how Graphion currently runs profile-guided optimization builds.

It is intentionally practical:

- how to run PGO locally
- what the pipeline does
- where artifacts end up

It is not a policy archive.

## Local run

Recommended entrypoints:

```bash
python3 scripts/bench/pgo/run_pgo_pipeline.py --build-dir build-pgo -- -G Ninja -DCMAKE_C_COMPILER=clang
```

```bash
python3 scripts/bench/pgo/run_pgo_pipeline.py --build-dir build-pgo -- -G Ninja -DCMAKE_C_COMPILER=gcc
```

```powershell
python scripts/bench/pgo/run_pgo_pipeline.py --build-dir build-pgo
```

## Modes

The build uses:

- `GRAPHION_PGO_MODE=OFF`
- `GRAPHION_PGO_MODE=GENERATE`
- `GRAPHION_PGO_MODE=USE`

Relevant knobs:

- `GRAPHION_PGO_PROFILE_DIR=<dir>`
- `--corpus-profile representative|ci`

## What the pipeline does

The current pipeline:

1. configures a `GENERATE` build
2. builds the project
3. trains on the benchmark corpus
4. merges raw profiling data when the toolchain requires it
5. reconfigures with `USE`
6. rebuilds and runs tests

## Corpus profiles

The default corpus profile is:

- `representative`

There is also a lighter profile for smoke-style runs:

- `ci`

The corpus should remain:

- small enough to run routinely
- representative enough to exercise real hot paths
- versioned with the repo rather than treated as an undocumented local habit

## Artifacts

PGO-related result artifacts are written under:

- `benchmarks/results/optimization/`

Cross-compiler result artifacts are written under:

- `benchmarks/results/cross-compiler/`

Generated profile data is kept under the configured profile directory, and the pipeline records a manifest so invalidation is explicit.

## CI

Workflow:

- `.github/workflows/pgo.yml`

Current CI use is mainly for:

- periodic smoke validation
- release-oriented confidence checks
- manual dispatch when we want a controlled run

## Toolchain notes

- Clang uses source-based profiling and requires `llvm-profdata`
- GCC uses `-fprofile-generate` / `-fprofile-use`
- MSVC uses `/GENPROFILE` / `/USEPROFILE`

## Recommended usage

Use PGO when:

- you are validating a performance-sensitive change
- you want a realistic optimized build
- you are checking whether a VM or `.gion` lane still behaves after profile-guided optimization

Do not use PGO results as a substitute for:

- functional validation
- test coverage
- representative benchmark interpretation
