# Builtins

This page documents the builtins currently available in Graphion.

## `print(...)`

`print(...)` writes a value to the output followed by a newline.

Examples:

```gion
print(42)
print("graphion")
print(7 / 2)
```

Expected output:

```text
42
graphion
3.5
```

### Print-Only String Coercion

Inside `print(...)`, string concatenation currently allows non-string scalar values to be rendered inline:

```gion
count = 7
print("count=" + count)
print("value=" + (3 + 4))
```

Expected output:

```text
count=7
value=7
```

This coercion is specific to `print(...)`.

## `abs(...)`

`abs(...)` returns the absolute value of a numeric expression.

Examples:

```gion
print(abs(-42))
print(abs(-3.5))
print(abs(-5 + 2))
```

Expected output:

```text
42
3.5
3
```

### Valid Inputs

`abs(...)` currently accepts:

- integer expressions
- float expressions

### Invalid Inputs

This is a runtime error:

```gion
value = abs("graphion")
```

Current message:

```text
incompatible operand types
```

## `min(a, b)`

`min(a, b)` returns the smaller of two numeric expressions.

Examples:

```gion
print(min(7, 3))
print(min(3.5, 2))
print(min(10 - 2, 3 * 3))
```

Expected output:

```text
3
2
8
```

### Valid Inputs

`min(a, b)` currently accepts:

- integer expressions
- float expressions
- mixed integer/float numeric expressions

### Invalid Inputs

This is a runtime error:

```gion
value = min("graphion", 1)
```

Current message:

```text
incompatible operand types
```

## `max(a, b)`

`max(a, b)` returns the larger of two numeric expressions.

Examples:

```gion
print(max(7, 3))
print(max(3.5, 2))
print(max(10 - 2, 3 * 3))
```

Expected output:

```text
7
3.5
9
```

### Valid Inputs

`max(a, b)` currently accepts:

- integer expressions
- float expressions
- mixed integer/float numeric expressions

### Invalid Inputs

This is a runtime error:

```gion
value = max("graphion", 1)
```

Current message:

```text
incompatible operand types
```

## `clamp(x, lo, hi)`

`clamp(x, lo, hi)` constrains a numeric expression to a numeric range.

- if `x < lo`, the result is `lo`
- if `x > hi`, the result is `hi`
- otherwise the result is `x`

Examples:

```gion
print(clamp(-2, 0, 10))
print(clamp(5, 0, 10))
print(clamp(17, 0, 10))
print(clamp(12.5, 0, 10))
```

Expected output:

```text
0
5
10
10
```

### Valid Inputs

`clamp(x, lo, hi)` currently accepts:

- integer expressions
- float expressions
- mixed integer/float numeric expressions

### Invalid Inputs

This is a runtime error:

```gion
value = clamp("graphion", 0, 1)
```

Current message:

```text
incompatible operand types
```

## `sqrt(x)`

`sqrt(x)` returns the square root of a numeric expression.

Examples:

```gion
print(sqrt(9))
print(sqrt(2.25))
print(sqrt(1 + 8))
```

Expected output:

```text
3
1.5
3
```

### Valid Inputs

`sqrt(x)` currently accepts:

- integer expressions
- float expressions

The result is currently returned as a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = sqrt("graphion")
value = sqrt(-1)
```

Current messages:

```text
incompatible operand types
sqrt requires non-negative input
```

## `len(x)`

`len(x)` returns the length of a string expression.

Examples:

```gion
print(len("graphion"))
print(len("graph" + "ion"))
```

Expected output:

```text
8
8
```

### Valid Inputs

`len(x)` currently accepts:

- string expressions

The result is currently returned as an `int`.

### Invalid Inputs

This is a runtime error:

```gion
value = len(42)
```

Current message:

```text
incompatible operand types
```
