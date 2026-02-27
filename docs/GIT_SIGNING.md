# Git Signing

## Goal

Improve provenance by signing commits and tags.

## Recommended setup

- Use SSH signing (simple on GitHub) or GPG signing.
- Enable automatic signing in local git config.

Example:

```bash
git config --global commit.gpgsign true
git config --global tag.gpgSign true
```

## Repository setting

In GitHub branch protection for `main`, enable:
- Require signed commits.

See `docs/BRANCH_PROTECTION.md` for full protection policy.
