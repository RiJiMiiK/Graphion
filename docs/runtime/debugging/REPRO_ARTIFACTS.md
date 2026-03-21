# Repro Artifact Policy

## Goal

Standardize the artifact set attached to VM/ISA bug reports and CI failures so that deterministic reproduction is possible without reinterpreting ad hoc logs.

## Scope

This policy applies to:

- local bug reports
- PR investigation notes
- CI failures involving VM, ISA, deterministic mode, dispatch parity, or asm parity

It does not define benchmark artifact policy. Benchmark outputs remain governed by the benchmark and optimization-report policies.

## Required Artifact Names

The preferred artifact set for a deterministic VM/ISA repro is:

- `fixture.txt`
- `expected.txt`
- `actual.txt`
- `vm_snapshot.txt`
- `environment.json`

If the repro is decode-specific, add:

- `bytecode.bin`

If the repro uses an inline VM program instead of a fixture name, use:

- `program.txt`

If the repro compares execution modes, add one or more of:

- `snapshot.c.txt`
- `snapshot.deterministic.txt`
- `snapshot.asm.txt`
- `snapshot.dispatch-switch.txt`
- `snapshot.dispatch-jumptable.txt`
- `snapshot.dispatch-computed-goto.txt`

## Artifact Contents

### `fixture.txt`

Contains one of:

- the fixture name from `tests/test_isa.c`
- the targeted unit-test name
- a short repro identifier if no fixture exists yet

### `expected.txt`

Must capture the expected externally visible behavior:

- return code
- halted state
- final `pc`
- relevant register expectations
- expected decode/load/execute outcome

### `actual.txt`

Must capture the observed behavior in the same shape as `expected.txt`.

### `vm_snapshot.txt`

Must contain the exact output of:

- `graphion_vm_write_snapshot(...)`

The snapshot format version header must be preserved.

### `environment.json`

Must include at minimum:

- `git_rev`
- `platform`
- `compiler`
- `build_type`
- `dispatch`
- `deterministic_mode`
- `graphion_enable_asm`

Optional but recommended:

- `ci_job`
- `docker_image`
- `generator`
- `notes`

## Naming Rules

- artifact names are lowercase
- words are separated with `_`
- mode-specific snapshots use suffixes, not prefixes
- avoid issue-specific or temporary filenames like `test2.txt`, `weird_bug.txt`, or `final_final.txt`

## CI Failure Guidance

For CI failures in VM/ISA-oriented jobs, the preferred artifact subset is:

- `actual.txt`
- `vm_snapshot.txt`
- `environment.json`

If the CI failure came from asm or dispatch parity, add the mode-specific snapshot variant that demonstrates the divergence.

## Bug Report Guidance

For user-facing or maintainer-facing bug reports, attach:

- the smallest fixture or program representation possible
- the exact snapshot
- one environment file

Do not attach:

- large benchmark dumps when the issue is semantic
- unrelated logs
- screenshots instead of textual artifacts

## Relationship With Other Docs

- `docs/runtime/debugging/VM_REPRO.md`: overall deterministic repro workflow
- `docs/runtime/debugging/VM_SNAPSHOT.md`: snapshot format
- `docs/runtime/contracts/ISA_FIXTURES.md`: fixture structure
- `docs/runtime/debugging/VM_ERRORS.md`: return-code interpretation
