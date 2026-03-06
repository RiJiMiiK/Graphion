# Maintainer Release Checklist

1. Confirm `main` is green on required checks.
2. Confirm `CHANGELOG.md` updated and `[Unreleased]` trimmed.
3. Confirm no open critical security issues.
4. Run local quality gate (`scripts/quality/quality_gate.*`).
5. Review the release-candidate PGO smoke alert and confirm no hard failures.
6. Re-run benchmark and ensure no unacceptable regression.
7. Create signed tag (`vMAJOR.MINOR.PATCH`).
8. Push tag and verify GitHub release workflow succeeds.
9. Validate generated release notes and source archives.
10. Announce release in Discussions (optional).
