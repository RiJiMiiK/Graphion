# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

## Current lane

This branch is for active work on non-scalar language types.

### Non-scalar language types

- [x] add `list` as the first non-scalar container type
  - literals
  - runtime / VM representation
  - basic operations such as indexing, equality, printing, and `len`
  - tests, docs, and examples
- [x] add `dict` as the second non-scalar container type
  - recommended first version with `string` keys
  - literals
  - runtime / VM representation
  - basic operations such as lookup, equality, printing, and `len`
  - tests, docs, and examples
- [x] `tuple`
  - useful for fixed-size structured returns if `list` is too loose semantically
- [ ] `set`
  - especially relevant for graph-oriented membership, uniqueness, and frontier-like value sets
- [ ] first-class `graph` values in `.gion`
  - distinct from backend-only VM/runtime support
- [ ] first-class `hypergraph` values in `.gion`
  - distinct from backend-only VM/runtime support
- [ ] `path` or `walk` value type
  - useful if traversal results should become first-class language values
- [ ] `record` / `struct`
  - optional future typed composite if `dict` is too dynamic for some language features

## Future additions gated by other features

These items stay visible for traceability and future planning.

### Toolchain and platform

- [ ] optional future support for Windows GCC / MinGW
  - only if the project later chooses to support it as a first-class toolchain
  - would require adding it to CI and keeping its warning policy green

### Builtins gated by future types

- [ ] builtin `modf(x)`
  - needs a multi-value return shape such as a future list/tuple-like type
- [ ] builtin `cis(x)`
  - needs a future complex-number type to be meaningful as more than a shorthand pair
