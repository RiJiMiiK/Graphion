#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys

from report_metadata import base_metadata, validate_metadata


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
  return subprocess.run(cmd, capture_output=True, text=True)


def bench_binary_path(build_dir: pathlib.Path) -> pathlib.Path:
  if sys.platform.startswith("win"):
    exe = build_dir / "graphion_bench.exe"
    if exe.exists():
      return exe
    return build_dir / "Release" / "graphion_bench.exe"
  return build_dir / "graphion_bench"


def main() -> int:
  parser = argparse.ArgumentParser(description="Compare VM dispatch variants on vm_dispatch benchmark.")
  parser.add_argument("--iterations", type=int, default=500000)
  parser.add_argument("--runs", type=int, default=10)
  parser.add_argument("--output", default="benchmarks/results/performance/dispatch_variants.json")
  parser.add_argument(
      "--cmake-arg",
      action="append",
      default=[],
      help="Extra CMake argument forwarded on each configure step (repeatable).",
  )
  parser.add_argument("--build-root", default="", help="Optional build root prefix for generated build dirs")
  parser.add_argument("--platform-label", default="", help="Human-readable platform label")
  parser.add_argument("--compiler-kind", default="unknown", help="Compiler/toolchain label for this lane")
  parser.add_argument("--asm-enabled", choices=["on", "off"], default="off", help="Whether asm is enabled for this lane")
  args = parser.parse_args()

  variants = ["switch", "jumptable", "computed-goto"]
  rows: list[dict[str, object]] = []
  platform_tag = "win" if sys.platform.startswith("win") else "linux"
  build_root = pathlib.Path(args.build_root) if args.build_root else pathlib.Path(".")

  for variant in variants:
    build_dir = build_root / f"build-disp-{platform_tag}-{variant}"
    cfg = run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            str(build_dir),
            "-DGRAPHION_VM_DISPATCH=" + variant,
            "-DCMAKE_BUILD_TYPE=Release",
            *args.cmake_arg,
        ]
    )
    if cfg.returncode != 0:
      rows.append(
          {
              "variant": variant,
              "status": "skipped",
              "reason": cfg.stderr.strip() or cfg.stdout.strip() or "configure failed",
          }
      )
      continue

    bld = run(["cmake", "--build", str(build_dir), "--config", "Release"])
    if bld.returncode != 0:
      rows.append(
          {
              "variant": variant,
              "status": "skipped",
              "reason": bld.stderr.strip() or bld.stdout.strip() or "build failed",
          }
      )
      continue

    exe = bench_binary_path(build_dir)
    if not exe.exists():
      rows.append({"variant": variant, "status": "skipped", "reason": f"missing benchmark binary: {exe}"})
      continue

    seconds = []
    mips = []
    ns = []
    for _ in range(args.runs):
      proc = run([str(exe), str(args.iterations)])
      if proc.returncode != 0:
        rows.append(
            {
                "variant": variant,
                "status": "skipped",
                "reason": proc.stderr.strip() or proc.stdout.strip() or "benchmark failed",
            }
        )
        break
      line = proc.stdout.strip().splitlines()[-1]
      payload = json.loads(line)
      seconds.append(float(payload["seconds"]))
      mips.append(float(payload["mips"]))
      ns.append(float(payload["ns_per_instruction"]))
    else:
      rows.append(
          {
              "variant": variant,
              "status": "ok",
              "runs": args.runs,
              "seconds_avg": round(sum(seconds) / len(seconds), 6),
              "mips_avg": round(sum(mips) / len(mips), 3),
              "ns_per_instruction_avg": round(sum(ns) / len(ns), 3),
          }
      )

  platform_label = args.platform_label if args.platform_label else ("Graphion Windows" if sys.platform.startswith("win") else "Graphion Linux")
  payload = {
      "metadata": base_metadata(
          platform_label,
          args.runs,
          {
              "report_kind": "dispatch-variants",
              "compiler_kind": args.compiler_kind,
              "asm_enabled": args.asm_enabled == "on",
              "iterations": args.iterations,
          },
      ),
      "rows": rows,
  }
  validate_metadata(payload["metadata"], "compare_dispatch_variants", ["report_kind", "compiler_kind", "asm_enabled", "iterations"])

  out_path = pathlib.Path(args.output)
  out_path.parent.mkdir(parents=True, exist_ok=True)
  out_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
  print(json.dumps(payload, indent=2))
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
