# Deterministic VM Repro Workflow

## Goal

Provide a repeatable workflow for reproducing VM and ISA issues with the same:

- fixture or minimal program
- VM snapshot
- execution mode
- environment metadata

This workflow is the reference path for bug reports involving:

- opcode semantics
- decode/load/execute failures
- deterministic-mode behavior
- dispatch-variant drift
- asm-vs-C semantic drift

## Required Repro Bundle

Every deterministic repro should capture all of the following:

1. fixture or minimal program
2. expected behavior
3. actual behavior
4. VM snapshot
5. environment metadata

Minimum bundle contents:

- fixture name, if the issue already maps to `tests/test_isa.c`
- otherwise, the exact instruction sequence or byte payload
- whether deterministic mode was enabled
- return code from load/run
- final VM snapshot from `graphion_vm_write_snapshot(...)`
- compiler / platform / build mode used for reproduction
- whether the repro was observed on:
  - portable C path
  - asm-enabled build
  - specific dispatch variant

## Repro Order

Follow this order:

1. reduce to the smallest fixture or inline program
2. rerun with deterministic mode enabled
3. capture the VM snapshot
4. rerun on the portable C path
5. if relevant, rerun on asm-enabled or alternate dispatch builds

This order matters:

- the smallest fixture makes semantic review easier
- deterministic mode removes fastpath and dispatch ambiguity
- snapshot capture records the final observable VM state
- the C path remains the semantic reference

## Preferred Repro Shapes

Use one of these shapes:

### Existing ISA fixture

If the bug already fits an existing fixture class:

- add or adapt a fixture in `tests/test_isa.c`
- record the fixture name in the issue or PR

This is preferred for:

- decode failures
- unknown opcode behavior
- invalid-register behavior
- documented opcode semantics

### Inline VM program

Use a short `graphion_insn[]` program when the bug is easier to express as a VM execution snippet.

This is preferred for:

- runtime-only edge cases
- deterministic-mode checks
- snapshot-format validation
- graph or hypergraph runtime binding behavior

### Byte payload

Use raw bytes only when the issue is specifically about decoding or encoded instruction layout.

## Snapshot Capture

Use:

- `graphion_vm_write_snapshot(...)`

The snapshot must be taken after the relevant load/run step that demonstrates the issue.

For deterministic repro work:

- enable deterministic mode first when execution semantics are under investigation
- capture the snapshot after the failing or suspicious run
- attach the exact `GRAPHION_VM_SNAPSHOT_V1` payload when possible

Relevant snapshot doc:

- `docs/VM_SNAPSHOT.md`

## Environment Capture

Always capture:

- git revision
- platform / OS
- compiler or toolchain
- build configuration
- dispatch strategy, if non-default
- whether `GRAPHION_ENABLE_ASM` was enabled

If the issue is Linux asm-specific, note whether the repro came from:

- local Docker run
- CI Linux run

## Suggested Commands

Base test run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Targeted local test execution:

```powershell
& .\build\graphion_tests.exe vm_deterministic_mode_unknown_opcode
```

Dispatch-variant parity run:

```powershell
python scripts/quality/test_dispatch_variants.py --build-root build-dispatch-tests --build-type Release
```

Linux asm hardening parity run:

```powershell
docker compose run --rm graphion-dev bash -lc "python3 scripts/quality/test_asm_hardening_parity.py --build-root build-asm-hardening -- -G Ninja -DCMAKE_C_COMPILER=gcc"
```

## Triage Rules

When a repro differs across execution modes:

- deterministic mode is the first semantic reference
- the portable C path is the final semantic reference
- asm and specialized dispatch paths must explain themselves against that reference

If deterministic mode and the portable C path agree, but another path disagrees:

- treat it as a dispatch or asm parity issue

If decode, load, and execute all disagree across reports:

- reduce the case to one failing layer first

## Relationship With Other Docs

- `docs/ISA.md`: opcode contract
- `docs/ISA_FIXTURES.md`: fixture format and expansion policy
- `docs/VM_ERRORS.md`: error-code interpretation
- `docs/VM_SNAPSHOT.md`: snapshot format
- `docs/ASM_FALLBACK_POLICY.md`: asm parity and policy rules
