#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import statistics
import subprocess
import sys
from datetime import datetime, timezone


BENCH_SPECS = [
    {
        "target": "graphion_bench",
        "benchmark": "vm_dispatch",
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
        "minimum_speedup_x": 1.05,
        "target_speedup_x": 1.15,
        "rationale": "Assembly is only justified here if the arithmetic hotpath is measurably faster than the C fallback.",
    },
    {
        "target": "graphion_bench_vm_graph",
        "benchmark": "vm_graph_ops",
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
        "minimum_speedup_x": 0.98,
        "target_speedup_x": 1.00,
        "rationale": "Non-targeted graph VM workloads must not materially regress when asm is enabled.",
    },
]


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    print("+", " ".join(cmd))
    try:
        return subprocess.run(cmd, check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as exc:
        if exc.stdout:
            print(exc.stdout)
        if exc.stderr:
            print(exc.stderr, file=sys.stderr)
        raise


def exe_path(build_dir: pathlib.Path, target: str, config: str) -> pathlib.Path:
    if sys.platform.startswith("win"):
        root = build_dir / config
        if root.exists():
            return root / f"{target}.exe"
        return build_dir / f"{target}.exe"
    return build_dir / target


def configure(build_dir: pathlib.Path, build_type: str, enable_asm: bool, extra_args: list[str]) -> None:
    cmd = [
        "cmake",
        "-S",
        ".",
        "-B",
        str(build_dir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DGRAPHION_ENABLE_BENCHMARKS=ON",
        f"-DGRAPHION_ENABLE_ASM={'ON' if enable_asm else 'OFF'}",
    ]
    cmd.extend(extra_args)
    run(cmd)


def build(build_dir: pathlib.Path, config: str) -> None:
    run(["cmake", "--build", str(build_dir), "--config", config])


def ctest(build_dir: pathlib.Path, config: str) -> None:
    run(["ctest", "--test-dir", str(build_dir), "--output-on-failure", "-C", config])


def parse_last_json(stdout: str) -> dict[str, object]:
    for line in reversed(stdout.splitlines()):
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            return json.loads(line)
    raise ValueError("benchmark output did not contain a JSON payload")


def run_benchmark(build_dir: pathlib.Path, config: str, target: str, iterations: int) -> dict[str, object]:
    proc = run([str(exe_path(build_dir, target, config)), str(iterations)])
    return parse_last_json(proc.stdout)


def average_payloads(payloads: list[dict[str, object]], latency_key: str, throughput_key: str) -> dict[str, object]:
    seconds = [float(item["seconds"]) for item in payloads]
    latency = [float(item[latency_key]) for item in payloads]
    throughput = [float(item[throughput_key]) for item in payloads]
    checksums = [int(item["checksum"]) for item in payloads]
    sample = payloads[0]
    return {
        "benchmark": sample["benchmark"],
        "runs": len(payloads),
        "seconds_avg": round(statistics.fmean(seconds), 6),
        latency_key + "_avg": round(statistics.fmean(latency), 3),
        throughput_key + "_avg": round(statistics.fmean(throughput), 3),
        "checksum_set": sorted(set(checksums)),
        "iterations": sample["iterations"],
        "instructions_per_iteration": sample.get("instructions_per_iteration", 0),
    }


def status_for(speedup_x: float, minimum_speedup_x: float, target_speedup_x: float) -> str:
    if speedup_x >= target_speedup_x:
        return "meets-target"
    if speedup_x >= minimum_speedup_x:
        return "meets-minimum"
    return "below-minimum"


def render_markdown(payload: dict[str, object]) -> str:
    lines = [
        "# ASM Fallback Report",
        "",
        f"Generated: {payload['metadata']['generated_utc']}",
        "",
        "This report reflects the latest local asm-vs-C comparison run. It is a policy and parity checkpoint, not an automatic release baseline.",
        "",
        "## Metadata",
        "",
        f"- Platform: {payload['metadata']['platform']}",
        f"- Compiler kind: {payload['metadata']['compiler_kind']}",
        f"- Config: {payload['metadata']['config']}",
        f"- Iterations: {payload['metadata']['iterations']}",
        f"- Runs: {payload['metadata']['runs']}",
        "",
        "## Parity",
        "",
        f"- C fallback tests: {'pass' if payload['parity']['c_tests_passed'] else 'fail'}",
        f"- ASM tests: {'pass' if payload['parity']['asm_tests_passed'] else 'fail'}",
        f"- Benchmark checksum parity: {'pass' if payload['parity']['checksum_parity'] else 'fail'}",
        "",
        "## Performance",
        "",
        "| Benchmark | C s | ASM s | C ns_per_X | ASM ns_per_X | C thr | ASM thr | Speedup x | Min x | Target x | Status |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|",
    ]

    for row in payload["rows"]:
        lines.append(
            "| {benchmark} | {c_s:.6f} | {a_s:.6f} | {c_l:.3f} | {a_l:.3f} | {c_t:.3f} | {a_t:.3f} | {sx:.3f} | {mn:.3f} | {tg:.3f} | {status} |".format(
                benchmark=row["benchmark"],
                c_s=float(row["c"]["seconds_avg"]),
                a_s=float(row["asm"]["seconds_avg"]),
                c_l=float(row["c"][row["latency_key"] + "_avg"]),
                a_l=float(row["asm"][row["latency_key"] + "_avg"]),
                c_t=float(row["c"][row["throughput_key"] + "_avg"]),
                a_t=float(row["asm"][row["throughput_key"] + "_avg"]),
                sx=float(row["speedup_x"]),
                mn=float(row["minimum_speedup_x"]),
                tg=float(row["target_speedup_x"]),
                status=row["status"],
            )
        )

    lines.extend([
        "",
        "## Policy",
        "",
        "- The C path remains the semantic reference implementation.",
        "- Assembly is acceptable only if unit tests pass on both builds and benchmark checksums match.",
        "- `vm_dispatch` must show measured improvement to justify the extra maintenance cost of asm.",
        "- Non-targeted workloads may not materially regress when asm is enabled.",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare assembly hotpath against the C fallback.")
    parser.add_argument("--build-root", default="build-asm-fallback", help="Build root used for C/ASM comparison")
    parser.add_argument("--config", default="Release", help="Build configuration")
    parser.add_argument("--build-type", default="Release", help="CMAKE_BUILD_TYPE for single-config generators")
    parser.add_argument("--iterations", type=int, default=500000, help="Iterations per benchmark run")
    parser.add_argument("--runs", type=int, default=20, help="Runs per benchmark")
    parser.add_argument("--output-json", default="benchmarks/results/asm_fallback_report_latest.json", help="JSON output path")
    parser.add_argument("--output-md", default="docs/ASM_FALLBACK_REPORT.md", help="Markdown output path")
    parser.add_argument("cmake_args", nargs="*", help="Extra CMake args, for example -G Ninja -DCMAKE_C_COMPILER=clang")
    args = parser.parse_args()

    if sys.platform.startswith("win"):
        print("asm fallback comparison requires a SysV x86_64 environment; run it in Docker/Linux", file=sys.stderr)
        return 2

    build_root = pathlib.Path(args.build_root)
    c_build = build_root / "c-fallback"
    asm_build = build_root / "asm-hotpath"

    configure(c_build, args.build_type, False, args.cmake_args)
    build(c_build, args.config)
    ctest(c_build, args.config)

    configure(asm_build, args.build_type, True, args.cmake_args)
    build(asm_build, args.config)
    ctest(asm_build, args.config)

    rows: list[dict[str, object]] = []
    checksum_parity = True

    for spec in BENCH_SPECS:
        c_payloads = [run_benchmark(c_build, args.config, spec["target"], args.iterations) for _ in range(args.runs)]
        asm_payloads = [run_benchmark(asm_build, args.config, spec["target"], args.iterations) for _ in range(args.runs)]
        c_avg = average_payloads(c_payloads, spec["latency_key"], spec["throughput_key"])
        asm_avg = average_payloads(asm_payloads, spec["latency_key"], spec["throughput_key"])
        if c_avg["checksum_set"] != asm_avg["checksum_set"]:
            checksum_parity = False
        speedup_x = float(c_avg[spec["latency_key"] + "_avg"]) / float(asm_avg[spec["latency_key"] + "_avg"])
        rows.append({
            "benchmark": spec["benchmark"],
            "latency_key": spec["latency_key"],
            "throughput_key": spec["throughput_key"],
            "minimum_speedup_x": spec["minimum_speedup_x"],
            "target_speedup_x": spec["target_speedup_x"],
            "rationale": spec["rationale"],
            "c": c_avg,
            "asm": asm_avg,
            "speedup_x": round(speedup_x, 3),
            "status": status_for(speedup_x, spec["minimum_speedup_x"], spec["target_speedup_x"]),
        })

    payload = {
        "metadata": {
            "generated_utc": datetime.now(timezone.utc).isoformat(),
            "platform": sys.platform,
            "compiler_kind": "auto",
            "config": args.config,
            "build_type": args.build_type,
            "iterations": args.iterations,
            "runs": args.runs,
            "cmake_args": args.cmake_args,
        },
        "parity": {
            "c_tests_passed": True,
            "asm_tests_passed": True,
            "checksum_parity": checksum_parity,
        },
        "rows": rows,
    }

    output_json = pathlib.Path(args.output_json)
    output_md = pathlib.Path(args.output_md)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    output_md.write_text(render_markdown(payload), encoding="utf-8")
    print(json.dumps(payload, indent=2))
    print(f"markdown report written to {output_md}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
