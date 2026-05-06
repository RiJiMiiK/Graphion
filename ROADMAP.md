# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

This roadmap is temporarily focused on project and code hygiene after the `feat/gion-arithmetic` lane.
Everything above the final future section belongs to the active maintenance lane.
Future language additions stay isolated at the end for traceability and are not part of the current execution lane.

## Current focus

The current active lane is hygiene, maintainability, and project health.

## Immediate problems

### Build health and portability

- [x] restore a clean `-Werror` build on the Windows GCC / MinGW toolchain
- [x] add a regression check for the scalar builtin `isinf` / `isfinite` warning path
- [x] decide explicitly whether Windows GCC / MinGW is a supported toolchain
- [x] if Windows GCC / MinGW is not supported, document the supported Windows toolchain clearly

### Safety and correctness

- [x] add the missing invalid-register guard in the affected scalar builtin path
- [x] add a VM regression test that the affected builtin returns `GVM_ERR_INVALID_REG` on malformed bytecode
- [x] review neighboring scalar builtin opcodes for similar defensive-check gaps
- [x] keep malformed-bytecode behavior consistent across scalar, graph, and hypergraph opcode families
- [x] replace the pseudo-private Discord security contact path with a genuinely private reporting channel or explicitly downgrade the wording in `SECURITY.md` and issue templates

## Maintenance irritants

### Roadmap and docs consistency

- [x] resync `README.md` with the implemented `.gion` scalar surface
- [x] keep `ROADMAP.md` focused on the active maintenance lane while preserving future language work below
- [x] remove duplicated or drifted builtin entries in the user docs
- [x] make the documented current scope consistent across README, docs, examples, and roadmap
- [x] review docs wording so the public entry points match the post-`gion-arithmetic` project state
- [x] align `.github/ISSUE_TEMPLATE/config.yml` support links with the current `README.md` / `SECURITY.md` contact policy

### Test robustness

- [x] replace fixed temporary output filenames in tests with unique per-test temp paths
- [x] make the test suite safe to run in parallel without file collisions
- [x] reduce repeated file-open / file-read / cleanup boilerplate through shared test helpers
- [x] split the single `graphion_tests` CTest entry into smaller logical test targets or suites
- [x] reduce the manual maintenance burden of the central test registry in `tests/runner/test_main.c`

### CI and checks

- [x] run the docs build on pull requests, not only on pushes to `main`
- [x] extend `cspell` coverage to `ROADMAP.md`, `examples/README.md`, and maintained project docs outside `docs/`
- [x] extend link-check coverage to roadmap and examples entry points
- [x] align the local quality-gate scripts across Bash and PowerShell
- [x] decide whether `clang-tidy` and `cppcheck` should stay `src`-only or also cover part of `tests`
- [x] keep sanitize, warning, static-analysis, and docs expectations documented in one obvious place
- [x] retire or repurpose `check_asm_safety.py` and the related assembly-only gate after removal of the VM ASM path
- [x] remove stale assembly-safety references from maintainer notes, local helpers, and CI if the asm gate is dropped
- [x] align the `cppcheck` option set between CI and the local quality-gate scripts
- [x] decide whether the local repo gate should mirror the Docker-based `lychee` workflow or document the intentional tool mismatch
- [x] replace the floating Docker `lychee:latest` usage with a deterministic image reference that is actually published
- [x] keep the local repo gate and `links-check` workflow aligned on the same pinned `lychee` image reference

## Opportunistic improvements

### Code health

- [x] remove duplicated CMake logic such as the repeated VM dispatch block
- [x] improve runtime error wording where distinct failures currently collapse into overly generic messages
- [x] normalize formatting and indentation drift in recently expanded scalar code and tests
- [x] split oversized files in the scalar/runtime path into smaller units
- [x] reduce builtin duplication between parser, VM, docs, and tests where a shared definition table or helper layer makes sense
- [x] review the project code for obvious legacy or dead paths after the hygiene lane
  - current audit did not find a major dead-code block in `src/`; remaining legacy signs are mostly in repository/workflow config, not in the VM/runtime/parser core
