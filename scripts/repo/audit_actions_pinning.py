#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
WF_DIR = ROOT / ".github" / "workflows"
USES_RE = re.compile(r"uses:\s*([^\s]+)")
SHA_RE = re.compile(r"@[0-9a-fA-F]{40}$")


def main() -> int:
  unpinned: list[str] = []
  for wf in sorted(WF_DIR.glob("*.yml")):
    for idx, line in enumerate(wf.read_text(encoding="utf-8", errors="replace").splitlines(), start=1):
      m = USES_RE.search(line)
      if not m:
        continue
      action = m.group(1).strip()
      if "@local" in action:
        continue
      if not SHA_RE.search(action):
        unpinned.append(f"{wf.relative_to(ROOT)}:{idx}: {action}")

  if unpinned:
    print("actions pinning audit: unpinned actions found")
    for item in unpinned:
      print(f"  - {item}")
    return 1

  print("actions pinning audit: all actions pinned to full commit SHA")
  return 0


if __name__ == "__main__":
  sys.exit(main())
