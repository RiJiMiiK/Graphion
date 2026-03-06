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

## Effectiveness Thresholds

The official optimization report tracks advisory PGO effectiveness thresholds per workload family.

Current thresholds:

- VM arithmetic dispatch
  - minimum: `1.05x`
  - target: `1.15x`
- CSR/BFS traversal
  - minimum: `0.98x`
  - target: `1.05x`
- Hypergraph incidence traversal
  - minimum: `1.03x`
  - target: `1.10x`
- Hypergraph reducers
  - minimum: `1.05x`
  - target: `1.15x`
- Graph-oriented VM opcodes
  - minimum: `1.00x`
  - target: `1.08x`

These thresholds are intentionally asymmetric:

- dispatch and reducer-heavy loops are expected to benefit clearly from PGO
- CSR/BFS kernels are allowed to be near break-even because control-flow and memory behavior dominate more of the runtime
- graph-oriented VM opcodes must at least not regress

## CI Scheduling And Retention

PGO smoke execution is governed separately from the local corpus choice:

- weekly scheduled smoke runs use the `ci` corpus and keep artifacts for `7 days`
- release-gated or PGO-policy pull requests use the `ci` corpus and keep artifacts for `21 days`
- manual `workflow_dispatch` runs use the `representative` corpus at a moderate scale and keep artifacts for `14 days`

This policy exists to balance three constraints:

- keep a recurring signal that the PGO pipeline still works
- keep release-sensitive evidence around long enough for review
- avoid paying full representative-corpus cost on every unrelated pull request

## Release-Candidate Alert Policy

Release-candidate review uses a small clang-based PGO smoke report and applies these alert rules:

- hard failure if `vm_dispatch` falls below its minimum threshold
- hard failure if any workload falls below `0.95x`
- hard failure if three or more benchmark families fall below their minimum thresholds
- advisory warning for any single workload that is below minimum but does not trigger a hard failure

This policy is intentionally conservative:

- `vm_dispatch` is the interpreter canary and must not quietly regress into a release
- severe regressions should stop a candidate even if one family is noisy
- isolated misses are reported for human review without turning every release PR into noise
