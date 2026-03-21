#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))
import argparse
import json
from datetime import datetime, timezone

from bench_paths import OPTIMIZATION_REPORT_JSON, OPTIMIZATION_REPORT_MD
from report_metadata import validate_metadata


BENCHMARK_ORDER = [
    "vm_dispatch",
    "bfs_levels",
    "hypergraph_incidence",
    "hypergraph_incident_sum",
    "hypergraph_hyperedge_node_sum",
    "vm_graph_ops",
]


def load_payload(path: pathlib.Path) -> dict[str, object]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or "metadata" not in payload:
        raise ValueError(f"{path}: expected optimization report payload with metadata")
    metadata = payload["metadata"]
    if not isinstance(metadata, dict):
        raise ValueError(f"{path}: metadata must be an object")
    validate_metadata(
        metadata,
        str(path),
        ["report_kind", "compiler_kind", "asm_enabled", "config", "build_type", "iterations", "iterations_scale", "corpus_profile", "dispatch", "dispatch_variants", "cmake_args"],
    )
    return payload


def fmt_seconds(value: object) -> str:
    if isinstance(value, (int, float)):
        return f"{float(value):.6f}"
    return str(value)


def fmt_metric(value: object) -> str:
    if isinstance(value, (int, float)):
        return f"{float(value):.3f}"
    return str(value)


def row_map(payload: dict[str, object]) -> dict[str, dict[str, object]]:
    return {str(row["benchmark"]): row for row in payload["report_rows"]}


def render_main_table(payloads: list[dict[str, object]]) -> str:
    lines = [
        "| Platform | Benchmark | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline thr | PGO thr | Speedup x |",
        "|---|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for payload in payloads:
        platform_label = str(payload["metadata"]["platform_label"])
        rows = row_map(payload)
        for benchmark in BENCHMARK_ORDER:
            row = rows[benchmark]
            baseline = row["baseline"]
            pgo = row["pgo"]
            latency_key = str(row["latency_metric"]) + "_avg"
            throughput_key = str(row["throughput_metric"]) + "_avg"
            lines.append(
                "| {platform_label} | {benchmark} | {bs} | {ps} | {bl} | {pl} | {bt} | {pt} | {sx} |".format(
                    platform_label=platform_label,
                    benchmark=benchmark,
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


def render_dispatch_tables(payloads: list[dict[str, object]]) -> list[str]:
    sections: list[str] = []
    for payload in payloads:
        meta = payload["metadata"]
        platform_label = str(meta["platform_label"])
        lines = [
            f"### {platform_label}",
            "",
            "| Variant | Baseline s | PGO s | Baseline ns_per_X | PGO ns_per_X | Baseline mips | PGO mips | Speedup x | Status |",
            "|---|---:|---:|---:|---:|---:|---:|---:|---|",
        ]
        for row in payload["dispatch_variant_rows"]:
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
        sections.append("\n".join(lines))
    return sections


def render_metadata_table(payloads: list[dict[str, object]]) -> str:
    lines = [
        "| Lane | Compiler | ASM | CPU | Git | Dispatch | Corpus | Runs | Iterations |",
        "|---|---|---|---|---|---|---|---:|---:|",
    ]
    for payload in payloads:
        meta = payload["metadata"]
        lines.append(
            "| {lane} | {compiler} | {asm} | {cpu} | {git_rev} | {dispatch} | {corpus} | {runs} | {iterations} |".format(
                lane=meta["platform_label"],
                compiler=meta["compiler_kind"],
                asm="on" if meta["asm_enabled"] else "off",
                cpu=meta["cpu_model"],
                git_rev=str(meta["git_rev"])[:12],
                dispatch=meta["dispatch"],
                corpus=meta["corpus_profile"],
                runs=meta["runs"],
                iterations=meta["iterations"],
            )
        )
    return "\n".join(lines)


def render_markdown(payloads: list[dict[str, object]]) -> str:
    generated = datetime.now(timezone.utc).isoformat()
    platform_labels = ", ".join(str(payload["metadata"]["platform_label"]) for payload in payloads)
    lines = [
        "# Official Optimization Report",
        "",
        f"Generated: {generated}",
        "",
        "## Scope",
        "",
        f"- Platforms covered: {platform_labels}",
        "- This report merges local per-platform `baseline` vs `PGO` runs.",
        "- Linux sections are intended to include Docker-based GCC/Clang workflows, including `computed-goto` when supported.",
        "",
        "## Environment Metadata",
        "",
        render_metadata_table(payloads),
        "",
        "## Baseline vs PGO",
        "",
        render_main_table(payloads),
        "",
        "## vm_dispatch By Dispatch Variant",
        "",
        *render_dispatch_tables(payloads),
        "",
        "## Notes",
        "",
        "- Latency columns (`ns_per_X`) are the primary comparison metric; lower is better.",
        "- Throughput columns (`mips` / `mteps`) are secondary confirmation metrics; higher is better.",
        "- `computed-goto` is expected only on non-MSVC paths.",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a unified optimization report from per-platform JSON artifacts.")
    parser.add_argument("--input-json", action="append", required=True, help="Input optimization report JSON (repeatable)")
    parser.add_argument("--output-md", default=str(OPTIMIZATION_REPORT_MD), help="Output Markdown path")
    parser.add_argument("--output-json", default=str(OPTIMIZATION_REPORT_JSON), help="Output merged JSON path")
    args = parser.parse_args()

    payloads = [load_payload(pathlib.Path(path)) for path in args.input_json]
    merged = {
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "platform_reports": payloads,
    }

    output_md = pathlib.Path(args.output_md)
    output_json = pathlib.Path(args.output_json)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.write_text(render_markdown(payloads), encoding="utf-8")
    output_json.write_text(json.dumps(merged, indent=2), encoding="utf-8")
    print(output_md.read_text(encoding="utf-8"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
