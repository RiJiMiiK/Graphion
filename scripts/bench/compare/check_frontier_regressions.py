#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
from typing import Any


DEFAULT_THRESHOLDS: dict[str, float] = {
    "frontier_primitives": 1.15,
    "neighbor_iteration": 1.15,
    "weighted_neighbor_sums": 1.15,
    "hypergraph_incidence": 1.15,
    "hypergraph_traversal": 1.15,
}


def load_rows(path: pathlib.Path) -> list[dict[str, Any]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(payload, dict):
        rows = payload.get("rows")
        if isinstance(rows, list):
            return rows
        raise ValueError(f"{path} does not contain a 'rows' list")
    if isinstance(payload, list):
        return payload
    raise ValueError(f"{path} has an unsupported benchmark JSON shape")


def index_rows(rows: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    indexed: dict[str, dict[str, Any]] = {}
    for row in rows:
        benchmark = str(row["benchmark"])
        indexed[benchmark] = row
    return indexed


def latency_value(row: dict[str, Any]) -> float:
    latency_key = str(row["latency_key"])
    value = row.get(latency_key + "_avg", row.get(latency_key))
    if value is None:
        raise KeyError(f"missing latency value for key '{latency_key}' in benchmark '{row['benchmark']}'")
    latency = float(value)
    if latency <= 0.0:
        raise ValueError(f"non-positive latency for benchmark '{row['benchmark']}': {latency}")
    return latency


def format_row(benchmark: str, graphion_latency: float, rust_latency: float, max_ratio: float) -> str:
    ratio = graphion_latency / rust_latency
    status = "OK" if ratio <= max_ratio else "FAILED"
    return (
        f"{benchmark}: graphion={graphion_latency:.3f} rust={rust_latency:.3f} "
        f"ratio={ratio:.3f}x limit={max_ratio:.3f}x status={status}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check Graphion frontier/traversal benchmark regressions against Rust parity thresholds."
    )
    parser.add_argument("--graphion-json", required=True, help="Path to Graphion lane JSON")
    parser.add_argument("--rust-json", required=True, help="Path to Rust lane JSON")
    parser.add_argument(
        "--benchmark",
        action="append",
        dest="benchmarks",
        help="Benchmark to check. May be repeated. Defaults to the official frontier/traversal gate set.",
    )
    parser.add_argument(
        "--max-ratio",
        type=float,
        default=1.15,
        help="Fallback maximum allowed Graphion/Rust latency ratio for benchmarks without an explicit threshold.",
    )
    args = parser.parse_args()

    graphion_rows = index_rows(load_rows(pathlib.Path(args.graphion_json)))
    rust_rows = index_rows(load_rows(pathlib.Path(args.rust_json)))
    benchmarks = args.benchmarks or list(DEFAULT_THRESHOLDS.keys())

    failures: list[str] = []

    print("frontier regression gate:")
    for benchmark in benchmarks:
        if benchmark not in graphion_rows:
            failures.append(f"missing Graphion benchmark row '{benchmark}'")
            continue
        if benchmark not in rust_rows:
            failures.append(f"missing Rust benchmark row '{benchmark}'")
            continue

        graphion_latency = latency_value(graphion_rows[benchmark])
        rust_latency = latency_value(rust_rows[benchmark])
        threshold = DEFAULT_THRESHOLDS[benchmark] if benchmark in DEFAULT_THRESHOLDS else args.max_ratio
        max_ratio = float(threshold)
        ratio = graphion_latency / rust_latency

        print("  - " + format_row(benchmark, graphion_latency, rust_latency, max_ratio))
        if ratio > max_ratio:
            failures.append(
                f"{benchmark}: Graphion/Rust latency ratio {ratio:.3f}x exceeds allowed {max_ratio:.3f}x"
            )

    if failures:
        print("frontier regression gate: FAILED", flush=True)
        for failure in failures:
            print("  - " + failure, flush=True)
        return 1

    print("frontier regression gate: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
