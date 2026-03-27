#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))
import argparse
import ctypes
import json
import os
import statistics
import subprocess
import shutil
from typing import NotRequired, TypedDict, cast

from report_metadata import base_metadata, validate_metadata


class BenchPayload(TypedDict):
    seconds: float
    benchmark: str
    iterations: NotRequired[int]
    frontier_items_per_iteration: NotRequired[int]
    instructions_per_iteration: NotRequired[int]
    edges_per_iteration: NotRequired[int]
    neighbors_per_iteration: NotRequired[int]
    edge_data_items_per_iteration: NotRequired[int]
    frontier_len: NotRequired[int]
    frontier_neighbor_work: NotRequired[int]
    recommended_frontier_mode: NotRequired[str]
    memberships_per_iteration: NotRequired[int]
    incidence_per_iteration: NotRequired[int]
    calls_per_iteration: NotRequired[int]
    typed_value_ops_per_iteration: NotRequired[int]
    expr_ops_per_iteration: NotRequired[int]
    source_ops_per_iteration: NotRequired[int]
    print_ops_per_iteration: NotRequired[int]
    ns_per_frontier_item: NotRequired[float]
    ns_per_instruction: NotRequired[float]
    ns_per_edge: NotRequired[float]
    ns_per_neighbor: NotRequired[float]
    ns_per_edge_data: NotRequired[float]
    ns_per_incidence: NotRequired[float]
    ns_per_membership: NotRequired[float]
    ns_per_call: NotRequired[float]
    ns_per_operation: NotRequired[float]
    ns_per_iteration: NotRequired[float]
    mips: NotRequired[float]
    mteps: NotRequired[float]
    mops: NotRequired[float]


BENCH_SPECS = [
    {
        "benchmark": "frontier_primitives",
        "target": "graphion_bench_frontier",
        "iterations": 10000000,
        "latency_key": "ns_per_frontier_item",
        "throughput_key": "mips",
    },
    {
        "benchmark": "vm_dispatch",
        "target": "graphion_bench",
        "iterations": 5000000,
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    },
    {
        "benchmark": "bfs_levels",
        "target": "graphion_bench_bfs",
        "iterations": 5000000,
        "latency_key": "ns_per_edge",
        "throughput_key": "mteps",
    },
    {
        "benchmark": "neighbor_iteration",
        "target": "graphion_bench_neighbors",
        "iterations": 10000000,
        "latency_key": "ns_per_neighbor",
        "throughput_key": "mteps",
    },
    {
        "benchmark": "weighted_neighbor_sums",
        "target": "graphion_bench_weighted_graph",
        "iterations": 300000,
        "latency_key": "ns_per_edge_data",
        "throughput_key": "mteps",
    },
    {
        "benchmark": "hypergraph_incidence",
        "target": "graphion_bench_hypergraph",
        "iterations": 10000000,
        "latency_key": "ns_per_incidence",
        "throughput_key": "mips",
    },
    {
        "benchmark": "hypergraph_traversal",
        "target": "graphion_bench_hypergraph_traversal",
        "iterations": 10000000,
        "latency_key": "ns_per_membership",
        "throughput_key": "mteps",
    },
    {
        "benchmark": "hypergraph_incident_sum",
        "target": "graphion_bench_hypergraph_incident_sum",
        "iterations": 10000000,
        "latency_key": "ns_per_call",
        "throughput_key": "mips",
    },
    {
        "benchmark": "hypergraph_hyperedge_node_sum",
        "target": "graphion_bench_hypergraph_hyperedge_node_sum",
        "iterations": 10000000,
        "latency_key": "ns_per_call",
        "throughput_key": "mips",
    },
    {
        "benchmark": "vm_graph_ops",
        "target": "graphion_bench_vm_graph",
        "iterations": 10000000,
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    },
    {
        "benchmark": "gion_source",
        "target": "graphion_bench_gion",
        "iterations": 5000000,
        "latency_key": "ns_per_operation",
        "throughput_key": "mops",
    },
    {
        "benchmark": "vm_print_dispatch",
        "target": "graphion_bench_vm_print",
        "iterations": 5000000,
        "latency_key": "ns_per_iteration",
        "throughput_key": "mips",
    },
    {
        "benchmark": "gion_print_source",
        "target": "graphion_bench_gion_print",
        "iterations": 5000000,
        "latency_key": "ns_per_iteration",
        "throughput_key": "mops",
    },
    {
        "benchmark": "vm_expr_dispatch",
        "target": "graphion_bench_vm_expr",
        "iterations": 10000000,
        "latency_key": "ns_per_iteration",
        "throughput_key": "mips",
    },
    {
        "benchmark": "gion_expr_source",
        "target": "graphion_bench_gion_expr",
        "iterations": 10000000,
        "latency_key": "ns_per_iteration",
        "throughput_key": "mops",
    },
]


def exe_path(build_dir: pathlib.Path, target: str, config: str) -> pathlib.Path:
    if sys.platform.startswith("win"):
        root = build_dir / config
        if root.exists():
            return root / f"{target}.exe"
        return build_dir / f"{target}.exe"
    return build_dir / target


