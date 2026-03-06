#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import platform
import shutil
import statistics
import subprocess
import sys
from datetime import datetime, timezone

from pgo_corpus import corpus_profile_names, coverage_classes, expanded_workloads


BENCH_SPECS = [
    {
        "target": "graphion_bench",
        "benchmark": "vm_dispatch",
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    },
    {
        "target": "graphion_bench_bfs",
        "benchmark": "bfs_levels",
        "latency_key": "ns_per_edge",
        "throughput_key": "mteps",
    },
    {
        "target": "graphion_bench_hypergraph",
        "benchmark": "hypergraph_incidence",
        "latency_key": "ns_per_incidence",
        "throughput_key": "mips",
    },
    {
        "target": "graphion_bench_hypergraph_incident_sum",
        "benchmark": "hypergraph_incident_sum",
        "latency_key": "ns_per_call",
        "throughput_key": "mips",
    },
    {
        "target": "graphion_bench_hypergraph_hyperedge_node_sum",
        "benchmark": "hypergraph_hyperedge_node_sum",
        "latency_key": "ns_per_call",
        "throughput_key": "mips",
    },
    {
        "target": "graphion_bench_vm_graph",
        "benchmark": "vm_graph_ops",
        "latency_key": "ns_per_instruction",
        "throughput_key": "mips",
    },
]

