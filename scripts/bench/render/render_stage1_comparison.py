#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import sys
from datetime import datetime, timezone

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))

from bench_paths import STAGE1_COMPARISON_MD
from report_metadata import validate_metadata


LATENCY_LABELS = {
    "ns_per_instruction": "ns/instruction",
    "ns_per_operation": "ns/operation",
}

THROUGHPUT_LABELS = {
    "mips": "mips",
    "mops": "mops",
}


def load_payload(path: pathlib.Path) -> tuple[dict[str, object], dict[str, object] | None]:
    if not path.exists():
        return ({}, None)
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or "metadata" not in payload or "rows" not in payload:
        raise ValueError(f"{path}: expected top-level metadata + rows payload")
    metadata = payload["metadata"]
    rows = payload["rows"]
    if not isinstance(metadata, dict) or not isinstance(rows, list):
        raise ValueError(f"{path}: invalid payload shape")
    validate_metadata(metadata, str(path), ["report_kind", "lane_kind", "compiler_kind", "asm_enabled"])
    if not rows:
        return (metadata, None)
    row = rows[0]
    if not isinstance(row, dict):
        raise ValueError(f"{path}: expected first row to be an object")
    return (metadata, row)


def fmt_seconds(value: object) -> str:
    if isinstance(value, (int, float)):
        return f"{float(value):.6f}"
    return "-"


def fmt_metric(value: object) -> str:
    if isinstance(value, (int, float)):
        return f"{float(value):.3f}"
    return "-"


def render_row(label: str, metadata: dict[str, object], row: dict[str, object] | None) -> str:
    if row is None:
        return f"| {label} | - | - | - | - | - | - |"
    throughput_key = str(row["throughput_key"])
    latency_key = str(row["latency_key"])
    throughput_value = row.get(throughput_key + "_avg", "-")
    latency_value = row.get(latency_key + "_avg", "-")
    compiler = str(metadata.get("compiler_kind", "-"))
    asm_state = "on" if metadata.get("asm_enabled") else "off"
    workload = str(row["benchmark"])
    return (
        f"| {label} | {workload} | {fmt_seconds(row.get('seconds_avg'))} | "
        f"{fmt_metric(throughput_value)} {THROUGHPUT_LABELS.get(throughput_key, throughput_key)} | "
        f"{fmt_metric(latency_value)} {LATENCY_LABELS.get(latency_key, latency_key)} | "
        f"{compiler} | {asm_state} |"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Render the stage-1 comparison table.")
    parser.add_argument("--vm-windows-json", default="", help="VM Windows stage-1 lane JSON")
    parser.add_argument("--vm-linux-json", default="", help="VM Linux stage-1 lane JSON")
    parser.add_argument("--gion-windows-json", default="", help="Graphion Windows stage-1 lane JSON")
    parser.add_argument("--gion-linux-json", default="", help="Graphion Linux stage-1 lane JSON")
    parser.add_argument("--rust-windows-json", default="", help="Rust Windows stage-1 lane JSON")
    parser.add_argument("--rust-linux-json", default="", help="Rust Linux stage-1 lane JSON")
    parser.add_argument("--output", default=str(STAGE1_COMPARISON_MD), help="Output markdown path")
    args = parser.parse_args()

    lanes = [
        ("VM (Windows)", pathlib.Path(args.vm_windows_json) if args.vm_windows_json else None),
        ("VM (Linux)", pathlib.Path(args.vm_linux_json) if args.vm_linux_json else None),
        ("Graphion (.gion) (Windows)", pathlib.Path(args.gion_windows_json) if args.gion_windows_json else None),
        ("Graphion (.gion) (Linux)", pathlib.Path(args.gion_linux_json) if args.gion_linux_json else None),
        ("Rust (Windows)", pathlib.Path(args.rust_windows_json) if args.rust_windows_json else None),
        ("Rust (Linux)", pathlib.Path(args.rust_linux_json) if args.rust_linux_json else None),
    ]

    loaded = []
    for label, path in lanes:
        if path is None:
            loaded.append((label, {}, None))
        else:
            metadata, row = load_payload(path)
            loaded.append((label, metadata, row))
    generated = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")

    lines = [
        "# Stage 1 Comparison",
        "",
        f"Generated on {generated}.",
        "",
        "This table keeps the stage-1 lanes separate on purpose:",
        "- `VM` rows measure the direct typed-value VM kernel (`vm_dispatch`).",
        "- `Graphion (.gion)` rows measure the `.gion` source workload end-to-end (`gion_stage1`).",
        "- `Rust` rows measure the Rust comparison lane for the same stage-1 VM kernel family.",
        "",
        "| Lane | Workload | s | Throughput | Latency | Compiler | ASM |",
        "|---|---|---:|---:|---:|---|---|",
    ]
    for label, metadata, row in loaded:
        lines.append(render_row(label, metadata, row))
    lines.extend(
        [
            "",
            "Notes:",
            "",
            "- Missing rows mean the corresponding lane was not collected on this machine or in Docker.",
            "- `Graphion (.gion)` includes frontend/runtime overhead that the direct VM lane intentionally does not include.",
            "",
        ]
    )

    text = "\n".join(lines)
    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
