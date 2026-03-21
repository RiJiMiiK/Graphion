# Decode / Load / Execute Failure Classification

## Goal

Provide a single debugging table that classifies failures by phase:

- decode
- load
- execute

This is intended to reduce ambiguity during:

- test failures
- bug triage
- deterministic repro work
- opcode review

## Phase Definitions

### Decode

Transforms byte-oriented input into `graphion_insn[]`.

Primary API:

- `graphion_decode_bytecode(...)`

Typical symptoms:

- malformed byte stream
- truncation
- output-capacity failure

### Load

Binds a decoded or inline program to a VM instance and computes static execution metadata.

Primary API:

- `graphion_vm_load(...)`

Typical symptoms:

- invalid API argument
- invalid load entry conditions

### Execute

Runs the loaded program and applies opcode semantics.

Primary API:

- `graphion_vm_run(...)`

Typical symptoms:

- invalid register usage
- unknown opcode
- missing graph bindings
- invalid graph or hypergraph ids
- runtime kernel failure

## Classification Table

| Phase | Primary API | Success code | Failure families | Typical artifacts |
| --- | --- | --- | --- | --- |
| Decode | `graphion_decode_bytecode(...)` | `GBC_OK` | invalid arg, truncated input, capacity | `bytecode.bin`, `expected.txt`, `actual.txt` |
| Load | `graphion_vm_load(...)` | `GVM_OK` | invalid arg | `program.txt`, `actual.txt`, `environment.json` |
| Execute | `graphion_vm_run(...)` | `GVM_OK` | invalid register, unknown opcode, missing bindings, invalid ids, runtime kernel failure | `vm_snapshot.txt`, `actual.txt`, mode-specific snapshots |

## Current Codes By Phase

### Decode

| Symbol | Meaning |
| --- | --- |
| `GBC_ERR_INVALID_ARG` | invalid decode API argument |
| `GBC_ERR_TRUNCATED` | byte stream is not a whole number of instructions |
| `GBC_ERR_CAPACITY` | output buffer is too small |

### Load

| Symbol | Meaning |
| --- | --- |
| `GVM_ERR_INVALID_ARG` | invalid VM pointer, program pointer, or program length |

### Execute

| Symbol | Meaning |
| --- | --- |
| `GVM_ERR_INVALID_ARG` | invalid VM state at run entry |
| `GVM_ERR_INVALID_MOV_IMM_REG` | invalid destination register in `MOV_IMM` |
| `GVM_ERR_INVALID_REG` | invalid register operands for register-based opcodes |
| `GVM_ERR_UNKNOWN_OPCODE` | unknown opcode encountered during execution |
| `GVM_ERR_CSR_UNBOUND` | CSR/BFS runtime state missing |
| `GVM_ERR_INVALID_BFS_SOURCE` | invalid BFS source id |
| `GVM_ERR_BFS_RUNTIME` | graph BFS kernel failed |
| `GVM_ERR_HYPERGRAPH_UNBOUND` | hypergraph runtime state missing |
| `GVM_ERR_INVALID_NODE_ID` | invalid node id |
| `GVM_ERR_INVALID_HYPEREDGE_ID` | invalid hyperedge id |

## Triage Guidance

### If decode fails

First inspect:

- byte payload length
- instruction framing
- output capacity

Do not start from VM snapshot work if decode already failed.

### If load fails

First inspect:

- null / invalid VM pointer use
- null or empty program input
- whether the repro is actually a load problem rather than an execute problem

Load failures happen before opcode semantics matter.

### If execute fails

First inspect:

- opcode under test
- register operands
- deterministic-mode result
- required graph/hypergraph bindings
- VM snapshot

If the failure changes across dispatch or asm modes:

- reduce to deterministic mode first
- then compare against the portable C path

## Testing Guidance

For new or changed behavior:

- decode-only failures belong in decode fixtures or parser/decode tests
- load failures belong in VM load tests
- execute failures belong in execute fixtures, unit tests, and parity workflows

Avoid mixing phases in a single bug description unless the reduction proves the earlier phase is clean.

## Relationship With Other Docs

- `docs/runtime/debugging/VM_ERRORS.md`
- `docs/runtime/debugging/VM_REPRO.md`
- `docs/runtime/debugging/REPRO_ARTIFACTS.md`
- `docs/runtime/contracts/ISA_FIXTURES.md`
