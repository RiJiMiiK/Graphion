#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))

from bench_paths import (
    STAGE1_COMPARISON_MD,
    STAGE1_GION_LINUX_JSON,
    STAGE1_GION_WINDOWS_JSON,
    STAGE1_RUST_LINUX_JSON,
    STAGE1_RUST_WINDOWS_JSON,
    STAGE1_VM_LINUX_JSON,
    STAGE1_VM_WINDOWS_JSON,
)


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def shell_join(parts: list[str]) -> str:
    return " && ".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description="Refresh the stage-1 comparison report.")
    parser.add_argument("--windows-build-dir", default="build-stage1-win", help="Windows Graphion build directory")
    parser.add_argument("--linux-build-dir", default="build-stage1-linux", help="Linux Graphion build directory inside Docker")
    parser.add_argument("--runs", type=int, default=100, help="Runs per lane")
    parser.add_argument("--skip-windows", action="store_true", help="Skip local Windows lanes")
    parser.add_argument("--skip-linux", action="store_true", help="Skip Linux Docker lanes")
    parser.add_argument("--skip-rust-windows", action="store_true", help="Skip local Windows Rust lane")
    parser.add_argument("--skip-rust-linux", action="store_true", help="Skip Linux Rust lane inside Docker")
    args = parser.parse_args()

    if not args.skip_windows:
        run([
            "cmake",
            "-S",
            ".",
            "-B",
            args.windows_build_dir,
            "-DCMAKE_BUILD_TYPE=Release",
            "-DGRAPHION_ENABLE_BENCHMARKS=ON",
        ])
        run(["cmake", "--build", args.windows_build_dir, "--config", "Release"])
        run([
            "python",
            "scripts/bench/collect/collect_stage1_graphion_benchmarks.py",
            "--mode",
            "vm",
            "--build-dir",
            args.windows_build_dir,
            "--config",
            "Release",
            "--runs",
            str(args.runs),
            "--platform-label",
            "VM Windows",
            "--compiler-kind",
            "msvc",
            "--asm-enabled",
            "off",
            "--output",
            str(STAGE1_VM_WINDOWS_JSON),
        ])
        run([
            "python",
            "scripts/bench/collect/collect_stage1_graphion_benchmarks.py",
            "--mode",
            "gion",
            "--build-dir",
            args.windows_build_dir,
            "--config",
            "Release",
            "--runs",
            str(args.runs),
            "--platform-label",
            "Graphion (.gion) Windows",
            "--compiler-kind",
            "msvc",
            "--asm-enabled",
            "off",
            "--output",
            str(STAGE1_GION_WINDOWS_JSON),
        ])
        if not args.skip_rust_windows:
            run([
                "python",
                "scripts/bench/collect/collect_stage1_rust_benchmarks.py",
                "--runs",
                str(args.runs),
                "--platform-label",
                "Rust Windows",
                "--output",
                str(STAGE1_RUST_WINDOWS_JSON),
                "--skip-missing",
            ])

    if not args.skip_linux:
        linux_commands = [
            "cmake -S . -B {build_dir} -DCMAKE_BUILD_TYPE=Release -DGRAPHION_ENABLE_ASM=ON -DGRAPHION_ENABLE_BENCHMARKS=ON".format(
                build_dir=args.linux_build_dir
            ),
            "cmake --build {build_dir} --config Release".format(build_dir=args.linux_build_dir),
            (
                "python3 scripts/bench/collect/collect_stage1_graphion_benchmarks.py --mode vm --build-dir {build_dir} "
                "--config Release --runs {runs} --platform-label \"VM Linux\" --compiler-kind gcc --asm-enabled on "
                "--output benchmarks/results/performance/stage1_vm_linux.json"
            ).format(build_dir=args.linux_build_dir, runs=args.runs),
            (
                "python3 scripts/bench/collect/collect_stage1_graphion_benchmarks.py --mode gion --build-dir {build_dir} "
                "--config Release --runs {runs} --platform-label \"Graphion (.gion) Linux\" --compiler-kind gcc --asm-enabled on "
                "--output benchmarks/results/performance/stage1_gion_linux.json"
            ).format(build_dir=args.linux_build_dir, runs=args.runs),
        ]
        if not args.skip_rust_linux:
            linux_commands.append(
                ". /root/.cargo/env && "
                "if command -v cargo >/dev/null 2>&1; then "
                "python3 scripts/bench/collect/collect_stage1_rust_benchmarks.py --runs {runs} --platform-label \"Rust Linux\" "
                "--output benchmarks/results/performance/stage1_rust_linux.json --skip-missing; "
                "else echo 'Rust Linux lane skipped: cargo not available'; fi".format(runs=args.runs)
            )
        run([
            "docker",
            "compose",
            "run",
            "--rm",
            "graphion-dev",
            "bash",
            "-lc",
            shell_join(linux_commands),
        ])

    render_cmd = [
        "python",
        "scripts/bench/render/render_stage1_comparison.py",
        "--vm-windows-json",
        str(STAGE1_VM_WINDOWS_JSON),
        "--vm-linux-json",
        str(STAGE1_VM_LINUX_JSON),
        "--gion-windows-json",
        str(STAGE1_GION_WINDOWS_JSON),
        "--gion-linux-json",
        str(STAGE1_GION_LINUX_JSON),
        "--rust-windows-json",
        str(STAGE1_RUST_WINDOWS_JSON),
        "--rust-linux-json",
        str(STAGE1_RUST_LINUX_JSON),
        "--output",
        str(STAGE1_COMPARISON_MD),
    ]
    run(render_cmd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
