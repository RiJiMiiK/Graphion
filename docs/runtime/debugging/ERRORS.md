# Errors

## Scope

This document describes the current user-visible error model around:

- `.gion` source parsing
- source-level runtime execution
- VM execution

It focuses on the behavior that matters today for the actively supported scalar/arithmetic subset.

See also:

- [ISA.md](../core/ISA.md) for VM-visible instruction semantics

## High-level split

Graphion does not currently use one giant global error enum for everything.

Instead, errors are still grouped by subsystem:

- source/runtime errors for `.gion`
- bytecode decode errors for VM tooling
- VM result codes for VM execution

That means the same numeric value may mean different things in different APIs.

## Success convention

- `0` means success
- negative values mean failure

No positive warning codes are currently used.

## `.gion` source/runtime errors

Current source execution has three broad families of failures.

### 1. Parse errors

These are raised when the source form itself is invalid or unsupported.

Examples:

- malformed assignment
- malformed `print(...)`
- missing operand
- unmatched parenthesis
- unsupported expression form

Typical messages include:

- `expected '='`
- `expected scalar literal`
- `expected ')' after expression`
- `unsupported assignment expression`

### 2. Unknown name errors

These are raised when source evaluation references a name that does not exist.

Current distinction:

- `unknown variable`
  - typically used when the target of a mutation-style operation does not exist yet
  - example: `count += 1` when `count` is not defined
- `unknown operand`
  - used when an expression references a missing value
  - examples:
    - `copy = missing`
    - `print(missing)`

### 3. Runtime arithmetic/type errors

These are raised when the syntax is valid but execution is not.

Examples:

- division by zero
- floor division by zero
- modulo by zero
- arithmetic with non-numeric operands
- invalid mixed operation outside the allowed `print(...)` coercion behavior

Typical messages include:

- `division by zero`
- `incompatible operand types`
- `sqrt requires non-negative input`
- `ln requires strictly positive input`
- `log requires x > 0 and base > 0 with base != 1`
- `log10 requires strictly positive input`
- `factorial requires non-negative integer input`

## Current `.gion` behavior examples

### Parse error

Input:

```gion
value = 1 + * 2
```

Typical result:

- parse failure
- message similar to `expected scalar literal`

### Unknown operand

Input:

```gion
value = missing
```

Typical result:

- source/runtime failure
- message `unknown operand`

### Unknown variable

Input:

```gion
count += 1
```

Typical result:

- source/runtime failure
- message `unknown variable`

### Runtime arithmetic error

Input:

```gion
value = "x" + 1
```

Typical result:

- runtime failure
- message `incompatible operand types`

Input:

```gion
value = (-1)!
```

Typical result:

- runtime failure
- message `factorial requires non-negative integer input`
- message `ln requires strictly positive input`
- message `log requires x > 0 and base > 0 with base != 1`

## Bytecode decode errors

These matter mainly for VM-oriented tooling and tests.

API:

- `graphion_decode_bytecode(...)`

Header:

- `src/parser/bytecode.h`

Codes:

| Code | Symbol | Meaning |
| --- | --- | --- |
| `0` | `GBC_OK` | Success |
| `-1` | `GBC_ERR_INVALID_ARG` | Null pointer or invalid API argument |
| `-2` | `GBC_ERR_TRUNCATED` | Input is not a whole number of instructions |
| `-3` | `GBC_ERR_CAPACITY` | Output instruction buffer is too small |

## VM runtime errors

APIs:

- `graphion_vm_load(...)`
- `graphion_vm_run(...)`

Implementation:

- `src/vm/vm.c`

Current `v0.x` status:

- VM errors are exposed as named results in `src/vm/vm.h`
- numeric stability is still best-effort, not frozen

### Generic VM codes

| Code | Symbol | Meaning |
| --- | --- | --- |
| `0` | `GVM_OK` | Success |
| `-1` | `GVM_ERR_INVALID_ARG` | Invalid VM/program argument |
| `-4` | `GVM_ERR_UNKNOWN_OPCODE` | Unknown opcode during execution |

### Register/arithmetic family

| Code | Symbol | Meaning |
| --- | --- | --- |
| `-2` | `GVM_ERR_INVALID_MOV_IMM_REG` | Invalid register in `MOV_IMM` |
| `-3` | `GVM_ERR_INVALID_REG` | Invalid register operands |

Current arithmetic notes:

- `MOV_IMM` is exact for `int32 -> int64`
- `ADD` uses two's-complement wraparound semantics

### Graph family

| Code | Symbol | Meaning |
| --- | --- | --- |
| `-5` | `GVM_ERR_CSR_UNBOUND` | Required CSR/BFS runtime resources are not bound |
| `-6` | `GVM_ERR_INVALID_BFS_SOURCE` | Invalid BFS source register value or out-of-range node id |
| `-7` | `GVM_ERR_BFS_RUNTIME` | CSR BFS kernel returned a runtime failure |

### Hypergraph family

| Code | Symbol | Meaning |
| --- | --- | --- |
| `-8` | `GVM_ERR_HYPERGRAPH_UNBOUND` | Required hypergraph runtime object is not bound |
| `-9` | `GVM_ERR_INVALID_NODE_ID` | Invalid node id for node-to-hyperedge operations |
| `-10` | `GVM_ERR_INVALID_HYPEREDGE_ID` | Invalid hyperedge id for hyperedge-to-node operations |

### Frontier family

| Code | Symbol | Meaning |
| --- | --- | --- |
| `-11` | `GVM_ERR_FRONTIER_UNBOUND` | Frontier input/output buffers are not bound |
| `-12` | `GVM_ERR_FRONTIER_OVERFLOW` | Frontier output would exceed configured capacity |
| `-13` | `GVM_ERR_INVALID_FRONTIER_VALUE` | Frontier value or mapped result violated the documented range contract |

## Interpretation rule

Always interpret an error in the context of the API that returned it.

Example:

- `-3` can mean a parse failure in one subsystem
- and an invalid register error in another

So callers should not treat the current project as if it already had one fully unified error-code namespace.

## Stability policy

### `v0.x`

Current behavior:

- subsystem-local codes are acceptable
- numeric assignments may still evolve
- externally visible changes should still be documented and tested

When behavior changes, update:

- this document
- [ISA.md](../core/ISA.md) when VM-visible behavior changes
- tests that assert codes or messages
- `CHANGELOG.md`

### `v1.0`

Hardening goal:

- freeze the VM-visible numeric assignments that matter publicly
- make decode/load/execute classes more explicit
- keep the user-visible `.gion` error model clear and documented
