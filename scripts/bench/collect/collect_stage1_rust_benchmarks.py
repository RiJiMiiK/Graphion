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


def parse_last_json(stdout: str) -> dict[str, object]:
    for line in reversed(stdout.splitlines()):
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            return json.loads(line)
    raise ValueError("rust benchmark output did not contain a JSON payload")


def exe_path(manifest_path: pathlib.Path) -> pathlib.Path:
    project_root = manifest_path.parent
    if sys.platform.startswith("win"):
        return project_root / "target" / "release" / "graphion_rust.exe"
    return project_root / "target" / "release" / "graphion_rust"


def average_payloads(payloads: list[dict[str, object]], platform_label: str) -> dict[str, object]:
    sample = payloads[0]
    seconds = [float(row["seconds"]) for row in payloads]
    latency = [float(row["ns_per_instruction"]) for row in payloads]
    throughput = [float(row["mips"]) for row in payloads]
    result: dict[str, object] = {
        "benchmark": "vm_dispatch",
        "platform": platform_label,
        "runs": len(payloads),
        "seconds_avg": round(statistics.fmean(seconds), 6),
        "ns_per_instruction_avg": round(statistics.fmean(latency), 3),
        "mips_avg": round(statistics.fmean(throughput), 3),
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    }
    for key in ("iterations", "instructions_per_iteration", "typed_value_ops_per_iteration"):
        if key in sample:
            result[key] = sample[key]
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Collect the Rust stage-1 comparison lane.")
    parser.add_argument("--manifest-path", default="graphion_rust/Cargo.toml", help="Path to graphion_rust Cargo.toml")
    parser.add_argument("--runs", type=int, default=100, help="Number of runs per benchmark")
    parser.add_argument("--platform-label", required=True, help="Human-readable platform label")
    parser.add_argument("--output", required=True, help="Output JSON path")
    parser.add_argument("--skip-missing", action="store_true", help="Exit successfully if the Rust toolchain or project is absent")
    args = parser.parse_args()

    manifest_path = pathlib.Path(args.manifest_path)
    if not manifest_path.exists():
        if args.skip_missing:
            print(f"rust stage1 benchmark skipped: missing manifest {manifest_path}")
            return 0
        raise FileNotFoundError(f"missing rust manifest: {manifest_path}")

    try:
        subprocess.run(
            ["cargo", "build", "--release", "--manifest-path", str(manifest_path)],
            check=True,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError:
        if args.skip_missing:
            print("rust stage1 benchmark skipped: cargo not found")
            return 0
        raise

    exe = exe_path(manifest_path)
    if not exe.exists():
        raise FileNotFoundError(f"missing rust benchmark executable: {exe}")

    payloads = []
    for _ in range(args.runs):
        proc = subprocess.run(
            [str(exe), "vm_dispatch", "500000"],
            capture_output=True,
            text=True,
            check=True,
        )
        payloads.append(parse_last_json(proc.stdout))

    payload = {
        "metadata": base_metadata(
            args.platform_label,
            args.runs,
            {
                "report_kind": "stage1-lane",
                "lane_kind": "rust",
                "compiler_kind": "rustc",
                "asm_enabled": False,
                "manifest_path": str(manifest_path),
            },
        ),
        "rows": [average_payloads(payloads, args.platform_label)],
    }
    validate_metadata(payload["metadata"], "collect_stage1_rust_benchmarks", ["report_kind", "lane_kind", "compiler_kind", "asm_enabled", "manifest_path"])

    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps(payload, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
