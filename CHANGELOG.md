# Changelog

All notable changes to this project are documented in this file.

The format follows Keep a Changelog and Semantic Versioning.

## [Unreleased]

### Added
- Unit test suite (`ctest`) for VM and arena.
- Benchmark scaffold with JSON output.
- Fuzzing target scaffold (`fuzz_vm`) for Clang/libFuzzer.
- Pre-commit hooks for asm safety and static checks.
- Supply-chain workflow (SBOM + vulnerability scan).
- Release workflow for semantic version tags.
- Architecture, release, benchmark, and branch-protection docs.
- Coverage workflow producing HTML artifacts.
- Parser scaffold for bytecode decoding (`src/parser/bytecode.*`).
- ISA specification document (`docs/ISA.md`).
- Benchmark regression comparison script and baseline.
- Maintainers and roadmap files.
- Explicit compiler CI matrix (`gcc`, `clang`, `msvc`).
- `clang-tidy` CI job and local runner script.
- Nightly long fuzz workflow.
- Optional local VM vs Rust benchmark comparison script.
- Label sync automation and path-based PR auto-labeling.
- Stale issue/PR automation workflow.
- Semantic PR title enforcement workflow.
- Support/discussion templates and support policy file.
- Git workflow policy documentation (branches, titles, merge rules).
