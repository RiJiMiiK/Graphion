#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import pathlib
import subprocess
import sys


def run(cmd: list[str], *, env: dict[str, str] | None = None) -> None:
  print("+", " ".join(cmd))
  subprocess.run(cmd, check=True, env=env)


def exe_path(build_dir: pathlib.Path, target: str, config: str) -> pathlib.Path:
  if sys.platform.startswith("win"):
    root = build_dir / config
    if root.exists():
      return root / f"{target}.exe"
    return build_dir / f"{target}.exe"
  return build_dir / target


def configure(build_dir: pathlib.Path,
              build_type: str,
              pgo_mode: str,
              profile_dir: pathlib.Path,
              extra_args: list[str]) -> None:
  cmd = [
      "cmake",
      "-S",
      ".",
      "-B",
      str(build_dir),
      f"-DCMAKE_BUILD_TYPE={build_type}",
      f"-DGRAPHION_PGO_MODE={pgo_mode}",
      f"-DGRAPHION_PGO_PROFILE_DIR={profile_dir}",
  ]
  cmd.extend(extra_args)
  run(cmd)


def build(build_dir: pathlib.Path, config: str) -> None:
  run(["cmake", "--build", str(build_dir), "--config", config])


def ctest(build_dir: pathlib.Path, config: str) -> None:
  run(["ctest", "--test-dir", str(build_dir), "--output-on-failure", "-C", config])


def cleanup_msvc_profile_artifacts(build_dir: pathlib.Path, config: str) -> None:
  root = build_dir / config
  if not root.exists():
    return
  for pattern in ("*.pgc", "*.pgd"):
    for path in root.glob(pattern):
      path.unlink()


def train_workload(build_dir: pathlib.Path,
                   config: str,
                   profile_dir: pathlib.Path,
                   iterations_scale: float,
                   compiler_kind: str) -> None:
  bench_specs = [
      ("graphion_bench", 200000),
      ("graphion_bench_bfs", 200000),
      ("graphion_bench_hypergraph", 200000),
      ("graphion_bench_hypergraph_incident_sum", 200000),
      ("graphion_bench_hypergraph_hyperedge_node_sum", 200000),
      ("graphion_bench_vm_graph", 100000),
  ]
  env = os.environ.copy()
  if compiler_kind == "clang":
    env["LLVM_PROFILE_FILE"] = str(profile_dir / "graphion-%p-%m.profraw")

  run([str(exe_path(build_dir, "graphion", config))], env=env)

  for target, iterations in bench_specs:
    scaled = max(1000, int(iterations * iterations_scale))
    run([str(exe_path(build_dir, target, config)), str(scaled)], env=env)

  run([str(exe_path(build_dir, "graphion_tests", config))], env=env)


def merge_clang_profiles(profile_dir: pathlib.Path, llvm_profdata: str) -> None:
  profraw = sorted(str(path) for path in profile_dir.glob("*.profraw"))
  if not profraw:
    raise FileNotFoundError(f"no .profraw files found in {profile_dir}")
  run([llvm_profdata, "merge", "-output", str(profile_dir / "graphion.profdata"), *profraw])


def detect_compiler_kind(extra_args: list[str]) -> str:
  text = " ".join(extra_args).lower()
  if "clang" in text:
    return "clang"
  if "gcc" in text:
    return "gcc"
  if sys.platform.startswith("win"):
    return "msvc"
  return "gcc"


def main() -> int:
  parser = argparse.ArgumentParser(description="Run a two-phase PGO pipeline for Graphion.")
  parser.add_argument("--build-dir", default="build-pgo", help="CMake build directory reused across both phases")
  parser.add_argument("--profile-dir", default="", help="Directory for profile artifacts (default: <build-dir>/pgo-data)")
  parser.add_argument("--config", default="Release", help="Build configuration")
  parser.add_argument("--build-type", default="Release", help="CMAKE_BUILD_TYPE for single-config generators")
  parser.add_argument("--compiler-kind", choices=["auto", "msvc", "gcc", "clang"], default="auto")
  parser.add_argument("--llvm-profdata", default="llvm-profdata", help="Clang profile merge tool")
  parser.add_argument("--iterations-scale", type=float, default=1.0, help="Scale training iterations")
  parser.add_argument("--skip-tests", action="store_true", help="Do not run ctest after optimized rebuild")
  parser.add_argument("cmake_args", nargs="*", help="Extra CMake args, for example -G Ninja -DCMAKE_C_COMPILER=clang")
  args = parser.parse_args()

  build_dir = pathlib.Path(args.build_dir)
  profile_dir = pathlib.Path(args.profile_dir) if args.profile_dir else build_dir / "pgo-data"
  compiler_kind = args.compiler_kind
  if compiler_kind == "auto":
    compiler_kind = detect_compiler_kind(args.cmake_args)

  profile_dir.mkdir(parents=True, exist_ok=True)
  if build_dir.exists():
    for path in profile_dir.glob("*"):
      if path.is_file():
        path.unlink()
  if compiler_kind == "msvc":
    cleanup_msvc_profile_artifacts(build_dir, args.config)

  configure(build_dir, args.build_type, "GENERATE", profile_dir, args.cmake_args)
  build(build_dir, args.config)
  train_workload(build_dir, args.config, profile_dir, args.iterations_scale, compiler_kind)

  if compiler_kind == "clang":
    merge_clang_profiles(profile_dir, args.llvm_profdata)

  configure(build_dir, args.build_type, "USE", profile_dir, args.cmake_args)
  build(build_dir, args.config)
  if not args.skip_tests:
    ctest(build_dir, args.config)

  print(f"pgo pipeline complete: compiler={compiler_kind}, build_dir={build_dir}, profile_dir={profile_dir}")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
