# Graphion User Guide

This is the starting point for Graphion user documentation.

Graphion is currently an early-stage language with a small but already usable subset:

- scalar assignments
- variable copies
- `print(...)`
- scalar types:
  - `int`
  - `float`
  - `bool`
  - `string`
- arithmetic expressions
- grouped expressions with parentheses
- compound assignments
- boolean `if / elif / else` blocks
- line comments with `#`
- block comments with `/* ... */`
- equality comparisons with `==`, `!=`, and numeric `<`
- the `abs()` builtin

This guide only documents behavior that is implemented today.

## Sections

```{toctree}
:maxdepth: 2

tutorial
language-reference
builtins
```

## Current Scope

The current language subset is centered on scalar values and expressions.

Example:

```gion
count = 42
ratio = 7 / 2
message = "graph" + "ion"
ready = true

count += 1

if ready:
    message += "!"

print("count=" + count)
print("ratio=" + ratio)
print("message=" + message)
```

Expected output:

```text
count=43
ratio=3.5
message=graphion!
```

## Current Limits

This user guide describes the current implemented subset, not the long-term target language.

That means some areas are intentionally still missing or incomplete, including:

- most ordered comparisons beyond numeric `<`, plus `<=`, `>`, and `>=`
- boolean logic operators
- tuples
- functions
- graph-specific language constructs in the `.gion` frontend path

Those can be added later, but they are not documented here until they are actually implemented.
