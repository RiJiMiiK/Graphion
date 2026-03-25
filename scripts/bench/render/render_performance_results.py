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

from bench_paths import PERFORMANCE_RESULTS_MD
from report_metadata import validate_metadata


BENCHMARK_ORDER = [
    "frontier_primitives",
    "vm_dispatch",
    "bfs_levels",
    "neighbor_iteration",
    "weighted_neighbor_sums",
    "hypergraph_incidence",
    "hypergraph_traversal",
    "hypergraph_incident_sum",
    "hypergraph_hyperedge_node_sum",
    "vm_graph_ops",
    "gion_source",
]

DISPLAY_NAMES = {
    "frontier_primitives": "frontier_primitives",
    "vm_dispatch": "vm_dispatch",
    "bfs_levels": "bfs_levels",
    "neighbor_iteration": "neighbor_iteration",
    "weighted_neighbor_sums": "weighted_neighbor_sums",
    "hypergraph_incidence": "hypergraph_incidence",
    "hypergraph_traversal": "hypergraph_traversal",
    "hypergraph_incident_sum": "hypergraph_incident_sum",
    "hypergraph_hyperedge_node_sum": "hypergraph_hyperedge_node_sum",
    "vm_graph_ops": "vm_graph_ops",
    "gion_source": "gion_source",
}

LATENCY_LABELS = {
    "ns_per_frontier_item": "ns_per_frontier_item",
    "ns_per_instruction": "ns_per_instruction",
    "ns_per_edge": "ns_per_edge",
    "ns_per_neighbor": "ns_per_neighbor",
    "ns_per_edge_data": "ns_per_edge_data",
    "ns_per_incidence": "ns_per_incidence",
    "ns_per_membership": "ns_per_membership",
    "ns_per_call": "ns_per_call",
    "ns_per_operation": "ns_per_operation",
}


def load_payload(path: pathlib.Path) -> dict[str, object]:
    if not path.exists():
        return {"metadata": {}, "rows": []}
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or "metadata" not in payload or "rows" not in payload:
        raise ValueError(f"{path}: expected payload with top-level metadata + rows")
    return payload


def load_rows(path: pathlib.Path, extra_required: list[str]) -> tuple[dict[str, object], list[dict[str, object]]]:
    payload = load_payload(path)
    metadata = payload["metadata"]
    if not isinstance(metadata, dict):
        raise ValueError(f"{path}: metadata must be an object")
    validate_metadata(metadata, str(path), extra_required)
    rows = payload["rows"]
    if not isinstance(rows, list):
        raise ValueError(f"{path}: rows must be a list")
    return metadata, rows


def index_rows(rows: list[dict[str, object]]) -> dict[str, dict[str, object]]:
    return {str(row["benchmark"]): row for row in rows}


def fmt(value: object) -> str:
    if isinstance(value, (int, float)):
        return f"{float(value):.3f}"
    return str(value)


def fmt_seconds(value: object) -> str:
    if isinstance(value, (int, float)):
        return f"{float(value):.6f}"
    return str(value)


def meta_int(meta: dict[str, object], key: str) -> int:
    value = meta.get(key)
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value)
    raise ValueError(f"metadata field {key!r} must be an int-compatible value")


def meta_str(meta: dict[str, object], key: str) -> str:
    value = meta.get(key)
    if isinstance(value, str):
        return value
    raise ValueError(f"metadata field {key!r} must be a string")


def meta_bool(meta: dict[str, object], key: str) -> bool:
    value = meta.get(key)
    if isinstance(value, bool):
        return value
    raise ValueError(f"metadata field {key!r} must be a bool")


def metric_value(row: dict[str, object], key: str) -> object:
    return row.get(key + "_avg", "-")


def throughput_cell(row: dict[str, object]) -> str:
    key = str(row["throughput_key"])
    return fmt(metric_value(row, key))


def mteps_cell(row: dict[str, object]) -> str:
    return throughput_cell(row) if str(row["throughput_key"]) == "mteps" else "-"


def mips_cell(row: dict[str, object]) -> str:
    return throughput_cell(row) if str(row["throughput_key"]) == "mips" else "-"


def render_benchmark_section(name: str, row_sets: list[dict[str, dict[str, object]]]) -> str:
    available = [rows[name] for rows in row_sets if name in rows]
    if not available:
        return ""
    latency_key = str(available[0]["latency_key"])
    lines = [
        f"## {DISPLAY_NAMES[name]} (`{LATENCY_LABELS[latency_key]}`)",
        "",
        "| Platform | s | mteps | mips | ns_per_X |",
        "|---|---:|---:|---:|---:|",
    ]
    for row in available:
        lines.append(
            f"| {row['platform']} | {fmt_seconds(row['seconds_avg'])} | {mteps_cell(row)} | {mips_cell(row)} | {fmt(metric_value(row, latency_key))} |"
        )
    mode_rows = [row for row in available if "recommended_frontier_mode" in row]
    if mode_rows:
        lines.append("")
        lines.append("Frontier mode notes:")
        lines.append("")
        for row in mode_rows:
            lines.append(
                "- {platform}: mode=`{mode}` frontier_len={frontier_len} frontier_neighbor_work={neighbor_work}".format(
                    platform=row["platform"],
                    mode=row["recommended_frontier_mode"],
                    frontier_len=row.get("frontier_len", "?"),
                    neighbor_work=row.get("frontier_neighbor_work", "?"),
                )
            )
    lines.append("")
    return "\n".join(lines)


