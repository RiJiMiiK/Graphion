param()

$ErrorActionPreference = "Stop"

function Require-Command {
  param(
    [string]$Name,
    [string]$Hint
  )

  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    throw "repo gate: missing required tool '$Name'. $Hint"
  }
}

Require-Command python "Install Python 3 to run repo-health and docs checks."
Require-Command cspell "Install cspell-cli to mirror the spellcheck workflow locally."
Require-Command lychee "Install lychee to mirror the links-check workflow locally."

Push-Location (Resolve-Path "$PSScriptRoot\..\..")
try {
  python scripts/quality/check_repo_health.py
  python -m sphinx -b html docs docs/_build/html

  $CspellFiles = @(
    "README.md",
    "QUALITY.md",
    "ROADMAP.md",
    "CHANGELOG.md",
    "CODE_OF_CONDUCT.md",
    "CONTRIBUTING.md",
    "MAINTAINERS.md",
    "SECURITY.md",
    "examples/README.md",
    "docs/**/*.md",
    ".github/**/*.md"
  )

  cspell --config .cspell.json @CspellFiles

  $LycheeTargets = @(
    "README.md",
    "QUALITY.md",
    "ROADMAP.md",
    "examples/README.md",
    "docs/**/*.md",
    "CONTRIBUTING.md",
    "SECURITY.md"
  )

  lychee --no-progress --verbose @LycheeTargets
}
finally {
  Pop-Location
}

Write-Host "Repo gate passed"
