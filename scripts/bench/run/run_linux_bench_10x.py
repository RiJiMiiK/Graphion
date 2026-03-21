#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))
import json
import statistics
import subprocess

from bench_paths import LINUX_10X_JSON


def main() -> int:
  benches = [
      ("vm_dispatch", "./build-linux/graphion_bench", "mips"),
      ("bfs_levels", "./build-linux/graphion_bench_bfs", "mteps"),
      ("hypergraph_incidence", "./build-linux/graphion_bench_hypergraph", "mips"),
      ("hypergraph_incident_sum", "./build-linux/graphion_bench_hypergraph_incident_sum", "mips"),
      ("hypergraph_hyperedge_node_sum", "./build-linux/graphion_bench_hypergraph_hyperedge_node_sum", "mips"),
      ("vm_graph_ops", "./build-linux/graphion_bench_vm_graph", "mips"),
  ]

  rows = []
  for name, exe, thr_key in benches:
    secs = []
    thrs = []
    for _ in range(10):
      out = subprocess.check_output([exe, "500000"], text=True).strip().splitlines()[-1]
      payload = json.loads(out)
      secs.append(float(payload["seconds"]))
      thrs.append(float(payload[thr_key]))
    rows.append(
        {
            "benchmark": name,
            "graphion_linux_seconds_avg": round(statistics.mean(secs), 6),
            "graphion_linux_throughput_avg": round(statistics.mean(thrs), 3),
        }
    )

  out_path = LINUX_10X_JSON
  out_path.parent.mkdir(parents=True, exist_ok=True)
  out_path.write_text(json.dumps(rows, indent=2), encoding="utf-8")
  print(json.dumps(rows, indent=2))
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
