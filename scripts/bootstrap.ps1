$ErrorActionPreference = "Stop"

& "$PSScriptRoot\setup_hooks.ps1"

if (Get-Command python -ErrorAction SilentlyContinue) {
  python scripts/check_repo_health.py
} else {
  Write-Host "python not found; skipping repo-health check"
}

Write-Host "Bootstrap complete."
