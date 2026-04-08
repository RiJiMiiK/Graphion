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

## `cbrt(x)`

`cbrt(x)` returns the cube root of a numeric expression.

Examples:

```gion
print(cbrt(27))
print(cbrt(-8))
print(cbrt(1 + 26))
```

Expected output:

```text
3
-2
3
```

### Valid Inputs

`cbrt(x)` currently accepts:

- integer expressions
- float expressions

The result is currently returned as a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = cbrt("graphion")
```

Current messages:

```text
incompatible operand types
```

## `exp(x)`

`exp(x)` returns `e` raised to the value of a numeric expression.

Examples:

```gion
print(exp(1))
print(exp(0.0))
print(exp(1 + 1))
```

Expected output:

```text
2.71828
1
7.38906
```

### Valid Inputs

`exp(x)` currently accepts:

- integer expressions
- float expressions

The result is currently returned as a `float`.

### Invalid Inputs

This is a runtime error:

```gion
value = exp("graphion")
```

Current message:

```text
incompatible operand types
```

## `ln(x)`

`ln(x)` returns the natural logarithm of a numeric expression.

Examples:

```gion
print(ln(1))
print(ln(e))
print(ln(e ** 2))
```

Expected output:

```text
0
1
2
```

### Valid Inputs

`ln(x)` currently accepts:

- integer expressions
- float expressions

The result is currently returned as a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = ln("graphion")
value = ln(0)
value = ln(-1)
```

Current messages:

```text
incompatible operand types
ln requires strictly positive input
```

## `log(x, base)`

`log(x, base)` returns the logarithm of a numeric expression in an explicit base.

Examples:

```gion
print(log(8, 2))
print(log(100, 10))
print(log(2 ** 5, 2))
```

Expected output:

```text
3
2
5
```

### Valid Inputs

`log(x, base)` currently accepts:

- integer expressions
- float expressions
- mixed integer/float numeric expressions

The result is currently returned as a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = log("graphion", 2)
value = log(8, "base")
value = log(0, 10)
value = log(8, 1)
```

Current messages:

```text
incompatible operand types
log requires x > 0 and base > 0 with base != 1
```

## `log10(x)`

`log10(x)` returns the base-10 logarithm of a numeric expression.

Examples:

```gion
print(log10(1000))
print(log10(10.0))
print(log10(10 ** 4))
```

Expected output:

```text
3
1
4
```

### Valid Inputs

`log10(x)` currently accepts:

- integer expressions
- float expressions

The result is currently returned as a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = log10("graphion")
value = log10(0)
value = log10(-1)
```

Current messages:

```text
incompatible operand types
log requires x > 0 and base > 0 with base != 1
```

## `log2(x)`

`log2(x)` returns the base-2 logarithm of a numeric expression.

Examples:

```gion
print(log2(8))
print(log2(2.0))
print(log2(2 ** 6))
```

Expected output:

```text
3
1
6
```

### Valid Inputs

`log2(x)` currently accepts:

- integer expressions
- float expressions

The result is currently returned as a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = log2("graphion")
value = log2(0)
value = log2(-1)
```

Current messages:

```text
incompatible operand types
log requires x > 0 and base > 0 with base != 1
```

## `floor(x)`

`floor(x)` rounds a numeric expression down to the greatest integer less than or equal to it.

Examples:

```gion
print(floor(7))
print(floor(7.5))
print(floor(-3.2))
```

Expected output:

```text
7
7
-4
```

### Valid Inputs

`floor(x)` currently accepts:

- integer expressions
- float expressions

Integer inputs stay as `int`. Float inputs currently return a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = floor("graphion")
```

Current messages:

```text
incompatible operand types
```

## `ceil(x)`

`ceil(x)` rounds a numeric expression up to the smallest integer greater than or equal to it.

Examples:

```gion
print(ceil(7))
print(ceil(7.5))
print(ceil(-3.2))
```

Expected output:

```text
7
8
-3
```

### Valid Inputs

`ceil(x)` currently accepts:

- integer expressions
- float expressions

Integer inputs stay as `int`. Float inputs currently return a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = ceil("graphion")
```

Current messages:

```text
incompatible operand types
```

## `round(x)`

`round(x)` rounds a numeric expression to the nearest integer value.

Halfway values are currently rounded away from zero.

Examples:

```gion
print(round(7))
print(round(7.4))
print(round(7.5))
print(round(-3.2))
print(round(-3.5))
```

Expected output:

```text
7
7
8
-3
-4
```

### Valid Inputs

`round(x)` currently accepts:

- integer expressions
- float expressions

Integer inputs stay as `int`. Float inputs currently return a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = round("graphion")
```

Current messages:

```text
incompatible operand types
```

## `trunc(x)`

`trunc(x)` removes the fractional part of a numeric expression.

It currently truncates toward zero.

Examples:

```gion
print(trunc(7))
print(trunc(7.9))
print(trunc(-3.9))
print(trunc(-0.4))
```

Expected output:

```text
7
7
-3
0
```

### Valid Inputs

`trunc(x)` currently accepts:

- integer expressions
- float expressions

Integer inputs stay as `int`. Float inputs currently return a `float`.

### Invalid Inputs

These are runtime errors:

```gion
value = trunc("graphion")
```

Current messages:

```text
incompatible operand types
```

## `sign(x)`

`sign(x)` returns `-1` for negative numeric values, `0` for zero, and `1` for positive numeric values.

Examples:

```gion
print(sign(7))
print(sign(-3.9))
print(sign(0))
```

Expected output:

```text
1
-1
0
```

### Valid Inputs

`sign(x)` currently accepts:

- integer expressions
- float expressions

The result is currently returned as an `int`.

### Invalid Inputs

These are runtime errors:

```gion
value = sign("graphion")
```

Current messages:

```text
incompatible operand types
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
