# Branch Protection (GitHub)

Configure in: `Settings -> Branches -> Add rule -> main`

## Recommended required checks

- `build-and-check (ubuntu-latest, gcc, Debug)`
- `build-and-check (ubuntu-latest, gcc, Release)`
- `build-and-check (ubuntu-latest, clang, Debug)`
- `build-and-check (ubuntu-latest, clang, Release)`
- `build-and-check (windows-latest, msvc, Debug)`
- `build-and-check (windows-latest, msvc, Release)`
- `sanitize-linux`
- `clang-tidy`
- `actionlint`
- `links-check`
- `spellcheck`
- `license-headers`
- `repo-health`
- `docker-build`
- `coverage-linux`
- `Analyze C/C++`
- `gitleaks`
- `sbom-scan`
- `fuzz-vm-smoke`
- `semantic-pr-title`

## Recommended policy

- Require pull request before merging.
- Require approvals: at least 1.
- Require status checks to pass before merging.
- Require conversation resolution before merging.
- Require linear history.
- Dismiss stale approvals on new commits.
- Restrict force pushes and deletions.
- Require signed commits.
- Allow squash merge only.
