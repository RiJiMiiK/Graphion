#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
SEARCH_DIRS = ["src", "tests", "benchmarks", "fuzz"]
EXTENSIONS = {".c", ".h", ".s"}
SPDX = "SPDX-License-Identifier: MIT"


def main() -> int:
  failures: list[str] = []
  for d in SEARCH_DIRS:
    base = ROOT / d
    if not base.exists():
      continue
    for path in base.rglob("*"):
      if not path.is_file() or path.suffix not in EXTENSIONS:
        continue
      lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
      head = "\n".join(lines[:5])
      if SPDX not in head:
        failures.append(str(path.relative_to(ROOT)))

  if failures:
    print("license header check: FAILED")
    for f in failures:
      print(f"  - missing SPDX header: {f}")
    return 1

  print("license header check: OK")
  return 0


if __name__ == "__main__":
  sys.exit(main())
