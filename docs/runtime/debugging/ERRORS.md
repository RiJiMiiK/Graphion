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

Source diagnostics carry a source position:

- `line` is one-based and points to the physical `.gion` source line
- `column` is one-based and points to the most useful token or boundary for the error
- CLI errors are printed as `error:line:column: message`

Column precision is best-effort but intentionally user-facing. Common syntax and runtime diagnostics now point at the relevant assignment operator, missing identifier, delimiter, builtin argument, control header, block boundary, graph/hypergraph body item, literal token, or remaining expression token instead of defaulting to column 1. Diagnostics that describe whole-line failures, resource limits, or internal argument failures may still use column 1.

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
- `expected print argument`
- `expected ')' after print argument`
- `expected abs argument`
- `expected min first argument`
- `expected match case literal`
- `expected match case or default`
- `expected expression after '='`
- `expected expression after '+'`
- `expected expression after '+='`
- `expected expression before '!'`
- `expected expression before '=='`
- `expected ')' after expression`
- `unexpected trailing tokens after print`
- `unexpected trailing tokens after assignment`
- `unexpected trailing tokens after expression`
- `multiline condition requires grouping parentheses`

Malformed `print(...)` calls report their syntax error before name resolution. For example, `print(missing` reports the missing `)` instead of `unknown operand 'missing'`, while a complete call such as `print(missing)` reports the unknown operand.

Embedded expressions in graph and hypergraph declaration bodies follow the same rule. A malformed attribute dictionary or grouped edge-weight expression reports its delimiter/trailing-comma error before an unresolved value inside it; a syntactically complete expression still reports the unresolved operand.

### 2. Unknown name errors

These are raised when source evaluation references a name that does not exist.

Current distinction:

- `unknown variable 'count'`
  - typically used when the target of a mutation-style operation does not exist yet
  - example: `count += 1` when `count` is not defined
- `unknown graph variable 'G'`
  - used when a graph mutation statement references a graph variable that does not exist
- `unknown hypergraph variable 'H'`
  - used when a hypergraph mutation statement references a hypergraph variable that does not exist
- `unknown operand 'missing'`
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
- a graph node or hypergraph vertex expression that does not evaluate to `int` or `string`
- a graph edge `weight` value that is not numeric
- an empty hyperedge or non-dictionary hyperedge attributes
- a graph/hypergraph body whose evaluated attributes violate its declared schema
- a graph body whose evaluated edges violate its orientation or node-id constraints
- a `struct` field default whose value has the wrong declared type

Typical messages include:

- `division by zero`
- `incompatible operand types`
- `sqrt requires non-negative input`
- `asin requires input in [-1, 1]`
- `acos requires input in [-1, 1]`
- `acosh requires input >= 1`
- `atanh requires input in (-1, 1)`
- `log1p requires input > -1`
- `gamma is undefined at 0 and negative integers`
- `lgamma is undefined at 0 and negative integers`
- `ln requires strictly positive input`
- `log requires x > 0 and base > 0 with base != 1`
- `factorial requires non-negative integer input`
- `graph node variable must be int or string`
- `graph edge weight must be int or float`
- `hyperedge must contain at least one vertex`
- `hypergraph hyperedge attributes must be a dict literal`
- `graph node attributes must use declared default keys`
- `directed graph cannot use undirected '-' edges`
- `struct field default has wrong type`

Graph, hypergraph, and struct declaration bodies may evaluate values while building the declared object. Missing delimiters, malformed body entries, malformed headers, and malformed field syntax remain parse failures. Once declaration input has valid syntax, rejected value types, duplicate graph data, or graph/hypergraph object invariants are runtime failures.

## Current `.gion` behavior examples

### Parse error

Input:

```gion
value = 1 + * 2
```

Typical result:

- parse failure
- message `expected expression after '+'`
- position on the `+` operator

Input:

```gion
value = == 1
```

Typical result:

- parse failure
- message `expected expression before '=='`
- position on the `==` operator

### Unknown operand

Input:

```gion
value = missing
```

Typical result:

- source/runtime failure
- message `unknown operand 'missing'`
- position on `missing`

### Unknown variable

Input:

```gion
count += 1
```

Typical result:

- source/runtime failure
- message `unknown variable 'count'`
- position on `count`

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

## Textual IR frontend errors

These codes belong to the small textual IR/assembly parser used by VM-facing tests and tooling. Despite the `frontend` filename, this is not the user-facing `.gion` parser.

API:

- `graphion_parse_source_to_ir(...)`

Header:

- `src/parser/frontend.h`

Codes:

| Code | Symbol | Meaning |
| --- | --- | --- |
| `0` | `GFE_OK` | Success |
| `-1` | `GFE_ERR_INVALID_ARG` | Null pointer or invalid API argument |
| `-2` | `GFE_ERR_CAPACITY` | Output IR buffer is too small |
| `-3` | `GFE_ERR_PARSE` | Invalid textual IR instruction or operands |

Decision for `v0.x`: `GFE_*` remains a code-only result family and does not gain a line/column diagnostic object. It has no `.gion` CLI or runtime entry path today, while `.gion` errors already use `graphion_runtime_diagnostic`. Revisit this decision only if textual IR becomes a supported user-facing input format or a tool needs detailed source feedback.

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

Decision for `v0.x`: `GBC_*` remains a code-only result family and does not gain user-facing message text or a source diagnostic object. The decoder is used by VM-facing tests/tooling and has no bytecode CLI input path today. Revisit this decision if Graphion later exposes bytecode loading, validation, disassembly, or editor diagnostics as a supported user workflow.

## VM runtime errors

APIs:

- `graphion_vm_load(...)`
- `graphion_vm_run(...)`

Implementation:

- `src/vm/vm.c`

Current `v0.x` status:

- VM errors are exposed as named results in `src/vm/vm.h`
- numeric stability is still best-effort, not frozen
- VM errors without a `.gion`-specific message surface as `unmapped VM runtime error: GVM_ERR_*`

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
