#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import sys


def load_json(path: pathlib.Path) -> dict:
  return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
  parser = argparse.ArgumentParser(description="Compare benchmark result against baseline.")
  parser.add_argument("--baseline", required=True, help="Path to baseline JSON")
  parser.add_argument("--current", required=True, help="Path to current benchmark JSON")
  parser.add_argument(
      "--max-regression",
      type=float,
      default=0.50,
      help="Maximum allowed MIPS regression ratio (0.50 means 50%% slower allowed)",
  )
  args = parser.parse_args()

  baseline = load_json(pathlib.Path(args.baseline))
  current = load_json(pathlib.Path(args.current))

  base_mips = float(baseline["mips"])
  curr_mips = float(current["mips"])
  floor = base_mips * (1.0 - args.max_regression)

  print(
      f"benchmark compare: baseline={base_mips:.3f} mips current={curr_mips:.3f} mips "
      f"allowed_floor={floor:.3f} mips"
  )

  if curr_mips < floor:
    print("benchmark compare: FAILED (regression above threshold)", file=sys.stderr)
    return 1
  print("benchmark compare: OK")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
