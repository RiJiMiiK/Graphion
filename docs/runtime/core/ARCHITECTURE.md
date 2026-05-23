# Architecture

> Current language-runtime reconstruction is governed by [REBUILD_CHARTER.md](REBUILD_CHARTER.md).

## Purpose

This page explains how the current Graphion runtime is organized.

It is not a language tutorial and it is not the ISA reference.

Use it to answer questions like:

- where `.gion` source enters the system
- which layer parses and validates source
- where execution semantics actually live
- which parts belong to the current scalar-language path
- which VM capabilities exist beyond the current `.gion` frontend

## Executive Summary

The active architecture is deliberately simple:

- `.gion` source is the normal user entry point
- the source path is parsed and lowered toward VM execution
- the VM is the execution backend
- unsupported forms must fail clearly rather than falling back to a second semantic engine

The repository still contains broader graph, hypergraph, frontier, and traversal machinery.
Those parts are real, but they should not be confused with the currently documented `.gion` language surface.

## Current Execution Pipeline

The intended pipeline is:

- `source Graphion -> tokens/parsing -> internal representation -> bytecode -> VM`

In repository terms, that currently means:

1. `src/runtime/entry.*`
- opens the source file
- validates the path and input text
- hands the source to the runtime/frontend path

2. `src/runtime/interpreter.*` and `src/runtime/interpreter/*`
- own the current `.gion` source-level pipeline
- handle parsing of the implemented subset
- lower supported forms toward VM-oriented execution
- map failures into user-visible source/runtime diagnostics

3. `src/vm/*`
- execute the lowered program
- hold the scalar opcode semantics used by the current language subset
- also contain broader VM families not yet fully exposed by `.gion`

4. `src/parser/frontend.*` and `src/parser/bytecode.*`
- parse textual IR/assembly and decode fixed-width bytecode for VM-facing tools and tests
- document and enforce the VM instruction encoding contract
- remain separate from the user-facing `.gion` frontend in `src/runtime/interpreter/*`

5. `src/runtime/arena.*`
- provide temporary allocation support for runtime/frontend work

## Runtime Layer Map

### Source entry

Relevant files:

- `src/runtime/entry.c`
- `src/runtime/entry.h`

Responsibilities:

- file-oriented entry into Graphion source execution
- source loading
- path validation and top-level orchestration handoff

This layer should stay thin. It is the boundary between the outside world and the language/runtime pipeline, not the place where language semantics should accumulate.

### Source frontend and orchestration

Relevant files:

- `src/runtime/interpreter.c`
- `src/runtime/interpreter.h`
- `src/runtime/interpreter/source.c`
- `src/runtime/interpreter/program.c`
- `src/runtime/interpreter/base.c`
- `src/runtime/interpreter/operands.c`
- `src/runtime/interpreter/stmt.c`
- `src/runtime/interpreter/expr.c`
- `src/runtime/interpreter/exec.c`

Responsibilities:

- define the currently supported `.gion` subset
- recognize statements, expressions, operands, and reserved names
- enforce source-level rules
- prepare execution through the VM path
- translate VM/runtime failures into language-facing errors when needed

Practical reading guide:

- `stmt.*` is where statement forms are recognized
- `expr.*` is where expression forms and builtin calls are recognized
- `operands.*` covers literals and source operands such as constants like `pi` and `e`
- `exec.*` is the main bridge between source execution and VM-visible result handling

This layer is where the current language surface is defined.
If a feature is "supported in `.gion`", it should be traceable here.

### VM backend

Relevant files:

- `src/vm/vm.c`
- `src/vm/vm.h`
- `src/vm/internal/core/dispatch.c`
- `src/vm/internal/core/value.*`
- `src/vm/internal/opcodes/op_state.*`
- `src/vm/internal/opcodes/op_io.*`
- `src/vm/internal/opcodes/op_scalar.*`
- `src/vm/internal/opcodes/op_graph.*`
- `src/vm/internal/opcodes/op_hypergraph.*`
- `src/vm/internal/opcodes/op_frontier.*`

Responsibilities:

- define the VM program model and result codes
- dispatch instructions
- hold the concrete semantics of opcodes
- provide scalar execution used by the current `.gion` subset
- preserve broader VM capabilities that are not yet equivalent to documented `.gion` surface area

Important architectural rule:

