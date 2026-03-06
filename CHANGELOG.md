# Changelog

All notable changes to this project are documented in this file.

The format follows Keep a Changelog and Semantic Versioning.

## [Unreleased]

### Added
- Merge workflow validation note (test branch `test/merge-flow`).
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
- Action workflow linting (`actionlint`) in CI.
- Markdown link checking workflow.
- Spellcheck workflow for docs.
- License header verification workflow and SPDX header checker script.
- Repository health check workflow and script.
- Contributors auto-sync workflow and generator script.
- Bootstrap scripts for local setup.
- Security contacts + SLA document.
- Maintainer release checklist document.
- README status badges for core workflows.
- Release dry-run workflow for archive simulation on release-related PRs.
- Monthly audit issue workflow and template.
- Action pinning audit script/workflow (report mode) and actions security policy doc.
- Explicit artifact retention windows on uploaded CI artifacts.
- `.mailmap` for contributor identity normalization.
- Ownership policy document.
- Incident postmortem template.
- Support policy document.
- Security email contact added to security docs.
- CSR graph core runtime and BFS kernel with tests.
- Hypergraph incidence runtime with tests.
- VM graph/hypergraph opcodes:
  - `GVM_OP_BFS_LEVELS`
  - `GVM_OP_INCIDENT_COUNT`
  - `GVM_OP_HYPEREDGE_SIZE`
  - `GVM_OP_INCIDENT_SUM`
  - `GVM_OP_HYPEREDGE_NODE_SUM`
- Dedicated graph and hypergraph benchmark binaries.
- Dedicated hypergraph sum benchmark binaries:
  - `graphion_bench_hypergraph_incident_sum`
  - `graphion_bench_hypergraph_hyperedge_node_sum`
- Linux benchmark helper scripts (`scripts/bench/run_linux_bench_*.py`).
- Assembly ABI/register reference doc (`docs/ASM_REGISTERS.md`).
- Performance snapshot doc with x100 benchmark runs (`docs/PERFORMANCE_RESULTS.md`).
- Scripts index and categorized scripts layout (`scripts/README.md`).
- Parser front-end skeleton (`source -> IR`) and IR lowering bridge (`IR -> VM bytecode`) with tests.
- End-to-end parser bridge execution test (`source -> IR -> bytecode -> VM`) and IR v0 contract doc (`docs/IR.md`).
- Cross-toolchain PGO pipeline doc, workflow, and local runner script (`docs/PGO.md`, `scripts/bench/run_pgo_pipeline.py`).
- Dispatch-variant parity runner and extra VM edge-case tests for shape-cache / dispatch semantics.
- Official optimization report generator and report doc for `baseline` vs `PGO`, including per-variant `vm_dispatch` sections.
- Automated rolling performance snapshot tooling for Windows, Docker Linux, dispatch variants, and optional local Rust comparison.
- Unified optimization report refresh flow for Windows + Docker Linux, including Linux `computed-goto` coverage.
- Named PGO corpus profiles and representative-workload policy documentation.
- Scheduled and release-gated PGO smoke policy with trigger-specific artifact retention rules.
- Cross-compiler optimization comparison policy with a dedicated portable-lane governance report for `MSVC`, `GCC`, and `Clang`.

### Changed
- VM arithmetic fastpath refined with halt-terminated specialization.
- VM arithmetic fastpath now includes an initial super-instruction fusion (`ADD` + `ADD` on same destination).
- VM fastpath selection now uses a shape cache on `graphion_vm_load` to avoid repeated candidate scans.
- VM dispatch now supports selectable variants (`switch`, `jumptable`, `computed-goto`) via `GRAPHION_VM_DISPATCH`.
- VM dispatch performance improved and benchmark outputs extended with latency metrics (`ns_per_*`).
- CMake now exposes a two-phase PGO mode (`OFF`, `GENERATE`, `USE`) for MSVC, GCC, and Clang.
- Hypergraph benchmark hot loop optimized for lower overhead.
- Repository scripts reorganized into purpose-based folders:
  - `scripts/bench`
  - `scripts/dev`
  - `scripts/quality`
  - `scripts/repo`
- CI/workflows/docs updated to new script paths.
- Assembly source now marks non-executable stack section (`.note.GNU-stack`).
