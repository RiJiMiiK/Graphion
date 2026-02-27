#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "CONTRIBUTORS.md"


def collect() -> list[tuple[int, str, str]]:
  proc = subprocess.run(
      ["git", "shortlog", "-sne", "HEAD"],
      cwd=ROOT,
      capture_output=True,
      text=True,
      check=False,
  )
  if proc.returncode != 0:
    raise RuntimeError(proc.stderr.strip() or "git shortlog failed")

  rows: list[tuple[int, str, str]] = []
  for raw in proc.stdout.splitlines():
    line = raw.strip()
    if not line:
      continue
    count_part, author_part = line.split("\t", 1)
    count = int(count_part.strip())
    if "<" in author_part and ">" in author_part:
      name = author_part.rsplit("<", 1)[0].strip()
      email = author_part.rsplit("<", 1)[1].rstrip(">").strip()
    else:
      name = author_part.strip()
      email = ""
    rows.append((count, name, email))
  return rows


def render(rows: list[tuple[int, str, str]]) -> str:
  lines = [
      "# Contributors",
      "",
      "Auto-generated from `git shortlog -sne HEAD`.",
      "",
      "| Commits | Name | Email |",
      "|---:|---|---|",
  ]
  for count, name, email in rows:
    safe_email = email if email else "-"
    lines.append(f"| {count} | {name} | {safe_email} |")
  lines.append("")
  return "\n".join(lines)


def main() -> int:
  try:
    rows = collect()
  except Exception as exc:  # noqa: BLE001
    print(f"failed to collect contributors: {exc}", file=sys.stderr)
    return 1

  OUTPUT.write_text(render(rows), encoding="utf-8")
  print(f"updated {OUTPUT}")
  return 0


if __name__ == "__main__":
  sys.exit(main())
