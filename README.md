# Graphion
[![CI](https://github.com/RiJiMiiK/Graphion/actions/workflows/ci.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/ci.yml)
[![CodeQL](https://github.com/RiJiMiiK/Graphion/actions/workflows/codeql.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/codeql.yml)
[![Coverage](https://github.com/RiJiMiiK/Graphion/actions/workflows/coverage.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/coverage.yml)
[![Fuzz Nightly](https://github.com/RiJiMiiK/Graphion/actions/workflows/fuzz-nightly.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/fuzz-nightly.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Graphion is a language project built around a `.gion` source pipeline and a VM backend.

Today, the active language surface is the scalar subset:

- assignment
- `print(...)`
- arithmetic expressions
- grouped expressions with parentheses
- compound assignments
- `abs(...)`

## Current direction

The rebuild is guided by one target pipeline:

- `source Graphion -> tokens/parsing -> internal representation -> bytecode -> VM`

The project also contains broader VM work for graph and hypergraph execution, but the user-facing `.gion` language is currently documented only for the subset that is actually implemented.

## Quick start

Configure, build, and test:

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

Run the main executable on the current sample:

```powershell
.\build\Release\graphion.exe .\examples\sample_test.gion
```

## Developer helpers

Quick local dev build:

```bash
./scripts/dev/dev_build.sh
```

```powershell
./scripts/dev/dev_build.ps1
```

Repository bootstrap:

```bash
./scripts/dev/bootstrap.sh
```

```powershell
./scripts/dev/bootstrap.ps1
```

Enable local hooks:

```bash
./scripts/dev/setup_hooks.sh
```

```powershell
./scripts/dev/setup_hooks.ps1
```

## Benchmarks

Build benchmark targets:

```bash
cmake -S . -B build-bench -G Ninja -DGRAPHION_ENABLE_BENCHMARKS=ON
cmake --build build-bench
```

Collect local benchmark JSON:

```bash
python3 scripts/bench/run/run_bench.py --build-dir build-bench --iterations 500000
```

Key benchmark docs:

- [docs/performance/guides/BENCHMARKS.md](docs/performance/guides/BENCHMARKS.md)
- [docs/performance/guides/PGO.md](docs/performance/guides/PGO.md)
- [docs/performance/reports/PERFORMANCE_RESULTS.md](docs/performance/reports/PERFORMANCE_RESULTS.md)

Current scalar-language tracking uses the `scalar_values_print` workload.

## Documentation

Primary documentation entry points:

- user docs: [docs/graphion/index.md](docs/graphion/index.md)
- site home: [docs/index.md](docs/index.md)
- architecture: [docs/runtime/core/ARCHITECTURE.md](docs/runtime/core/ARCHITECTURE.md)
- ISA: [docs/runtime/core/ISA.md](docs/runtime/core/ISA.md)
- errors: [docs/runtime/debugging/ERRORS.md](docs/runtime/debugging/ERRORS.md)
- rebuild charter: [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md)

Generate the HTML site locally:

```powershell
python -m sphinx -b html docs docs/_build/html
```

## Contact

Use the Discord server for questions, support, and private contact:

- https://discord.gg/mPzDQ7TYkj

## License

MIT (see [LICENSE](LICENSE)).
