# PGO Pipeline

Graphion supports a two-phase profile-guided optimization workflow for:

- MSVC
- GCC
- Clang

The build is controlled with:

- `GRAPHION_PGO_MODE=OFF|GENERATE|USE`
- `GRAPHION_PGO_PROFILE_DIR=<dir>`

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

The script does:

1. Configure with `GRAPHION_PGO_MODE=GENERATE`
2. Build `Release`
3. Train on the benchmark set
4. Merge Clang `.profraw` files into `graphion.profdata` when needed
5. Reconfigure with `GRAPHION_PGO_MODE=USE`
6. Rebuild and run `ctest`

## Training Set

The current training workload uses:

- `graphion_bench`
- `graphion_bench_bfs`
- `graphion_bench_hypergraph`
- `graphion_bench_vm_graph`

This is intentionally small enough for CI and broad enough to cover:

- arithmetic dispatch
- CSR/BFS graph traversal
- hypergraph incidence traversal
- graph-specific VM opcodes

## CI

GitHub Actions workflow:

- [`.github/workflows/pgo.yml`](../.github/workflows/pgo.yml)

It runs on demand via `workflow_dispatch` and covers:

- `ubuntu-latest` with `gcc`
- `ubuntu-latest` with `clang`
- `windows-latest` with `msvc`

## Notes

- Clang uses source-based profiling and requires `llvm-profdata`.
- GCC uses `-fprofile-generate` / `-fprofile-use`.
- MSVC uses `/GENPROFILE` and `/USEPROFILE` on `Release`.
- MSVC writes `.pgc` / `.pgd` artifacts next to the `Release` binaries; the runner cleans stale files before a new training pass.
- The recommended mode is to reuse the same build directory across both PGO phases.
