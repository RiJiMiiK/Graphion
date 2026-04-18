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
- the Bash and PowerShell quality-gate entry points should stay aligned on build, test, asm-safety, clang-tidy, and optional cppcheck behavior
- sanitizers are enabled by the local quality gate on non-Windows platforms and skipped explicitly on Windows toolchains
