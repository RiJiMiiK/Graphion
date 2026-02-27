# GitHub Actions Security

## Current policy

- Prefer pinned actions (`owner/repo@<full-commit-sha>`).
- If version tags are used (`@vX`), monitor updates and security advisories.
- Keep permissions minimal per workflow.

## Automation

- `actions-pinning-audit.yml` reports non-pinned actions.
- Monthly audit issue includes action pinning review.

## Migration plan

1. Prioritize pinning actions used in release/security workflows.
2. Pin remaining workflows progressively.
3. Keep Dependabot enabled for GitHub Actions.
