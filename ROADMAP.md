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
  - VM boolean operators and conditional jumps currently accept only `bool` and `int` values `0` / `1`.
  - `match` is value-based scalar branching, not predicate-based branching.
- [x] map the existing complex predicate surface
  - `set` already exposes membership through `contains(set, value)`.
  - `graph` already exposes existence predicates through `has_node(graph, node)` and `has_edge(graph, from, to)`.
  - `hypergraph` already exposes existence predicates through `has_vertex(hypergraph, vertex)` and `has_hyperedge(hypergraph, id)`.
  - `list`, `tuple`, `dict`, and `struct` do not yet expose non-throwing membership/existence predicates.
  - `list`, `dict`, `tuple`, `set`, and `struct` already support deep `==` / `!=` results that can be used as conditions.
- [ ] define the official complex predicate API
  - [x] list: implement `contains(list, value)` for membership; keep ordered deep `==` / `!=` as valid conditions
  - [x] tuple: implement `contains(tuple, value)` for membership; keep ordered deep `==` / `!=` as valid conditions
  - [ ] set: keep `contains(set, value)` and decide whether any alias is needed
  - [ ] dict: define key-existence predicate without requiring throwing indexing
  - [ ] graph: keep `has_node` / `has_edge` and define condition examples around them
  - [ ] hypergraph: keep `has_vertex` / `has_hyperedge` and define condition examples around them
  - [ ] struct: decide field-existence predicate or explicitly reject it
  - [ ] decide whether `in` syntax belongs in this scope or is deferred
- [ ] align predicate results across conditional contexts
  - [ ] `if` / `elif` / `else`
  - [ ] ternary conditions
  - [ ] boolean operators and short-circuit behavior
  - [ ] grouped multiline conditions
  - [ ] diagnostics when predicates receive unsupported complex value combinations
- [ ] add targeted `.gion` coverage
  - [ ] predicate conditions for each supported complex type
  - [x] list: cover `contains(list, value)` in `if`, `elif`, ternary, and boolean operators
  - [x] tuple: cover `contains(tuple, value)` in `if`, `elif`, ternary, and boolean operators
  - [ ] equality/inequality conditions for supported complex values
  - [ ] missing key/field/member checks that should return `false` instead of throwing
  - [ ] unsupported predicate combinations with stable diagnostics
  - [ ] regression coverage for existing scalar condition behavior
- [ ] update user-facing documentation
  - [ ] document complex predicate APIs with `if` / `elif` examples
  - [ ] document equality/inequality as valid complex condition inputs
  - [ ] document unsupported predicate combinations and their diagnostics
  - [ ] clarify that `match` remains value-based scalar branching
  - [ ] keep truthiness decisions documented only if direct complex values become official conditions

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
