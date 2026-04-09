# Builtins

This page documents the builtins currently available in Graphion.

## Shared Rules

Unless a builtin says otherwise:

- arguments are evaluated before the builtin runs
- invalid argument types raise `incompatible operand types`
- numeric builtins accept `int`, `float`, or the documented mix of both
- builtins return a value that can be stored, printed, or reused in a larger expression

## Quick Reference

| Builtin | Purpose | Returns | Notes |
| --- | --- | --- | --- |
| `print(x)` | write one value and a newline | no value | special print-only string coercion |
| `abs(x)` | absolute value | numeric | numeric only |
| `min(a, b)` | smaller of two values | numeric | mixed `int`/`float` allowed |
| `max(a, b)` | larger of two values | numeric | mixed `int`/`float` allowed |
| `clamp(x, lo, hi)` | constrain to a range | numeric | mixed `int`/`float` allowed |
| `sqrt(x)` | square root | `float` | requires `x >= 0` |
| `cbrt(x)` | cube root | `float` | negatives allowed |
| `sin(x)` | sine in radians | `float` | |
| `cos(x)` | cosine in radians | `float` | |
| `tan(x)` | tangent in radians | `float` | values near asymptotes can grow very large |
| `asin(x)` | arcsine in radians | `float` | requires `x in [-1, 1]` |
| `acos(x)` | arccosine in radians | `float` | requires `x in [-1, 1]` |
| `atan(x)` | arctangent in radians | `float` | |
| `atan2(y, x)` | angle of vector `(x, y)` | `float` | two numeric inputs |
| `hypot(x, y)` | length of vector `(x, y)` | `float` | two numeric inputs |
| `degrees(x)` | convert radians to degrees | `float` | |
| `radians(x)` | convert degrees to radians | `float` | |
| `isnan(x)` | test whether a numeric value is NaN | `bool` | non-float numerics return `false` |
| `isinf(x)` | test whether a numeric value is infinite | `bool` | non-float numerics return `false` |
| `isfinite(x)` | test whether a numeric value is finite | `bool` | ints and finite floats return `true` |
| `sinh(x)` | hyperbolic sine | `float` | |
| `asinh(x)` | inverse hyperbolic sine | `float` | |
| `acosh(x)` | inverse hyperbolic cosine | `float` | requires `x >= 1` |
| `cosh(x)` | hyperbolic cosine | `float` | |
| `tanh(x)` | hyperbolic tangent | `float` | |
| `atanh(x)` | inverse hyperbolic tangent | `float` | requires `x in (-1, 1)` |
| `exp(x)` | `e ** x` | `float` | |
| `expm1(x)` | `e ** x - 1` | `float` | useful for small values near zero |
| `log1p(x)` | `ln(1 + x)` | `float` | useful for small values near zero, requires `x > -1` |
| `erf(x)` | error function | `float` | often used with Gaussian-style formulas |
| `erfc(x)` | complementary error function | `float` | equal to `1 - erf(x)` |
| `ln(x)` | natural logarithm | `float` | requires `x > 0` |
| `log(x, base)` | logarithm in explicit base | `float` | requires `x > 0`, `base > 0`, `base != 1` |
| `log10(x)` | base-10 logarithm | `float` | lowered to `log(x, 10)` |
| `log2(x)` | base-2 logarithm | `float` | lowered to `log(x, 2)` |
| `floor(x)` | round down | same numeric family | `int -> int`, `float -> float` |
| `ceil(x)` | round up | same numeric family | `int -> int`, `float -> float` |
| `round(x)` | nearest integer value | same numeric family | `.5` rounds away from zero |
| `trunc(x)` | drop fractional part | same numeric family | toward zero |
| `fract(x)` | fractional part | `float` | defined as `x - floor(x)` |
| `sign(x)` | sign of a number | `int` | `-1`, `0`, or `1` |
| `len(x)` | string length | `int` | strings only |

## `print(...)`

`print(...)` writes a value followed by a newline.

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

Inside `print(...)`, Graphion currently allows string concatenation with non-string scalar values.

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

