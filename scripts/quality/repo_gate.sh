#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

require_cmd() {
  local cmd="$1"
  local install_hint="$2"
  if ! command -v "${cmd}" >/dev/null 2>&1; then
    echo "repo gate: missing required tool '${cmd}'"
    echo "repo gate: ${install_hint}"
    exit 1
  fi
}

require_cmd python3 "install Python 3 to run repo-health and docs checks"
require_cmd cspell "install cspell-cli to mirror the spellcheck workflow locally"
require_cmd lychee "install lychee to mirror the links-check workflow locally"

cd "${ROOT_DIR}"

python3 scripts/quality/check_repo_health.py
python3 -m sphinx -b html docs docs/_build/html

cspell \
  --config .cspell.json \
  README.md \
  QUALITY.md \
  ROADMAP.md \
  CHANGELOG.md \
  CODE_OF_CONDUCT.md \
  CONTRIBUTING.md \
  MAINTAINERS.md \
  OWNERSHIP.md \
  SECURITY.md \
  SECURITY_CONTACTS.md \
  SUPPORT.md \
  examples/README.md \
  docs/**/*.md \
  .github/**/*.md

lychee \
  --no-progress \
  --verbose \
  README.md \
  QUALITY.md \
  ROADMAP.md \
  examples/README.md \
  docs/**/*.md \
  SUPPORT.md \
  CONTRIBUTING.md \
  SECURITY.md

echo "Repo gate passed"
