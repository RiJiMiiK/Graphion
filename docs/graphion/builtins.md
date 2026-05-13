# Builtins

This page documents the builtins currently available in Graphion.

Graphion also has graph mutation statements such as `add_node(G, node)`, `add_edge(G, from, to)`,
`remove_node(G, node)`, `remove_edge(G, from, to)`, `set_node_attrs(...)`, `set_edge_attrs(...)`,
and `set_edge_weight(...)`.
They are documented on the [Language Reference](language-reference.md) page because they mutate a named graph variable rather than returning a standalone value.

Graph listing builtins include `node_ids(graph)`, `nodes(graph)`, and `edges(graph)` for
inspecting graph contents without knowing node names or IDs ahead of time.
Hypergraph hyperedge builtins currently include `hyperedge_vertices(hypergraph, id)` and
`hyperedge_attrs(hypergraph, id)` for inspecting implicit hyperedge IDs.

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
| `csc(x)` | cosecant in radians | `float` | values near `sin(x) = 0` can grow very large |
| `sec(x)` | secant in radians | `float` | values near `cos(x) = 0` can grow very large |
| `cot(x)` | cotangent in radians | `float` | values near `sin(x) = 0` can grow very large |
| `acsc(x)` | inverse cosecant in radians | `float` | requires `x <= -1` or `x >= 1` |
| `asec(x)` | inverse secant in radians | `float` | requires `x <= -1` or `x >= 1` |
| `acot(x)` | inverse cotangent in radians | `float` | principal branch implemented with `atan2(1, x)` |
| `sech(x)` | hyperbolic secant | `float` | implemented as `1 / cosh(x)` |
| `csch(x)` | hyperbolic cosecant | `float` | implemented as `1 / sinh(x)` |
| `coth(x)` | hyperbolic cotangent | `float` | implemented as `1 / tanh(x)` |
| `cos(x)` | cosine in radians | `float` | |
| `tan(x)` | tangent in radians | `float` | values near asymptotes can grow very large |
| `asin(x)` | arcsine in radians | `float` | requires `x in [-1, 1]` |
| `acos(x)` | arccosine in radians | `float` | requires `x in [-1, 1]` |
| `atan(x)` | arctangent in radians | `float` | |
| `atan2(y, x)` | angle of vector `(x, y)` | `float` | two numeric inputs |
| `hypot(x, y)` | length of vector `(x, y)` | `float` | two numeric inputs |
| `copysign(x, y)` | magnitude of `x` with sign of `y` | `float` | two numeric inputs |
| `fma(a, b, c)` | fused multiply-add | `float` | three numeric inputs |
| `fdim(x, y)` | positive difference | `float` | equal to `max(x - y, 0)` |
| `remainder(x, y)` | IEEE-style remainder | `float` | divisor must be non-zero |
| `rint(x)` | platform rounding-mode integer value | `float` | lower-level than `round(x)` |
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
| `exp2(x)` | `2 ** x` | `float` | |
| `expm1(x)` | `e ** x - 1` | `float` | useful for small values near zero |
| `log1p(x)` | `ln(1 + x)` | `float` | useful for small values near zero, requires `x > -1` |
| `erf(x)` | error function | `float` | often used with Gaussian-style formulas |
| `erfc(x)` | complementary error function | `float` | equal to `1 - erf(x)` |
| `gamma(x)` | gamma function | `float` | undefined at `0, -1, -2, ...` |
| `lgamma(x)` | natural log of absolute gamma | `float` | undefined at `0, -1, -2, ...` |
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
| `len(x)` | container or string length | `int` | strings, lists, dicts, tuples, and sets |
| `contains(set, value)` | set membership | `bool` | first argument must be a set |
| `node_count(graph)` | number of logical nodes in a graph | `int` | counts present nodes, not ID gaps |
| `edge_count(graph)` | number of logical edges in a graph | `int` | `<->` counts as one edge |
| `is_directed(graph)` | whether a graph has directed syntax | `bool` | `->` and `<->` make this true |
| `is_weighted(graph)` | whether a graph has edge weights | `bool` | true when any edge has `weight` |
| `orientation(graph)` | graph orientation summary | `string` | `empty`, `undirected`, or `directed` |
| `node_ids(graph)` | present node IDs | `list` | IDs remain stable and can have gaps |
| `nodes(graph)` | present node descriptors | `list` | each item is a dict with `id` and optional `name` |
| `edges(graph)` | logical edge descriptors | `list` | each item has `from`, `to`, `directed`, and `bidirectional` |
| `node_attrs(graph, node)` | attributes for one node | `dict` | `node` can be an ID or string node name |
| `edge_attrs(graph, from, to)` | attributes for one edge | `dict` | bidirectional edges can be queried both ways |
| `edge_weight(graph, from, to)` | reserved `weight` for one edge | numeric | missing `weight` is a runtime error |
| `hyperedge_vertices(hypergraph, id)` | vertex IDs in one hyperedge | `list` | hyperedge IDs are assigned in declaration order |
| `hyperedge_attrs(hypergraph, id)` | attributes for one hyperedge | `dict` | missing attributes return an empty dict |
| `has_node(graph, node)` | whether a node exists | `bool` | `node` can be an ID or string node name |
| `has_edge(graph, from, to)` | whether an edge exists | `bool` | respects directed edge orientation |
| `neighbors(graph, node)` | adjacent neighbor IDs | `list` | includes incoming and outgoing directed edges |
| `indegree(graph, node)` | incoming neighbor IDs | `list` | use `len(...)` for the count |
| `outdegree(graph, node)` | outgoing neighbor IDs | `list` | use `len(...)` for the count |

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

