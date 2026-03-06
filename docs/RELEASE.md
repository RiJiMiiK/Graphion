# Release Process

## Versioning

- Use Semantic Versioning tags: `vMAJOR.MINOR.PATCH`.
- Keep unreleased notes in `CHANGELOG.md`.

## Steps

1. Update `CHANGELOG.md` and move relevant entries from `[Unreleased]`.
2. Ensure CI checks are green.
3. Follow `docs/RELEASE_CHECKLIST.md`.
4. Ensure the release-candidate PGO smoke alert is green on release-related PRs.
5. Create and push tag:

```bash
git tag v0.1.0
git push origin v0.1.0
```

6. GitHub Actions `release.yml` creates a release with source archives.

## Policy

- No direct release from unreviewed commits.
- Security fixes can ship as patch releases.
- Prefer signed tags for releases.
- Release candidates must surface a PGO/non-PGO alert summary before tagging.