def render_dispatch_variants(
    win_meta: dict[str, object],
    win_rows: list[dict[str, object]],
    linux_meta: dict[str, object],
    linux_rows: list[dict[str, object]],
) -> str:
    runs = 0
    for meta in (win_meta, linux_meta):
        if meta:
            runs = meta_int(meta, "runs")
            break
    lines = [
        f"## vm_dispatch dispatch variants (`ns_per_instruction`, x{runs if runs else '?'})",
        "",
        "| Platform | s | mteps | mips | ns_per_X |",
        "|---|---:|---:|---:|---:|",
    ]
    for platform_label, rows in ((str(win_meta.get("platform_label", "Graphion Windows")), win_rows), (str(linux_meta.get("platform_label", "Graphion Linux")), linux_rows)):
        for row in rows:
            variant = str(row["variant"])
            status = str(row["status"])
            if status != "ok":
                continue
            lines.append(
                f"| {platform_label} ({variant}) | {fmt_seconds(row['seconds_avg'])} | - | {fmt(row['mips_avg'])} | {fmt(row['ns_per_instruction_avg'])} |"
            )
    lines.append("")
    return "\n".join(lines)


def render_environment_table(metas: list[dict[str, object]]) -> str:
    lines = [
        "## Environment Metadata",
        "",
        "| Lane | Compiler | ASM | CPU | Machine | Git | Runs |",
        "|---|---|---|---|---|---|---:|",
    ]
    for meta in metas:
        if not meta:
            continue
        lines.append(
            "| {lane} | {compiler} | {asm} | {cpu} | {machine} | {git_rev} | {runs} |".format(
                lane=meta_str(meta, "platform_label"),
                compiler=meta_str(meta, "compiler_kind"),
                asm="on" if meta_bool(meta, "asm_enabled") else "off",
                cpu=meta_str(meta, "cpu_model"),
                machine=meta_str(meta, "machine"),
                git_rev=meta_str(meta, "git_rev")[:12],
                runs=meta_int(meta, "runs"),
            )
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Render docs/performance/reports/PERFORMANCE_RESULTS.md from collected benchmark JSON artifacts.")
    parser.add_argument("--windows-json", required=True, help="Graphion Windows benchmark JSON")
    parser.add_argument("--linux-json", required=True, help="Graphion Linux benchmark JSON")
    parser.add_argument("--rust-json", default="", help="Rust benchmark JSON")
    parser.add_argument("--dispatch-windows-json", required=True, help="Windows dispatch variants JSON")
    parser.add_argument("--dispatch-linux-json", required=True, help="Linux dispatch variants JSON")
    parser.add_argument("--output", default=str(PERFORMANCE_RESULTS_MD), help="Output markdown path")
    args = parser.parse_args()

    windows_meta, windows_rows = load_rows(pathlib.Path(args.windows_json), ["report_kind", "compiler_kind", "asm_enabled", "config", "build_dir"])
    linux_meta, linux_rows = load_rows(pathlib.Path(args.linux_json), ["report_kind", "compiler_kind", "asm_enabled", "config", "build_dir"])
    rust_meta, rust_rows = load_rows(pathlib.Path(args.rust_json), ["report_kind", "compiler_kind", "asm_enabled", "manifest_path"]) if args.rust_json else ({}, [])
    dispatch_windows_meta, dispatch_windows_rows = load_rows(pathlib.Path(args.dispatch_windows_json), ["report_kind", "compiler_kind", "asm_enabled", "iterations"])
    dispatch_linux_meta, dispatch_linux_rows = load_rows(pathlib.Path(args.dispatch_linux_json), ["report_kind", "compiler_kind", "asm_enabled", "iterations"])

    indexed_sets = [index_rows(windows_rows), index_rows(linux_rows), index_rows(rust_rows)]
    generated = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    runs = 0
    for meta in (windows_meta, linux_meta, rust_meta):
        if meta:
            runs = meta_int(meta, "runs")
            break

    sections = []
    for benchmark in BENCHMARK_ORDER:
        section = render_benchmark_section(benchmark, indexed_sets)
        if section:
            sections.append(section)
        if benchmark == "vm_dispatch":
            sections.append(render_dispatch_variants(dispatch_windows_meta, dispatch_windows_rows, dispatch_linux_meta, dispatch_linux_rows))

    text = "\n".join(
        [
            f"# Performance Snapshot (x{runs if runs else '?'})",
            "",
            f"This snapshot is generated from the latest local benchmark artifacts on {generated}.",
            "",
            f"Benchmark runs use x{runs if runs else '?'} averages with benchmark-specific default iteration counts committed in the bench sources.",
            "",
            "Format requested: `s | mteps | mips | ns_per_X`.",
            "",
            "For official `baseline` vs `PGO` before/after reports, see [OPTIMIZATION_REPORTS.md](OPTIMIZATION_REPORTS.md).",
            "",
            render_environment_table([windows_meta, linux_meta, rust_meta]),
            "",
            *sections,
            "Notes:",
            "",
            "- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).",
            "- `computed-goto` is expected only on Linux/GCC/Clang paths.",
            "- Rust comparison uses the versioned `graphion_rust` benchmark lane when present in the checkout.",
            "- Numbers vary by CPU governor, thermal state, and host load.",
            "- Treat this as a rolling engineering checkpoint, not a publication-grade benchmark.",
            "",
        ]
    )

    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
