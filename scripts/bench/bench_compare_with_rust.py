#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys


def load_json(path: pathlib.Path) -> dict:
  return json.loads(path.read_text(encoding="utf-8"))


def parse_last_json_line(stdout: str) -> dict:
  for line in reversed(stdout.splitlines()):
    line = line.strip()
    if line.startswith("{") and line.endswith("}"):
      return json.loads(line)
  raise ValueError("no JSON payload found in rust command output")


def main() -> int:
  parser = argparse.ArgumentParser(description="Compare Graphion VM benchmark to optional Rust benchmark.")
  parser.add_argument("--vm-json", default="benchmarks/results/latest.json", help="Path to VM benchmark JSON")
  parser.add_argument(
      "--rust-json",
      default="",
      help="Path to Rust benchmark JSON with key 'mips' or equivalent throughput metric",
  )
  parser.add_argument(
      "--rust-cmd",
      default="",
      help="Optional command that prints a JSON payload on stdout (last line) for Rust benchmark",
  )
  args = parser.parse_args()

  vm = load_json(pathlib.Path(args.vm_json))
  vm_mips = float(vm["mips"])

  rust_payload = None
  if args.rust_json:
    rust_payload = load_json(pathlib.Path(args.rust_json))
  elif args.rust_cmd:
    proc = subprocess.run(args.rust_cmd, shell=True, capture_output=True, text=True)
    if proc.returncode != 0:
      print(proc.stdout)
      print(proc.stderr, file=sys.stderr)
      return proc.returncode
    rust_payload = parse_last_json_line(proc.stdout)

  if rust_payload is None:
    print("rust comparison skipped: provide --rust-json or --rust-cmd")
    print(json.dumps({"vm_mips": vm_mips}, indent=2))
    return 0

  rust_mips = float(rust_payload["mips"])
  ratio = vm_mips / rust_mips if rust_mips != 0.0 else 0.0
  result = {
      "vm_mips": vm_mips,
      "rust_mips": rust_mips,
      "vm_vs_rust_ratio": ratio,
      "winner": "vm" if vm_mips >= rust_mips else "rust",
  }
  print(json.dumps(result, indent=2))
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
