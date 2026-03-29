# Architecture

> Current language-runtime reconstruction is governed by [REBUILD_CHARTER.md](REBUILD_CHARTER.md).

## Scope

Graphion currently exposes a small but real `.gion` language surface on top of a C runtime and a VM.

At this stage, the important architectural truth is:

- user programs enter as `.gion`
- supported source is lowered toward VM execution
- the VM remains the execution backend

The project still contains older graph- and hypergraph-oriented runtime pieces, but they should not be read as proof that the current `.gion` language already exposes that whole surface.

## Runtime layers

- `src/runtime/entry.*`
  - file entrypoint for `.gion`
  - validates input path and reads source text
- `src/runtime/interpreter.*`
  - current source-level runtime/orchestration layer
  - parses the supported `.gion` subset and prepares execution
- `src/vm/vm.*`
  - VM implementation and opcode semantics
- `src/parser/bytecode.*`
  - fixed-width bytecode decoding for VM-oriented tests and tooling
- `src/runtime/arena.*`
  - temporary allocation support used by the runtime

## Current `.gion` language surface

The currently supported user-facing subset is centered on scalar values and expressions:

- assignment
- variable reuse
- `print(...)`
- arithmetic expressions
- grouped expressions with parentheses
- compound assignments
- builtin `abs(...)`

### Scalar values

Supported scalar values are:

- `int`
- `float`
- `bool`
- `string`

### Arithmetic

Currently supported arithmetic operators:

- `+`
- `-`
- `*`
- `/`
- `//`
- `%`
- `**`

Currently supported compound assignments:

- `+=`
- `-=`
- `*=`
- `/=`
- `//=`
- `%=`
- `**=`

### String behavior

- `string + string` performs concatenation
- mixed string coercion such as `"Test " + 7` is currently supported only inside `print(...)`
- outside `print(...)`, mixed arithmetic/string operands remain runtime errors

## Execution model

The active design target is a single execution pipeline:

- `source Graphion -> tokens/parsing -> representation interne du code -> bytecode -> VM`

The implementation is still being rebuilt toward that target, but the project should be evaluated against that direction rather than against historical intermediate structures.

Two rules matter here:

- no alternate semantic engine should silently take over when a form is unsupported
- unsupported language forms should fail with a clear error

## VM model

The VM remains the execution backend and the main performance anchor.

Current VM characteristics:

- fixed instruction encoding documented in [ISA.md](ISA.md)
- explicit VM result codes exposed in `src/vm/vm.h`
- deterministic mode available through `graphion_vm_set_deterministic(vm, true)`

The VM also contains older graph/hypergraph opcodes and runtime bindings. Those remain part of the broader project, but they should be treated separately from the currently documented `.gion` scalar-language surface.

## Error model

Runtime-visible error behavior is documented in:

- [ERRORS.md](../debugging/ERRORS.md)

At a high level, the current user-visible split is:

- parse errors for unsupported/invalid source forms
- unknown variable / unknown operand diagnostics
- runtime errors for invalid arithmetic at execution time

## Benchmarks

Performance tracking is documented in:

- [BENCHMARKS.md](../../performance/guides/BENCHMARKS.md)
- [PERFORMANCE_RESULTS.md](../../performance/reports/PERFORMANCE_RESULTS.md)

The important architectural benchmark distinction is:

- VM performance is validated on VM-oriented lanes
- `.gion` performance is validated separately on source-level lanes

That separation matters because the VM can already be good while the source frontend still has significant work left.
