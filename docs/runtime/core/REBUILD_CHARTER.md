# Rebuild Charter

## Goal

Graphion must be rebuilt around a single pipeline:

- `source Graphion -> tokens/parsing -> internal representation -> bytecode -> VM`

The language must be optimized in a general way.
It must never be optimized for a single benchmark, a single example file, or one special test case.

## Principles

### 1. Single pipeline

Every supported language form must go through one pipeline:

- `source Graphion -> tokens/parsing -> internal representation -> bytecode -> VM`

There must not be another hidden semantic engine.
The produced bytecode must remain inspectable.

### 2. Preserve `.gion` as the normal entry point

`.gion` remains the normal source entry point of the language.
It must not be bypassed just to make one feature, test, or benchmark pass.

### 3. No semantic fallback

If a language form is not supported yet:

- fail with a clear error

There must not be a second execution engine that "still makes it work anyway."

### 4. Same semantics everywhere

The same `.gion` program must produce the same semantics in:

- release
- test
- benchmark

Behavior must not depend on a special execution path, a bench-only binary, or a hidden optimization.

### 5. General-purpose optimization only

Any optimization must target:

- parsing
- internal representation
- bytecode lowering
- the VM

We must never optimize:

- for one test only
- for one benchmark only
- for one particular file
- for an artificial case that does not represent the language

## Feature validation

The validation order is strict:

1. general behavior
2. tests
3. benchmarks

### 1. General behavior

A feature must work in a general `.gion` program as long as the user program stays within the supported scope.

That must remain true:

- regardless of identifier names
- regardless of supported values
- regardless of the legal order of lines
- regardless of the combination with already supported features

### 2. Cumulative compatibility

A new feature must not break earlier ones.

A validated feature must work:

- on its own
- and in combination with already validated features

### 3. Required tests

Every feature must have:

- targeted tests
- regression tests
- error-path tests
- inter-feature integration tests

Tests must vary enough to limit false positives:

- names
- values
- negative cases
- feature combinations

### 4. Clear errors

If code is outside the supported scope:

- fail with a clear error

If the user program is invalid:

- fail with a clear error

Examples:

- invalid syntax
- unknown variable
- unknown operand
- unsupported operation for the provided types

### 5. Traceable validation state

The state of a feature must remain explicit.

When useful, distinguish between:

- implemented
- tested
- validated
- benchmarked

## Benchmarks

### 1. Role of benchmarks

A benchmark measures performance.
It does not prove that a feature works.

### 2. Representativeness

A benchmark must represent a general form of the language.

Its value must not come from special treatment applied to:

- one specific file
- one special line order
- one special naming pattern
- one overly specialized case

### 3. Acceptance thresholds

For representative benchmarks:

- `VM / Rust < 1.15x`
- `.gion / Rust = 2x to 3x` as the main target
- `.gion / Rust < 2x` only as a stretch goal if real general-purpose levers remain
- `variation < 10%`

## Current state

At the current state of the repository:

- the functional priority is the scalar `.gion` subset
- the VM is already a real and measurable backend
- the rebuild must continue without recreating a semantic fallback
- user documentation must describe only what is actually implemented
