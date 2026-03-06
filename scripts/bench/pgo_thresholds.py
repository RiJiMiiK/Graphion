#!/usr/bin/env python3
from __future__ import annotations

from typing import Any


WORKLOAD_THRESHOLDS: dict[str, dict[str, Any]] = {
    "vm_dispatch": {
        "family": "vm_dispatch",
        "label": "VM arithmetic dispatch",
        "minimum_speedup_x": 1.05,
        "target_speedup_x": 1.15,
        "rationale": "PGO should materially improve interpreter dispatch hotpaths.",
    },
    "bfs_levels": {
        "family": "csr_bfs",
        "label": "CSR/BFS traversal",
        "minimum_speedup_x": 0.98,
        "target_speedup_x": 1.05,
        "rationale": "PGO should not regress traversal-heavy kernels; gains are welcome but not mandatory.",
    },
    "hypergraph_incidence": {
        "family": "hypergraph_traversal",
        "label": "Hypergraph incidence traversal",
        "minimum_speedup_x": 1.03,
        "target_speedup_x": 1.10,
        "rationale": "Traversal-heavy hypergraph kernels should see at least modest improvement.",
    },
    "hypergraph_incident_sum": {
        "family": "hypergraph_reducer",
        "label": "Hypergraph reducers",
        "minimum_speedup_x": 1.05,
        "target_speedup_x": 1.15,
        "rationale": "Reducer-style hot loops are expected to benefit clearly from PGO.",
    },
    "hypergraph_hyperedge_node_sum": {
        "family": "hypergraph_reducer",
        "label": "Hypergraph reducers",
        "minimum_speedup_x": 1.05,
        "target_speedup_x": 1.15,
        "rationale": "Reducer-style hot loops are expected to benefit clearly from PGO.",
    },
    "vm_graph_ops": {
        "family": "vm_graph_ops",
        "label": "Graph-oriented VM opcodes",
        "minimum_speedup_x": 1.00,
        "target_speedup_x": 1.08,
        "rationale": "Graph-specific opcode dispatch should at least break even under PGO.",
    },
}


def threshold_names() -> list[str]:
    return sorted(WORKLOAD_THRESHOLDS.keys())


def threshold_for(benchmark: str) -> dict[str, Any]:
    if benchmark not in WORKLOAD_THRESHOLDS:
        known = ", ".join(threshold_names())
        raise KeyError(f"unknown PGO threshold benchmark '{benchmark}'; expected one of: {known}")
    return WORKLOAD_THRESHOLDS[benchmark]


def evaluate_speedup(benchmark: str, speedup_x: float) -> str:
    threshold = threshold_for(benchmark)
    if speedup_x >= float(threshold["target_speedup_x"]):
        return "target"
    if speedup_x >= float(threshold["minimum_speedup_x"]):
        return "minimum"
    return "below"


def threshold_rows() -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for benchmark in threshold_names():
        threshold = threshold_for(benchmark)
        rows.append({
            "benchmark": benchmark,
            "family": threshold["family"],
            "label": threshold["label"],
            "minimum_speedup_x": threshold["minimum_speedup_x"],
            "target_speedup_x": threshold["target_speedup_x"],
            "rationale": threshold["rationale"],
        })
    return rows
