#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import pathlib
import subprocess
import sys

from pgo_artifacts import profile_manifest, reset_profile_dir
from pgo_corpus import corpus_profile_names, coverage_classes, expanded_workloads, get_corpus_profile


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


def msvc_runtime_dirs() -> list[str]:
  candidates: list[pathlib.Path] = []
  program_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
  vswhere = pathlib.Path(program_files_x86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
  if vswhere.exists():
    try:
      result = subprocess.run(
          [
              str(vswhere),
              "-latest",
              "-products",
              "*",
              "-requires",
              "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
              "-property",
              "installationPath",
          ],
          check=True,
          capture_output=True,
          text=True,
      )
      install_path = result.stdout.strip()
      if install_path:
        root = pathlib.Path(install_path)
        candidates.extend(root.glob(r"VC\Tools\MSVC\*\bin\Hostx64\x64"))
        candidates.extend(root.glob(r"VC\Redist\MSVC\*\x64\Microsoft.VC*.CRT"))
    except subprocess.CalledProcessError:
      pass

  found: list[str] = []
  for directory in candidates:
    if directory.is_dir() and any(directory.glob("pgort*.dll")):
      path_text = str(directory)
      if path_text not in found:
        found.append(path_text)
  return found


def training_env(profile_dir: pathlib.Path, compiler_kind: str) -> dict[str, str]:
  env = os.environ.copy()
  if compiler_kind == "clang":
    env["LLVM_PROFILE_FILE"] = str(profile_dir / "graphion-%p-%m.profraw")
  if compiler_kind == "msvc":
    runtime_dirs = msvc_runtime_dirs()
    if runtime_dirs:
      env["PATH"] = os.pathsep.join(runtime_dirs + [env.get("PATH", "")])
      print("msvc pgo runtime dirs:")
      for path in runtime_dirs:
        print(f"  - {path}")
    else:
      print("warning: no MSVC PGO runtime directory with pgort*.dll was found")
  return env


def train_workload(build_dir: pathlib.Path,
                   config: str,
                   profile_dir: pathlib.Path,
                   iterations_scale: float,
                   compiler_kind: str,
                   corpus_profile: str) -> None:
  profile = get_corpus_profile(corpus_profile)
  bench_specs = expanded_workloads(corpus_profile, iterations_scale)
  env = training_env(profile_dir, compiler_kind)

  print(f"pgo corpus profile: {corpus_profile}")
  print(f"coverage classes: {', '.join(coverage_classes(corpus_profile))}")
  for spec in bench_specs:
    print(
        "  - {name}: {target} [{family}] iterations={iterations} (base={base}) coverage={coverage}".format(
            name=spec["name"],
            target=spec["target"],
            family=spec["family"],
            iterations=spec["iterations"],
            base=spec["base_iterations"],
            coverage=spec["coverage"],
        )
    )

  if profile["run_graphion_binary"]:
    run([str(exe_path(build_dir, "graphion", config))], env=env)

  for spec in bench_specs:
    run([str(exe_path(build_dir, str(spec["target"]), config)), str(spec["iterations"])], env=env)

  if profile["run_tests"]:
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
  parser.add_argument("--corpus-profile", choices=corpus_profile_names(), default="representative",
                      help="Named PGO training corpus")
  parser.add_argument("--skip-tests", action="store_true", help="Do not run ctest after optimized rebuild")
  parser.add_argument("cmake_args", nargs="*", help="Extra CMake args, for example -G Ninja -DCMAKE_C_COMPILER=clang")
  args = parser.parse_args()

  build_dir = pathlib.Path(args.build_dir)
  profile_dir = pathlib.Path(args.profile_dir) if args.profile_dir else build_dir / "pgo-data"
  compiler_kind = args.compiler_kind
  if compiler_kind == "auto":
    compiler_kind = detect_compiler_kind(args.cmake_args)

  manifest = profile_manifest(
      compiler_kind=compiler_kind,
      corpus_profile=args.corpus_profile,
      iterations_scale=args.iterations_scale,
      config=args.config,
      build_type=args.build_type,
      dispatch="switch",
      extra_args=args.cmake_args,
      producer="run_pgo_pipeline.py",
  )
  reasons = reset_profile_dir(profile_dir, manifest)
  print("pgo profile invalidation:")
  for reason in reasons:
    print(f"  - {reason}")
  if compiler_kind == "msvc":
    cleanup_msvc_profile_artifacts(build_dir, args.config)

  configure(build_dir, args.build_type, "GENERATE", profile_dir, args.cmake_args)
  build(build_dir, args.config)
  train_workload(build_dir, args.config, profile_dir, args.iterations_scale, compiler_kind, args.corpus_profile)

  if compiler_kind == "clang":
    merge_clang_profiles(profile_dir, args.llvm_profdata)

  configure(build_dir, args.build_type, "USE", profile_dir, args.cmake_args)
  build(build_dir, args.config)
  if not args.skip_tests:
    ctest(build_dir, args.config)

  print(
      "pgo pipeline complete: compiler={compiler}, corpus={corpus}, build_dir={build_dir}, profile_dir={profile_dir}".format(
          compiler=compiler_kind,
          corpus=args.corpus_profile,
          build_dir=build_dir,
          profile_dir=profile_dir,
      )
  )
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
