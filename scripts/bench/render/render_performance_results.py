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
    "vm_print_dispatch",
    "gion_print_source",
    "vm_expr_dispatch",
    "gion_expr_source",
]

SECTION_BENCHMARKS = {
    "frontier_primitives": {
        "VM Windows": "frontier_primitives",
        "VM Linux": "frontier_primitives",
        "Rust Windows": "frontier_primitives",
        "Rust Linux": "frontier_primitives",
    },
    "vm_dispatch": {
        "VM Windows": "vm_dispatch",
        "VM Linux": "vm_dispatch",
        "Rust Windows": "vm_dispatch",
        "Rust Linux": "vm_dispatch",
    },
    "bfs_levels": {
        "VM Windows": "bfs_levels",
        "VM Linux": "bfs_levels",
        "Rust Windows": "bfs_levels",
        "Rust Linux": "bfs_levels",
    },
    "neighbor_iteration": {
        "VM Windows": "neighbor_iteration",
        "VM Linux": "neighbor_iteration",
        "Rust Windows": "neighbor_iteration",
        "Rust Linux": "neighbor_iteration",
    },
    "weighted_neighbor_sums": {
        "VM Windows": "weighted_neighbor_sums",
        "VM Linux": "weighted_neighbor_sums",
        "Rust Windows": "weighted_neighbor_sums",
        "Rust Linux": "weighted_neighbor_sums",
    },
    "hypergraph_incidence": {
        "VM Windows": "hypergraph_incidence",
        "VM Linux": "hypergraph_incidence",
        "Rust Windows": "hypergraph_incidence",
        "Rust Linux": "hypergraph_incidence",
    },
    "hypergraph_traversal": {
        "VM Windows": "hypergraph_traversal",
        "VM Linux": "hypergraph_traversal",
        "Rust Windows": "hypergraph_traversal",
        "Rust Linux": "hypergraph_traversal",
    },
    "hypergraph_incident_sum": {
        "VM Windows": "hypergraph_incident_sum",
        "VM Linux": "hypergraph_incident_sum",
        "Rust Windows": "hypergraph_incident_sum",
        "Rust Linux": "hypergraph_incident_sum",
    },
    "hypergraph_hyperedge_node_sum": {
        "VM Windows": "hypergraph_hyperedge_node_sum",
        "VM Linux": "hypergraph_hyperedge_node_sum",
        "Rust Windows": "hypergraph_hyperedge_node_sum",
        "Rust Linux": "hypergraph_hyperedge_node_sum",
    },
    "vm_graph_ops": {
        "VM Windows": "vm_graph_ops",
        "VM Linux": "vm_graph_ops",
        "Rust Windows": "vm_graph_ops",
        "Rust Linux": "vm_graph_ops",
    },
    "gion_source": {
        "VM Windows": "vm_dispatch",
        "VM Linux": "vm_dispatch",
        ".gion Windows": "gion_source",
        ".gion Linux": "gion_source",
        "Rust Windows": "vm_dispatch",
        "Rust Linux": "vm_dispatch",
    },
    "vm_print_dispatch": {
        "VM Windows": "vm_print_dispatch",
        "VM Linux": "vm_print_dispatch",
        "Rust Windows": "vm_print_dispatch",
        "Rust Linux": "vm_print_dispatch",
    },
    "gion_print_source": {
        "VM Windows": "vm_print_dispatch",
        "VM Linux": "vm_print_dispatch",
        ".gion Windows": "gion_print_source",
        ".gion Linux": "gion_print_source",
        "Rust Windows": "vm_print_dispatch",
        "Rust Linux": "vm_print_dispatch",
    },
    "vm_expr_dispatch": {
        "VM Windows": "vm_expr_dispatch",
        "VM Linux": "vm_expr_dispatch",
        "Rust Windows": "vm_expr_dispatch",
        "Rust Linux": "vm_expr_dispatch",
    },
    "gion_expr_source": {
        "VM Windows": "vm_expr_dispatch",
        "VM Linux": "vm_expr_dispatch",
        ".gion Windows": "gion_expr_source",
        ".gion Linux": "gion_expr_source",
        "Rust Windows": "vm_expr_dispatch",
        "Rust Linux": "vm_expr_dispatch",
    },
}

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
    "vm_print_dispatch": "vm_print_dispatch",
    "gion_print_source": "gion_print_source",
    "vm_expr_dispatch": "vm_expr_dispatch",
    "gion_expr_source": "gion_expr_source",
}

