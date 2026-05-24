# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

## Current lane

This branch is for active work on Graphion/Gion conditional behavior for complex
first-class values. The scope is limited to existing language values; it should
not introduce a large new language type.

### Complex conditions

- [x] audit existing condition evaluation before changing behavior
  - [x] trace `if`, `elif`, and `else` through the runtime/interpreter
  - [x] trace ternary conditions through expression compilation and VM jumps
  - [x] trace boolean operators and VM conditional jumps for shared truth rules
  - [x] decide whether `match` needs changes or only documentation for this scope: documentation only
- [ ] define official truth rules for existing first-class complex values
  - [x] decide truthiness for `list`: empty is false, non-empty is true
  - [x] decide truthiness for `dict`: empty is false, non-empty is true
  - [x] decide truthiness for `tuple`: empty is false, non-empty is true
  - [x] decide truthiness for `set`: empty is false, non-empty is true
  - [ ] decide truthiness for `graph`
  - [ ] decide truthiness for `hypergraph`
  - [ ] decide truthiness for `struct`
  - [ ] keep scalar condition compatibility explicit for `bool` and accepted `int` values
- [ ] implement complex condition behavior consistently
  - [ ] centralize condition truth conversion where practical
  - [ ] align runtime/interpreter direct condition evaluation with VM conditional jumps
  - [ ] align ternary expression conditions with block condition rules
  - [ ] keep unsupported condition types rejected with actionable diagnostics
  - [ ] preserve ownership and disposal behavior for complex values during condition evaluation
- [ ] verify comparisons and equality interactions
  - [ ] ensure equality results remain boolean and work as conditions
  - [ ] ensure complex value equality does not accidentally become implicit comparison ordering
  - [ ] ensure boolean operators keep their intended accepted operand rules
- [ ] add targeted `.gion` coverage
  - [ ] add `if` / `elif` / `else` tests for empty and non-empty complex values
  - [ ] add ternary tests for complex values used as conditions
  - [ ] add diagnostics tests for unsupported direct condition types
  - [ ] add regression tests for existing scalar condition behavior
  - [ ] add graph and hypergraph condition tests using representative empty and populated values
  - [ ] add struct condition tests once the official rule is chosen
- [ ] update user-facing documentation after behavior is stable
  - [ ] document official condition truth rules in the language reference
  - [ ] update operator documentation for boolean logic and conditions
  - [ ] update type documentation where complex value truthiness is part of the type contract
  - [ ] adjust examples only if the new behavior should be shown as recommended style

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
