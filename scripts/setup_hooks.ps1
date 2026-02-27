$ErrorActionPreference = "Stop"

git config core.hooksPath .githooks
Write-Host "Git hooks configured (core.hooksPath=.githooks)"
