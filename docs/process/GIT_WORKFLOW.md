# Git Workflow Policy

## Branch naming

Use:
- `feat/<short-topic>`
- `fix/<short-topic>`
- `chore/<short-topic>`
- `docs/<short-topic>`
- `ci/<short-topic>`
- `perf/<short-topic>`
- `test/<short-topic>`

## Pull request titles

Use semantic format:
- `feat(vm): add bytecode decode guard`
- `fix(parser): reject truncated instruction stream`
- `ci(actions): add nightly fuzz workflow`

## Merge policy

- Default merge method: squash merge.
- Keep linear history.
- Do not merge directly to `main`.
- Require all required checks to pass.
- Require at least 1 approval.

## Auto-merge

Allowed only when:
- Required checks are green.
- No unresolved conversations.
- PR is approved.
