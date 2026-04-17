# Runtime And Engineering Docs

This section documents the backend side of Graphion:

- runtime structure
- VM architecture
- instruction semantics
- debugging and error behavior
- rebuild constraints

This is not the place to learn `.gion` syntax as a user.
For the language itself, start in [Graphion User Guide](../graphion/index.md).

## What This Section Covers

Use the runtime docs when you need to understand:

- how `.gion` source reaches execution
- which layer owns parsing, lowering, and execution
- what the VM instruction set looks like
- how runtime-visible errors are organized
- which architectural constraints the rebuild is following

## Recommended Reading Order

If you are new to the backend, read these in order:

1. [Rebuild Charter](core/REBUILD_CHARTER.md)
2. [Architecture](core/ARCHITECTURE.md)
3. [VM ISA](core/ISA.md)
4. [Errors](debugging/ERRORS.md)

That order is intentional:

- the charter explains the constraints
- the architecture page explains the current structure
- the ISA page explains the VM surface
- the errors page explains failure behavior

## Sections

```{toctree}
:maxdepth: 2

core/REBUILD_CHARTER
core/ARCHITECTURE
core/ISA
debugging/ERRORS
```

## Scope Notes

Two distinctions matter when reading runtime documentation:

1. the VM is broader than the current documented `.gion` frontend
- some runtime and opcode families exist in the backend without being exposed directly in the current source language surface

2. runtime docs are backend-facing, not language-facing
- they describe structure, execution, and contracts
- they should not be read as proof that every backend capability is already a user-language feature

If you need the actual user-facing contract, use the pages under `docs/graphion/`.
