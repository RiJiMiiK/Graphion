# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

This roadmap tracks what is still ahead of us.
Completed historical work is intentionally not repeated here.

## Current focus

The current active lane is the `.gion` scalar language rebuild:

- source Graphion
- tokens/parsing
- internal representation
- bytecode
- VM

The goal is to keep growing that path without reintroducing alternate semantic engines or benchmark-only shortcuts.

## Near-term language work

### Comparisons

- [x] `==`
- [x] `!=`
- [x] `<`
- [x] `<=`
- [x] `>`
- [x] `>=`
- [x] finish and document comparison semantics across `int`, `float`, `bool`, and `string`
  - current `==` and `!=` support numeric comparison, same-kind `bool`, same-kind `string`, and a strict `int`/`bool` bridge only for `0` and `1`
  - current `==` and `!=` reject `float`/`bool`, `string`/non-`string`, and `int`/`bool` comparisons when the integer is outside `0` or `1`
  - current `<`, `<=`, `>`, and `>=` support numeric comparison only and reject `bool` / `string`

### Boolean logic

- [x] `and`
- [x] `or`
- [x] `not`
- [x] `nand`
- [x] `nor`
- [ ] documented truth rules
- [x] fix boolean precedence so `and` / `nand` bind tighter than `or` / `nor`
- [ ] decide and implement short-circuit behavior for `and` / `or`
- [ ] tests for precedence and short-circuit behavior after it exists

### More builtins

- [ ] decide and implement the next math-oriented builtins after `abs(...)`
- [ ] document every builtin as soon as it becomes real

### Tuples and parentheses

- [ ] introduce tuple semantics
- [ ] preserve the intended rule that `(x)` becomes a tuple form later
- [ ] clearly distinguish grouped arithmetic expressions from tuple syntax

## Runtime / frontend rebuild

- [ ] continue moving toward the explicit target pipeline:
  - `source Graphion -> tokens/parsing -> representation interne du code -> bytecode -> VM`
- [ ] reduce historical coupling between source handling and execution internals
- [ ] keep unsupported forms as clear errors instead of fallbacks
- [ ] keep the bytecode inspectable

## VM work

- [ ] keep VM-visible behavior aligned with the actively used `.gion` subset
- [ ] document which opcodes are active for the scalar language path
- [ ] continue validating the VM lane independently from the source lane
- [ ] revisit VM documentation so it separates:
  - active scalar/runtime work
  - broader graph/hypergraph VM capabilities

## Performance work

- [ ] keep `VM / Rust < 1.15x` on representative VM lanes
- [ ] work `.gion / Rust` toward the main target band `2x to 3x`
- [ ] treat `< 2x` as a stretch goal only if general-purpose levers remain
- [ ] keep benchmark variation below `10%` on accepted runs
- [ ] continue using `scalar_values_print` as the current scalar-language benchmark lane

## Documentation

- [ ] keep the Graphion user docs aligned with the actually implemented subset
- [ ] keep architecture / ISA / error docs aligned with real code, not historical intermediate states
- [ ] expand the tutorial as new language features land
- [ ] keep the HTML doc site as the primary documentation surface

## Quality

- [ ] keep growing targeted tests for each new language feature
- [ ] keep adding error-case coverage, not just happy-path coverage
- [ ] maintain cumulative integration tests as the language surface expands
- [ ] revisit fuzzing once the language surface is larger

## Later tracks

These remain interesting, but they are not the current driver of the rebuild:

- richer data types
- graph/hypergraph user-facing language features
- function model expansion
- deeper runtime memory model work
- broader assembly work beyond proven hot paths
