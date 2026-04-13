# Scripts

This directory is organized by purpose:

- `scripts/dev/`
  - local bootstrap, build, and hook helpers
- `scripts/quality/`
  - local quality and safety checks
- `scripts/repo/`
  - repository maintenance helpers

## Common entry points

- quality gate:
  - `scripts/quality/quality_gate.sh`
  - `scripts/quality/quality_gate.ps1`

## Notes

- if you add a new script, put it in the narrowest matching subdirectory instead of growing the root
