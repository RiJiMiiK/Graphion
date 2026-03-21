#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
from datetime import datetime, timezone
from typing import TypedDict


TEST_NAMES = [
    "vm_addition_program",
    "vm_invalid_register_fails",
    "vm_deterministic_mode_toggle",
    "vm_deterministic_mode_unknown_opcode",
    "vm_add_wraparound_semantics",
    "isa_execute_golden_fixtures",
]


class SelectedTestResult(TypedDict):
    test: str
    status: str
    stdout: str


class ReportRow(TypedDict):
    test: str
    c_status: str
    asm_status: str


class ReportMetadata(TypedDict):
    generated_utc: str
    platform: str
    config: str
    build_type: str
    tests: list[str]
    cmake_args: list[str]


class ReportPayload(TypedDict):
    metadata: ReportMetadata
    rows: list[ReportRow]


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
        f"-DGRAPHION_ENABLE_ASM={'ON' if enable_asm else 'OFF'}",
    ]
    cmd.extend(extra_args)
    run(cmd)


def build(build_dir: pathlib.Path, config: str) -> None:
    run(["cmake", "--build", str(build_dir), "--config", config])


def run_selected_tests(build_dir: pathlib.Path, config: str, tests: list[str]) -> list[SelectedTestResult]:
    exe = exe_path(build_dir, "graphion_tests", config)
    results: list[SelectedTestResult] = []
    for test_name in tests:
        proc = run([str(exe), test_name])
        results.append({
            "test": test_name,
            "status": "pass",
            "stdout": proc.stdout.strip(),
        })
    return results


def render_markdown(payload: ReportPayload) -> str:
    lines = [
        "# ASM Hardening Parity Report",
        "",
        f"Generated: {payload['metadata']['generated_utc']}",
        "",
        "This report covers hardening-sensitive ISA cases on both the portable C path and the asm-enabled path.",
        "",
        "## Metadata",
        "",
        f"- Platform: {payload['metadata']['platform']}",
        f"- Config: {payload['metadata']['config']}",
        f"- Build type: {payload['metadata']['build_type']}",
        f"- Tests: {', '.join(payload['metadata']['tests'])}",
        "",
        "## Results",
        "",
        "| Test | C fallback | ASM |",
        "|---|---|---|",
    ]
    for row in payload["rows"]:
        lines.append(f"| {row['test']} | {row['c_status']} | {row['asm_status']} |")
    lines.extend([
        "",
        "## Policy",
        "",
        "- Hardening-sensitive ISA cases must pass unchanged with `GRAPHION_ENABLE_ASM=OFF` and `GRAPHION_ENABLE_ASM=ON`.",
        "- Deterministic mode remains the portable semantic reference path even when asm is compiled in.",
        "- Wraparound, invalid-register handling, and unknown-opcode behavior must remain identical across both builds.",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run asm-vs-C parity checks for hardening-sensitive ISA cases.")
    parser.add_argument("--build-root", default="build-asm-hardening", help="Build root for C/ASM parity builds")
    parser.add_argument("--config", default="Release", help="Build configuration")
    parser.add_argument("--build-type", default="Release", help="CMAKE_BUILD_TYPE for single-config generators")
    parser.add_argument(
        "--output-json",
        default="benchmarks/results/asm/asm_hardening_parity_latest.json",
        help="JSON output path",
    )
    parser.add_argument(
        "--output-md",
        default="docs/performance/reports/ASM_HARDENING_PARITY.md",
        help="Markdown output path",
    )
    parser.add_argument("cmake_args", nargs="*", help="Extra CMake args, for example -G Ninja -DCMAKE_C_COMPILER=gcc")
    args = parser.parse_args()

    if sys.platform.startswith("win"):
        print("asm hardening parity requires a SysV x86_64 environment; run it in Docker/Linux", file=sys.stderr)
        return 2

    build_root = pathlib.Path(args.build_root)
    c_build = build_root / "c-fallback"
    asm_build = build_root / "asm-hotpath"

    configure(c_build, args.build_type, False, args.cmake_args)
    build(c_build, args.config)
    c_results = run_selected_tests(c_build, args.config, TEST_NAMES)

    configure(asm_build, args.build_type, True, args.cmake_args)
    build(asm_build, args.config)
    asm_results = run_selected_tests(asm_build, args.config, TEST_NAMES)

    rows: list[ReportRow] = []
    for c_row, asm_row in zip(c_results, asm_results):
        rows.append({
            "test": c_row["test"],
            "c_status": c_row["status"],
            "asm_status": asm_row["status"],
        })

    payload: ReportPayload = {
        "metadata": {
            "generated_utc": datetime.now(timezone.utc).isoformat(),
            "platform": sys.platform,
            "config": args.config,
            "build_type": args.build_type,
            "tests": TEST_NAMES,
            "cmake_args": args.cmake_args,
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
