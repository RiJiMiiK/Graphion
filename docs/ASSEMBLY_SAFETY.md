# Assembly Safety Guide

This project uses assembly only for measured VM hot paths.

## Important safety fact

User-mode assembly in a normal process cannot directly "break" your PC hardware.
Modern OS protections prevent privileged register access from user space.

Real risks are:
- Process crashes.
- Data corruption in memory.
- Security bugs (undefined behavior, out-of-bounds writes).

## Safe workflow (recommended)

1. Keep assembly disabled by default (`GRAPHION_ENABLE_ASM=OFF`).
2. Develop and run with sanitizers enabled first.
3. Enable assembly only for small, reviewed hotpath changes.
4. Run the assembly safety checker before commit.
5. Keep benchmark fixtures deterministic.

## Build commands

Default:

```bash
cmake -S . -B build
cmake --build build --config Release
```

With sanitizers (Linux/macOS):

```bash
cmake -S . -B build-sanitize -DGRAPHION_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
```

With assembly (x86_64, GCC/Clang toolchains):

```bash
cmake -S . -B build-asm -DGRAPHION_ENABLE_ASM=ON
cmake --build build-asm --config Release
```

## Assembly policy

- Forbidden by default: privileged/control-register/system-entry instructions.
- Enforced by `scripts/quality/check_asm_safety.py` in CI.
- Exception process: add `ALLOW_UNSAFE_ASM` on the same line and justify in PR.
- ABI/register mapping for current hotpath: `docs/ASM_REGISTERS.md`.

