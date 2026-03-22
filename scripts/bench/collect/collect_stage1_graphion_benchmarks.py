#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import statistics
import subprocess
import sys

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))

from report_metadata import base_metadata, validate_metadata


BENCH_SPECS = {
    "vm": {
        "benchmark": "vm_dispatch",
        "target": "graphion_bench",
        "iterations": 500000,
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    },
    "gion": {
        "benchmark": "gion_stage1",
        "target": "graphion_bench_gion_stage1",
        "iterations": 20000,
        "latency_key": "ns_per_operation",
        "throughput_key": "mops",
    },
}


def exe_path(build_dir: pathlib.Path, target: str, config: str) -> pathlib.Path:
    if sys.platform.startswith("win"):
        root = build_dir / config
        if root.exists():
            return root / f"{target}.exe"
        return build_dir / f"{target}.exe"
    return build_dir / target


def parse_last_json(stdout: str) -> dict[str, object]:
    for line in reversed(stdout.splitlines()):
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            return json.loads(line)
    raise ValueError("benchmark output did not contain a JSON payload")


def run_benchmark(exe: pathlib.Path, iterations: int) -> dict[str, object]:
    proc = subprocess.run([str(exe), str(iterations)], capture_output=True, text=True, check=True)
    return parse_last_json(proc.stdout)


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
    for key in ("iterations", "instructions_per_iteration", "source_ops_per_iteration", "typed_value_ops_per_iteration"):
        if key in sample:
            result[key] = sample[key]
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Collect the stage-1 Graphion lanes (VM or .gion).")
    parser.add_argument("--mode", choices=sorted(BENCH_SPECS.keys()), required=True, help="Stage-1 lane to collect")
    parser.add_argument("--build-dir", required=True, help="CMake build directory")
    parser.add_argument("--config", default="Release", help="Build configuration")
    parser.add_argument("--runs", type=int, default=100, help="Number of runs per benchmark")
    parser.add_argument("--platform-label", required=True, help="Human-readable platform label")
    parser.add_argument("--compiler-kind", default="unknown", help="Compiler/toolchain label for this lane")
    parser.add_argument("--asm-enabled", choices=["on", "off"], default="off", help="Whether asm is enabled for this lane")
    parser.add_argument("--output", required=True, help="Output JSON path")
    args = parser.parse_args()

    spec = BENCH_SPECS[args.mode]
    build_dir = pathlib.Path(args.build_dir)
    exe = exe_path(build_dir, str(spec["target"]), args.config)
    if not exe.exists():
        raise FileNotFoundError(f"missing benchmark binary: {exe}")

    payloads = [run_benchmark(exe, int(spec["iterations"])) for _ in range(args.runs)]
    rows = [
        average_payloads(
            str(spec["benchmark"]),
            payloads,
            str(spec["latency_key"]),
            str(spec["throughput_key"]),
            args.platform_label,
        )
    ]

    payload = {
        "metadata": base_metadata(
            args.platform_label,
            args.runs,
            {
                "report_kind": "stage1-lane",
                "lane_kind": args.mode,
                "compiler_kind": args.compiler_kind,
                "asm_enabled": args.asm_enabled == "on",
                "config": args.config,
                "build_dir": str(build_dir),
            },
        ),
        "rows": rows,
    }
    validate_metadata(payload["metadata"], "collect_stage1_graphion_benchmarks", ["report_kind", "lane_kind", "compiler_kind", "asm_enabled", "config", "build_dir"])

    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps(payload, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
