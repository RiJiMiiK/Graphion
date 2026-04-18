#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[2]

REQUIRED_FILES = [
    "README.md",
    "QUALITY.md",
    "LICENSE",
    "CHANGELOG.md",
    "CONTRIBUTING.md",
    "SECURITY.md",
    "SECURITY_CONTACTS.md",
    "SUPPORT.md",
    "OWNERSHIP.md",
    "MAINTAINERS.md",
    "ROADMAP.md",
    "examples/README.md",
    "docs/index.md",
    "scripts/README.md",
    "scripts/quality/check_repo_health.py",
    "scripts/quality/quality_gate.sh",
    "scripts/quality/quality_gate.ps1",
    ".gitignore",
    "CMakeLists.txt",
    ".github/CODEOWNERS",
    ".github/pull_request_template.md",
    ".github/workflows/ci.yml",
    ".github/workflows/docs.yml",
    ".github/workflows/spellcheck.yml",
    ".github/workflows/links-check.yml",
    ".github/workflows/repo-health.yml",
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
