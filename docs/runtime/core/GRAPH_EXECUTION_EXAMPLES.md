# Graph Execution Reference Examples

## Scope

This document provides small reference programs for the current frontier-based
graph and hypergraph execution model.

These examples are intentionally:

- small
- deterministic
- tied to the current parser mnemonics
- aligned with executable test coverage in `tests/test_parser.c`

Common binding assumptions across the examples below:

- initial frontier input: `[1, 4, 7, 10]`
- CSR graph:
  - node `0 -> [1, 2]`
  - node `1 -> [3]`
  - node `2 -> [0, 3]`
  - node `3 -> [1]`
- hypergraph:
  - node `1 -> hyperedges [0, 1]`
  - hyperedge `1 -> nodes [1, 3]`

## Frontier Pipeline

Intent:

- filter the bound frontier to values `< 7`
- swap it into input
- add `1` to every item
- swap again
- reduce to a single sum

Source:

```text
frontier_clear r0, 0
frontier_filter_lt_imm r1, 7
frontier_swap r2, 0
frontier_map_add_imm r3, 1
frontier_swap r4, 0
frontier_reduce_sum r5, 0
halt
```

Expected final state:

- registers:
  - `r1 = 2`
  - `r2 = 2`
  - `r3 = 2`
  - `r4 = 2`
  - `r5 = 7`
- frontier input:
  - `[2, 5]`
- frontier output:
  - `[]`

## Neighbor Traversal

Intent:

- materialize neighbors of node `2`
- swap them into the input frontier
- expand neighbors for each materialized node

Source:

```text
mov r0, 2
neighbors_of r0, 0
frontier_swap r1, 0
neighbors_expand r2, 0
halt
```

Expected final state:

- registers:
  - `r0 = 2`
  - `r1 = 2`
  - `r2 = 3`
- frontier input:
  - `[0, 3, 7, 10]`
- frontier output:
  - `[1, 2, 1]`

Notes:

- `neighbors_of` writes the adjacency list for node `2`: `[0, 3]`
- `frontier_swap` makes `[0, 3]` the new frontier input
- `neighbors_expand` concatenates adjacency lists in encounter order:
  - node `0 -> [1, 2]`
  - node `3 -> [1]`

## Hyperedge Traversal

Intent:

- materialize hyperedges incident to node `1`
- swap them into the input frontier
- materialize nodes of hyperedge `1`

Source:

```text
mov r0, 1
incident_of r0, 0
frontier_swap r1, 0
mov r2, 1
hyperedge_nodes_of r2, 0
halt
```

Expected final state:

- registers:
  - `r0 = 1`
  - `r1 = 2`
  - `r2 = 1`
- frontier input:
  - `[0, 1]`
- frontier output:
  - `[1, 3]`

Notes:

- `incident_of` writes incident hyperedges of node `1`: `[0, 1]`
- `frontier_swap` makes `[0, 1]` the new frontier input
- `hyperedge_nodes_of r2` then materializes nodes of hyperedge `1`: `[1, 3]`

## Maintenance Rule

If any of these examples changes semantically, update together:

- `docs/runtime/core/ISA.md`
- `docs/runtime/contracts/ISA_FIXTURES.md`
- `tests/test_parser.c`
