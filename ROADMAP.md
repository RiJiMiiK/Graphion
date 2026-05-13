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
- [x] `set`
  - especially relevant for graph-oriented membership, uniqueness, and frontier-like value sets
- [x] first-class `graph` values in `.gion`
  - distinct from backend-only VM/runtime support
  - [x] create an empty graph with `graph Name;`
  - [x] create graph values with node blocks
  - [x] support string node names with quoted literals
  - [x] support node variables that resolve to `string` names or integer IDs
  - [x] support explicit numeric node IDs without creating implicit gap nodes
  - [x] support undirected edges with `node - node`
  - [x] support directed edges with `node -> node`
  - [x] support bidirectional directed edges with `node <-> node`
  - [x] reject mixing directed syntax with undirected `-` edges in the same graph
  - [x] create missing endpoint nodes when an edge references them
  - [x] support node attributes as dict values
  - [x] support `defaults node` and enforce a shared node attribute schema
  - [x] support edge attributes as dict values
  - [x] support compact edge weights with reserved numeric `weight`
  - [x] support edge attribute expressions that evaluate to `int`, `float`, or `dict`
  - [x] support `defaults edge` and enforce a shared edge attribute schema
  - [x] print useful graph summaries
  - [x] cover graph declarations, attrs, edges, warnings, docs, and examples
  - [x] add `.gion` graph inspection operations
    - node count
    - edge count
    - directedness / orientation information
    - weightedness information
  - [x] add `.gion` graph attribute lookup operations
    - node attributes
    - edge attributes
    - reserved edge `weight`
  - [x] add basic graph membership/query operations
    - node exists
    - edge exists
    - neighbors / adjacency
  - [x] decide and add the first mutation surface after initialization
    - add node
    - add edge
  - [x] add graph attribute mutation after initialization
    - update node attributes
    - update edge attributes
    - update reserved edge `weight`
  - [x] add graph removal mutation after initialization
    - remove node
    - remove edge
    - define how node removal affects incident edges and attributes
  - [x] add graph listing/query operations
    - list nodes / node IDs
    - list edges
    - expose enough graph contents to inspect a value without knowing names or IDs ahead of time
  - [x] clarify directed adjacency semantics
    - `neighbors(...)` means incoming plus outgoing adjacency
    - `indegree(...)` and `outdegree(...)` expose incoming / outgoing node IDs
  - [x] harden graph mutation error coverage
    - unknown attribute keys
    - invalid `weight` types
    - missing nodes / edges
    - partial attribute patches on nodes / edges added after defaults
- [ ] first-class `hypergraph` values in `.gion`
  - distinct from backend-only VM/runtime support
  - [x] create an empty hypergraph with `hypergraph Name;`
  - [x] create hypergraph values with vertex blocks
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
