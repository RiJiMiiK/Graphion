# Structured VM Error Model

## Scope

This document defines the current Graphion error-model structure across:

- source frontend parsing
- IR lowering
- bytecode decoding
- VM load / execute path

The current implementation uses signed integer return codes, with `0` meaning
success and negative values meaning failure.

## Model overview

Graphion currently uses a layer-scoped error model rather than a single global
cross-subsystem enum.

That means:

- each subsystem has its own negative error-code space
- the same numeric value may mean different things in different subsystems
- the caller must interpret the code in the context of the API that returned it

This is acceptable for the current `v0.x` stage because APIs are still narrow
and explicit. It must be documented rigorously to avoid silent ambiguity.

## Success convention

- `0`: success

No positive return codes are currently used for warnings or partial success.

## Frontend parse errors

API:

- `graphion_parse_source_to_ir(...)`

Header:

- `src/parser/frontend.h`

Codes:

| Code | Symbol | Meaning |
| --- | --- | --- |
| `0` | `GFE_OK` | Success |
| `-1` | `GFE_ERR_INVALID_ARG` | Null pointer or otherwise invalid API argument |
| `-2` | `GFE_ERR_CAPACITY` | Output IR buffer is too small |
| `-3` | `GFE_ERR_PARSE` | Source text does not match accepted syntax |

## IR lowering errors

API:

- `graphion_ir_lower_to_bytecode(...)`

Header:

- `src/compiler/ir.h`

Codes:

| Code | Symbol | Meaning |
| --- | --- | --- |
| `0` | `GIR_OK` | Success |
| `-1` | `GIR_ERR_INVALID_ARG` | Null pointer or otherwise invalid API argument |
| `-2` | `GIR_ERR_CAPACITY` | Output program buffer is too small |
| `-3` | `GIR_ERR_INVALID_OPCODE` | IR opcode is not valid for the current lowering set |

## Bytecode decode errors

API:

- `graphion_decode_bytecode(...)`

Header:

- `src/parser/bytecode.h`

Codes:

| Code | Symbol | Meaning |
| --- | --- | --- |
| `0` | `GBC_OK` | Success |
| `-1` | `GBC_ERR_INVALID_ARG` | Null pointer or otherwise invalid API argument |
| `-2` | `GBC_ERR_TRUNCATED` | Input byte stream length is not a whole number of instructions |
| `-3` | `GBC_ERR_CAPACITY` | Output instruction buffer is too small |

## VM runtime errors

APIs:

- `graphion_vm_load(...)`
- `graphion_vm_run(...)`

Implementation:

- `src/vm/vm.c`

Current `v0.x` status:

- VM errors are structured by operation family
- they are not yet exposed as a public named enum in `src/vm/vm.h`
- numeric stability is therefore still `v0.x` / best-effort, not frozen

### Generic VM codes

| Code | Current meaning |
| --- | --- |
| `0` | Success |
| `-1` | Invalid VM/program argument for load or run entry points |
| `-4` | Unknown opcode during execution |

### Arithmetic opcode codes

| Code | Current meaning |
| --- | --- |
| `-2` | Invalid register in `MOV_IMM` |
| `-3` | Invalid register operands for register-based VM opcodes |

Arithmetic overflow currently does not raise a dedicated VM error:

- `MOV_IMM` is exact for its `int32 -> int64` conversion
- `ADD` uses explicit two's-complement wraparound semantics

### Graph VM codes

| Code | Current meaning |
| --- | --- |
| `-5` | Required CSR/BFS runtime resources are not bound |
| `-6` | Invalid BFS source register value or out-of-range node id |
| `-7` | CSR BFS kernel returned a runtime failure |

### Hypergraph VM codes

| Code | Current meaning |
| --- | --- |
| `-8` | Required hypergraph runtime object is not bound |
| `-9` | Invalid node id for node-to-hyperedge operations |
| `-10` | Invalid hyperedge id for hyperedge-to-node operations |

## Interpretation rule

Callers must interpret VM runtime codes by both:

- API surface (`graphion_vm_load` vs `graphion_vm_run`)
- opcode family involved at the failure site

Example:

- `-3` means "invalid register operands" in the VM
- but `-3` means `GFE_ERR_PARSE` in the frontend
- and `-3` means `GBC_ERR_CAPACITY` in bytecode decoding

This is intentional in `v0.x`, but must not be treated as a single global code
space.

## Stability policy

### `v0.x`

Current behavior:

- subsystem-local error codes are allowed
- numeric assignments may still change
- new subsystem-specific error codes may still be added

Any incompatible change must update:

- this document
- `docs/ISA.md` when VM-visible behavior changes
- `docs/IR.md` when frontend/IR contracts change
- tests that assert specific codes
- `CHANGELOG.md`

### `v1.0`

Planned hardening target:

- public named VM error codes for runtime/load path
- frozen numeric assignments for VM-visible execution errors
- explicit distinction between decode/load/execute failure classes
- compatibility guarantees documented alongside ISA `v1.0`

## Design rules for new errors

Before `v1.0`, new errors should follow these rules:

- prefer subsystem-local symbols over undocumented raw integers
- do not reuse an existing code for a new meaning within the same subsystem
- document the failure trigger and expected caller action
- add tests when the code is part of externally observable behavior

## Near-term hardening follow-up

This document is the policy layer.

The next hardening steps are:

- golden ISA conformance fixtures
- deterministic execution policy
- overflow / checked arithmetic policy
- public VM error-code naming in headers when runtime semantics are frozen