- language semantics should converge on the VM backend
- the VM should not be bypassed just because a source feature is inconvenient to lower cleanly

### Bytecode tooling layer

Relevant files:

- `src/parser/frontend.c`
- `src/parser/frontend.h`
- `src/parser/bytecode.c`
- `src/parser/bytecode.h`

Responsibilities:

- parse the compact textual IR/assembly syntax used by VM-facing tests and examples
- decode the fixed-width instruction stream
- support VM-facing tests and tooling
- keep the bytecode contract explicit and testable

This layer is VM tooling infrastructure, not the `.gion` language frontend. In
particular, `GFE_*` results from `graphion_parse_source_to_ir(...)` are not
the source diagnostics printed for `.gion` programs, and `GBC_*` results from
`graphion_decode_bytecode(...)` describe tooling/decoder failures rather than
user source errors.

## Current `.gion` Surface In Architectural Terms

The current source-language path is centered on scalar programs.

Implemented today at the user-language level:

- variable assignment and reuse
- `print(...)`
- arithmetic expressions
- postfix factorial `!`
- grouped expressions with parentheses
- compound assignments
- boolean logic and comparisons
- `if / elif / else`
- ternary expressions
- `match`
- comments and debug-mode warnings
- `bits` literals and bitwise operators
- scalar builtins and constants

Current scalar value kinds:

- `int`
- `float`
- `bool`
- `string`
- `bits`

This is the language surface that the docs in `docs/graphion/` should describe.
Anything beyond that belongs to broader runtime/VM capability, not to the current user-language contract.

## What The VM Contains Beyond `.gion`

The VM currently contains more than the active scalar-language subset.
That includes families related to:

- graph operations
- hypergraph operations
- frontier primitives
- traversal-oriented helpers
- weighted graph support

Architecturally, that means:

- the VM is broader than the current language frontend
- the presence of an opcode or runtime helper does not automatically mean `.gion` exposes it
- user documentation must not infer source-language support from VM capability alone

This separation is intentional and important.
It lets the backend evolve while the source language stays explicitly documented.

## Design Invariants

These are the architectural rules that matter most right now.

### 1. One execution model

There should be one real language pipeline, not a "normal path" plus a hidden fallback path.

### 2. `.gion` stays the normal frontend

Features should not bypass `.gion` just to make one example or one test pass.

### 3. Unsupported forms fail clearly

If a language form is outside the supported subset, it should produce a clear parse/runtime error rather than silently switching engines or semantics.

### 4. Docs must match implementation

- `docs/graphion/*` describe the currently implemented source-language subset
- `docs/runtime/*` describe backend structure and VM behavior
- architecture docs must be careful not to imply language features that only exist deeper in the VM

## Error Ownership

At a high level, error ownership is split like this:

- source/frontend layer
  - parse errors
  - source-shape errors
  - unknown variable / unknown operand diagnostics
- VM layer
  - execution result codes
  - type/runtime/domain failures during opcode execution

Result-code ownership is intentionally subsystem-local in `v0.x`.
`GENTRY_*` and `GINT_*` own the `.gion`/CLI boundary, while `GFE_*`,
`GIR_*`, `GBC_*`, and `GVM_*` keep their textual IR, lowering, bytecode,
or VM meanings. Backend failures are translated only when they cross into
the language-facing interpreter path.

`GENTRY_ERR_LOWER`, `GENTRY_ERR_LOAD`, `GINT_ERR_CALL`, and
`GINT_ERR_RETURN` are reserved enum members in `v0.x`; the current runtime
does not emit them as observable outcomes.

Reserved-name assignment is rejected during source handling:
`GINT_ERR_RESERVED_NAME` is translated to `GENTRY_ERR_PARSE` at the
`.gion` file-entry boundary rather than being presented as a runtime failure.

User-visible behavior is documented in:

- [ERRORS.md](../debugging/ERRORS.md)

VM-visible instruction and code behavior is documented in:

- [ISA.md](ISA.md)

## Reading Order

If you are working on runtime behavior, the best order is:

1. [REBUILD_CHARTER.md](REBUILD_CHARTER.md)
2. this page
3. [ISA.md](ISA.md)
4. [ERRORS.md](../debugging/ERRORS.md)

That order keeps design rules, structure, opcode surface, and failure behavior aligned.
