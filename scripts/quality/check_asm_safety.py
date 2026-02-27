#!/usr/bin/env python3
"""
Basic safety gate for assembly sources.

This script blocks a list of privileged or high-risk x86 instructions by default.
For rare exceptions, add "ALLOW_UNSAFE_ASM" on the same line and justify in PR.
"""

from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
ASM_DIRS = [ROOT / "src"]
ASM_EXTS = {".s", ".S", ".asm"}

# Keep this list intentionally strict for an interpreter hotpath project.
BANNED_PATTERNS = [
    r"\bcli\b",
    r"\bsti\b",
    r"\bhlt\b",
    r"\blgdt\b",
    r"\blidt\b",
    r"\blldt\b",
    r"\bltr\b",
    r"\bswapgs\b",
    r"\bwrmsr\b",
    r"\brdmsr\b",
    r"\binvlpg\b",
    r"\binvd\b",
    r"\bwbinvd\b",
    r"\biretq?\b",
    r"\bmov\s+cr[0-8]\b",
    r"\bmov\s+dr[0-7]\b",
    r"\bint\s+0x[0-9a-fA-F]+\b",
    r"\bsyscall\b",
    r"\bsysenter\b",
    r"\bout\s",
    r"\bin\s",
]

ALLOW_TAG = "ALLOW_UNSAFE_ASM"


def iter_asm_files() -> list[pathlib.Path]:
  files: list[pathlib.Path] = []
  for d in ASM_DIRS:
    if not d.exists():
      continue
    for p in d.rglob("*"):
      if p.is_file() and p.suffix in ASM_EXTS:
        files.append(p)
  return files


def check_file(path: pathlib.Path) -> list[str]:
  violations: list[str] = []
  try:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
  except OSError as exc:
    return [f"{path}: failed to read file: {exc}"]

  for idx, line in enumerate(lines, start=1):
    normalized = line.strip()
    if not normalized or normalized.startswith(";") or normalized.startswith("#"):
      continue
    if ALLOW_TAG in line:
      continue
    for pattern in BANNED_PATTERNS:
      if re.search(pattern, line, flags=re.IGNORECASE):
        violations.append(
            f"{path.relative_to(ROOT)}:{idx}: banned instruction pattern `{pattern}` in `{line.strip()}`"
        )
        break
  return violations


def main() -> int:
  all_files = iter_asm_files()
  if not all_files:
    print("asm-safety: no assembly files found")
    return 0

  errors: list[str] = []
  for f in all_files:
    errors.extend(check_file(f))

  if errors:
    print("asm-safety: FAILED")
    for err in errors:
      print(f"  - {err}")
    return 1

  print(f"asm-safety: OK ({len(all_files)} file(s) scanned)")
  return 0


if __name__ == "__main__":
  sys.exit(main())
