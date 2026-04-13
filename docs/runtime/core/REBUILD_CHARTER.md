# Rebuild Charter

## Goal

Graphion must be rebuilt around a single pipeline:

- `source Graphion -> tokens/parsing -> internal representation -> bytecode -> VM`

The language must be optimized in a general way.
It must never be optimized for a single example file or one special test case.

## Principles

### 1. Single pipeline

Every supported language form must go through one pipeline:

- `source Graphion -> tokens/parsing -> internal representation -> bytecode -> VM`

There must not be another hidden semantic engine.
The produced bytecode must remain readable and debuggable.

### 2. Preserve `.gion` as the normal entry point

`.gion` remains the normal source entry point of the language.
It must not be bypassed just to make one feature or test pass.

### 3. No semantic fallback

If a language form is not supported yet:

- fail with a clear error

There must not be a second execution engine that "still makes it work anyway."

### 4. Same semantics everywhere

The same `.gion` program must produce the same semantics in:

- release
- test

Behavior must not depend on a special execution path or a hidden optimization.

### 5. General-purpose optimization only

Any optimization must target:

- parsing
- internal representation
- bytecode lowering
- the VM

We must never optimize:

- for one test only
- for one particular file
- for an artificial case that does not represent the language

## Feature validation

The validation order is strict:

1. general behavior
2. tests

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

## Current state

At the current state of the repository:

- the functional priority is the scalar `.gion` subset
- the VM is already a real and measurable backend
- the rebuild must continue without recreating a semantic fallback
- user documentation must describe only what is actually implemented