## Numeric Helpers

### `abs(x)`

Returns the absolute value of a numeric expression.

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

### `min(a, b)` and `max(a, b)`

Return the smaller or larger of two numeric expressions.

```gion
print(min(7, 3))
print(max(7, 3))
print(min(3.5, 2))
print(max(3.5, 2))
```

Expected output:

```text
3
7
2
3.5
```

Mixed `int` / `float` inputs are allowed.

### `clamp(x, lo, hi)`

Constrains `x` to the inclusive range `[lo, hi]`.

- if `x < lo`, the result is `lo`
- if `x > hi`, the result is `hi`
- otherwise the result is `x`

```gion
print(clamp(-2, 0, 10))
print(clamp(5, 0, 10))
print(clamp(17, 0, 10))
```

Expected output:

```text
0
5
10
```

## Roots And Exponentials

### `sqrt(x)`

Returns the square root of `x`.

```gion
print(sqrt(9))
print(sqrt(2.25))
```

Expected output:

```text
3
1.5
```

Domain restriction:

- `x >= 0`

Current domain error:

```text
sqrt requires non-negative input
```

### `cbrt(x)`

Returns the cube root of `x`.

```gion
print(cbrt(27))
print(cbrt(-8))
```

Expected output:

```text
3
-2
```

Negative values are allowed.

### `exp(x)`

Returns `e ** x`.

```gion
print(exp(1))
print(exp(0.0))
```

Expected output:

```text
2.71828
1
```

### `expm1(x)`

Returns `e ** x - 1`.

```gion
print(expm1(1))
print(expm1(0.0))
```

Expected output:

```text
1.71828
0
```

### `log1p(x)`

Returns `ln(1 + x)`.

```gion
print(log1p(1))
print(log1p(0.0))
```

Expected output:

```text
0.693147
0
```

`log1p(x)` has one domain restriction:

```text
log1p requires input > -1
```

### `erf(x)`

Returns the error function of `x`.

```gion
print(erf(0))
print(erf(1))
```

Expected output:

```text
0
0.842701
```

### `erfc(x)`

Returns the complementary error function of `x`.

```gion
print(erfc(0))
print(erfc(1))
```

```text
1
0.157299
```

## Logarithms

All logarithm builtins return a `float`.

### `ln(x)`

Returns the natural logarithm of `x`.

```gion
print(ln(1))
print(ln(e))
```

Expected output:

```text
0
1
```

Domain restriction:

- `x > 0`

Current domain error:

```text
ln requires strictly positive input
```

### `log(x, base)`

Returns the logarithm of `x` in the explicit base `base`.

```gion
print(log(8, 2))
print(log(100, 10))
```

Expected output:

```text
3
2
```

Domain restrictions:

- `x > 0`
- `base > 0`
- `base != 1`

Current domain error:

```text
log requires x > 0 and base > 0 with base != 1
```

### `log10(x)` and `log2(x)`

Convenience forms for base-10 and base-2 logarithms.

```gion
print(log10(1000))
print(log2(8))
```

Expected output:

```text
3
3
```

Implementation note:

- `log10(x)` is lowered to `log(x, 10)`
- `log2(x)` is lowered to `log(x, 2)`

They therefore follow the same domain rule and error wording as `log(x, base)`.

## Trigonometry

All trigonometric builtins use radians and return a `float`.

### `sin(x)`, `cos(x)`, `tan(x)`

```gion
print(sin(0))
print(cos(pi))
print(tan(pi / 4))
```

Expected output:

```text
0
-1
1
```

For `tan(x)`, values near asymptotes such as `pi / 2 + k*pi` can become very large. Graphion currently follows ordinary floating-point behavior here rather than raising a special error.

### `asin(x)` and `acos(x)`

Inverse sine and inverse cosine, in radians.

```gion
print(asin(0.5))
print(acos(0.5))
```

Expected output:

```text
0.523599
1.0472
```

Domain restriction for both:

- `x` must be in `[-1, 1]`

Current domain errors:

```text
asin requires input in [-1, 1]
acos requires input in [-1, 1]
```

### `atan(x)` and `atan2(y, x)`

`atan(x)` returns the arctangent of one numeric expression.

`atan2(y, x)` returns the angle of the vector `(x, y)` and keeps quadrant information.

```gion
print(atan(1))
print(atan2(1, -1))
```

Expected output:

```text
0.785398
2.35619
```

### `hypot(x, y)`

Returns the Euclidean length of the vector `(x, y)`.

```gion
print(hypot(3, 4))
```

Expected output:

```text
5
```

### `degrees(x)`

Converts an angle from radians to degrees.

```gion
print(degrees(0))
print(degrees(pi / 2))
print(degrees(-0.7853981633974483))
```

Expected output:

```text
0
90
-45
```

### `radians(x)`

Converts an angle from degrees to radians.

```gion
print(radians(0))
print(radians(180))
print(radians(-45))
```

Expected output:

```text
0
3.14159
-0.785398
```

### `isnan(x)`

Tests whether a numeric value is `nan`.

```gion
print(isnan(nan))
print(isnan(1.0))
print(isnan(7))
```

Expected output:

```text
true
false
false
```

### `isinf(x)`

Tests whether a numeric value is `inf`.

```gion
print(isinf(inf))
print(isinf(1.0))
print(isinf(7))
```

Expected output:

```text
true
false
false
```

### `isfinite(x)`

Tests whether a numeric value is finite.

```gion
print(isfinite(inf))
print(isfinite(nan))
print(isfinite(1.0))
print(isfinite(7))
```

Expected output:

```text
false
false
true
true
```

## Hyperbolic Functions

### `sinh(x)`, `asinh(x)`, `acosh(x)`, `cosh(x)`, `tanh(x)`, `atanh(x)`

```gion
print(sinh(1))
print(asinh(1))
print(acosh(2))
print(cosh(1))
print(tanh(1))
print(atanh(0.5))
```

Expected output:

```text
1.1752
0.881374
1.31696
1.54308
0.761594
0.549306
```

These accept numeric input and return a `float`.

`acosh(x)` has one domain restriction:

- `x >= 1`

Current domain error:

```text
acosh requires input >= 1
```

`atanh(x)` has one domain restriction:

- `x` must be strictly inside `(-1, 1)`

Current domain error:

```text
atanh requires input in (-1, 1)
```

## Rounding And Shape

### `floor(x)` and `ceil(x)`

```gion
print(floor(7.5))
print(ceil(-3.2))
```

Expected output:

```text
7
-3
```

Current result rule:

- `int` input stays `int`
- `float` input returns a `float`

### `round(x)`

Rounds to the nearest integer value.

Current tie-breaking rule:

- `.5` rounds away from zero

```gion
print(round(7.5))
print(round(-3.5))
```

Expected output:

```text
8
-4
```

### `trunc(x)`

Removes the fractional part.

Current rule:

- truncates toward zero

```gion
print(trunc(7.9))
print(trunc(-3.9))
```

Expected output:

```text
7
-3
```

### `fract(x)`

Returns the fractional part of a numeric value.

Rule:

- `fract(x) = x - floor(x)`

```gion
print(fract(7))
print(fract(7.25))
print(fract(-3.75))
```

Expected output:

```text
0
0.25
0.25
```

### `sign(x)`

Returns the sign of a numeric value.

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

Current result values:

- `-1`
- `0`
- `1`

The result type is currently `int`.

## Strings

### `len(x)`

Returns the length of a string expression.

```gion
print(len("graphion"))
print(len("graph" + "ion"))
```

Expected output:

```text
8
8
```

Accepted input:

- string expressions only

Result type:

- `int`

## Error Summary

Common current runtime messages used by builtins:

```text
incompatible operand types
sqrt requires non-negative input
asin requires input in [-1, 1]
acos requires input in [-1, 1]
ln requires strictly positive input
log requires x > 0 and base > 0 with base != 1
```

For the full language-level context around these errors, see [Language Reference](language-reference.md).