- [x] continue splitting oversized interpreter and test files such as `src/runtime/interpreter/expr.c`, `tests/gion/test_gion_scalars.c`, and `tests/vm/test_vm_scalars.c`
- [x] continue splitting oversized files such as `src/runtime/interpreter/expr.c`, `src/vm/internal/opcodes/op_scalar.c`, `tests/gion/test_gion_scalars.c`, `tests/gion/test_gion_operators.c`, and `tests/vm/test_vm_scalars.c`
- [x] review the remaining VM fastpaths and decide which ones are still worth carrying
  - keep `arith_only_fastpath`, `weighted_sum_fastpath`, and `frontier_fastpath`
  - remove `value_move_fastpath`, `global_materialize_fastpath`, and `global_print_fastpath`
- [x] reduce duplication between the remaining fastpath executors and the main VM execution path where practical
- [x] add targeted coverage or simplification for the remaining fastpath planner / executor combinations
- [x] remove small structural drift such as duplicated includes in `src/vm/internal/core/fastpath_plan.c`
- [x] split the still-oversized scalar math test monolith in `tests/gion/test_gion_scalar_math_suite.c`
- [ ] split the still-oversized VM suites in `tests/vm/test_vm_logic_bits.c` and `tests/vm/test_vm_graph_ops.c`
- [ ] continue slimming the remaining near-1k code units such as `src/vm/internal/opcodes/op_scalar.c`, `src/runtime/interpreter/source.c`, `src/runtime/interpreter/expr.c`, `src/vm/internal/opcodes/op_scalar_math_builtins.c`, and `tests/vm/test_vm_scalar_builtins_misc.c`
- [ ] factor the repeated `frontier_is_bound` helper logic shared across `src/vm/vm.c`, `src/vm/internal/core/fastpath_exec.c`, `src/vm/internal/opcodes/op_frontier.c`, and `src/vm/internal/opcodes/op_graph.c`

### Repository hygiene

- [x] keep root-level local build and debug artefacts ignored consistently
- [x] review whether repo-health checks should assert a few more actively maintained files
- [x] keep local helper scripts and CI workflows aligned so the same quality bar is enforceable both locally and in CI

### Solo-dev simplification

- [x] review whether `CODEOWNERS` still adds value with a single maintainer, or remove it from the required repo-health baseline
- [x] review whether `.github/pull_request_template.md` and PR-title enforcement still justify their maintenance cost for a solo workflow
- [x] review whether PR auto-labeling and label-sync automation still provide enough value for a solo-maintained repo
- [x] review whether `contributors-sync.yml` and `CONTRIBUTORS.md` automation are still worth keeping while the project is effectively solo-maintained
- [x] review whether `stale.yml` and `monthly-audit.yml` reduce workload or mostly create repo noise in a solo-maintainer workflow
- [x] review whether issue templates, RFC discussion templates, and contributor-oriented labels such as `good first issue`, `help wanted`, and `triage` still match the actual maintenance model
- [x] review whether `MAINTAINERS.md` and `OWNERSHIP.md` should stay separate or collapse into a single lightweight maintainer note
- [x] review whether `SUPPORT.md` and `SECURITY_CONTACTS.md` should stay separate or merge into `README.md` / `SECURITY.md`

## Future additions gated by other features

These items stay visible for traceability, but they are intentionally outside the current maintenance lane.

- [ ] optional future support for Windows GCC / MinGW
  - only if the project later chooses to support it as a first-class toolchain
  - would require adding it to CI and keeping its warning policy green
- [ ] builtin `modf(x)`
  - needs a multi-value return shape such as a future list/tuple-like type
- [ ] builtin `cis(x)`
  - needs a future complex-number type to be meaningful as more than a shorthand pair