def parse_last_json(stdout: str) -> BenchPayload:
    for line in reversed(stdout.splitlines()):
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            payload = json.loads(line)
            if not isinstance(payload, dict):
                break
            return cast(BenchPayload, payload)
    raise ValueError("benchmark output did not contain a JSON payload")


def run_benchmark(exe: pathlib.Path, iterations: int) -> BenchPayload:
    cmd = [str(exe), str(iterations)]
    if not sys.platform.startswith("win") and shutil.which("taskset") is not None:
        cmd = ["taskset", "-c", "0", *cmd]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return parse_last_json(proc.stdout)


def stabilize_windows_benchmark_host() -> None:
    if not sys.platform.startswith("win"):
        return
    kernel32 = ctypes.windll.kernel32
    handle = kernel32.GetCurrentProcess()
    HIGH_PRIORITY_CLASS = 0x00000080
    affinity_mask = 0x4
    kernel32.SetPriorityClass(handle, HIGH_PRIORITY_CLASS)
    kernel32.SetProcessAffinityMask(handle, affinity_mask)


def stabilize_posix_benchmark_host() -> None:
    if sys.platform.startswith("win"):
        return
    if hasattr(os, "sched_setaffinity"):
        try:
            os.sched_setaffinity(0, {0})
        except OSError:
            pass


def average_payloads(
    benchmark: str,
    payloads: list[BenchPayload],
    latency_key: str,
    throughput_key: str,
    platform_label: str,
) -> dict[str, object]:
    sample = payloads[0]
    seconds = [float(row["seconds"]) for row in payloads]
    latency = [float(row[latency_key]) for row in payloads]
    throughput = [float(row[throughput_key]) for row in payloads]
    variation_pct = 0.0
    if len(latency) > 1:
        mean_latency = statistics.fmean(latency)
        if mean_latency != 0.0:
            variation_pct = statistics.stdev(latency) / mean_latency * 100.0
    result: dict[str, object] = {
        "benchmark": benchmark,
        "platform": platform_label,
        "runs": len(payloads),
        "seconds_avg": round(statistics.fmean(seconds), 6),
        latency_key + "_avg": round(statistics.fmean(latency), 3),
        throughput_key + "_avg": round(statistics.fmean(throughput), 3),
        "variation_pct": round(variation_pct, 3),
        "latency_key": latency_key,
        "throughput_key": throughput_key,
    }
    for key in (
        "iterations",
        "frontier_items_per_iteration",
        "instructions_per_iteration",
        "edges_per_iteration",
        "neighbors_per_iteration",
        "edge_data_items_per_iteration",
        "frontier_len",
        "frontier_neighbor_work",
        "recommended_frontier_mode",
        "memberships_per_iteration",
        "incidence_per_iteration",
        "calls_per_iteration",
        "typed_value_ops_per_iteration",
        "expr_ops_per_iteration",
        "source_ops_per_iteration",
        "print_ops_per_iteration",
        "ns_per_iteration",
    ):
        value = sample.get(key)
        if value is not None:
            result[key] = value
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Collect Graphion benchmark averages from a built tree.")
    parser.add_argument("--build-dir", required=True, help="CMake build directory")
    parser.add_argument("--config", default="Release", help="Build configuration")
    parser.add_argument("--runs", type=int, default=100, help="Number of runs per benchmark")
    parser.add_argument("--platform-label", required=True, help="Human-readable platform label")
    parser.add_argument("--compiler-kind", default="unknown", help="Compiler/toolchain label for this lane")
    parser.add_argument("--asm-enabled", choices=["on", "off"], default="off", help="Whether asm is enabled for this lane")
    parser.add_argument("--output", required=True, help="Output JSON path")
    args = parser.parse_args()
    stabilize_windows_benchmark_host()
    stabilize_posix_benchmark_host()

    build_dir = pathlib.Path(args.build_dir)
    rows: list[dict[str, object]] = []

    for spec in BENCH_SPECS:
        exe = exe_path(build_dir, spec["target"], args.config)
        if not exe.exists():
            raise FileNotFoundError(f"missing benchmark binary: {exe}")
        payloads = [run_benchmark(exe, int(spec["iterations"])) for _ in range(args.runs)]
        rows.append(
            average_payloads(
                str(spec["benchmark"]),
                payloads,
                str(spec["latency_key"]),
                str(spec["throughput_key"]),
                args.platform_label,
            )
        )

    payload = {
        "metadata": base_metadata(
            args.platform_label,
            args.runs,
            {
                "report_kind": "performance-lane",
                "compiler_kind": args.compiler_kind,
                "asm_enabled": args.asm_enabled == "on",
                "config": args.config,
                "build_dir": str(build_dir),
            },
        ),
        "rows": rows,
    }
    validate_metadata(payload["metadata"], "collect_graphion_benchmarks", ["report_kind", "compiler_kind", "asm_enabled", "config", "build_dir"])

    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps(payload, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
