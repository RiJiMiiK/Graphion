# Language Reference

This reference describes the currently implemented `.gion` language subset.

## Statements

Current supported top-level statements:

- assignment
- compound assignment
- `print(...)`

Examples:

```gion
count = 42
count += 1
print(count)
```

Unsupported statements are parse errors in the current `.gion` frontend path.

## Values

Current scalar value kinds:

- `int`
- `float`
- `bool`
- `string`

## Identifiers

Identifiers:

- must start with a letter or `_`
- can then contain letters, digits, or `_`

Examples:

- `count`
- `_tmp`
- `alpha_1`

## Reserved Names

These names are currently reserved and cannot be assigned:

- `print`
- `true`
- `false`
- `abs`

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

## Assignment

Simple assignment:

```gion
count = 42
copy = count
ratio = 7 / 2
```

Compound assignment:

```gion
count += 1
count -= 1
count *= 2
count /= 2
count //= 2
count %= 2
count **= 2
```

Compound assignment requires the target variable to already exist.

## Arithmetic Operators

Supported arithmetic operators:

- `+`
- `-`
- `*`
- `/`
- `//`
- `%`
- `**`

### Operator Notes

`/`
: division, produces a float result

`//`
: floor division

`%`
: modulo

`**`
: power, right-associative

## Precedence

Current precedence order:

1. grouped expressions with parentheses
2. `abs(...)`
3. `**`
4. `*`, `/`, `//`, `%`
5. `+`, `-`

Examples:

```gion
value = 1 + 2 * 3
```

is interpreted as:

```gion
value = 1 + (2 * 3)
```

And:

```gion
value = 2 ** 3 ** 2
```

is interpreted as:

```gion
value = 2 ** (3 ** 2)
```

## Parentheses

Parentheses currently group arithmetic expressions:

```gion
grouped = (1 + 2) * 3
```

This is the current implemented behavior.

Tuple syntax is not part of the current documented language subset.

## Strings

String concatenation currently supports:

```gion
message = "graph" + "ion"
message += "!"
```

General mixed-type addition is not supported in assignment expressions.

This is invalid:

```gion
value = "count=" + 7
```

## `print(...)`

`print(...)` outputs a scalar value followed by a newline.

Examples:

```gion
print(42)
print("graphion")
print(7 / 2)
```

Inside `print(...)`, string concatenation can coerce non-string scalar values into text:

```gion
count = 7
print("count=" + count)
```

## Builtins

Current builtin functions:

- `abs(...)`

See [Builtins](builtins.md).

## Errors

Current high-level error classes:

- parse errors
- unknown variable errors
- unknown operand errors
- runtime errors

Examples:

`unknown variable`
: compound assignment target does not exist

`unknown operand`
: expression references a missing value

`division by zero`
: runtime arithmetic error

`arithmetic requires numeric operands`
: runtime type error for numeric operators
