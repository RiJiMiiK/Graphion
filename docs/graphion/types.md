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
- `dict`
- `tuple`
- `set`

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

### Dictionaries

```gion
weights = {"a": 1, "b": 2}
nested = {"inner": weights, "empty": {}}
```

Current `dict` literals:

- start with `{`
- end with `}`
- use commas between entries
- require `string` literal keys in the form `"key": value`
- may contain nested `dict` and `list` values
- currently reject trailing commas

### Tuples

```gion
pair = (1, 2)
mixed = (pair, "graphion", true)
```

Current `tuple` literals:

- start with `(`
- end with `)`
- use commas between elements
- may contain nested tuples
- currently require at least two elements
- currently reject trailing commas

### Sets

```gion
frontier = set(1, 2, 2, "a")
empty = set()
```

Current `set` literals:

- use `set(...)`
- use commas between elements
- remove duplicate elements during construction
- may contain nested container values
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

### Dictionaries

`dict` values are key-value non-scalar containers.

Current dict support includes:

- literal construction with `{ ... }`
- lookup with `dict_expr[key_expr]`
- key assignment with `dict_expr[key_expr] = value_expr`
- equality and inequality with other dicts
- nested dict values
- `len(x)`
- printing as braced values

Current dict key rules:

- literal keys must be `string` literals
- lookup keys must evaluate to `string`
- missing keys are runtime errors

### Tuples

`tuple` values are ordered non-scalar containers with fixed size semantics.

Current tuple support includes:

- literal construction with `( ... )`
- indexing with `tuple_expr[index_expr]`
- equality and inequality with other tuples
- nested tuple values
- `len(x)`
- printing as parenthesized values

Current tuple rules:

- tuples currently require at least two elements
- `(expr)` remains a grouped expression, not a tuple
- indexes must be `int`
- indexes must be non-negative
- out-of-range access is a runtime error

### Sets

`set` values are non-scalar containers of unique values.

Current set support includes:

- literal construction with `set(...)`
- duplicate removal during construction
- membership checks with `contains(set_expr, value_expr)`
- equality and inequality with other sets
- nested set values
- `len(x)`
- printing as `set(...)`

Current set rules:

- sets are compared without considering insertion order
- printing preserves first-insertion order for deterministic output
- `set()` is the empty set
- trailing commas are currently rejected

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
