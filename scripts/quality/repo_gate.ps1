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
Require-Command docker "Install Docker to mirror the links-check workflow locally."

Push-Location (Resolve-Path "$PSScriptRoot\..\..")
try {
  $RootPath = (Get-Location).Path
  $LycheeImage = "lycheeverse/lychee:latest"
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

  for ($attempt = 1; $attempt -le 3; $attempt++) {
    docker pull $LycheeImage
    if ($LASTEXITCODE -eq 0) {
      break
    }
    if ($attempt -eq 3) {
      throw "repo gate: failed to pull $LycheeImage"
    }
    Start-Sleep -Seconds 5
  }

  docker run --rm `
    -e "GITHUB_TOKEN=$env:GITHUB_TOKEN" `
    -v "${RootPath}:/input" `
    -w /input `
    $LycheeImage `
    --no-progress `
    --verbose `
    README.md `
    QUALITY.md `
    ROADMAP.md `
    examples/README.md `
    docs/**/*.md `
    CONTRIBUTING.md `
    SECURITY.md
}
finally {
  Pop-Location
}

Write-Host "Repo gate passed"
