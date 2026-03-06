#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import statistics
import subprocess
import sys


BENCH_SPECS = [
    {
        "benchmark": "vm_dispatch",
        "iterations": 500000,
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    },
    {
        "benchmark": "bfs_levels",
        "iterations": 200000,
        "latency_key": "ns_per_edge",
        "throughput_key": "mteps",
    },
    {
        "benchmark": "hypergraph_incidence",
        "iterations": 500000,
        "latency_key": "ns_per_incidence",
        "throughput_key": "mips",
    },
    {
        "benchmark": "hypergraph_incident_sum",
        "iterations": 500000,
        "latency_key": "ns_per_call",
        "throughput_key": "mips",
    },
    {
        "benchmark": "hypergraph_hyperedge_node_sum",
        "iterations": 500000,
        "latency_key": "ns_per_call",
        "throughput_key": "mips",
    },
    {
        "benchmark": "vm_graph_ops",
        "iterations": 300000,
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    },
]


def parse_last_json(stdout: str) -> dict[str, object]:
    for line in reversed(stdout.splitlines()):
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            return json.loads(line)
    raise ValueError("rust benchmark output did not contain a JSON payload")


def average_payloads(
    benchmark: str,
    payloads: list[dict[str, object]],
    latency_key: str,
    throughput_key: str,
    platform_label: str,
) -> dict[str, object]:
    sample = payloads[0]
    seconds = [float(row["seconds"]) for row in payloads]
    latency = [float(row[latency_key]) for row in payloads]
    throughput = [float(row[throughput_key]) for row in payloads]
    result: dict[str, object] = {
        "benchmark": benchmark,
        "platform": platform_label,
        "runs": len(payloads),
        "seconds_avg": round(statistics.fmean(seconds), 6),
        latency_key + "_avg": round(statistics.fmean(latency), 3),
        throughput_key + "_avg": round(statistics.fmean(throughput), 3),
        "latency_key": latency_key,
        "throughput_key": throughput_key,
    }
    for key in (
        "iterations",
        "instructions_per_iteration",
        "edges_per_iteration",
        "incidence_per_iteration",
        "calls_per_iteration",
    ):
        if key in sample:
            result[key] = sample[key]
    return result


def exe_path(manifest_path: pathlib.Path) -> pathlib.Path:
    project_root = manifest_path.parent
    if sys.platform.startswith("win"):
        return project_root / "target" / "release" / "graphion_rust.exe"
    return project_root / "target" / "release" / "graphion_rust"


def main() -> int:
    parser = argparse.ArgumentParser(description="Collect Rust comparison benchmark averages.")
    parser.add_argument("--manifest-path", default="graphion_rust/Cargo.toml", help="Path to graphion_rust Cargo.toml")
    parser.add_argument("--runs", type=int, default=100, help="Number of runs per benchmark")
    parser.add_argument("--platform-label", default="Rust Windows", help="Human-readable platform label")
    parser.add_argument("--output", required=True, help="Output JSON path")
    parser.add_argument("--skip-missing", action="store_true", help="Exit successfully if the Rust sandbox is absent")
    args = parser.parse_args()

    manifest_path = pathlib.Path(args.manifest_path)
    if not manifest_path.exists():
        if args.skip_missing:
            print(f"rust benchmark skipped: missing manifest {manifest_path}")
            return 0
        raise FileNotFoundError(f"missing rust manifest: {manifest_path}")

    subprocess.run(
        ["cargo", "build", "--release", "--manifest-path", str(manifest_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    exe = exe_path(manifest_path)
    if not exe.exists():
        raise FileNotFoundError(f"missing rust benchmark executable: {exe}")

    rows: list[dict[str, object]] = []
    for spec in BENCH_SPECS:
        payloads = []
        for _ in range(args.runs):
            proc = subprocess.run(
                [str(exe), str(spec["benchmark"]), str(spec["iterations"])],
                capture_output=True,
                text=True,
                check=True,
            )
            payloads.append(parse_last_json(proc.stdout))
        rows.append(
            average_payloads(
                str(spec["benchmark"]),
                payloads,
                str(spec["latency_key"]),
                str(spec["throughput_key"]),
                args.platform_label,
            )
        )

    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(rows, indent=2), encoding="utf-8")
    print(json.dumps(rows, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
