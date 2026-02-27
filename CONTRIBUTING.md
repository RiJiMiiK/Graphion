# Contributing

## Development setup

1. Install CMake (>= 3.20) and a C11 compiler.
2. Configure and build:

```bash
cmake -S . -B build
cmake --build build
```

## Coding rules

- Keep C code C11 compatible.
- Run formatting (`clang-format`) before opening a PR.
- Keep warnings at zero on supported toolchains.
- Avoid introducing UB; prefer explicit bounds checks.

## Pull requests

- Keep PRs focused and small.
- Include rationale and benchmark notes for performance-sensitive changes.
- Add or update tests when behavior changes.
- For VM hot paths, include before/after performance evidence.

## Commit message style

Use concise imperative messages, for example:
- `vm: add bounds check on register access`
- `ci: add cppcheck step for Linux`
