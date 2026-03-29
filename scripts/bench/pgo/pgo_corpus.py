#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))
from typing import Any


WORKLOADS: dict[str, dict[str, Any]] = {
    "frontier_primitives": {
        "target": "graphion_bench_frontier",
        "iterations": 200000,
        "family": "frontier",
        "coverage": "frontier filter/map/reduce primitives and buffer choreography",
    },
    "vm_dispatch": {
        "target": "graphion_bench",
        "iterations": 200000,
        "family": "vm",
        "coverage": "arithmetic dispatch hotpath",
    },
    "bfs_levels": {
        "target": "graphion_bench_bfs",
        "iterations": 200000,
        "family": "csr",
        "coverage": "CSR traversal and BFS frontier expansion",
    },
    "neighbor_iteration": {
        "target": "graphion_bench_neighbors",
        "iterations": 200000,
        "family": "frontier-csr",
        "coverage": "bounded CSR frontier neighbor iteration and sparse/dense recommendation path",
    },
    "weighted_neighbor_sums": {
        "target": "graphion_bench_weighted_graph",
        "iterations": 200000,
        "family": "weighted-csr",
        "coverage": "weighted CSR VM opcodes for neighbor weight sums and edge-attribute sums",
    },
    "frontier_threshold_calibration": {
        "target": "graphion_bench_frontier_thresholds",
        "iterations": 1000,
        "family": "frontier-calibration",
        "coverage": "frontier sparse/dense threshold calibration benchmark",
    },
    "hypergraph_incidence": {
        "target": "graphion_bench_hypergraph",
        "iterations": 200000,
        "family": "hypergraph",
        "coverage": "incidence traversal across node-edge relationships",
    },
    "hypergraph_traversal": {
        "target": "graphion_bench_hypergraph_traversal",
        "iterations": 200000,
        "family": "hypergraph",
        "coverage": "node-to-edge and edge-to-node traversal primitives",
    },
    "hypergraph_incident_sum": {
        "target": "graphion_bench_hypergraph_incident_sum",
        "iterations": 200000,
        "family": "hypergraph",
        "coverage": "incident reducer path",
    },
    "hypergraph_hyperedge_node_sum": {
        "target": "graphion_bench_hypergraph_hyperedge_node_sum",
        "iterations": 200000,
        "family": "hypergraph",
        "coverage": "hyperedge-to-node reducer path",
    },
    "vm_graph_ops": {
        "target": "graphion_bench_vm_graph",
        "iterations": 100000,
        "family": "vm-graph",
        "coverage": "graph-specific VM opcode dispatch",
    },
    "vm_scalar_values_print": {
        "target": "graphion_bench_vm_scalars",
        "iterations": 50000,
        "family": "vm-scalars",
        "coverage": "scalar VM typed values, globals, and print-oriented opcode flow",
    },
    "gion_scalar_values_print": {
        "target": "graphion_bench_gion_scalars",
        "iterations": 25000,
        "family": "gion-scalars",
        "coverage": "scalar .gion prepare/execute path for assignments, copies, and prints",
    },
}


CORPUS_PROFILES: dict[str, dict[str, Any]] = {
    "representative": {
        "description": "Balanced default corpus spanning frontier primitives, VM dispatch, CSR/BFS, hypergraph traversal, reducers, and VM graph opcodes.",
        "workloads": [
            "frontier_primitives",
            "vm_dispatch",
            "bfs_levels",
            "neighbor_iteration",
            "weighted_neighbor_sums",
            "frontier_threshold_calibration",
            "hypergraph_incidence",
            "hypergraph_traversal",
            "hypergraph_incident_sum",
            "hypergraph_hyperedge_node_sum",
            "vm_graph_ops",
            "vm_scalar_values_print",
            "gion_scalar_values_print",
        ],
        "run_graphion_binary": True,
        "run_tests": True,
        "intended_for": "local optimization work and release-quality report generation",
    },
    "ci": {
        "description": "Same workload families as the representative corpus, intended for CI with reduced iteration scale.",
        "workloads": [
            "frontier_primitives",
            "vm_dispatch",
            "bfs_levels",
            "neighbor_iteration",
            "weighted_neighbor_sums",
            "frontier_threshold_calibration",
            "hypergraph_incidence",
            "hypergraph_traversal",
            "hypergraph_incident_sum",
            "hypergraph_hyperedge_node_sum",
            "vm_graph_ops",
            "vm_scalar_values_print",
            "gion_scalar_values_print",
        ],
        "run_graphion_binary": True,
        "run_tests": True,
        "intended_for": "GitHub Actions smoke validation of the PGO pipeline",
    },
}


def corpus_profile_names() -> list[str]:
    return sorted(CORPUS_PROFILES.keys())


def get_corpus_profile(name: str) -> dict[str, Any]:
    if name not in CORPUS_PROFILES:
        names = ", ".join(corpus_profile_names())
        raise ValueError(f"unknown PGO corpus profile '{name}'; expected one of: {names}")
    return CORPUS_PROFILES[name]


def scaled_iterations(base_iterations: int, iterations_scale: float) -> int:
    return max(1000, int(base_iterations * iterations_scale))


def expanded_workloads(profile_name: str, iterations_scale: float) -> list[dict[str, Any]]:
    profile = get_corpus_profile(profile_name)
    rows: list[dict[str, Any]] = []
    for workload_name in profile["workloads"]:
        workload = WORKLOADS[workload_name]
        rows.append({
            "name": workload_name,
            "target": workload["target"],
            "family": workload["family"],
            "coverage": workload["coverage"],
            "iterations": scaled_iterations(int(workload["iterations"]), iterations_scale),
            "base_iterations": workload["iterations"],
        })
    return rows


def coverage_classes(profile_name: str) -> list[str]:
    profile = get_corpus_profile(profile_name)
    classes: list[str] = []
    for workload_name in profile["workloads"]:
        family = str(WORKLOADS[workload_name]["family"])
        if family not in classes:
            classes.append(family)
    return classes


def workload_targets(profile_name: str) -> list[str]:
    return [str(item["target"]) for item in expanded_workloads(profile_name, 1.0)]
