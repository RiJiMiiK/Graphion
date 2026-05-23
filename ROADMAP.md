# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

## Current lane

This branch is for active work on Graphion/Gion diagnostics: parser and frontend errors,
runtime/interpreter errors, VM-originated failures that surface through `.gion`, and debug warnings.

### Diagnostics and warnings

- [x] audit current diagnostic sources before changing behavior
- [x] make common `.gion` errors more actionable
  - [x] name the missing identifier in unknown operand diagnostics
  - [x] name the missing assignment or indexed-assignment target in unknown variable diagnostics
  - [x] name unknown graph variables in graph mutation diagnostics
  - [x] name unknown hypergraph variables in hypergraph mutation diagnostics
  - [x] replace ambiguous parse fallbacks such as `expected scalar literal` where a more specific message is available
  - [x] replace broad messages such as `unsupported assignment expression` for obvious trailing tokens
  - [x] replace unmapped VM failures with a stable diagnostic that includes the VM result class
  - [x] replace operator-missing operand fallbacks with operator-specific messages
  - [x] replace bare assignment RHS fallbacks such as `count =` with assignment-specific messages
  - [x] replace unexpected prefix-token fallbacks such as `value = !` with token-specific messages
  - [x] replace late execution parse fallbacks that still surface as `expected scalar literal`
- [x] improve line and column precision
  - [x] keep current line accuracy for late source errors
  - [x] report useful columns for assignment operator errors
  - [x] report useful columns for unknown identifiers in expressions
  - [x] report useful columns for missing delimiters in `print`, grouped expressions, indexing, lists, dicts, tuples, and sets
  - [x] report useful columns for control headers (`if`, `elif`, `else`, `match`, `default`)
  - [x] report useful columns for graph and hypergraph declaration/body errors
  - [x] report useful columns for warnings emitted by `-d`
  - [x] report useful columns for builtin/function call syntax errors
  - [x] report useful columns for ternary expression errors
  - [x] report useful columns for struct declaration/body/instance errors
  - [x] report useful columns for block-shape and indentation diagnostics
  - [x] report useful columns for multiline condition, assignment, and match grouping diagnostics
  - [x] report useful columns for scalar and math builtin argument syntax errors
  - [x] report useful columns for graph and hypergraph body semantic errors
  - [x] report useful columns for literal parser errors still surfaced through direct operand parsing
  - [x] report useful columns for VM expression trailing-token diagnostics
- [ ] align parse/runtime categories where user-visible behavior is surprising
  - [x] distinguish syntax errors from name resolution errors in `print(...)`
  - [x] distinguish graph declaration syntax errors from expression/name resolution errors
  - [x] keep runtime type/domain errors separate from frontend parse errors
  - [ ] decide whether parser/frontend `GFE_*` errors need a diagnostic object or remain code-only
  - [ ] decide whether bytecode decode `GBC_*` errors need user-facing text or remain code-only
  - [ ] document intentional subsystem-local result codes
- [ ] harden debug warnings exposed by `-d`
  - [ ] make `process_file_level_directives` either implement supported warning directives or reject/ignore them by documented rule
  - [ ] decide how CLI should handle `-d` warning collection failures before execution
  - [ ] document current `-d` behavior
  - [ ] ensure warnings include stable `warning:line:column: message` output
  - [ ] add coverage for impossible literal `match` warnings
  - [ ] add coverage for graph numeric node-id gap warnings
  - [ ] add coverage for warning capacity and warning collection failures
- [ ] improve test coverage for diagnostics
  - [ ] add focused parser/frontend tests for result-code diagnostics
  - [ ] add runtime/interpreter tests for exact message and line/column pairs
  - [ ] add runtime/interpreter tests for representative runtime errors across scalar and non-scalar values
  - [ ] add VM-to-runtime mapping tests for every `.gion`-visible `GVM_ERR_*`
  - [ ] add CLI-path tests where formatting differs from direct API diagnostics
  - [ ] keep tests scoped to user-visible behavior, not internal implementation details
- [ ] update documentation when diagnostics change
  - [x] update `docs/runtime/debugging/ERRORS.md`
  - [x] update Graphion language reference diagnostics section
  - [ ] update examples or tutorial notes only when user-facing behavior changes

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
