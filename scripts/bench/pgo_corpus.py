#!/usr/bin/env python3
from __future__ import annotations

from typing import Any


WORKLOADS: dict[str, dict[str, Any]] = {
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
    "hypergraph_incidence": {
        "target": "graphion_bench_hypergraph",
        "iterations": 200000,
        "family": "hypergraph",
        "coverage": "incidence traversal across node-edge relationships",
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
}


CORPUS_PROFILES: dict[str, dict[str, Any]] = {
    "representative": {
        "description": "Balanced default corpus spanning VM dispatch, CSR/BFS, hypergraph traversal, reducers, and VM graph opcodes.",
        "workloads": [
            "vm_dispatch",
            "bfs_levels",
            "hypergraph_incidence",
            "hypergraph_incident_sum",
            "hypergraph_hyperedge_node_sum",
            "vm_graph_ops",
        ],
        "run_graphion_binary": True,
        "run_tests": True,
        "intended_for": "local optimization work and release-quality report generation",
    },
    "ci": {
        "description": "Same workload families as the representative corpus, intended for CI with reduced iteration scale.",
        "workloads": [
            "vm_dispatch",
            "bfs_levels",
            "hypergraph_incidence",
            "hypergraph_incident_sum",
            "hypergraph_hyperedge_node_sum",
            "vm_graph_ops",
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
