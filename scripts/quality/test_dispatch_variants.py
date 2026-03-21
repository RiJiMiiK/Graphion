#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys


def run(cmd: list[str]) -> None:
  print("+", " ".join(cmd))
  subprocess.run(cmd, check=True)


def variant_supported(variant: str) -> bool:
  if variant == "computed-goto" and sys.platform.startswith("win"):
    return False
  return True


def main() -> int:
  parser = argparse.ArgumentParser(description="Build and run Graphion tests across VM dispatch variants.")
  parser.add_argument("--build-root", default="build-dispatch-tests", help="Root directory for per-variant build trees")
  parser.add_argument("--build-type", default="Release", help="Build type / config")
  parser.add_argument("--compiler", default="", help="Optional C compiler path or name")
  parser.add_argument(
      "--variants",
      nargs="+",
      default=["switch", "jumptable", "computed-goto"],
      help="Dispatch variants to test",
  )
  args = parser.parse_args()

  build_root = pathlib.Path(args.build_root)
  cmake_base = ["cmake", "-S", "."]
  for variant in args.variants:
    if not variant_supported(variant):
      print(f"skip variant {variant}: unsupported on this platform")
      continue
    build_dir = build_root / variant
    configure = cmake_base + [
        "-B",
        str(build_dir),
        f"-DGRAPHION_VM_DISPATCH={variant}",
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
    ]
    if args.compiler:
      configure.append(f"-DCMAKE_C_COMPILER={args.compiler}")
    run(configure)
    run(["cmake", "--build", str(build_dir), "--config", args.build_type])
    run(["ctest", "--test-dir", str(build_dir), "--output-on-failure", "-C", args.build_type])

  return 0


if __name__ == "__main__":
  raise SystemExit(main())
