# Types

This page documents the scalar value kinds currently available in Graphion.

Use it when you need:

- the current scalar types
- literal forms
- built-in numeric constants
- the current `bits` value model

For syntax and statement rules, see [Language Reference](language-reference.md).

## Current Scalar Types

Graphion currently exposes these scalar value kinds:

- `int`
- `float`
- `bool`
- `string`
- `bits`

## Built-In Numeric Constants

Graphion currently provides four built-in numeric constants:

- `pi`
- `e`
- `nan`
- `inf`

Current values:

- `pi = 3.141592653589793`
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
