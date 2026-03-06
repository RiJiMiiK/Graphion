#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys


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
  parser.add_argument("--output", default="benchmarks/results/dispatch_variants.json")
  args = parser.parse_args()

  variants = ["switch", "jumptable", "computed-goto"]
  rows: list[dict[str, object]] = []
  platform_tag = "win" if sys.platform.startswith("win") else "linux"

  for variant in variants:
    build_dir = pathlib.Path(f"build-disp-{platform_tag}-{variant}")
    cfg = run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            str(build_dir),
            "-DGRAPHION_VM_DISPATCH=" + variant,
            "-DCMAKE_BUILD_TYPE=Release",
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

  out_path = pathlib.Path(args.output)
  out_path.parent.mkdir(parents=True, exist_ok=True)
  out_path.write_text(json.dumps(rows, indent=2), encoding="utf-8")
  print(json.dumps(rows, indent=2))
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
