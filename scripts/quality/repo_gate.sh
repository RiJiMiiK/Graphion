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
require_cmd docker "install Docker to mirror the links-check workflow locally"

cd "${ROOT_DIR}"

LYCHEE_IMAGE="lycheeverse/lychee:latest"

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
  SECURITY.md \
  examples/README.md \
  docs/**/*.md \
  .github/**/*.md

for attempt in 1 2 3; do
  if docker pull "${LYCHEE_IMAGE}"; then
    break
  fi
  if [ "${attempt}" -eq 3 ]; then
    echo "repo gate: failed to pull ${LYCHEE_IMAGE}"
    exit 1
  fi
  sleep 5
done

docker run --rm \
  -e GITHUB_TOKEN="${GITHUB_TOKEN:-}" \
  -v "${ROOT_DIR}:/input" \
  -w /input \
  "${LYCHEE_IMAGE}" \
  --no-progress \
  --verbose \
  README.md \
  QUALITY.md \
  ROADMAP.md \
  examples/README.md \
  docs/**/*.md \
  CONTRIBUTING.md \
  SECURITY.md

echo "Repo gate passed"
