# Frontier Regression Gates

## Scope

This policy defines the automatic regression gate for the frontier and traversal
benchmark family.

The current gate is enforced in:

- `.github/workflows/performance-report.yml`
- `scripts/bench/compare/check_frontier_regressions.py`

## Gate rule

For the official Windows parity lane, Graphion must remain below:

- `Graphion latency / Rust latency <= 1.15x`

for the following workloads:

- `frontier_primitives`
- `neighbor_iteration`
- `weighted_neighbor_sums`
- `hypergraph_incidence`
- `hypergraph_traversal`

The gate uses latency, not throughput, because:

- all official reports already render latency as the primary parity signal
- latency keeps the comparison uniform across `mips` and `mteps` workloads
- the user-visible contract for these workloads is already expressed as a
  maximum slowdown versus Rust

## Failure policy

If a workload exceeds `1.15x`:

- the automated performance workflow must fail
- the refreshed report PR must not auto-merge
- the offending runtime path must be optimized before the benchmark is treated
  as stable

If a required row is missing or has a non-positive latency:

- the gate must fail

This prevents silent acceptance of incomplete or invalid parity data.

## Workflow placement

The gate runs after:

- the Graphion Windows lane
- the Rust Windows lane

and before:

- automated report publication / PR auto-merge

This keeps the weekly report flow automatic while still blocking parity
regressions on the workloads that matter for the current graph execution model.