def run(cmd: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    print("+", " ".join(cmd))
    try:
        return subprocess.run(cmd, check=True, capture_output=True, text=True, env=env)
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


def configure(
    build_dir: pathlib.Path,
    build_type: str,
    pgo_mode: str,
    profile_dir: pathlib.Path,
    dispatch: str,
    extra_args: list[str],
) -> None:
    cmd = [
        "cmake",
        "-S",
        ".",
        "-B",
        str(build_dir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DGRAPHION_PGO_MODE={pgo_mode}",
        f"-DGRAPHION_PGO_PROFILE_DIR={profile_dir}",
        f"-DGRAPHION_VM_DISPATCH={dispatch}",
    ]
    cmd.extend(extra_args)
    run(cmd)


def build(build_dir: pathlib.Path, config: str) -> None:
    run(["cmake", "--build", str(build_dir), "--config", config])


def ctest(build_dir: pathlib.Path, config: str) -> None:
    run(["ctest", "--test-dir", str(build_dir), "--output-on-failure", "-C", config])


def detect_compiler_kind(extra_args: list[str]) -> str:
    text = " ".join(extra_args).lower()
    if "clang" in text:
        return "clang"
    if "gcc" in text:
        return "gcc"
    if sys.platform.startswith("win"):
        return "msvc"
    return "gcc"


def cleanup_profile_dir(profile_dir: pathlib.Path) -> None:
    if profile_dir.exists():
        shutil.rmtree(profile_dir)
    profile_dir.mkdir(parents=True, exist_ok=True)


def cleanup_msvc_profile_artifacts(build_dir: pathlib.Path, config: str) -> None:
    root = build_dir / config
    if not root.exists():
        return
    for pattern in ("*.pgc", "*.pgd"):
        for path in root.glob(pattern):
            path.unlink()


def train_workload(
    build_dir: pathlib.Path,
    config: str,
    profile_dir: pathlib.Path,
    iterations_scale: float,
    compiler_kind: str,
    corpus_profile: str,
) -> None:
    training_rows = expanded_workloads(corpus_profile, iterations_scale)
    env = os.environ.copy()
    if compiler_kind == "clang":
        env["LLVM_PROFILE_FILE"] = str(profile_dir / "graphion-%p-%m.profraw")

    run([str(exe_path(build_dir, "graphion", config))], env=env)
    for row in training_rows:
        run([str(exe_path(build_dir, str(row["target"]), config)), str(row["iterations"])], env=env)
    run([str(exe_path(build_dir, "graphion_tests", config))], env=env)


def merge_clang_profiles(profile_dir: pathlib.Path, llvm_profdata: str) -> None:
    profraw = sorted(str(path) for path in profile_dir.glob("*.profraw"))
    if not profraw:
        raise FileNotFoundError(f"no .profraw files found in {profile_dir}")
    run([llvm_profdata, "merge", "-output", str(profile_dir / "graphion.profdata"), *profraw])


def parse_last_json(stdout: str) -> dict[str, object]:
    for line in reversed(stdout.splitlines()):
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            return json.loads(line)
    raise ValueError("benchmark output did not contain a JSON payload")


def average_payloads(payloads: list[dict[str, object]], latency_key: str, throughput_key: str) -> dict[str, object]:
    seconds = [float(item["seconds"]) for item in payloads]
    latency = [float(item[latency_key]) for item in payloads]
    throughput = [float(item[throughput_key]) for item in payloads]
    sample = payloads[0]
    result: dict[str, object] = {
        "benchmark": sample["benchmark"],
        "runs": len(payloads),
        "seconds_avg": round(statistics.fmean(seconds), 6),
        latency_key + "_avg": round(statistics.fmean(latency), 3),
        throughput_key + "_avg": round(statistics.fmean(throughput), 3),
        "seconds_min": round(min(seconds), 6),
        "seconds_max": round(max(seconds), 6),
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


def run_benchmark(build_dir: pathlib.Path, config: str, target: str, iterations: int) -> dict[str, object]:
    proc = run([str(exe_path(build_dir, target, config)), str(iterations)])
    return parse_last_json(proc.stdout)


def run_suite(build_dir: pathlib.Path, config: str, iterations: int, runs: int) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for spec in BENCH_SPECS:
        payloads = [run_benchmark(build_dir, config, spec["target"], iterations) for _ in range(runs)]
        rows.append(average_payloads(payloads, spec["latency_key"], spec["throughput_key"]))
    return rows


def run_dispatch_variants(
    root_dir: pathlib.Path,
    config: str,
    build_type: str,
    compiler_kind: str,
    iterations: int,
    runs: int,
    iterations_scale: float,
    llvm_profdata: str,
    extra_args: list[str],
    variants: list[str],
    corpus_profile: str,
) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []

    for variant in variants:
        if variant == "computed-goto" and sys.platform.startswith("win"):
            rows.append({
                "variant": variant,
                "status": "skipped",
                "reason": "computed-goto is not supported with MSVC/Windows",
            })
            continue

        base_build_dir = root_dir / f"dispatch-{variant}-baseline"
        pgo_build_dir = root_dir / f"dispatch-{variant}-pgo"
        base_profile_dir = base_build_dir / "pgo-data"
        pgo_profile_dir = pgo_build_dir / "pgo-data"

        configure(base_build_dir, build_type, "OFF", base_profile_dir, variant, extra_args)
        build(base_build_dir, config)
        baseline_payloads = [run_benchmark(base_build_dir, config, "graphion_bench", iterations) for _ in range(runs)]
        baseline = average_payloads(baseline_payloads, "ns_per_instruction", "mips")

        cleanup_profile_dir(pgo_profile_dir)
        if compiler_kind == "msvc":
            cleanup_msvc_profile_artifacts(pgo_build_dir, config)
        configure(pgo_build_dir, build_type, "GENERATE", pgo_profile_dir, variant, extra_args)
        build(pgo_build_dir, config)
        train_workload(pgo_build_dir, config, pgo_profile_dir, iterations_scale, compiler_kind, corpus_profile)
        if compiler_kind == "clang":
            merge_clang_profiles(pgo_profile_dir, llvm_profdata)
        configure(pgo_build_dir, build_type, "USE", pgo_profile_dir, variant, extra_args)
        build(pgo_build_dir, config)
        ctest(pgo_build_dir, config)
        pgo_payloads = [run_benchmark(pgo_build_dir, config, "graphion_bench", iterations) for _ in range(runs)]
        pgo = average_payloads(pgo_payloads, "ns_per_instruction", "mips")

        rows.append({
            "variant": variant,
            "status": "ok",
            "baseline": baseline,
            "pgo": pgo,
            "speedup_x": round(float(baseline["ns_per_instruction_avg"]) / float(pgo["ns_per_instruction_avg"]), 3),
            "throughput_gain_pct": round(
                ((float(pgo["mips_avg"]) - float(baseline["mips_avg"])) / float(baseline["mips_avg"])) * 100.0,
                2,
            ),
        })
    return rows


def build_report_rows(baseline_rows: list[dict[str, object]], pgo_rows: list[dict[str, object]]) -> list[dict[str, object]]:
    baseline_map = {str(row["benchmark"]): row for row in baseline_rows}
    pgo_map = {str(row["benchmark"]): row for row in pgo_rows}
    report_rows: list[dict[str, object]] = []

    for spec in BENCH_SPECS:
        name = spec["benchmark"]
        baseline = baseline_map[name]
        pgo = pgo_map[name]
        latency_key = spec["latency_key"] + "_avg"
        throughput_key = spec["throughput_key"] + "_avg"
        baseline_latency = float(baseline[latency_key])
        pgo_latency = float(pgo[latency_key])
        baseline_throughput = float(baseline[throughput_key])
        pgo_throughput = float(pgo[throughput_key])
        report_rows.append({
            "benchmark": name,
            "latency_metric": spec["latency_key"],
            "throughput_metric": spec["throughput_key"],
            "baseline": baseline,
            "pgo": pgo,
            "speedup_x": round(baseline_latency / pgo_latency, 3),
            "latency_delta_pct": round(((pgo_latency - baseline_latency) / baseline_latency) * 100.0, 2),
            "throughput_gain_pct": round(((pgo_throughput - baseline_throughput) / baseline_throughput) * 100.0, 2),
        })
    return report_rows


def fmt_seconds(value: object) -> str:
    if isinstance(value, float):
        return f"{value:.6f}"
    return str(value)


def fmt_metric(value: object) -> str:
    if isinstance(value, float):
        return f"{value:.3f}"
    return str(value)


def markdown_table(report_rows: list[dict[str, object]]) -> str:
    lines = [
        "| Benchmark | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline thr | PGO thr | Speedup x |",
        "|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for row in report_rows:
        baseline = row["baseline"]
        pgo = row["pgo"]
        latency_key = row["latency_metric"] + "_avg"
        throughput_key = row["throughput_metric"] + "_avg"
        lines.append(
            "| {benchmark} | {bs} | {ps} | {bl} | {pl} | {bt} | {pt} | {sx} |".format(
                benchmark=row["benchmark"],
                bs=fmt_seconds(baseline["seconds_avg"]),
                ps=fmt_seconds(pgo["seconds_avg"]),
                bl=fmt_metric(baseline[latency_key]),
                pl=fmt_metric(pgo[latency_key]),
                bt=fmt_metric(baseline[throughput_key]),
                pt=fmt_metric(pgo[throughput_key]),
                sx=fmt_metric(row["speedup_x"]),
            )
        )
    return "\n".join(lines)


def markdown_dispatch_table(dispatch_rows: list[dict[str, object]]) -> str:
    lines = [
        "| Variant | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline mips | PGO mips | Speedup x | Status |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---|",
    ]
    for row in dispatch_rows:
        if row["status"] != "ok":
            lines.append(f"| {row['variant']} | - | - | - | - | - | - | - | {row['reason']} |")
            continue
        baseline = row["baseline"]
        pgo = row["pgo"]
        lines.append(
            "| {variant} | {bs} | {ps} | {bl} | {pl} | {bt} | {pt} | {sx} | ok |".format(
                variant=row["variant"],
                bs=fmt_seconds(baseline["seconds_avg"]),
                ps=fmt_seconds(pgo["seconds_avg"]),
                bl=fmt_metric(baseline["ns_per_instruction_avg"]),
                pl=fmt_metric(pgo["ns_per_instruction_avg"]),
                bt=fmt_metric(baseline["mips_avg"]),
                pt=fmt_metric(pgo["mips_avg"]),
                sx=fmt_metric(row["speedup_x"]),
            )
        )
    return "\n".join(lines)


def render_markdown(payload: dict[str, object]) -> str:
    meta = payload["metadata"]
    improved = sum(1 for row in payload["report_rows"] if float(row["speedup_x"]) > 1.0)
    total = len(payload["report_rows"])
    lines = [
        "# Official Optimization Report",
        "",
        f"Generated: {meta['generated_utc']}",
        "",
        "## Metadata",
        "",
        f"- Platform label: {meta['platform_label']}",
        f"- Platform: {meta['platform']}",
        f"- Compiler: {meta['compiler_kind']}",
        f"- Config: {meta['config']}",
        f"- Iterations per benchmark run: {meta['iterations']}",
        f"- Averaging runs: {meta['runs']}",
        f"- PGO training scale: {meta['iterations_scale']}",
        f"- PGO corpus profile: {meta['corpus_profile']}",
        f"- PGO corpus coverage classes: {', '.join(meta['corpus_coverage_classes'])}",
        f"- PGO training targets: {', '.join(meta['corpus_targets'])}",
        f"- Main suite dispatch: {meta['dispatch']}",
        f"- Dispatch variants checked: {', '.join(meta['dispatch_variants'])}",
        "",
        "## Summary",
        "",
        f"- PGO improved {improved}/{total} benchmark families on this snapshot.",
        f"- The comparison set covers `vm_dispatch`, `bfs_levels`, `vm_graph_ops`, and all current `hypergraph_*` benches.",
        "",
        "## Baseline vs PGO",
        "",
        markdown_table(payload["report_rows"]),
        "",
        "## vm_dispatch By Dispatch Variant",
        "",
        markdown_dispatch_table(payload["dispatch_variant_rows"]),
        "",
        "## Notes",
        "",
        "- Latency columns (`ns_per_X`) are the primary comparison metric; lower is better.",
        "- Throughput columns (`mips` / `mteps`) are secondary confirmation metrics; higher is better.",
        "- PGO training uses the committed Graphion representative-workload policy before rebuilding in `USE` mode.",
    ]
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate an official Graphion optimization report (baseline vs PGO).")
    parser.add_argument("--build-root", default="build-optimization-report", help="Directory root used for report builds")
    parser.add_argument("--output-json", default="benchmarks/results/optimization_report_latest.json", help="JSON report output path")
    parser.add_argument("--output-md", default="docs/OPTIMIZATION_REPORTS.md", help="Markdown report output path")
    parser.add_argument("--platform-label", default="", help="Human-readable platform label used in the report metadata")
    parser.add_argument("--config", default="Release", help="Build configuration")
    parser.add_argument("--build-type", default="Release", help="CMAKE_BUILD_TYPE for single-config generators")
    parser.add_argument("--compiler-kind", choices=["auto", "msvc", "gcc", "clang"], default="auto")
    parser.add_argument("--dispatch", default="switch", help="Dispatch strategy used for the main benchmark suite")
    parser.add_argument("--dispatch-variants", default="switch,jumptable,computed-goto", help="Comma-separated vm_dispatch variants to report")
    parser.add_argument("--iterations", type=int, default=500000, help="Iterations per benchmark run")
    parser.add_argument("--runs", type=int, default=100, help="Averaging runs for the main suite")
    parser.add_argument("--variant-iterations", type=int, default=500000, help="Iterations per vm_dispatch variant run")
    parser.add_argument("--variant-runs", type=int, default=100, help="Averaging runs for dispatch-variant vm_dispatch")
    parser.add_argument("--iterations-scale", type=float, default=1.0, help="Scale applied to PGO training iterations")
    parser.add_argument("--corpus-profile", choices=corpus_profile_names(), default="representative",
                        help="Named PGO training corpus")
    parser.add_argument("--llvm-profdata", default="llvm-profdata", help="Clang profile merge tool")
    parser.add_argument("cmake_args", nargs="*", help="Extra CMake args, for example -G Ninja -DCMAKE_C_COMPILER=clang")
    args = parser.parse_args()

    build_root = pathlib.Path(args.build_root)
    output_json = pathlib.Path(args.output_json)
    output_md = pathlib.Path(args.output_md)
    baseline_build_dir = build_root / "baseline"
    pgo_build_dir = build_root / "pgo"
    baseline_profile_dir = baseline_build_dir / "pgo-data"
    pgo_profile_dir = pgo_build_dir / "pgo-data"
    compiler_kind = args.compiler_kind
    if compiler_kind == "auto":
        compiler_kind = detect_compiler_kind(args.cmake_args)

    dispatch_variants = [item.strip() for item in args.dispatch_variants.split(",") if item.strip()]
    platform_label = args.platform_label if args.platform_label else platform.platform()

    cleanup_profile_dir(baseline_profile_dir)
    cleanup_profile_dir(pgo_profile_dir)
    if compiler_kind == "msvc":
        cleanup_msvc_profile_artifacts(pgo_build_dir, args.config)

    configure(baseline_build_dir, args.build_type, "OFF", baseline_profile_dir, args.dispatch, args.cmake_args)
    build(baseline_build_dir, args.config)
    ctest(baseline_build_dir, args.config)
    baseline_rows = run_suite(baseline_build_dir, args.config, args.iterations, args.runs)

    configure(pgo_build_dir, args.build_type, "GENERATE", pgo_profile_dir, args.dispatch, args.cmake_args)
    build(pgo_build_dir, args.config)
    train_workload(pgo_build_dir, args.config, pgo_profile_dir, args.iterations_scale, compiler_kind, args.corpus_profile)
    if compiler_kind == "clang":
        merge_clang_profiles(pgo_profile_dir, args.llvm_profdata)
    configure(pgo_build_dir, args.build_type, "USE", pgo_profile_dir, args.dispatch, args.cmake_args)
    build(pgo_build_dir, args.config)
    ctest(pgo_build_dir, args.config)
    pgo_rows = run_suite(pgo_build_dir, args.config, args.iterations, args.runs)

    report_rows = build_report_rows(baseline_rows, pgo_rows)
    dispatch_variant_rows = run_dispatch_variants(
        build_root,
        args.config,
        args.build_type,
        compiler_kind,
        args.variant_iterations,
        args.variant_runs,
        args.iterations_scale,
        args.llvm_profdata,
        args.cmake_args,
        dispatch_variants,
        args.corpus_profile,
    )

    payload = {
        "metadata": {
            "generated_utc": datetime.now(timezone.utc).isoformat(),
            "platform_label": platform_label,
            "platform": platform.platform(),
            "compiler_kind": compiler_kind,
            "config": args.config,
            "build_type": args.build_type,
            "iterations": args.iterations,
            "runs": args.runs,
            "iterations_scale": args.iterations_scale,
            "corpus_profile": args.corpus_profile,
            "corpus_coverage_classes": coverage_classes(args.corpus_profile),
            "corpus_targets": [str(row["target"]) for row in expanded_workloads(args.corpus_profile, 1.0)],
            "dispatch": args.dispatch,
            "dispatch_variants": dispatch_variants,
        },
        "baseline_rows": baseline_rows,
        "pgo_rows": pgo_rows,
        "report_rows": report_rows,
        "dispatch_variant_rows": dispatch_variant_rows,
    }

    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    output_md.write_text(render_markdown(payload), encoding="utf-8")

    print(json.dumps(payload, indent=2))
    print(f"markdown report written to {output_md}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
