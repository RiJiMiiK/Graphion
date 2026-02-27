#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
from datetime import datetime, timezone


def main() -> int:
  parser = argparse.ArgumentParser(description="Run Graphion benchmark binary and store JSON results.")
  parser.add_argument("--build-dir", default="build-dev", help="CMake build directory")
  parser.add_argument("--iterations", type=int, default=500000, help="Iterations for bench program")
  parser.add_argument("--output", default="benchmarks/results/latest.json", help="Output JSON path")
  args = parser.parse_args()

  build_dir = pathlib.Path(args.build_dir)
  output = pathlib.Path(args.output)

  if sys.platform.startswith("win"):
    exe = build_dir / "graphion_bench.exe"
  else:
    exe = build_dir / "graphion_bench"

  if not exe.exists():
    print(f"benchmark binary not found: {exe}", file=sys.stderr)
    return 2

  proc = subprocess.run([str(exe), str(args.iterations)], capture_output=True, text=True)
  if proc.returncode != 0:
    print(proc.stderr.strip(), file=sys.stderr)
    return proc.returncode

  line = proc.stdout.strip().splitlines()[-1]
  payload = json.loads(line)
  payload["timestamp_utc"] = datetime.now(timezone.utc).isoformat()

  output.parent.mkdir(parents=True, exist_ok=True)
  output.write_text(json.dumps(payload, indent=2), encoding="utf-8")
  print(json.dumps(payload, indent=2))
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
