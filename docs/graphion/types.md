# Types

This page documents the currently implemented value kinds in Graphion.

Use it when you need:

- the current scalar and non-scalar types
- literal forms
- built-in numeric constants
- the current `bits` value model

For syntax and statement rules, see [Language Reference](language-reference.md).

## Current Types

Graphion currently exposes these value kinds:

- `int`
- `float`
- `bool`
- `string`
- `bits`
- `list`

## Built-In Numeric Constants

Graphion currently provides six built-in numeric constants:

- `pi`
- `tau`
- `phi`
- `e`
- `nan`
- `inf`

Current values:

- `pi = 3.141592653589793`
- `tau = 6.283185307179586`
- `phi = 1.618033988749895`
- `e = 2.718281828459045`
- `nan = not-a-number`
- `inf = positive infinity`

These names are reserved and cannot be reassigned.

## Literals

### Integers

```gion
count = 42
negative = -7
```

### Floats

```gion
ratio = 3.5
negative_ratio = -2.25
circle = pi
turn = tau
golden = phi
growth = e
unknown = nan
limit = inf
```

### Booleans

```gion
ready = true
failed = false
```

### Strings

```gion
name = "graphion"
```

Current string literals are double-quoted.

### Bits

```gion
short_bits = 0b10
wide_bits = 0b0010
```

Current `bits` literals:

- start with `0b`
- require one or more binary digits after the prefix
- preserve the written width

That means:

- `0b10` has width `2`
- `0b0010` has width `4`

### Lists

```gion
values = [1, 2, 3]
mixed = [1, true, "graphion"]
nested = [values, [4, 5], []]
```

Current `list` literals:

- start with `[`
- end with `]`
- use commas between elements
- may contain nested lists
- currently reject trailing commas

## Type Notes

### Numeric Values

Graphion currently treats `int` and `float` as the numeric scalar family used by:

- arithmetic expressions
- comparisons
- most numeric builtins

Some operations preserve the input family, while others always return `float`. See [Builtins](builtins.md) for exact per-builtin result rules.

### Booleans

`bool` values are written as:

- `true`
- `false`

Graphion currently uses a strict boolean subset for conditions and boolean logic:

- `true`
- `false`
- `1`
- `0`

Other integers, floats, strings, and `bits` are rejected in boolean contexts.

### Strings

`string` values are scalar text values written with double quotes.

Current string support includes:

- storage in variables
- equality and inequality comparisons with other strings
- concatenation with other strings
- `len(x)`
- print-only scalar coercion inside `print(...)`

### Lists

`list` values are ordered non-scalar containers.

Current list support includes:

- literal construction with `[ ... ]`
- indexing with `list_expr[index_expr]`
- equality and inequality with other lists
- nested list values
- `len(x)`
- printing as bracketed values

Current index rules:

- indexes must be `int`
- indexes must be non-negative
- out-of-range access is a runtime error

### Bits

`bits` values are fixed-width binary scalars whose width comes from the literal spelling.

Current `bits` behavior:

- width is preserved for display and bitwise operations
- equality and inequality compare normalized values
- leading zeroes affect width, but not normalized equality

Examples:

```gion
print(0b10)
print(0b0010)
print(0b10 == 0b0010)
```

Expected output:

```text
0b10
0b0010
true
```

For the exact operator rules on `bits`, see [Operators](operators.md).
