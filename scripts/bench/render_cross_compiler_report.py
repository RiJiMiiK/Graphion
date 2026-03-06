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


def load_payload(path: pathlib.Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def row_map(payload: dict[str, object]) -> dict[str, dict[str, object]]:
    return {str(row["benchmark"]): row for row in payload["report_rows"]}


def policy_status(best_speedup: float, worst_speedup: float) -> str:
    ratio = worst_speedup / best_speedup if best_speedup > 0.0 else 0.0
    if ratio >= 0.90:
        return "aligned"
    if ratio >= 0.80:
        return "review"
    return "investigate"


def render_lane_table(payloads: list[dict[str, object]]) -> str:
    labels = [str(payload["metadata"]["platform_label"]) for payload in payloads]
    header = "| Benchmark | " + " | ".join(labels) + " | Best lane | Worst lane | Status |"
    sep = "|" + "---|" * (len(labels) + 4)
    lines = [header, sep]
    maps = [row_map(payload) for payload in payloads]

    for benchmark in BENCHMARK_ORDER:
        speedups: list[tuple[str, float]] = []
        values: list[str] = []
        for label, rows in zip(labels, maps):
            speedup = float(rows[benchmark]["speedup_x"])
            speedups.append((label, speedup))
            values.append(f"{speedup:.3f}")
        best_label, best_speedup = max(speedups, key=lambda item: item[1])
        worst_label, worst_speedup = min(speedups, key=lambda item: item[1])
        lines.append(
            "| {benchmark} | {values} | {best_lane} ({best_speedup:.3f}) | {worst_lane} ({worst_speedup:.3f}) | {status} |".format(
                benchmark=benchmark,
                values=" | ".join(values),
                best_lane=best_label,
                best_speedup=best_speedup,
                worst_lane=worst_label,
                worst_speedup=worst_speedup,
                status=policy_status(best_speedup, worst_speedup),
            )
        )
    return "\n".join(lines)


def render_policy_table(payloads: list[dict[str, object]]) -> str:
    lines = [
        "| Rule | Decision |",
        "|---|---|",
        "| Canonical lane | Portable configuration only: `GRAPHION_ENABLE_ASM=OFF`, `dispatch=switch`, same corpus profile, same run count, same benchmark iterations. |",
        "| Primary metric | Compare `speedup_x` from official `baseline` vs `PGO` reports; use latency-derived `speedup_x` as the source of truth. |",
        "| Toolchain set | `MSVC` on Windows, `GCC` in Linux Docker, `Clang` in Linux Docker. |",
        "| Status thresholds | `aligned` when worst/best `speedup_x >= 0.90`; `review` when `>= 0.80`; `investigate` otherwise. |",
        "| Interpretation | `MSVC` is a Windows release lane; `GCC` and `Clang` are Linux portable lanes. Treat it as governance, not a pure compiler-only microbenchmark. |",
    ]
    return "\n".join(lines)


def render_metadata_table(payloads: list[dict[str, object]]) -> str:
    lines = [
        "| Lane | Compiler | Platform | ASM | Dispatch | Corpus | Runs | Iterations |",
        "|---|---|---|---|---|---|---:|---:|",
    ]
    for payload in payloads:
        meta = payload["metadata"]
        lines.append(
            "| {lane} | {compiler} | {platform} | {asm} | {dispatch} | {corpus} | {runs} | {iterations} |".format(
                lane=meta["platform_label"],
                compiler=meta["compiler_kind"],
                platform=meta["platform"],
                asm="on" if meta.get("asm_enabled", True) else "off",
                dispatch=meta["dispatch"],
                corpus=meta["corpus_profile"],
                runs=meta["runs"],
                iterations=meta["iterations"],
            )
        )
    return "\n".join(lines)


def render_markdown(payloads: list[dict[str, object]]) -> str:
    generated = datetime.now(timezone.utc).isoformat()
    lines = [
        "# Cross-Compiler Optimization Report",
        "",
        f"Generated: {generated}",
        "",
        "This report reflects the latest local portable-lane comparison run. It is a governance checkpoint, not an automatic release baseline.",
        "",
        "## Policy",
        "",
        render_policy_table(payloads),
        "",
        "## Lanes",
        "",
        render_metadata_table(payloads),
        "",
        "## Speedup Comparison",
        "",
        render_lane_table(payloads),
        "",
        "## Notes",
        "",
        "- This report compares optimization effectiveness, not absolute wall-clock ranking between operating systems.",
        "- `speedup_x` is derived from the official `baseline` vs `PGO` latency metric for each workload family.",
        "- If a row is `investigate`, refresh the lane snapshots at higher run counts before making a release decision.",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Render the cross-compiler optimization governance report.")
    parser.add_argument("--input-json", action="append", required=True, help="Input optimization report JSON (repeatable)")
    parser.add_argument("--output-md", default="docs/CROSS_COMPILER_REPORT.md", help="Output Markdown path")
    parser.add_argument("--output-json", default="benchmarks/results/cross_compiler_report_latest.json", help="Output merged JSON path")
    args = parser.parse_args()

    payloads = [load_payload(pathlib.Path(path)) for path in args.input_json]
    merged = {
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "lane_reports": payloads,
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
