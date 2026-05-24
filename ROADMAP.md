# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

## Current lane

This branch is for active work on Graphion/Gion conditional behavior for complex
first-class values. The scope is limited to existing language values; it should
not introduce a large new language type.

### Complex conditions

- [x] establish the current condition model
  - Direct `if` / `elif` conditions are evaluated by the interpreter through `evaluate_condition_text`.
  - Ternary expressions and short-circuit boolean operators compile through VM conditional jumps.
  - VM boolean operators and conditional jumps share `vm_value_get_boolean`.
  - `match` is value-based scalar branching and does not need behavioral changes for this scope.
- [ ] define official truth rules
  - [x] collections: empty is false, non-empty is true for `list`, `dict`, `tuple`, and `set`
  - [ ] graph: define truth from visible node count and/or edge count
  - [ ] hypergraph: define truth from visible vertex count and/or active hyperedge count
  - [ ] struct: define whether instances are always true or field-count based
  - [ ] scalars: keep compatibility for `bool` and accepted `int` values, and keep unsupported scalar types explicit
- [ ] implement one shared truth conversion path
  - [ ] move condition truth conversion into a VM/core helper usable by interpreter and VM paths
  - [ ] align `evaluate_condition_text` with VM conditional jumps
  - [ ] align `if`, `elif`, ternary expressions, and boolean operators on the same accepted value set
  - [ ] preserve ownership and disposal behavior for complex values during truth conversion
  - [ ] keep unsupported condition diagnostics actionable and stable
- [ ] add targeted `.gion` coverage
  - [ ] cover `if` / `elif` / `else` for empty and non-empty complex values
  - [ ] cover ternary conditions for the same accepted complex values
  - [ ] cover boolean operators with accepted and rejected complex values
  - [ ] cover unsupported condition types and diagnostic messages
  - [ ] preserve regression coverage for current scalar condition behavior
- [ ] update user-facing documentation
  - [ ] document official truth rules in the language reference
  - [ ] update operator documentation for boolean logic and conditions
  - [ ] update type documentation where truthiness becomes part of the value contract
  - [ ] clarify that `match` remains value-based scalar branching, not truthiness-based branching

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