### `exp2(x)`

Returns `2 ** x`.

```gion
print(exp2(1))
print(exp2(0.0))
```

Expected output:

```text
2
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

### `gamma(x)`

Returns the gamma function.

For positive integers, `gamma(n)` matches `(n - 1)!`.

```gion
print(gamma(1))
print(gamma(5))
print(gamma(0.5))
```

Expected output:

```text
1
24
1.77245
```

Current domain restriction:

- undefined at `0` and negative integers

Current domain error:

```text
gamma is undefined at 0 and negative integers
```

### `lgamma(x)`

Returns the natural logarithm of the absolute value of the gamma function.

For positive integers, `lgamma(n)` matches `ln((n - 1)!)`.

```gion
print(lgamma(1))
print(lgamma(5))
print(lgamma(0.5))
```

Expected output:

```text
0
3.17805
0.572365
```

Current domain restriction:

- undefined at `0` and negative integers

Current domain error:

```text
lgamma is undefined at 0 and negative integers
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

### `sin(x)`, `csc(x)`, `sec(x)`, `cot(x)`, `cos(x)`, `tan(x)`

```gion
print(sin(0))
print(csc(pi / 2))
print(sec(0))
print(cot(pi / 4))
print(cos(pi))
print(tan(pi / 4))
```

Expected output:

```text
0
1
1
1
-1
1
```

For `csc(x)`, `sec(x)`, `cot(x)`, and `tan(x)`, values near singularities such as `k*pi` for `csc(x)` and `cot(x)`, `pi / 2 + k*pi` for `sec(x)`, or `pi / 2 + k*pi` for `tan(x)` can become very large. Graphion currently follows ordinary floating-point behavior here rather than raising a special error.

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

### `acsc(x)`

`acsc(x)` returns the inverse cosecant in radians. It is implemented as `asin(1 / x)`.

```gion
print(acsc(1))
print(acsc(2))
print(acsc(-2))
```

Expected output:

```text
1.5708
0.523599
-0.523599
```

Domain restriction:

- `x` must satisfy `x <= -1` or `x >= 1`

Current domain error:

```text
acsc requires input <= -1 or >= 1
```

### `asec(x)`

`asec(x)` returns the inverse secant in radians. It is implemented as `acos(1 / x)`.

```gion
print(asec(1))
print(asec(2))
print(asec(-2))
```

Expected output:

