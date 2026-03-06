#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import subprocess
import tempfile


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def shell_join(parts: list[str]) -> str:
    return " && ".join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description="Refresh the cross-compiler optimization governance report.")
    parser.add_argument("--windows-build-root", default="build-cross-compiler-win", help="Windows MSVC build root")
    parser.add_argument("--linux-gcc-build-root", default="build-cross-compiler-gcc", help="Linux GCC build root inside Docker")
    parser.add_argument("--linux-clang-build-root", default="build-cross-compiler-clang", help="Linux Clang build root inside Docker")
    parser.add_argument("--runs", type=int, default=20, help="Runs per benchmark")
    parser.add_argument("--iterations", type=int, default=500000, help="Iterations per benchmark run")
    parser.add_argument("--skip-windows", action="store_true", help="Skip the local Windows MSVC lane")
    parser.add_argument("--skip-linux-gcc", action="store_true", help="Skip the Linux Docker GCC lane")
    parser.add_argument("--skip-linux-clang", action="store_true", help="Skip the Linux Docker Clang lane")
    args = parser.parse_args()

    windows_json = pathlib.Path("benchmarks/results/cross_compiler_windows_msvc.json")
    linux_gcc_json = pathlib.Path("benchmarks/results/cross_compiler_linux_gcc.json")
    linux_clang_json = pathlib.Path("benchmarks/results/cross_compiler_linux_clang.json")
    input_jsons: list[str] = []
    temp_root = pathlib.Path(tempfile.mkdtemp(prefix="graphion-cross-compiler-", dir="benchmarks/results"))
    windows_md = temp_root / "cross_compiler_windows.md"
    gcc_md = temp_root / "cross_compiler_gcc.md"
    clang_md = temp_root / "cross_compiler_clang.md"

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
            "Windows MSVC portable",
            "--dispatch",
            "switch",
            "--dispatch-variants",
            "switch,jumptable",
            "--runs",
            str(args.runs),
            "--variant-runs",
            str(args.runs),
            "--iterations",
            str(args.iterations),
            "--variant-iterations",
            str(args.iterations),
            "--",
            "-DGRAPHION_ENABLE_ASM=OFF",
        ])
        input_jsons.append(str(windows_json))

    if not args.skip_linux_gcc:
        run([
            "docker",
            "compose",
            "run",
            "--rm",
            "graphion-dev",
            "bash",
            "-lc",
            shell_join([
                (
                    "python3 scripts/bench/generate_optimization_report.py "
                    "--build-root {build_root} "
                    "--output-json benchmarks/results/cross_compiler_linux_gcc.json "
                    "--output-md benchmarks/results/{md_name} "
                    "--platform-label \"Linux GCC portable\" "
                    "--compiler-kind gcc --dispatch switch "
                    "--iterations {iterations} --variant-iterations {iterations} "
                    "--runs {runs} --variant-runs {runs} -- "
                    "-G Ninja -DCMAKE_C_COMPILER=gcc -DGRAPHION_ENABLE_ASM=OFF -DGRAPHION_ENABLE_IPO=OFF"
                ).format(build_root=args.linux_gcc_build_root, iterations=args.iterations, runs=args.runs, md_name=gcc_md.name)
            ]),
        ])
        input_jsons.append(str(linux_gcc_json))

    if not args.skip_linux_clang:
        run([
            "docker",
            "compose",
            "run",
            "--rm",
            "graphion-dev",
            "bash",
            "-lc",
            shell_join([
                (
                    "python3 scripts/bench/generate_optimization_report.py "
                    "--build-root {build_root} "
                    "--output-json benchmarks/results/cross_compiler_linux_clang.json "
                    "--output-md benchmarks/results/{md_name} "
                    "--platform-label \"Linux Clang portable\" "
                    "--compiler-kind clang --dispatch switch "
                    "--iterations {iterations} --variant-iterations {iterations} "
                    "--runs {runs} --variant-runs {runs} --llvm-profdata llvm-profdata -- "
                    "-G Ninja -DCMAKE_C_COMPILER=clang -DGRAPHION_ENABLE_ASM=OFF -DGRAPHION_ENABLE_IPO=OFF"
                ).format(build_root=args.linux_clang_build_root, iterations=args.iterations, runs=args.runs, md_name=clang_md.name)
            ]),
        ])
        input_jsons.append(str(linux_clang_json))

    render_cmd = [
        "python",
        "scripts/bench/render_cross_compiler_report.py",
        "--output-md",
        "docs/CROSS_COMPILER_REPORT.md",
        "--output-json",
        "benchmarks/results/cross_compiler_report_latest.json",
    ]
    for path in input_jsons:
        render_cmd.extend(["--input-json", path])
    run(render_cmd)
    for temp_file in (windows_md, gcc_md, clang_md):
        if temp_file.exists():
            temp_file.unlink()
    try:
        temp_root.rmdir()
    except OSError:
        pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
