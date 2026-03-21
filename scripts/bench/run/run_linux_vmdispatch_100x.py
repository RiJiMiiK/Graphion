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


def main() -> int:
  secs = []
  for _ in range(100):
    out = subprocess.check_output(["./build-linux/graphion_bench", "500000"], text=True).strip().splitlines()[-1]
    secs.append(float(json.loads(out)["seconds"]))
  print(
      json.dumps(
          {
              "benchmark": "vm_dispatch",
              "runs": 100,
              "graphion_linux_seconds_avg": round(statistics.mean(secs), 6),
          },
          indent=2,
      )
  )
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