```text
0
1.0472
2.0944
```

Domain restriction:

- `x` must satisfy `x <= -1` or `x >= 1`

Current domain error:

```text
asec requires input <= -1 or >= 1
```

### `acot(x)`

`acot(x)` returns the inverse cotangent in radians. It is implemented as `atan2(1, x)`, which keeps `acot(0)` well-defined and uses the principal branch in `(0, pi)`.

```gion
print(acot(1))
print(acot(0))
print(acot(-1))
```

Expected output:

```text
0.785398
1.5708
2.35619
```

### `sech(x)`

`sech(x)` returns the hyperbolic secant. It is implemented as `1 / cosh(x)`, so `sech(0)` is exactly `1`, and large-magnitude inputs shrink toward `0`.

```gion
print(sech(0))
print(sech(1))
print(sech(-1))
```

Expected output:

```text
1
0.648054
0.648054
```

### `csch(x)`

`csch(x)` returns the hyperbolic cosecant. It is implemented as `1 / sinh(x)`, so positive and negative inputs preserve their sign and larger magnitudes move toward `0`.

```gion
print(csch(1))
print(csch(-1))
print(csch(2))
```

Expected output:

```text
0.850918
-0.850918
0.275721
```

### `coth(x)`

`coth(x)` returns the hyperbolic cotangent. It is implemented as `1 / tanh(x)`, so positive and negative inputs preserve their sign while larger magnitudes approach `1` or `-1`.

```gion
print(coth(1))
print(coth(-1))
print(coth(2))
```

Expected output:

```text
1.313035
-1.313035
1.037315
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

### `copysign(x, y)`

Returns the magnitude of `x` with the sign of `y`.

```gion
print(copysign(3, -2))
print(copysign(-3.5, 2))
```

```text
-3
3.5
```

### `fma(a, b, c)`

Returns `a * b + c` as a single numeric builtin.

```gion
print(fma(2, 3, 4))
print(fma(0.5, 8, -1))
```

```text
10
3
```

### `fdim(x, y)`

Returns the positive difference between `x` and `y`.

```gion
print(fdim(7, 3))
print(fdim(3, 7))
```

```text
4
0
```

### `remainder(x, y)`

Returns the IEEE-style remainder of `x` divided by `y`. This is not the same operation as `%`: it uses the nearest integer quotient instead of truncation.

```gion
print(remainder(7, 4))
print(remainder(5.5, 2))
```

```text
-1
-0.5
```

### `rint(x)`

Returns the nearest integer-valued `float` using the platform's active floating-point rounding mode.

This is a lower-level builtin than `round(x)`:

- `round(x)` is the predictable language-level choice
- `rint(x)` follows the C math runtime and can depend on platform rounding mode

```gion
print(rint(7))
print(rint(7.4))
print(rint(-3.2))
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

### `sinh(x)`, `csch(x)`, `asinh(x)`, `acosh(x)`, `cosh(x)`, `sech(x)`, `tanh(x)`, `coth(x)`, `atanh(x)`

```gion
print(sinh(1))
print(csch(1))
print(asinh(1))
print(acosh(2))
print(cosh(1))
print(sech(1))
print(tanh(1))
print(coth(1))
print(atanh(0.5))
```

Expected output:

```text
1.1752
0.850918
0.881374
1.31696
1.54308
0.648054
0.761594
1.313035
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
- use `rint(x)` only if you explicitly want the platform floating-point rounding behavior

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

Returns the length of a string, list, dict, tuple, or set expression.

```gion
print(len("graphion"))
print(len([1, 2, 3]))
print(len({"a": 1, "b": 2}))
print(len((1, 2)))
print(len(set(1, 2, 2)))
```

Expected output:

```text
8
3
2
2
2
```

Accepted input:

- string expressions
- list expressions
- dict expressions
- tuple expressions
- set expressions

Result type:

- `int`

### `contains(set, value)`

Returns whether a set contains a value.

```gion
frontier = set(1, 2, "a")
print(contains(frontier, 2))
print(contains(frontier, 3))
```

Expected output:

```text
true
false
```

Accepted input:

- first argument: `set`
- second argument: any currently comparable value

Result type:

- `bool`

## Graphs

### `node_count(graph)`

Returns the number of logical nodes currently present in a graph.

```gion
graph G:
    1 - 2
    3 - 2

