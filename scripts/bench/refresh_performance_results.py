#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def shell_join(parts: list[str]) -> str:
    return " && ".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description="Refresh PERFORMANCE_RESULTS.md from local Windows/Rust and Docker Linux benchmark runs.")
    parser.add_argument("--windows-build-dir", default="build-perf-win", help="Windows Graphion build directory")
    parser.add_argument("--linux-build-dir", default="build-linux", help="Linux Graphion build directory inside Docker")
    parser.add_argument("--runs", type=int, default=100, help="Runs per benchmark")
    parser.add_argument("--skip-rust", action="store_true", help="Skip Rust sandbox benchmarks")
    parser.add_argument("--skip-linux", action="store_true", help="Skip Linux Docker benchmarks")
    parser.add_argument("--skip-windows", action="store_true", help="Skip local Windows Graphion benchmarks")
    parser.add_argument("--dispatch-build-root", default="", help="Optional build root prefix for dispatch-variant study")
    args = parser.parse_args()

    windows_json = pathlib.Path("benchmarks/results/windows_100x_latest.json")
    linux_json = pathlib.Path("benchmarks/results/linux_100x_latest.json")
    rust_json = pathlib.Path("benchmarks/results/rust_100x_latest.json")
    dispatch_windows_json = pathlib.Path("benchmarks/results/dispatch_variants_windows.json")
    dispatch_linux_json = pathlib.Path("benchmarks/results/dispatch_variants.json")

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
            "scripts/bench/collect_graphion_benchmarks.py",
            "--build-dir",
            args.windows_build_dir,
            "--config",
            "Release",
            "--runs",
            str(args.runs),
            "--platform-label",
            "Graphion Windows",
            "--output",
            str(windows_json),
        ])
        dispatch_cmd = [
            "python",
            "scripts/bench/compare_dispatch_variants.py",
            "--iterations",
            "500000",
            "--runs",
            str(args.runs),
            "--output",
            str(dispatch_windows_json),
        ]
        if args.dispatch_build_root:
            dispatch_cmd.extend(["--build-root", args.dispatch_build_root])
        run(dispatch_cmd)

    if not args.skip_rust:
        run([
            "python",
            "scripts/bench/collect_rust_benchmarks.py",
            "--runs",
            str(args.runs),
            "--platform-label",
            "Rust Windows",
            "--output",
            str(rust_json),
            "--skip-missing",
        ])

    if not args.skip_linux:
        linux_commands = [
            "cmake -S . -B {build_dir} -DCMAKE_BUILD_TYPE=Release -DGRAPHION_ENABLE_ASM=ON -DGRAPHION_ENABLE_BENCHMARKS=ON".format(
                build_dir=args.linux_build_dir
            ),
            "cmake --build {build_dir} --config Release".format(build_dir=args.linux_build_dir),
            (
                "python3 scripts/bench/collect_graphion_benchmarks.py --build-dir {build_dir} --config Release --runs {runs} "
                "--platform-label \"Graphion Linux\" --output benchmarks/results/linux_100x_latest.json"
            ).format(build_dir=args.linux_build_dir, runs=args.runs),
            (
                "python3 scripts/bench/compare_dispatch_variants.py --iterations 500000 --runs {runs} "
                "--output benchmarks/results/dispatch_variants.json --cmake-arg=-DGRAPHION_ENABLE_ASM=ON"
            ).format(runs=args.runs),
        ]
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
        "scripts/bench/render_performance_results.py",
        "--windows-json",
        str(windows_json),
        "--linux-json",
        str(linux_json),
        "--dispatch-windows-json",
        str(dispatch_windows_json),
        "--dispatch-linux-json",
        str(dispatch_linux_json),
    ]
    if rust_json.exists():
        render_cmd.extend(["--rust-json", str(rust_json)])
    run(render_cmd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
