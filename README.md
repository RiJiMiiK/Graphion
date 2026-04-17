# Graphion
[![CI](https://github.com/RiJiMiiK/Graphion/actions/workflows/ci.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/ci.yml)
[![CodeQL](https://github.com/RiJiMiiK/Graphion/actions/workflows/codeql.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/codeql.yml)
[![Coverage](https://github.com/RiJiMiiK/Graphion/actions/workflows/coverage.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/coverage.yml)
[![Fuzz Nightly](https://github.com/RiJiMiiK/Graphion/actions/workflows/fuzz-nightly.yml/badge.svg)](https://github.com/RiJiMiiK/Graphion/actions/workflows/fuzz-nightly.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Graphion is a language project built around a `.gion` source pipeline and a VM backend.

Today, the implemented user-facing `.gion` surface is a scalar language subset with:

- variable assignment, reuse, and compound assignments
- `print(...)`
- scalar values: `int`, `float`, `bool`, `string`, and `bits`
- built-in numeric constants such as `pi`, `tau`, `phi`, `e`, `nan`, and `inf`
- arithmetic expressions, grouped expressions, and postfix factorial
- comparisons and boolean logic
- `if / elif / else`, ternary expressions, and `match ... default`
- line comments and block comments
- `bits` literals and bitwise operators
- documented scalar builtins such as `abs`, `min`, `max`, `clamp`, trigonometric helpers, rounding helpers, and numeric predicates

## Current direction

The rebuild is guided by one target pipeline:

- `source Graphion -> tokens/parsing -> internal representation -> bytecode -> VM`

Current project state:

- the user-facing `.gion` frontend is documented for the implemented scalar subset only
- the repository also contains broader VM work for graph and hypergraph execution
- the active repo lane is currently hygiene and maintenance rather than new language-surface expansion

## Quick start

Supported toolchains today:

- Linux: GCC and Clang are supported and covered in CI.
- Windows: MSVC is supported and covered in CI.
- Windows GCC / MinGW: not currently a supported toolchain. Local builds may work, but this path is not covered in CI and is not treated as a release-blocking configuration.

Configure, build, and test:

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

Run the main executable on any example:

```powershell
.\build\Release\graphion.exe .\examples\01_scalars_and_print.gion
```

Start with the examples index:

- [examples/README.md](examples/README.md)

## Developer helpers

Quick local dev build:

```bash
./scripts/dev/dev_build.sh
```

```powershell
./scripts/dev/dev_build.ps1
```

Repository bootstrap:

```bash
./scripts/dev/bootstrap.sh
```

```powershell
./scripts/dev/bootstrap.ps1
```

Enable local hooks:

```bash
./scripts/dev/setup_hooks.sh
```

```powershell
./scripts/dev/setup_hooks.ps1
```

## Documentation

Primary documentation entry points:

- user docs: [docs/graphion/index.md](docs/graphion/index.md)
- site home: [docs/index.md](docs/index.md)
- architecture: [docs/runtime/core/ARCHITECTURE.md](docs/runtime/core/ARCHITECTURE.md)
- ISA: [docs/runtime/core/ISA.md](docs/runtime/core/ISA.md)
- errors: [docs/runtime/debugging/ERRORS.md](docs/runtime/debugging/ERRORS.md)
- rebuild charter: [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md)

Generate the HTML site locally:

```powershell
python -m sphinx -b html docs docs/_build/html
```

## Contact

Use the Discord server for questions, support, and private contact:

- https://discord.com/invite/mPzDQ7TYkj

## License

MIT (see [LICENSE](LICENSE)).
