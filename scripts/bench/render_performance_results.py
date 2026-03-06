#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
from datetime import datetime, timezone


BENCHMARK_ORDER = [
    "vm_dispatch",
    "bfs_levels",
    "hypergraph_incidence",
    "hypergraph_incident_sum",
    "hypergraph_hyperedge_node_sum",
    "vm_graph_ops",
]

DISPLAY_NAMES = {
    "vm_dispatch": "vm_dispatch",
    "bfs_levels": "bfs_levels",
    "hypergraph_incidence": "hypergraph_incidence",
    "hypergraph_incident_sum": "hypergraph_incident_sum",
    "hypergraph_hyperedge_node_sum": "hypergraph_hyperedge_node_sum",
    "vm_graph_ops": "vm_graph_ops",
}

LATENCY_LABELS = {
    "ns_per_instruction": "ns_per_instruction",
    "ns_per_edge": "ns_per_edge",
    "ns_per_incidence": "ns_per_incidence",
    "ns_per_call": "ns_per_call",
}


def load_rows(path: pathlib.Path) -> list[dict[str, object]]:
    if not path.exists():
        return []
    return json.loads(path.read_text(encoding="utf-8"))


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
    lines.append("")
    return "\n".join(lines)


def render_dispatch_variants(win_rows: list[dict[str, object]], linux_rows: list[dict[str, object]]) -> str:
    runs = 0
    for rows in (win_rows, linux_rows):
        if rows:
            runs = int(rows[0]["runs"])
            break
    lines = [
        f"## vm_dispatch dispatch variants (`ns_per_instruction`, x{runs if runs else '?'})",
        "",
        "| Platform | s | mteps | mips | ns_per_X |",
        "|---|---:|---:|---:|---:|",
    ]
    for platform_label, rows in (("Graphion Windows", win_rows), ("Graphion Linux", linux_rows)):
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


def main() -> int:
    parser = argparse.ArgumentParser(description="Render docs/PERFORMANCE_RESULTS.md from collected benchmark JSON artifacts.")
    parser.add_argument("--windows-json", required=True, help="Graphion Windows benchmark JSON")
    parser.add_argument("--linux-json", required=True, help="Graphion Linux benchmark JSON")
    parser.add_argument("--rust-json", default="", help="Rust benchmark JSON")
    parser.add_argument("--dispatch-windows-json", required=True, help="Windows dispatch variants JSON")
    parser.add_argument("--dispatch-linux-json", required=True, help="Linux dispatch variants JSON")
    parser.add_argument("--output", default="docs/PERFORMANCE_RESULTS.md", help="Output markdown path")
    args = parser.parse_args()

    windows_rows = load_rows(pathlib.Path(args.windows_json))
    linux_rows = load_rows(pathlib.Path(args.linux_json))
    rust_rows = load_rows(pathlib.Path(args.rust_json)) if args.rust_json else []
    dispatch_windows_rows = load_rows(pathlib.Path(args.dispatch_windows_json))
    dispatch_linux_rows = load_rows(pathlib.Path(args.dispatch_linux_json))

    indexed_sets = [index_rows(windows_rows), index_rows(linux_rows), index_rows(rust_rows)]
    generated = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    runs = 0
    for rows in (windows_rows, linux_rows, rust_rows):
        if rows:
            runs = int(rows[0]["runs"])
            break

    sections = []
    for benchmark in BENCHMARK_ORDER:
        section = render_benchmark_section(benchmark, indexed_sets)
        if section:
            sections.append(section)
        if benchmark == "vm_dispatch":
            sections.append(render_dispatch_variants(dispatch_windows_rows, dispatch_linux_rows))

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
            *sections,
            "Notes:",
            "",
            "- Linux measurements are intended to come from Docker (`GRAPHION_ENABLE_ASM=ON`).",
            "- `computed-goto` is expected only on Linux/GCC/Clang paths.",
            "- Rust comparison uses the local `graphion_rust` sandbox when present; that sandbox stays gitignored.",
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
