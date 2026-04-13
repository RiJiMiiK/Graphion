# Graphion User Guide

This section documents the user-facing `.gion` language.

Use it in this order:

1. [Tutorial](tutorial.md)
2. [Types](types.md)
3. [Operators](operators.md)
4. [Language Reference](language-reference.md)
5. [Builtins](builtins.md)

That split is intentional:

- the tutorial teaches by building small scripts step by step
- the types page centralizes scalar values, literals, and built-in constants
- the operators page centralizes precedence, arithmetic, comparisons, boolean logic, strings, and `bits`
- the language reference states the exact rules of the currently implemented language
- the builtins page is a focused catalog of callable builtins

## Current Scope

Graphion currently exposes a small but usable scalar language.

Implemented today:

- variable assignment and reuse
- `print(...)`
- scalar values:
  - `int`
  - `float`
  - `bool`
  - `string`
  - `bits`
- numeric constants:
  - `pi`
  - `tau`
  - `phi`
  - `e`
  - `nan`
  - `inf`
- arithmetic expressions
- postfix factorial `!`
- grouped expressions with parentheses
- compound assignments
- boolean logic with `and`, `nand`, `or`, `nor`, and `not`
- comparisons with `==`, `!=`, `<`, `<=`, `>`, and `>=`
- `if / elif / else`
- ternary expressions
- `match` blocks with `default`
- line comments with `#`
- block comments with `/* ... */`
- `bits` literals and bitwise operators
- scalar builtins documented in [Builtins](builtins.md)

This user guide documents only behavior that is implemented now.

## Sections

```{toctree}
:maxdepth: 2

tutorial
types
operators
language-reference
builtins
```

## Reading Advice

Start with the tutorial if you want to learn the language.

Jump straight to the language reference if you want exact rules for:

- condition rules
- reserved names
- current error behavior

Use the operators page when you need:

- precedence
- arithmetic and comparison rules
- boolean logic details
- `bits` operator semantics
- string concatenation behavior

Use the builtins page when you need:

- the accepted input kinds for a builtin
- its current result type
- domain restrictions
- the current runtime error wording

Use the types page when you need:

- the current scalar value kinds
- literal syntax
- `bits` width basics
- built-in constants such as `pi`, `tau`, `phi`, `e`, `nan`, and `inf`

## Current Limits

This is still the implemented `v0.x` subset, not the long-term target language.

Still missing or intentionally deferred:

- loops and broader control-flow forms beyond the current subset
- tuples or structured multi-value returns
- user-defined functions
- graph-specific source-language constructs in the `.gion` frontend path
- broader type-system features beyond the current scalar focus

Those can be added later, but they are not documented here until they actually exist in the language.
