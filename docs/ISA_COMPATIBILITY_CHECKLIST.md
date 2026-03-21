# VM/ISA Compatibility Checklist

## Goal

Provide a mandatory checklist for any change that adds, removes, renumbers, or changes the semantics of a VM opcode.

This checklist is intended for:

- PR authors
- reviewers
- roadmap hardening work

It is not optional for externally observable ISA behavior.

## When To Use It

Run this checklist whenever a change affects one or more of:

- opcode numeric assignment
- operand layout or meaning
- encoding or decoding behavior
- runtime return codes
- halted / `pc` / register outcomes
- deterministic-mode behavior
- asm-vs-C parity expectations

## Checklist

### 1. Opcode contract

- [ ] Opcode number is documented in `docs/ISA.md`
- [ ] Operand meaning is documented in `docs/ISA.md`
- [ ] success behavior is documented
- [ ] failure behavior is documented
- [ ] overflow or exactness rules are documented if arithmetic is involved

### 2. Compatibility classification

- [ ] Change is classified as one of:
  - documentation-only clarification
  - backward-compatible extension
  - backward-incompatible pre-`v1.0` change
  - future version-gated change
- [ ] `docs/ISA_VERSIONING.md` remains accurate after the change
- [ ] `CHANGELOG.md` calls out incompatible semantic changes

### 3. Decode / encode impact

- [ ] `src/parser/bytecode.*` was reviewed for impact
- [ ] decode behavior still matches the documented encoding
- [ ] decode failure behavior was reviewed when encoding changed

### 4. Runtime behavior

- [ ] `src/vm/vm.c` implements the documented semantics
- [ ] deterministic mode still matches the portable semantic reference
- [ ] `halted`, `pc`, and final registers remain documented and tested
- [ ] snapshot output remains sufficient to debug the new or changed behavior

### 5. Error behavior

- [ ] `docs/VM_ERRORS.md` is updated if any VM-visible error behavior changed
- [ ] named VM result codes are reused only for the same meaning
- [ ] new VM-visible errors are documented before use

### 6. Fixtures and tests

- [ ] `tests/test_isa.c` was updated if decode or execute semantics changed
- [ ] unit tests were added or updated for the changed behavior
- [ ] deterministic-mode tests were reviewed if execution semantics changed
- [ ] asm parity coverage was reviewed if the arithmetic hotpath or deterministic semantics changed

### 7. Repro and debugging

- [ ] `docs/VM_REPRO.md` remains accurate
- [ ] `docs/REPRO_ARTIFACTS.md` remains accurate if the repro bundle changed
- [ ] the issue can be reduced to a fixture, inline program, or byte payload

### 8. Frontend / IR bridge

- [ ] `docs/IR.md` was reviewed if frontend lowering is affected
- [ ] parser or IR tests were updated if source-level behavior changed

## Reviewer Shortcut

For review, the minimum acceptable evidence is:

1. updated opcode documentation
2. updated tests or fixtures
3. updated error or compatibility docs when behavior changed
4. deterministic and parity implications explicitly noted

If one of those is missing, the opcode change is incomplete.

## Relationship With Other Docs

- `docs/ISA.md`
- `docs/ISA_VERSIONING.md`
- `docs/ISA_FIXTURES.md`
- `docs/VM_ERRORS.md`
- `docs/VM_REPRO.md`
- `docs/REPRO_ARTIFACTS.md`
