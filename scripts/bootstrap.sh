#!/usr/bin/env bash
set -euo pipefail

./scripts/setup_hooks.sh

if command -v python3 >/dev/null 2>&1; then
  python3 scripts/check_repo_health.py
else
  echo "python3 not found; skipping repo-health check"
fi

echo "Bootstrap complete."
