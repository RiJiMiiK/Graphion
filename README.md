# Graphion
[![CI](https://github.com/RiJiMiiK/Graphion/actions/workflows/ci.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/ci.yml)
[![CodeQL](https://github.com/RiJiMiiK/Graphion/actions/workflows/codeql.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/codeql.yml)
[![Coverage](https://github.com/RiJiMiiK/Graphion/actions/workflows/coverage.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/coverage.yml)
[![Fuzz Nightly](https://github.com/RiJiMiiK/Graphion/actions/workflows/fuzz-nightly.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/fuzz-nightly.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Graphion is a domain-specific language project focused on graph and hypergraph workloads.

Current scope:
- High-performance interpreter in C.
- Optional hand-tuned assembly for VM hot paths.
- Compiled backend planned later.

## Status

Early stage repository. The current baseline provides:
- Minimal VM/runtime scaffold in C.
- Arena allocator primitive.
- CMake build with strict warnings.
- CI, static checks, and security workflows.
- Unit tests (`ctest`) and benchmark scaffold.
- Fuzzing scaffold for parser/VM robustness.
- Bytecode parser scaffold with fixed-width ISA encoding.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

Quick local dev build (Ninja):

```bash
./scripts/dev/dev_build.sh
```

PowerShell:

```powershell
./scripts/dev/dev_build.ps1
```

Repository bootstrap:

```bash
./scripts/dev/bootstrap.sh
```

PowerShell:

```powershell
./scripts/dev/bootstrap.ps1
```

Enable assembly hot paths (x86_64 only):

```bash
cmake -S . -B build-asm -DGRAPHION_ENABLE_ASM=ON
cmake --build build-asm --config Release
```

PGO pipeline:

```bash
python3 scripts/bench/run_pgo_pipeline.py --build-dir build-pgo -- -G Ninja -DCMAKE_C_COMPILER=clang
```

PowerShell / MSVC:

```powershell
python scripts/bench/run_pgo_pipeline.py --build-dir build-pgo
```

Sanitizer build (Linux/macOS with Clang/GCC):

```bash
cmake -S . -B build-sanitize -DGRAPHION_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure -C Debug
```

Fuzzing target (Clang/libFuzzer):

```bash
cmake -S . -B build-fuzz -G Ninja -DGRAPHION_ENABLE_FUZZING=ON -DCMAKE_C_COMPILER=clang
cmake --build build-fuzz
./build-fuzz/fuzz_vm
```

## Reproducible Dev Environment (Docker)

Build and open a shell:

```bash
docker compose build
docker compose run --rm graphion-dev
```

Inside the container:

```bash
./scripts/dev/dev_build.sh
```

VS Code users can also use the included devcontainer configuration.

## Benchmarks

Run benchmark smoke and produce JSON:

```bash
cmake -S . -B build-bench -G Ninja -DGRAPHION_ENABLE_BENCHMARKS=ON
cmake --build build-bench
python3 scripts/bench/run_bench.py --build-dir build-bench --iterations 500000
```

See [docs/BENCHMARKS.md](docs/BENCHMARKS.md).
Latest comparative snapshot: [docs/PERFORMANCE_RESULTS.md](docs/PERFORMANCE_RESULTS.md).
PGO workflow details: [docs/PGO.md](docs/PGO.md).

Additional graph-core benchmark:

```bash
./build-bench/graphion_bench_bfs 200000
```

Additional hypergraph-core benchmark:

```bash
./build-bench/graphion_bench_hypergraph 500000
```

VM graph-opcode benchmark:

```bash
./build-bench/graphion_bench_vm_graph 300000
```

## Developer Hooks

Enable local pre-commit checks:

```bash
./scripts/dev/setup_hooks.sh
```

PowerShell:

```powershell
./scripts/dev/setup_hooks.ps1
```

## Security

See [SECURITY.md](SECURITY.md) for vulnerability reporting and supported versions.
For assembly-specific safeguards and workflow, see [docs/ASSEMBLY_SAFETY.md](docs/ASSEMBLY_SAFETY.md).
For assembly ABI/register mapping, see [docs/ASM_REGISTERS.md](docs/ASM_REGISTERS.md).
For branch hardening settings, see [docs/BRANCH_PROTECTION.md](docs/BRANCH_PROTECTION.md).
For GitHub Actions security posture, see [docs/ACTIONS_SECURITY.md](docs/ACTIONS_SECURITY.md).
Coverage workflow runs on GitHub Actions and uploads HTML artifacts.
Nightly long fuzzing is scheduled in GitHub Actions.
Security contacts and response targets are documented in [SECURITY_CONTACTS.md](SECURITY_CONTACTS.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
VM instruction encoding and compatibility are documented in [docs/ISA.md](docs/ISA.md).
Git workflow policy is documented in [docs/GIT_WORKFLOW.md](docs/GIT_WORKFLOW.md).

## Support

See [SUPPORT.md](SUPPORT.md) for support and security reporting channels.
Support policy is described in [docs/SUPPORT_POLICY.md](docs/SUPPORT_POLICY.md).

## License

MIT (see [LICENSE](LICENSE)).

