# Contributing

## Development setup

1. Install CMake (>= 3.20) and a C11 compiler.
2. Configure and build:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -C Debug
```

Optional reproducible setup with Docker:

```bash
docker compose build
docker compose run --rm graphion-dev
```

Inside container:

```bash
./scripts/dev_build.sh
```

Enable local git hooks:

```bash
./scripts/setup_hooks.sh
```

## Coding rules

- Keep C code C11 compatible.
- Run formatting (`clang-format`) before opening a PR.
- Keep warnings at zero on supported toolchains.
- Avoid introducing UB; prefer explicit bounds checks.
- Run `python scripts/check_asm_safety.py` before pushing asm changes.
- Run tests with `ctest --test-dir <build-dir>`.
- Run tests with `ctest --test-dir <build-dir> -C Debug`.
- Include benchmark note for performance-sensitive PRs.

## Pull requests

- Keep PRs focused and small.
- Include rationale and benchmark notes for performance-sensitive changes.
- Add or update tests when behavior changes.
- For VM hot paths, include before/after performance evidence.

## Commit message style

Use concise imperative messages, for example:
- `vm: add bounds check on register access`
- `ci: add cppcheck step for Linux`

## Release and architecture docs

- Release process: `docs/RELEASE.md`
- Architecture overview: `docs/ARCHITECTURE.md`