SECTION_LATENCY_KEY_OVERRIDE = {
    "gion_source": "ns_per_iteration",
    "gion_print_source": "ns_per_iteration",
    "gion_expr_source": "ns_per_iteration",
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
    "ns_per_iteration": "ns_per_iteration",
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


def load_rows_optional(path_str: str, extra_required: list[str]) -> tuple[dict[str, object], list[dict[str, object]]]:
    if not path_str:
        return {}, []
    path = pathlib.Path(path_str)
    if not path.exists():
        return {}, []
    return load_rows(path, extra_required)


def index_rows(rows: list[dict[str, object]]) -> dict[str, dict[str, object]]:
    return {str(row["benchmark"]): row for row in rows}


LANE_ORDER = [
    "VM Windows",
    "VM Linux",
    ".gion Windows",
    ".gion Linux",
    "Rust Windows",
    "Rust Linux",
]


def lane_label(platform: str, benchmark: str) -> str:
    platform_lower = platform.lower()
    if "rust" in platform_lower:
      return "Rust Linux" if "linux" in platform_lower else "Rust Windows"
    if benchmark.startswith("gion_"):
      return ".gion Linux" if "linux" in platform_lower else ".gion Windows"
    return "VM Linux" if "linux" in platform_lower else "VM Windows"


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
    if key + "_avg" in row:
        return row[key + "_avg"]
    return row.get(key, "-")


def throughput_cell(row: dict[str, object]) -> str:
    key = str(row["throughput_key"])
    return fmt(metric_value(row, key))


def mteps_cell(row: dict[str, object]) -> str:
    return throughput_cell(row) if str(row["throughput_key"]) == "mteps" else "-"


def mips_cell(row: dict[str, object]) -> str:
    return throughput_cell(row) if str(row["throughput_key"]) in ("mips", "mops") else "-"


def variation_cell(row: dict[str, object]) -> str:
    runs = row.get("runs")
    variation = row.get("variation_pct")
    if isinstance(runs, int) and runs < 2:
        return "-"
    if isinstance(variation, (int, float)):
        return f"{float(variation):.3f}%"
    return "-"


def render_benchmark_section(name: str, lane_sources: dict[str, dict[str, dict[str, object]]]) -> str:
    lane_rows: dict[str, dict[str, object]] = {}
    section_map = SECTION_BENCHMARKS.get(name, {})
    section_lanes = [lane for lane in LANE_ORDER if lane in section_map]
    for lane in section_lanes:
        benchmark_name = section_map.get(lane)
        rows = lane_sources.get(lane, {})
        if benchmark_name and benchmark_name in rows:
            lane_rows[lane] = rows[benchmark_name]
    if not lane_rows:
        return ""
    sample = next(iter(lane_rows.values()))
    section_latency_key = SECTION_LATENCY_KEY_OVERRIDE.get(name, str(sample["latency_key"]))
    section_iterations = {
        int(row["iterations"])
        for row in lane_rows.values()
        if isinstance(row.get("iterations"), int)
    }
    if len(section_iterations) > 1:
        iterations_str = ", ".join(str(value) for value in sorted(section_iterations))
        raise ValueError(f"{name}: inconsistent iterations across lanes ({iterations_str})")
    iterations_suffix = ""
    if len(section_iterations) == 1:
        iterations_suffix = f", iterations={next(iter(section_iterations))}"
    lines = [
        f"## {DISPLAY_NAMES[name]} (`{LATENCY_LABELS[section_latency_key]}`{iterations_suffix})",
        "",
        "| Lane | var_% | s | mteps | mips | ns_per_X |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for lane in section_lanes:
        row = lane_rows.get(lane)
        if row is None:
            lines.append(f"| {lane} | - | - | - | - | - |")
            continue
        row_latency_key = SECTION_LATENCY_KEY_OVERRIDE.get(name, str(row["latency_key"]))
        lines.append(
            f"| {lane} | {variation_cell(row)} | {fmt_seconds(row['seconds_avg'])} | {mteps_cell(row)} | {mips_cell(row)} | {fmt(metric_value(row, row_latency_key))} |"
        )
    mode_rows = [row for row in lane_rows.values() if "recommended_frontier_mode" in row]
    if mode_rows:
        lines.append("")
        lines.append("Frontier mode notes:")
        lines.append("")
        for row in mode_rows:
            lines.append(
                "- {platform}: mode=`{mode}` frontier_len={frontier_len} frontier_neighbor_work={neighbor_work}".format(
                    platform=lane_label(str(row["platform"]), name),
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
    iterations = 0
    for meta in (win_meta, linux_meta):
        if meta:
            runs = meta_int(meta, "runs")
            iterations = meta_int(meta, "iterations")
            break
    lines = [
        f"## vm_dispatch dispatch variants (`ns_per_instruction`, iterations={iterations if iterations else '?'}, x{runs if runs else '?'})",
        "",
        "| Platform | var_% | s | mteps | mips | ns_per_X |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for platform_label, rows in ((str(win_meta.get("platform_label", "Graphion Windows")), win_rows), (str(linux_meta.get("platform_label", "Graphion Linux")), linux_rows)):
        for row in rows:
            variant = str(row["variant"])
            status = str(row["status"])
            if status != "ok":
                continue
            lines.append(
                f"| {platform_label} ({variant}) | {variation_cell(row)} | {fmt_seconds(row['seconds_avg'])} | - | {fmt(row['mips_avg'])} | {fmt(row['ns_per_instruction_avg'])} |"
            )
    lines.append("")
    return "\n".join(lines)


def render_environment_table(metas: list[dict[str, object]]) -> str:
    meta_by_lane: dict[str, dict[str, object]] = {}
    for meta in metas:
        if not meta:
            continue
        platform_label = meta_str(meta, "platform_label")
        if "Rust" in platform_label:
            lane = "Rust Linux" if "Linux" in platform_label else "Rust Windows"
        elif "Linux" in platform_label:
            lane = "VM Linux / .gion Linux"
        else:
            lane = "VM Windows / .gion Windows"
        meta_by_lane[lane] = meta
    lines = [
        "## Environment Metadata",
        "",
        "| Lane | Compiler | ASM | CPU | Machine | Git | Runs |",
        "|---|---|---|---|---|---|---:|",
    ]
    for lane in ("VM Windows / .gion Windows", "VM Linux / .gion Linux", "Rust Windows", "Rust Linux"):
        meta = meta_by_lane.get(lane)
        if not meta:
            lines.append(f"| {lane} | - | - | - | - | - | - |")
            continue
        lines.append(
            "| {lane} | {compiler} | {asm} | {cpu} | {machine} | {git_rev} | {runs} |".format(
                lane=lane,
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
    parser.add_argument("--rust-windows-json", default="", help="Rust Windows benchmark JSON")
    parser.add_argument("--rust-linux-json", default="", help="Rust Linux benchmark JSON")
    parser.add_argument("--dispatch-windows-json", required=True, help="Windows dispatch variants JSON")
    parser.add_argument("--dispatch-linux-json", required=True, help="Linux dispatch variants JSON")
    parser.add_argument("--output", default=str(PERFORMANCE_RESULTS_MD), help="Output markdown path")
    args = parser.parse_args()

    windows_meta, windows_rows = load_rows(pathlib.Path(args.windows_json), ["report_kind", "compiler_kind", "asm_enabled", "config", "build_dir"])
    linux_meta, linux_rows = load_rows(pathlib.Path(args.linux_json), ["report_kind", "compiler_kind", "asm_enabled", "config", "build_dir"])
    rust_windows_meta, rust_windows_rows = load_rows_optional(args.rust_windows_json, ["report_kind", "compiler_kind", "asm_enabled", "manifest_path"])
    rust_linux_meta, rust_linux_rows = load_rows_optional(args.rust_linux_json, ["report_kind", "compiler_kind", "asm_enabled", "manifest_path"])
    dispatch_windows_meta, dispatch_windows_rows = load_rows(pathlib.Path(args.dispatch_windows_json), ["report_kind", "compiler_kind", "asm_enabled", "iterations"])
    dispatch_linux_meta, dispatch_linux_rows = load_rows(pathlib.Path(args.dispatch_linux_json), ["report_kind", "compiler_kind", "asm_enabled", "iterations"])

    lane_sources = {
        "VM Windows": index_rows(windows_rows),
        "VM Linux": index_rows(linux_rows),
        ".gion Windows": index_rows(windows_rows),
        ".gion Linux": index_rows(linux_rows),
        "Rust Windows": index_rows(rust_windows_rows),
        "Rust Linux": index_rows(rust_linux_rows),
    }
    generated = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    runs = 0
    for meta in (windows_meta, linux_meta, rust_windows_meta, rust_linux_meta):
        if meta:
            runs = meta_int(meta, "runs")
            break

    sections = []
    for benchmark in BENCHMARK_ORDER:
        section = render_benchmark_section(benchmark, lane_sources)
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
            render_environment_table([windows_meta, linux_meta, rust_windows_meta, rust_linux_meta]),
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
