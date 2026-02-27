# Graphion

Graphion is a domain-specific language project focused on graph and hypergraph workloads.

Current scope:
- High-performance interpreter in C.
- Optional hand-tuned assembly for VM hot paths.
- Compiled backend planned later.

## Status

Early stage repository. The current baseline provides:
- Minimal VM/runtime scaffold in C.
- Arena allocator primitive.
- CMake build with strict warnings.
- CI, static checks, and security workflows.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Enable assembly hot paths (x86_64 only):

```bash
cmake -S . -B build-asm -DGRAPHION_ENABLE_ASM=ON
cmake --build build-asm --config Release
```

Sanitizer build (Linux/macOS with Clang/GCC):

```bash
cmake -S . -B build-sanitize -DGRAPHION_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
```

## Security

See [SECURITY.md](SECURITY.md) for vulnerability reporting and supported versions.
For assembly-specific safeguards and workflow, see [docs/ASSEMBLY_SAFETY.md](docs/ASSEMBLY_SAFETY.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

MIT (see [LICENSE](LICENSE)).
