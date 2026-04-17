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

## Maintenance irritants

### Roadmap and docs consistency

- [x] resync `README.md` with the implemented `.gion` scalar surface
- [x] keep `ROADMAP.md` focused on the active maintenance lane while preserving future language work below
- [x] remove duplicated or drifted builtin entries in the user docs
- [ ] make the documented current scope consistent across README, docs, examples, and roadmap
- [ ] review docs wording so the public entry points match the post-`gion-arithmetic` project state

### Test robustness

- [ ] replace fixed temporary output filenames in tests with unique per-test temp paths
- [ ] make the test suite safe to run in parallel without file collisions
- [ ] reduce repeated file-open / file-read / cleanup boilerplate through shared test helpers
- [ ] split the single `graphion_tests` CTest entry into smaller logical test targets or suites
- [ ] reduce the manual maintenance burden of the central test registry in `tests/runner/test_main.c`

### CI and checks

- [ ] run the docs build on pull requests, not only on pushes to `main`
- [ ] extend `cspell` coverage to `ROADMAP.md`, `examples/README.md`, and maintained project docs outside `docs/`
- [ ] extend link-check coverage to roadmap and examples entry points
- [ ] align the local quality-gate scripts across Bash and PowerShell
- [ ] decide whether `clang-tidy` and `cppcheck` should stay `src`-only or also cover part of `tests`
- [ ] keep sanitize, warning, static-analysis, and docs expectations documented in one obvious place

## Opportunistic improvements

### Code health

- [ ] remove duplicated CMake logic such as the repeated VM dispatch block
- [ ] improve runtime error wording where distinct failures currently collapse into overly generic messages
- [ ] normalize formatting and indentation drift in recently expanded scalar code and tests
- [ ] split oversized files in the scalar/runtime path into smaller units
- [ ] reduce builtin duplication between parser, VM, docs, and tests where a shared definition table or helper layer makes sense

### Repository hygiene

- [ ] keep root-level local build and debug artefacts ignored consistently
- [ ] review whether repo-health checks should assert a few more actively maintained files
- [ ] keep local helper scripts and CI workflows aligned so the same quality bar is enforceable both locally and in CI

### Solo-dev simplification

- [ ] review whether `CODEOWNERS` still adds value with a single maintainer, or remove it from the required repo-health baseline
- [ ] review whether `.github/pull_request_template.md` and PR-title enforcement still justify their maintenance cost for a solo workflow
- [ ] review whether PR auto-labeling and label-sync automation still provide enough value for a solo-maintained repo
- [ ] review whether `contributors-sync.yml` and `CONTRIBUTORS.md` automation are still worth keeping while the project is effectively solo-maintained
- [ ] review whether `stale.yml` and `monthly-audit.yml` reduce workload or mostly create repo noise in a solo-maintainer workflow
- [ ] review whether issue templates, RFC discussion templates, and contributor-oriented labels such as `good first issue`, `help wanted`, and `triage` still match the actual maintenance model
- [ ] review whether `MAINTAINERS.md` and `OWNERSHIP.md` should stay separate or collapse into a single lightweight maintainer note
- [ ] review whether `SUPPORT.md` and `SECURITY_CONTACTS.md` should stay separate or merge into `README.md` / `SECURITY.md`

## Future additions gated by other features

These items stay visible for traceability, but they are intentionally outside the current maintenance lane.

- [ ] optional future support for Windows GCC / MinGW
  - only if the project later chooses to support it as a first-class toolchain
  - would require adding it to CI and keeping its warning policy green
- [ ] builtin `modf(x)`
  - needs a multi-value return shape such as a future list/tuple-like type
- [ ] builtin `cis(x)`
  - needs a future complex-number type to be meaningful as more than a shorthand pair
