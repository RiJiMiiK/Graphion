# VM ISA

See also:

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [ERRORS.md](../debugging/ERRORS.md)

## Scope

This page documents the VM instruction set that exists in the repo today.

Two things matter when reading it:

- the VM contains more capability than the currently documented `.gion` scalar subset actively uses
- the presence of an opcode here does not automatically mean the current `.gion` frontend exposes it directly

## Encoding

Instruction encoding is fixed to 7 bytes:

1. `op` (1 byte)
2. `a` (1 byte)
3. `b` (1 byte)
4. `imm` (4 bytes, little-endian signed int32)

## Register model

- 16 general-purpose registers: `r0..r15`
- register width:
  - integer path: signed 64-bit
  - value/register runtime path: `graphion_vm_value` for the current scalar-language execution model

## Active scalar-language opcode families

These opcode families matter directly for the currently documented `.gion` scalar subset.

### Core control / movement

- `GVM_OP_NOP`
- `GVM_OP_HALT`
- `GVM_OP_MOV_IMM`
- `GVM_OP_MOV`
- `GVM_OP_LOAD_CONST`
- `GVM_OP_LOAD_GLOBAL`
- `GVM_OP_STORE_GLOBAL`
- `GVM_OP_STORE_CONST_GLOBAL`
- `GVM_OP_COPY_GLOBAL`

### Printing

- `GVM_OP_PRINT_CONST`
- `GVM_OP_PRINT_GLOBAL`
- `GVM_OP_PRINT_REG`
- `GVM_OP_PRINT_CONST_PART`
- `GVM_OP_PRINT_GLOBAL_PART`
- `GVM_OP_PRINT_REG_PART`
- `GVM_OP_PRINT_NEWLINE`

These support both ordinary printing and the current print-only string coercion behavior.

### Arithmetic

- `GVM_OP_ADD`
- `GVM_OP_SUB`
- `GVM_OP_MUL`
- `GVM_OP_DIV`
- `GVM_OP_FLOOR_DIV`
- `GVM_OP_MOD`
- `GVM_OP_POW`
- `GVM_OP_ABS`
- `GVM_OP_MIN`
- `GVM_OP_MAX`
- `GVM_OP_CLAMP`
- `GVM_OP_SQRT`
- `GVM_OP_CBRT`
- `GVM_OP_SIN`
- `GVM_OP_SINH`
- `GVM_OP_COSH`
- `GVM_OP_TANH`
- `GVM_OP_COS`
- `GVM_OP_TAN`
- `GVM_OP_ASIN`
- `GVM_OP_ACOS`
- `GVM_OP_ATAN`
- `GVM_OP_ATAN2`
- `GVM_OP_HYPOT`
- `GVM_OP_COPYSIGN`
- `GVM_OP_DEGREES`
- `GVM_OP_RADIANS`
- `GVM_OP_ISNAN`
- `GVM_OP_ISINF`
- `GVM_OP_ISFINITE`
- `GVM_OP_EXPM1`
- `GVM_OP_LOG1P`
- `GVM_OP_ERF`
- `GVM_OP_ERFC`
- `GVM_OP_EXP`
- `GVM_OP_LN`
- `GVM_OP_LOG`
- `GVM_OP_FLOOR`
- `GVM_OP_CEIL`
- `GVM_OP_ROUND`
- `GVM_OP_TRUNC`
- `GVM_OP_FRACT`
- `GVM_OP_SIGN`
- `GVM_OP_LEN`
- `GVM_OP_FACTORIAL`

These back the current `.gion` scalar language features:

- `+`
- `-`
- `*`
- `/`
- `//`
- `%`
- `**`
- postfix `!`
- `abs(...)`
- `min(a, b)`
- `max(a, b)`
- `clamp(x, lo, hi)`
- `sqrt(x)`
- `cbrt(x)`
- `sin(x)`
- `sinh(x)`
- `asinh(x)`
- `acosh(x)`
- `cosh(x)`
- `tanh(x)`
- `atanh(x)`
- `cos(x)`
- `tan(x)`
- `asin(x)`
- `acos(x)`
- `atan(x)`
- `atan2(y, x)`
- `hypot(x, y)`
- `copysign(x, y)`
- `degrees(x)`
- `radians(x)`
- `isnan(x)`
- `isinf(x)`
- `isfinite(x)`
- `expm1(x)`
- `log1p(x)`
- `erf(x)`
- `erfc(x)`
- `exp(x)`
- `ln(x)`
- `log(x, base)`
- `log10(x)`
- `log2(x)`
- `floor(x)`
- `ceil(x)`
- `round(x)`
- `trunc(x)`
- `fract(x)`
- `sign(x)`
- `len(x)`

## Other VM opcode families present in the repo

The VM also contains broader graph/hypergraph and frontier-oriented families:

- frontier primitives
- CSR neighbor traversal
- weighted graph helpers
- hypergraph traversal helpers
- BFS / incidence-oriented graph operations

These remain part of the broader VM surface, but they should be read separately from the current `.gion` scalar-language rebuild.

## Error behavior

Current VM-visible result codes are defined in:

- `src/vm/vm.h`

High-level error documentation lives in:

- [ERRORS.md](../debugging/ERRORS.md)

Important currently visible VM codes include:

- `GVM_ERR_INVALID_ARG`
- `GVM_ERR_INVALID_MOV_IMM_REG`
- `GVM_ERR_INVALID_REG`
- `GVM_ERR_UNKNOWN_OPCODE`
- `GVM_ERR_TYPE_MISMATCH`
- `GVM_ERR_CONST_UNBOUND`
- `GVM_ERR_GLOBALS_UNBOUND`
- `GVM_ERR_INVALID_CONST_INDEX`
- `GVM_ERR_INVALID_GLOBAL_INDEX`
- `GVM_ERR_OUTPUT_UNBOUND`
- `GVM_ERR_DIVIDE_BY_ZERO`
- `GVM_ERR_DOMAIN`
- `GVM_ERR_ASIN_DOMAIN`
- `GVM_ERR_ACOS_DOMAIN`
- `GVM_ERR_ACOSH_DOMAIN`
- `GVM_ERR_ATANH_DOMAIN`
- `GVM_ERR_LOG1P_DOMAIN`

Graph/frontier-specific errors also exist for the broader VM surface.

## Deterministic execution

The VM exposes:

- `graphion_vm_set_deterministic(vm, true)`

Deterministic mode keeps the VM on the portable path for reproducible debugging.

## Compatibility

Current status:

- this is still `v0.x`
- bytecode compatibility is not yet promised across all revisions

When VM-visible semantics change, update:

- this page
- [ERRORS.md](../debugging/ERRORS.md) when error behavior changes
- tests that assert opcode behavior
