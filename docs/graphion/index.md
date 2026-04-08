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
  - `bits` through `0b...` literals, normalized equality, `&`, `|`, `^`, `~`, `<<`, and `>>`
- numeric constants like `pi` and `e`
- arithmetic expressions
- postfix factorial `!`
- grouped expressions with parentheses
- compound assignments
- boolean `if / elif / else` blocks
- grouped multiline conditions
- ternary expressions
- grouped multiline ternary expressions
- value-based `match` blocks with `default`
- pre-execution warnings with `# graphion: warnings=off`
- boolean `and`
- boolean `nand`
- boolean `or`
- boolean `nor`
- boolean `not`
- `bits` literals and bitwise operators
- line comments with `#`
- block comments with `/* ... */`
- equality comparisons with `==`, `!=`, numeric ordering with `<` / `<=` / `>` / `>=`, and boolean `and` / `nand` / `or` / `nor` / `not`
- the `abs()`, `min()`, `max()`, `clamp()`, `sqrt()`, `cbrt()`, `sin()`, `cos()`, `exp()`, `ln()`, `log()`, `log10()`, `log2()`, `floor()`, `ceil()`, `round()`, `trunc()`, `sign()`, and `len()` builtins, plus postfix factorial `!` and numeric constants like `pi` and `e`

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

- any broader comparison semantics beyond the current numeric `<` / `<=` / `>` / `>=`
- additional control-flow forms such as loops and ternary expressions
- tuples
- functions
- graph-specific language constructs in the `.gion` frontend path

Those can be added later, but they are not documented here until they are actually implemented.
