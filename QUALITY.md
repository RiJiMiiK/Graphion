# Quality Expectations

This is the single reference point for the repository's active quality bar.

## Core expectations

- builds should stay green with the repository warning policy enabled
- supported toolchains should keep passing `cmake`, `build`, and `ctest`
- documentation entry points should build cleanly with Sphinx
- static analysis should stay actionable and low-noise

## Warning policy

- the project keeps `GRAPHION_ENABLE_WERROR=ON` by default
- supported CI toolchains are expected to stay warning-clean under that policy
- Windows MSVC is the supported Windows toolchain
- Windows GCC / MinGW is not currently a supported release-blocking toolchain

## Sanitizers

- sanitizers are part of the expected quality bar on non-Windows platforms
- the local quality gate enables `GRAPHION_ENABLE_SANITIZERS=ON` on non-Windows platforms
- local Windows quality-gate runs skip sanitizers explicitly
- the dedicated CI job for sanitizer coverage is `sanitize-linux`

## Static analysis

- `clang-tidy` and `cppcheck` stay `src`-only for now
- test code is validated primarily through normal builds, `ctest`, and sanitizer coverage
- the current intent is to avoid widening static-analysis scope until a smaller, low-noise test subset is chosen explicitly

## Documentation checks

- docs should build locally with:

```bash
python -m sphinx -b html docs docs/_build/html
```

- the docs workflow builds on pull requests
- GitHub Pages deployment stays limited to pushes on `main`
- maintained Markdown docs should stay covered by spellcheck and link-check workflows
- local repo/docs validation should be runnable through the repo gate entry points

## Local quality gate

Primary local entry points:

- `scripts/quality/quality_gate.sh`
- `scripts/quality/quality_gate.ps1`
- `scripts/quality/repo_gate.sh`
- `scripts/quality/repo_gate.ps1`

The quality gate should stay aligned on:

- configure a Debug build with compile commands enabled
- build the project
- run `ctest`
- run `run_clang_tidy.py`
- run `cppcheck` when available

The repo gate should stay aligned on:

- run `check_repo_health.py`
- build Sphinx docs
- run `cspell` against the maintained Markdown entry points
- run `lychee` against the maintained Markdown link-check entry points

## CI map

- `ci.yml`
  - supported build-and-test matrix
  - `clang-tidy`
  - `sanitize-linux`
- `docs.yml`
  - Sphinx docs build on PRs
  - Pages deploy on `main`
- `spellcheck.yml`
  - maintained Markdown spellcheck coverage
- `links-check.yml`
  - maintained Markdown link coverage
