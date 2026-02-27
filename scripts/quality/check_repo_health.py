#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]

REQUIRED_FILES = [
    "README.md",
    "LICENSE",
    "CHANGELOG.md",
    "CONTRIBUTING.md",
    "SECURITY.md",
    "SECURITY_CONTACTS.md",
    "SUPPORT.md",
    "OWNERSHIP.md",
    "MAINTAINERS.md",
    "ROADMAP.md",
    "docs/SUPPORT_POLICY.md",
    "docs/ACTIONS_SECURITY.md",
    ".github/CODEOWNERS",
    ".github/pull_request_template.md",
    ".github/workflows/ci.yml",
]


def main() -> int:
  missing: list[str] = []
  for rel in REQUIRED_FILES:
    if not (ROOT / rel).exists():
      missing.append(rel)

  if missing:
    print("repo-health: FAILED")
    for rel in missing:
      print(f"  - missing: {rel}")
    return 1

  print("repo-health: OK")
  return 0


if __name__ == "__main__":
  sys.exit(main())
