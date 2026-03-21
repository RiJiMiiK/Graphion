#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import subprocess
import tempfile

from bench_paths import (
    OPTIMIZATION_LINUX_JSON,
    OPTIMIZATION_REPORT_JSON,
    OPTIMIZATION_REPORT_MD,
    OPTIMIZATION_WINDOWS_JSON,
    SMOKE_RESULTS_DIR,
)


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def shell_join(parts: list[str]) -> str:
    return " && ".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description="Refresh unified optimization reports across local Windows and Docker Linux.")
    parser.add_argument("--windows-build-root", default="build-opt-report-win", help="Windows build root")
    parser.add_argument("--linux-build-root", default="build-opt-report-linux", help="Linux build root inside Docker")
    parser.add_argument("--runs", type=int, default=100, help="Runs per benchmark")
    parser.add_argument("--skip-linux", action="store_true", help="Skip Linux Docker report generation")
    parser.add_argument("--skip-windows", action="store_true", help="Skip Windows report generation")
    args = parser.parse_args()

    windows_json = OPTIMIZATION_WINDOWS_JSON
    linux_json = OPTIMIZATION_LINUX_JSON
    input_jsons: list[str] = []
    temp_root = pathlib.Path(tempfile.mkdtemp(prefix="graphion-opt-report-", dir=str(SMOKE_RESULTS_DIR)))
    windows_md = temp_root / "optimization_report_windows.md"
    linux_md = temp_root / "optimization_report_linux.md"

    if not args.skip_windows:
        run([
            "python",
            "scripts/bench/generate_optimization_report.py",
            "--build-root",
            args.windows_build_root,
            "--output-json",
            str(windows_json),
            "--output-md",
            str(windows_md),
            "--platform-label",
            "Graphion Windows (MSVC)",
            "--runs",
            str(args.runs),
            "--variant-runs",
            str(args.runs),
        ])
        input_jsons.append(str(windows_json))

    if not args.skip_linux:
        linux_commands = [
            (
                "python3 scripts/bench/generate_optimization_report.py --build-root {build_root} "
                "--output-json benchmarks/results/optimization/optimization_report_linux.json "
                "--output-md benchmarks/results/smoke/{temp_dir}/{linux_md_name} "
                "--platform-label \"Graphion Linux (Docker GCC)\" "
                "--compiler-kind gcc --runs {runs} --variant-runs {runs} -- "
                "-G Ninja -DCMAKE_C_COMPILER=gcc -DGRAPHION_ENABLE_ASM=ON -DGRAPHION_ENABLE_IPO=OFF"
            ).format(
                build_root=args.linux_build_root,
                runs=args.runs,
                temp_dir=temp_root.name,
                linux_md_name=linux_md.name,
            )
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
        input_jsons.append(str(linux_json))

    render_cmd = [
        "python",
        "scripts/bench/render_optimization_reports.py",
        "--output-md",
        str(OPTIMIZATION_REPORT_MD),
        "--output-json",
        str(OPTIMIZATION_REPORT_JSON),
    ]
    for path in input_jsons:
        render_cmd.extend(["--input-json", path])
    run(render_cmd)
    for temp_file in (windows_md, linux_md):
        if temp_file.exists():
            temp_file.unlink()
    try:
        temp_root.rmdir()
    except OSError:
        pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
