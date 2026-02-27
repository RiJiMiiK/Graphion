#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"${SCRIPT_DIR}/setup_hooks.sh"

if command -v python3 >/dev/null 2>&1; then
  python3 "${SCRIPT_DIR}/../quality/check_repo_health.py"
else
  echo "python3 not found; skipping repo-health check"
fi

echo "Bootstrap complete."
