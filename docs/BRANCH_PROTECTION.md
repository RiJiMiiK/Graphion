# Branch Protection (GitHub)

Configure in: `Settings -> Branches -> Add rule -> main`

## Recommended required checks

- `build-and-check (ubuntu-latest, Debug)`
- `build-and-check (ubuntu-latest, Release)`
- `build-and-check (windows-latest, Debug)`
- `build-and-check (windows-latest, Release)`
- `sanitize-linux`
- `docker-build`
- `coverage-linux`
- `Analyze C/C++`
- `gitleaks`
- `sbom-scan`
- `fuzz-vm-smoke`

## Recommended policy

- Require pull request before merging.
- Require approvals: at least 1.
- Require status checks to pass before merging.
- Require conversation resolution before merging.
- Require linear history.
- Dismiss stale approvals on new commits.
- Restrict force pushes and deletions.