print(node_count(G))
```

Expected output:

```text
3
```

Accepted input:

- `graph`

Result type:

- `int`

### `edge_count(graph)`

Returns the number of logical edges currently present in a graph. Bidirectional edges count as one logical edge.

```gion
graph G:
    1 -> 2
    3 <-> 4

print(edge_count(G))
```

Expected output:

```text
2
```

Accepted input:

- `graph`

Result type:

- `int`

### `is_directed(graph)`

Returns whether the graph uses directed edge syntax.

```gion
graph G:
    1 -> 2

print(is_directed(G))
```

Expected output:

```text
true
```

Accepted input:

- `graph`

Result type:

- `bool`

### `is_weighted(graph)`

Returns whether at least one logical edge has the reserved `weight` attribute. This includes weights provided through `defaults edge`.

```gion
graph G:
    defaults edge {"weight": 1}
    1 - 2

print(is_weighted(G))
```

Expected output:

```text
true
```

Accepted input:

- `graph`

Result type:

- `bool`

### `orientation(graph)`

Returns a string describing the graph's current global orientation state.

```gion
graph G:
    1 - 2

print(orientation(G))
```

Expected output:

```text
undirected
```

Current return values:

- `empty` when the graph has no edges
- `undirected` when the graph uses `node - node` edges
- `directed` when the graph uses `node -> node` or `node <-> node` edges

Accepted input:

- `graph`

Result type:

- `string`

### `node_ids(graph)`

Returns a list of present numeric node IDs.

```gion
graph G:
    "Alice"
    10

print(node_ids(G))
```

Expected output:

```text
[0, 10]
```

Accepted input:

- `graph`

Result type:

- `list` of `int` node IDs

### `nodes(graph)`

Returns a list of node descriptor dictionaries. Named nodes include `name`; numeric-only nodes only include `id`.

```gion
graph G:
    "Alice"
    10

print(nodes(G))
```

Expected output:

```text
[{"id": 0, "name": "Alice"}, {"id": 10}]
```

Accepted input:

- `graph`

Result type:

- `list` of `dict`

### `edges(graph)`

Returns a list of logical edge descriptor dictionaries with `from`, `to`, `directed`, and `bidirectional`.

```gion
graph G:
    1 -> 2
    2 <-> 3

print(edges(G))
```

Expected output:

```text
[{"from": 1, "to": 2, "directed": true, "bidirectional": false}, {"from": 2, "to": 3, "directed": true, "bidirectional": true}]
```

Accepted input:

- `graph`

Result type:

- `list` of `dict`

### `node_attrs(graph, node)`

Returns the attribute dictionary attached to a node. The `node` argument can be either an integer node ID or a string node name.

```gion
alice = "Alice"

graph G:
    defaults node {"label": "unknown", "score": 0}
    alice {"label": "start"}

print(node_attrs(G, alice)["label"])
```

Expected output:

```text
start
```

Accepted input:

- first argument: `graph`
- second argument: `int` node ID or `string` node name

Result type:

- `dict`

### `edge_attrs(graph, from, to)`

Returns the attribute dictionary attached to one logical edge. For undirected edges and `<->` edges, the lookup works in either endpoint order.

```gion
graph G:
    defaults edge {"kind": "normal", "weight": 1}
    1 - 2 {"weight": 3}

print(edge_attrs(G, 2, 1)["kind"])
```

Expected output:

```text
normal
```

Accepted input:

- first argument: `graph`
- second and third arguments: `int` node IDs or `string` node names

Result type:

- `dict`

### `edge_weight(graph, from, to)`

Returns the reserved `weight` attribute for one logical edge. This is equivalent to `edge_attrs(graph, from, to)["weight"]`, but makes weighted graph code easier to read.

```gion
graph G:
    1 - 2 15

