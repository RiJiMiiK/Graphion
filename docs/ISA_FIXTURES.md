# ISA Fixture Format Policy

## Scope

This document defines the expected structure and maintenance rules for Graphion
ISA conformance fixtures.

Current fixture implementation lives in:

- `tests/test_isa.c`

It covers:

- decode fixtures
- execute fixtures

## Goals

ISA fixtures exist to provide a stable, reviewable contract for:

- bytecode decoding
- VM-visible execution semantics
- error behavior that is part of the ISA contract

They are not intended to replace:

- microbenchmarks
- fuzzing
- broad subsystem tests

## Fixture classes

### Decode fixtures

Decode fixtures validate:

- exact byte layout
- instruction count
- decoded field values
- decode failure behavior for malformed inputs

Current format in `tests/test_isa.c`:

- `name`
- `bytes`
- `byte_len`
- `expected_rc`
- `out_capacity`
- `expected_count`
- `expected_program`

### Execute fixtures

Execute fixtures validate:

- program load success or failure
- runtime return code
- `halted` state
- final `pc`
- final register file
- required bound runtime state for graph/hypergraph opcodes

Current format in `tests/test_isa.c`:

- `name`
- `program`
- `program_len`
- `expected_load_rc`
- `expected_run_rc`
- `expect_halted`
- `expected_pc`
- `expected_regs`
- `bind_csr`
- `bind_hypergraph`

## Required properties of a fixture

Every fixture must be:

- deterministic
- self-contained
- small enough to review directly in code review
- tied to one clear semantic claim

Fixtures must not depend on:

- random input
- benchmark-only behavior
- machine timing
- allocator layout accidents

## Expansion policy

When ISA behavior changes or expands, fixture coverage must grow in the same
change if the behavior is externally observable.

At minimum, fixture expansion is required for:

- new opcodes
- changed opcode operand contracts
- changed error behavior
- changed deterministic execution expectations
- changed overflow semantics
- changed byte encoding rules

## Review policy

Each fixture should answer one of these questions:

1. does decoding produce the exact documented instruction sequence?
2. does execution produce the exact documented state transition?
3. does malformed input fail in the documented way?

If a fixture does not answer one of those questions, it likely belongs in a
different test layer.

## Naming policy

Fixture names should:

- describe the semantic claim
- avoid implementation details that may change
- stay stable unless the semantic claim itself changes

Recommended naming style:

- `decode_<claim>`
- `exec_<claim>`

Examples:

- `decode_valid_program`
- `decode_truncated_program`
- `exec_unknown_opcode`
- `exec_bfs_levels`

## Data-shape policy

Decode fixtures should prefer:

- inline byte arrays
- explicit expected instruction arrays

Execute fixtures should prefer:

- short inline instruction arrays
- explicit expected register snapshots

Avoid:

- generated fixture blobs
- opaque binary files
- large external fixture assets

Until the ISA grows significantly, keeping fixtures inline in C is the
preferred format because it is easier to review and maintain.

## When to split fixtures into external files

External fixture files are justified only if at least one of these becomes true:

- the ISA surface becomes too large for one test source file
- fixture blobs become difficult to review inline
- multiple tools need to consume the same fixture corpus

If that happens, the transition must define:

- file naming convention
- schema / layout
- loader behavior
- versioning policy for fixtures

## Relationship with versioning

Fixture changes must remain aligned with:

- `docs/ISA.md`
- `docs/ISA_VERSIONING.md`
- `docs/VM_ERRORS.md`

In `v0.x`, fixtures may evolve with incompatible ISA changes, but the same
change must update docs and fixtures together.

In `v1.x`, fixtures become part of the compatibility contract and should only
change in backward-compatible ways unless a new major ISA version is introduced.

## Minimum coverage expectations

The fixture suite should maintain coverage for:

- basic arithmetic path
- invalid-register behavior
- unknown-opcode behavior
- decode truncation / capacity errors
- graph opcode happy-path cases
- hypergraph opcode happy-path cases
- deterministic execution expectations
- overflow semantics for arithmetic opcodes

## Out of scope

These do not belong in ISA fixtures:

- performance thresholds
- compiler-specific codegen differences
- PGO behavior
- asm-vs-C speed comparisons

Those belong in benchmark or parity workflows, not in the ISA contract suite.
