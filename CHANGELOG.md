# Changelog

All notable project changes are documented here.

The format follows Keep a Changelog and Semantic Versioning in spirit, but this file is intentionally concise:

- it records meaningful project evolution
- it does not try to preserve every historical internal experiment

## [Unreleased]

### Added

- Sphinx-based HTML documentation site under `docs/`
- first Graphion user documentation:
  - tutorial
  - language reference
  - builtins
- arithmetic support in `.gion` for:
  - `+`
  - `-`
  - `*`
  - `/`
  - `//`
  - `%`
  - `**`
- compound assignments:
  - `+=`
  - `-=`
  - `*=`
  - `/=`
  - `//=`
  - `%=`
  - `**=`
- grouped arithmetic expressions with parentheses
- builtin `abs(...)`
- string concatenation for `string + string`
- print-only mixed string coercion such as `print("count=" + 7)`
- dedicated source/runtime error distinction for:
  - parse errors
  - `unknown variable`
  - `unknown operand`
  - runtime arithmetic/type failures
- expanded tests for arithmetic, grouped expressions, compound assignments, string concatenation, and error cases

### Changed

- documentation was heavily reduced and reorganized around the docs that are still relevant
- the main engineering docs were rewritten to match the current project state instead of historical intermediate states
- the roadmap now tracks remaining work instead of completed historical milestones

### Removed

- obsolete/internal-heavy documentation that no longer matched the current direction
- older policy/report/process pages that were creating noise rather than helping the active rebuild