print(edge_weight(G, 1, 2))
```

Expected output:

```text
15
```

Accepted input:

- first argument: `graph`
- second and third arguments: `int` node IDs or `string` node names

Result type:

- `int` or `float`

Current runtime notes:

- missing nodes are runtime errors
- missing edges are runtime errors
- an existing edge without `weight` is a missing-key runtime error

### `hyperedge_vertices(hypergraph, id)`

Returns the vertex IDs contained in one hyperedge. Hyperedge IDs are implicit numeric IDs assigned in declaration order, starting at `0`.

```gion
hypergraph H:
    ["Alice", "Bob", 2]

print(hyperedge_vertices(H, 0))
```

Expected output:

```text
[0, 1, 2]
```

Accepted input:

- first argument: `hypergraph`
- second argument: `int` hyperedge ID

Result type:

- `list`

### `hyperedge_attrs(hypergraph, id)`

Returns the attribute dictionary attached to one hyperedge. Missing hyperedge attributes return an empty dict.

```gion
hypergraph H:
    defaults hyperedge {"kind": "group"}
    ["Alice", "Bob"] {"kind": "team"}

print(hyperedge_attrs(H, 0)["kind"])
```

Expected output:

```text
team
```

Accepted input:

- first argument: `hypergraph`
- second argument: `int` hyperedge ID

Result type:

- `dict`

### `has_node(graph, node)`

Returns whether a node exists in a graph. The `node` argument can be either an integer node ID or a string node name.

```gion
graph G:
    "Alice"
    2

print(has_node(G, "Alice"))
print(has_node(G, 99))
```

Expected output:

```text
true
false
```

Accepted input:

- first argument: `graph`
- second argument: `int` node ID or `string` node name

Result type:

- `bool`

### `has_edge(graph, from, to)`

Returns whether one logical edge exists between two nodes. Directed `->` edges only match in their declared direction. Undirected `-` edges and bidirectional `<->` edges match in both endpoint orders.

```gion
graph G:
    1 -> 2
    3 <-> 4

print(has_edge(G, 1, 2))
print(has_edge(G, 2, 1))
print(has_edge(G, 4, 3))
```

Expected output:

```text
true
false
true
```

Accepted input:

- first argument: `graph`
- second and third arguments: `int` node IDs or `string` node names

Result type:

- `bool`

### `neighbors(graph, node)`

Returns all adjacent neighbor node IDs.

For undirected graphs, neighbors are the usual adjacent nodes. For directed graphs, `neighbors(graph, node)` includes both incoming and outgoing adjacency. Use `indegree(...)` and `outdegree(...)` when the direction matters.

```gion
graph G:
    1 -> 2
    3 -> 2

print(neighbors(G, 2))
```

Expected output:

```text
[1, 3]
```

Accepted input:

- first argument: `graph`
- second argument: `int` node ID or `string` node name

Result type:

- `list` of `int` node IDs

### `indegree(graph, node)`

Returns the incoming neighbor node IDs for a node. Undirected `-` and bidirectional `<->` edges count once as incoming for each endpoint. Use `len(indegree(graph, node))` when you need the count.

```gion
graph G:
    1 -> 2
    3 <-> 2

print(indegree(G, 2))
print(len(indegree(G, 2)))
```

Expected output:

```text
[1, 3]
2
```

Accepted input:

- first argument: `graph`
- second argument: `int` node ID or `string` node name

Result type:

- `list` of `int` node IDs

### `outdegree(graph, node)`

Returns the outgoing neighbor node IDs for a node. Undirected `-` and bidirectional `<->` edges count once as outgoing for each endpoint. Use `len(outdegree(graph, node))` when you need the count.

```gion
graph G:
    1 -> 2
    2 <-> 3

print(outdegree(G, 2))
print(len(outdegree(G, 2)))
```

Expected output:

```text
[3]
1
```

Accepted input:

- first argument: `graph`
- second argument: `int` node ID or `string` node name

Result type:

- `list` of `int` node IDs

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
