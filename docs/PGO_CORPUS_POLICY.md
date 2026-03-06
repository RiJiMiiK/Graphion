# PGO Corpus Policy

## Goal

Keep profile-guided optimization training representative of Graphion's intended interpreter workloads.

The corpus must optimize for real graph and hypergraph execution patterns, not only for a single VM microbenchmark.

## Coverage Requirements

A valid representative corpus must cover all of these workload families:

- VM arithmetic dispatch hotpath
- CSR/BFS traversal
- Hypergraph incidence traversal
- Hypergraph reducer paths
- Graph-oriented VM opcode dispatch

In practice, the committed `representative` corpus currently trains with:

- `graphion_bench`
- `graphion_bench_bfs`
- `graphion_bench_hypergraph`
- `graphion_bench_hypergraph_incident_sum`
- `graphion_bench_hypergraph_hyperedge_node_sum`
- `graphion_bench_vm_graph`

It also runs:

- `graphion`
- `graphion_tests`

## Profiles

The PGO runner exposes named corpus profiles:

- `representative`
  - default local and report-generation profile
  - balanced across all current workload families
- `ci`
  - same workload families as `representative`
  - intended to be paired with reduced `--iterations-scale` in CI

## Update Rules

Any change to the representative corpus must satisfy all of the following:

- keep all required workload families covered
- update the committed policy in this document
- update `docs/PGO.md` if the workflow or defaults change
- update the optimization report metadata if the corpus meaning changes
- be justified by measured before/after reports, not by assumption

## Anti-Patterns

These are not acceptable as the only PGO training basis:

- `vm_dispatch` alone
- CSR-only training
- hypergraph-only training
- tests-only training
- a corpus tuned only to make one platform/compiler look good

## Operational Guidance

- use `representative` for local optimization work and official reports
- use `ci` with reduced iteration scale for workflow smoke validation
- if a new major workload family is introduced, extend the representative corpus before relying on new PGO numbers
