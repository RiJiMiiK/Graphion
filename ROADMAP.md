# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

## Current lane

This branch is for active work on Graphion/Gion diagnostics: parser and frontend errors,
runtime/interpreter errors, VM-originated failures that surface through `.gion`, and debug warnings.

### Diagnostics and warnings

- [ ] audit current diagnostic sources before changing behavior
  - parser/frontend diagnostics in `src/parser`
  - runtime/interpreter diagnostics in `src/runtime`
  - VM errors that are translated into `.gion` runtime errors
  - CLI formatting in `src/main.c`
  - debug warning collection and emission for `-d`
  - existing tests and docs that lock current messages
- [ ] make common `.gion` errors more actionable
  - name the missing identifier in unknown operand diagnostics
  - name the missing assignment or indexed-assignment target in unknown variable diagnostics
  - replace ambiguous parse fallbacks such as `expected scalar literal` where a more specific message is available
  - replace broad messages such as `unsupported assignment expression` for obvious trailing tokens
- [ ] improve line and column precision
  - keep current line accuracy for late source errors
  - report useful columns for assignment operator errors
  - report useful columns for missing delimiters in `print`, grouped expressions, indexing, lists, dicts, tuples, and sets
  - report useful columns for control headers (`if`, `elif`, `else`, `match`, `default`)
  - report useful columns for graph and hypergraph declaration/body errors
- [ ] align parse/runtime categories where user-visible behavior is surprising
  - distinguish syntax errors from name resolution errors in `print(...)`
  - distinguish graph declaration syntax errors from expression/name resolution errors
  - keep runtime type/domain errors separate from frontend parse errors
  - document intentional subsystem-local result codes
- [ ] harden debug warnings exposed by `-d`
  - verify warning directives are either intentionally ignored or implemented consistently
  - document current `-d` behavior
  - ensure warnings include stable `warning:line:column: message` output
  - add coverage for warning capacity and warning collection failures
- [ ] improve test coverage for diagnostics
  - add focused parser/frontend tests for message text and line/column pairs
  - add runtime/interpreter tests for representative runtime errors across scalar and non-scalar values
  - add CLI-path tests where formatting differs from direct API diagnostics
  - keep tests scoped to user-visible behavior, not internal implementation details
- [ ] update documentation when diagnostics change
  - `docs/runtime/debugging/ERRORS.md`
  - Graphion language reference diagnostics section
  - examples or tutorial notes only when user-facing behavior changes

## Future additions gated by other features

These items stay visible for traceability and future planning.

### Toolchain and platform

- [ ] optional future support for Windows GCC / MinGW
  - only if the project later chooses to support it as a first-class toolchain
  - would require adding it to CI and keeping its warning policy green

### Builtins gated by future types

- [ ] future `path` value type
  - reserved for traversal builtins such as `shortest_path`, `bfs_path`, or `dfs_path`
  - not user-constructible directly in the first version
  - intended print shape: `path(1 -> 2 -> 4)`
- [ ] builtin `modf(x)`
  - needs a multi-value return shape such as a future list/tuple-like type
- [ ] builtin `cis(x)`
  - needs a future complex-number type to be meaningful as more than a shorthand pair
