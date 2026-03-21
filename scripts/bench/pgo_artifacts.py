#!/usr/bin/env python3
from __future__ import annotations

import json
import pathlib
import shutil
import subprocess
from datetime import datetime, timezone
from typing import Any


def current_git_rev() -> str:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return "unknown"


def profile_manifest(
    *,
    compiler_kind: str,
    corpus_profile: str,
    iterations_scale: float,
    config: str,
    build_type: str,
    dispatch: str,
    extra_args: list[str],
    producer: str,
) -> dict[str, Any]:
    return {
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "git_rev": current_git_rev(),
        "compiler_kind": compiler_kind,
        "corpus_profile": corpus_profile,
        "iterations_scale": iterations_scale,
        "config": config,
        "build_type": build_type,
        "dispatch": dispatch,
        "extra_args": list(extra_args),
        "producer": producer,
        "cache_policy": "single-run-generate-phase",
    }


def manifest_path(profile_dir: pathlib.Path) -> pathlib.Path:
    return profile_dir / "profile_manifest.json"


def load_manifest(profile_dir: pathlib.Path) -> dict[str, Any] | None:
    path = manifest_path(profile_dir)
    if not path.exists():
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def invalidation_reasons(old: dict[str, Any] | None, new: dict[str, Any]) -> list[str]:
    reasons: list[str] = []
    if old is None:
        reasons.append("no prior manifest")
        return reasons

    for key in ("git_rev", "compiler_kind", "corpus_profile", "config", "build_type", "dispatch", "producer"):
        if old.get(key) != new.get(key):
            reasons.append(f"{key} changed: {old.get(key)} -> {new.get(key)}")

    old_scale = float(old.get("iterations_scale", 0.0))
    new_scale = float(new.get("iterations_scale", 0.0))
    if abs(old_scale - new_scale) > 1e-12:
        reasons.append(f"iterations_scale changed: {old_scale} -> {new_scale}")

    if list(old.get("extra_args", [])) != list(new.get("extra_args", [])):
        reasons.append("extra_args changed")

    if old.get("cache_policy") != new.get("cache_policy"):
        reasons.append(f"cache_policy changed: {old.get('cache_policy')} -> {new.get('cache_policy')}")

    return reasons


def reset_profile_dir(profile_dir: pathlib.Path, manifest: dict[str, Any]) -> list[str]:
    old_manifest = load_manifest(profile_dir)
    reasons = invalidation_reasons(old_manifest, manifest)

    if profile_dir.exists():
        shutil.rmtree(profile_dir)
    profile_dir.mkdir(parents=True, exist_ok=True)
    manifest_path(profile_dir).write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return reasons
