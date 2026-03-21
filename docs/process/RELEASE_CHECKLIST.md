# Maintainer Release Checklist

1. Confirm `main` is green on required checks.
2. Confirm `CHANGELOG.md` updated and `[Unreleased]` trimmed.
3. Confirm no open critical security issues.
4. Run local quality gate (`scripts/quality/quality_gate.*`).
5. Review the release-candidate PGO smoke alert on the PR.
6. Run the manual blocking PGO alert validation and confirm no hard failures.
7. Re-run benchmark and ensure no unacceptable regression.
8. Create signed tag (`vMAJOR.MINOR.PATCH`).
9. Push tag and verify GitHub release workflow succeeds.
10. Validate generated release notes and source archives.
11. Announce release in Discussions (optional).
