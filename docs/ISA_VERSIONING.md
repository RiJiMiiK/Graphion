# ISA Version Policy

## Scope

This document defines how Graphion VM ISA versions evolve from the current `v0.x`
series toward a future stable `v1.0`.

It applies to:

- VM bytecode encoding
- VM opcode semantics
- VM-visible error behavior
- frontend-to-bytecode compatibility claims
- fixture, parser, and conformance test expectations

## Current state

- Current implemented ISA line: `v0.x`
- Current concrete ISA revision: `v0`
- Stability level: experimental

`v0.x` means the ISA is usable for development and benchmarking, but not yet
frozen for long-term bytecode compatibility.

## Version classes

### `v0.x`

Experimental pre-stable line.

Rules:

- Backward-incompatible changes are allowed.
- Opcode numbers may still change.
- Encoding details may still change.
- Error-code assignments may still change.
- Any incompatible change must update:
  - `docs/ISA.md`
  - `docs/IR.md` when lowering or parser contracts are affected
  - conformance tests / parser tests / fuzz expectations
  - `CHANGELOG.md`

Expectation:

- Source compatibility is preferred when practical.
- Bytecode compatibility is not guaranteed across `v0.x` revisions.

### `v1.0`

First stable ISA contract.

Rules:

- Opcode numeric assignments are frozen.
- Instruction encoding is frozen.
- Core execution semantics are frozen.
- Structured VM error-code assignments are frozen.
- Previously documented valid bytecode for `v1.0` must remain valid unless a
  version gate explicitly rejects unsupported future features.

Expectation:

- Bytecode compatibility is guaranteed within the `v1.x` line unless a later
  major version is introduced.

## Compatibility matrix

| Producer / Artifact | Consumer | Status | Notes |
| --- | --- | --- | --- |
| `v0.x` source frontend | `v0.x` IR bridge | supported | Current development path. |
| `v0.x` IR bridge | `v0.x` bytecode / VM | supported | Current development path. |
| `v0.x` bytecode | later `v0.x` VM | best effort only | No compatibility guarantee. |
| `v0.x` bytecode | `v1.0` VM | not guaranteed | Requires explicit migration or versioned decode policy. |
| `v1.0` source frontend | `v1.0` IR bridge | planned | Will become stable at `v1.0`. |
| `v1.0` IR bridge | `v1.0` bytecode / VM | planned | Will become stable at `v1.0`. |
| `v1.0` bytecode | later `v1.x` VM | planned guarantee | Intended stable compatibility contract. |

## Required workflow for ISA changes

Any ISA-affecting change must classify itself as one of:

1. documentation-only clarification
2. backward-compatible extension
3. backward-incompatible pre-`v1.0` change
4. version-gated post-`v1.0` change

At minimum, the change must update:

- `docs/ISA.md`
- `docs/IR.md` if the parser/IR bridge is affected
- tests or fixtures that encode the affected behavior
- `CHANGELOG.md`

## Rules for adding opcodes before `v1.0`

Before `v1.0`, new opcodes are allowed if they satisfy all of:

- documented numeric assignment
- documented operand contract
- documented failure behavior
- parser/IR lowering impact described when applicable
- tests added for decode and execute behavior

## Rules for changing existing opcodes before `v1.0`

Before `v1.0`, changing an existing opcode is allowed only if:

- the change is explicitly called out as incompatible in `CHANGELOG.md`
- all related docs are updated in the same change
- affected tests and benchmarks are updated together

Silent semantic drift is not allowed, even in `v0.x`.

## Transition criteria for `v1.0`

Graphion should not claim `v1.0` ISA stability until all of the following are
true:

- golden ISA conformance fixtures exist
- structured VM error model is documented
- overflow / checked arithmetic policy is documented
- deterministic execution policy is documented
- opcode table and encoding are treated as frozen

## Source of truth

Until `v1.0`, the normative sources are:

- `docs/ISA.md` for opcode and encoding definition
- this document for versioning and compatibility policy
- `docs/IR.md` for frontend-to-bytecode bridge expectations
