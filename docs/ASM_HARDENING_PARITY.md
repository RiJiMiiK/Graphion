# ASM Hardening Parity Report

Generated: 2026-03-21T12:11:35.903860+00:00

This report covers hardening-sensitive ISA cases on both the portable C path and the asm-enabled path.

## Metadata

- Platform: linux
- Config: Release
- Build type: Release
- Tests: vm_addition_program, vm_invalid_register_fails, vm_deterministic_mode_toggle, vm_deterministic_mode_unknown_opcode, vm_add_wraparound_semantics, isa_execute_golden_fixtures

## Results

| Test | C fallback | ASM |
|---|---|---|
| vm_addition_program | pass | pass |
| vm_invalid_register_fails | pass | pass |
| vm_deterministic_mode_toggle | pass | pass |
| vm_deterministic_mode_unknown_opcode | pass | pass |
| vm_add_wraparound_semantics | pass | pass |
| isa_execute_golden_fixtures | pass | pass |

## Policy

- Hardening-sensitive ISA cases must pass unchanged with `GRAPHION_ENABLE_ASM=OFF` and `GRAPHION_ENABLE_ASM=ON`.
- Deterministic mode remains the portable semantic reference path even when asm is compiled in.
- Wraparound, invalid-register handling, and unknown-opcode behavior must remain identical across both builds.
