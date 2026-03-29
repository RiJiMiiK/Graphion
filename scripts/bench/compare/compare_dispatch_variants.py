#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys

SCRIPT_BENCH_ROOT = pathlib.Path(__file__).resolve().parents[1]
if str(SCRIPT_BENCH_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPT_BENCH_ROOT))
import argparse
import ctypes
import json
import os
import statistics
import subprocess
import shutil

from bench_paths import DISPATCH_LINUX_JSON
from report_metadata import base_metadata, validate_metadata


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
  if not sys.platform.startswith("win") and shutil.which("taskset") is not None:
    cmd = ["taskset", "-c", "0", *cmd]
  return subprocess.run(cmd, capture_output=True, text=True)


def bench_binary_path(build_dir: pathlib.Path) -> pathlib.Path:
  if sys.platform.startswith("win"):
    exe = build_dir / "graphion_bench.exe"
    if exe.exists():
      return exe
    return build_dir / "Release" / "graphion_bench.exe"
  return build_dir / "graphion_bench"


def stabilize_windows_benchmark_host() -> None:
  if not sys.platform.startswith("win"):
    return
  kernel32 = ctypes.windll.kernel32
  handle = kernel32.GetCurrentProcess()
  HIGH_PRIORITY_CLASS = 0x00000080
  affinity_mask = 0x4
  kernel32.SetPriorityClass(handle, HIGH_PRIORITY_CLASS)
  kernel32.SetProcessAffinityMask(handle, affinity_mask)


def stabilize_posix_benchmark_host() -> None:
  if sys.platform.startswith("win"):
    return
  if hasattr(os, "sched_setaffinity"):
    try:
      os.sched_setaffinity(0, {0})
    except OSError:
      pass


def main() -> int:
  parser = argparse.ArgumentParser(description="Compare VM dispatch variants on vm_dispatch benchmark.")
  parser.add_argument("--iterations", type=int, default=5000000)
  parser.add_argument("--runs", type=int, default=100)
  parser.add_argument("--output", default=str(DISPATCH_LINUX_JSON))
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
  stabilize_windows_benchmark_host()
  stabilize_posix_benchmark_host()

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
      variation_pct = 0.0
      if len(ns) > 1:
        mean_ns = statistics.fmean(ns)
        if mean_ns != 0.0:
          variation_pct = statistics.stdev(ns) / mean_ns * 100.0
      rows.append(
          {
              "variant": variant,
              "status": "ok",
              "runs": args.runs,
              "seconds_avg": round(sum(seconds) / len(seconds), 6),
              "mips_avg": round(sum(mips) / len(mips), 3),
              "ns_per_instruction_avg": round(sum(ns) / len(ns), 3),
              "variation_pct": round(variation_pct, 3),
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
