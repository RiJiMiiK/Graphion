# Contributing

## Build and test first

Recommended baseline:

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

Useful local helpers:

```bash
./scripts/dev/bootstrap.sh
./scripts/dev/dev_build.sh
./scripts/dev/setup_hooks.sh
```

PowerShell equivalents exist in the same directories.

## Working style

- keep changes focused
- add or update tests when behavior changes
- prefer clear behavior over clever shortcuts
- keep the current rebuild charter in mind
- document user-visible changes when the language surface changes

## For performance-sensitive changes

- include a benchmark note
- distinguish VM impact from `.gion` source-level impact
- avoid benchmark-only shortcuts

## For runtime / VM changes

- keep VM-visible behavior aligned with the docs
- update [docs/runtime/core/ISA.md](docs/runtime/core/ISA.md) when instruction semantics change
- update [docs/runtime/debugging/ERRORS.md](docs/runtime/debugging/ERRORS.md) when error behavior changes

## For language changes

- update the user docs in `docs/graphion/`
- update the sample program when it helps demonstrate the new feature
- add happy-path and error-path tests

## Reference docs

- architecture: `docs/runtime/core/ARCHITECTURE.md`
- ISA: `docs/runtime/core/ISA.md`
- errors: `docs/runtime/debugging/ERRORS.md`
- rebuild charter: `docs/runtime/core/REBUILD_CHARTER.md`

## Contact

If you need to reach the maintainer directly, use:

- https://discord.gg/mPzDQ7TYkj
