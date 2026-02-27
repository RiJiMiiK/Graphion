# Maintainer Release Checklist

1. Confirm `main` is green on required checks.
2. Confirm `CHANGELOG.md` updated and `[Unreleased]` trimmed.
3. Confirm no open critical security issues.
4. Run local quality gate (`scripts/quality/quality_gate.*`).
5. Re-run benchmark and ensure no unacceptable regression.
6. Create signed tag (`vMAJOR.MINOR.PATCH`).
7. Push tag and verify GitHub release workflow succeeds.
8. Validate generated release notes and source archives.
9. Announce release in Discussions (optional).
