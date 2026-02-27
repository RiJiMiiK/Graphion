# Graphion

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
./scripts/dev_build.sh
```

PowerShell:

```powershell
./scripts/dev_build.ps1
```

Enable assembly hot paths (x86_64 only):

```bash
cmake -S . -B build-asm -DGRAPHION_ENABLE_ASM=ON
cmake --build build-asm --config Release
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
./scripts/dev_build.sh
```

VS Code users can also use the included devcontainer configuration.

## Benchmarks

Run benchmark smoke and produce JSON:

```bash
cmake -S . -B build-bench -G Ninja -DGRAPHION_ENABLE_BENCHMARKS=ON
cmake --build build-bench
python3 scripts/run_bench.py --build-dir build-bench --iterations 500000
```

See [docs/BENCHMARKS.md](docs/BENCHMARKS.md).

## Developer Hooks

Enable local pre-commit checks:

```bash
./scripts/setup_hooks.sh
```

PowerShell:

```powershell
./scripts/setup_hooks.ps1
```

## Security

See [SECURITY.md](SECURITY.md) for vulnerability reporting and supported versions.
For assembly-specific safeguards and workflow, see [docs/ASSEMBLY_SAFETY.md](docs/ASSEMBLY_SAFETY.md).
For branch hardening settings, see [docs/BRANCH_PROTECTION.md](docs/BRANCH_PROTECTION.md).
Coverage workflow runs on GitHub Actions and uploads HTML artifacts.
Nightly long fuzzing is scheduled in GitHub Actions.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
VM instruction encoding and compatibility are documented in [docs/ISA.md](docs/ISA.md).

## License

MIT (see [LICENSE](LICENSE)).
