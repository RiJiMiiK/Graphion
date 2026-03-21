#!/usr/bin/env python3
from __future__ import annotations

import os
import pathlib
import platform
import socket
import subprocess
import sys
from datetime import datetime, timezone


REQUIRED_BASE_FIELDS = [
    "generated_utc",
    "platform_label",
    "platform",
    "machine",
    "cpu_model",
    "hostname",
    "python",
    "git_rev",
    "runs",
]


def _run_text(cmd: list[str]) -> str:
    try:
        proc = subprocess.run(cmd, check=True, capture_output=True, text=True)
        return proc.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return ""


def git_rev() -> str:
    value = _run_text(["git", "rev-parse", "HEAD"])
    return value if value else "unknown"


def cpu_model() -> str:
    if sys.platform.startswith("win"):
        value = os.environ.get("PROCESSOR_IDENTIFIER", "").strip()
        if value:
            return value
    linux_cpuinfo = pathlib.Path("/proc/cpuinfo")
    if linux_cpuinfo.exists():
        for line in linux_cpuinfo.read_text(encoding="utf-8", errors="ignore").splitlines():
            if line.lower().startswith("model name"):
                _, _, value = line.partition(":")
                value = value.strip()
                if value:
                    return value
    value = _run_text(["sysctl", "-n", "machdep.cpu.brand_string"])
    if value:
        return value
    value = platform.processor().strip()
    if value:
        return value
    return platform.machine()


def base_metadata(platform_label: str, runs: int, extra: dict[str, object] | None = None) -> dict[str, object]:
    metadata: dict[str, object] = {
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "platform_label": platform_label,
        "platform": platform.platform(),
        "machine": platform.machine(),
        "cpu_model": cpu_model(),
        "hostname": socket.gethostname(),
        "python": platform.python_version(),
        "git_rev": git_rev(),
        "runs": runs,
    }
    if extra:
        metadata.update(extra)
    return metadata


def validate_metadata(metadata: dict[str, object], context: str, extra_required: list[str] | None = None) -> None:
    required = REQUIRED_BASE_FIELDS + (extra_required or [])
    missing = [key for key in required if key not in metadata or metadata[key] in ("", None, [])]
    if missing:
        raise ValueError(f"{context}: missing required metadata fields: {', '.join(missing)}")
