#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys


def main() -> int:
  parser = argparse.ArgumentParser(description="Run clang-tidy on project C sources.")
  parser.add_argument("--build-dir", default="build", help="CMake build directory containing compile_commands.json")
  args = parser.parse_args()

  root = pathlib.Path(__file__).resolve().parents[1]
  build_dir = root / args.build_dir
  if not (build_dir / "compile_commands.json").exists():
    print(f"missing compile_commands.json in {build_dir}", file=sys.stderr)
    return 2

  sources = sorted((root / "src").rglob("*.c"))
  if not sources:
    print("no source files found under src/", file=sys.stderr)
    return 3

  for src in sources:
    cmd = ["clang-tidy", str(src), "-p", str(build_dir)]
    proc = subprocess.run(cmd, text=True, capture_output=True)
    if proc.stdout:
      print(proc.stdout, end="")
    if proc.stderr:
      print(proc.stderr, end="", file=sys.stderr)
    if proc.returncode != 0:
      print(f"clang-tidy failed for {src}", file=sys.stderr)
      return proc.returncode

  print(f"clang-tidy: OK ({len(sources)} file(s))")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
