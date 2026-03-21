# Minimal Standard Library

## Scope

Graphion does not yet expose source-level `import`, `call`, or module syntax.
The current "standard library" is therefore a versioned catalog of named source
program snippets that compile through the normal frontend pipeline.

This keeps the surface usable now without inventing a higher-level language
feature ahead of `0.6`.

## Catalog API

Library entries are exposed from:

- `src/parser/stdlib.h`
- `src/parser/stdlib.c`

Primary entry points:

- `graphion_stdlib_program_count()`
- `graphion_stdlib_program_at(index)`
- `graphion_stdlib_find_program(name)`
- `graphion_stdlib_lower_program_to_ir(name, ...)`

Each catalog entry carries:

- stable name
- short description
- source snippet
- binding requirements:
  - `requires_csr`
  - `requires_hypergraph`
  - `requires_frontier`

## Current Program Names

### Graph

- `graph.neighbors.materialize`
- `graph.neighbors.expand`
- `graph.weights.sum`
- `graph.attrs.sum`
- `graph.bfs.levels`

### Hypergraph

- `hypergraph.incident.count`
- `hypergraph.incident.materialize`
- `hypergraph.hyperedge.size`
- `hypergraph.hyperedge.materialize_nodes`
- `hypergraph.incident.sum`
- `hypergraph.hyperedge.node_sum`

## Contract

- Names are stable within the current source prototype.
- Source snippets must remain valid under the current lexer/parser/AST pipeline.
- Library entries are tested for lookup and lowering in `tests/test_parser.c`.
- If a library program changes semantically, update together:
  - this document
  - `tests/test_parser.c`
  - `CHANGELOG.md`
