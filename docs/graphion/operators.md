# Operators

This page documents the operators currently implemented in Graphion.

Use it when you need:

- the available operators
- precedence and grouping rules
- current `bits` operator behavior
- current string concatenation rules
- current boolean logic and comparison rules

For statements, indentation rules, comments, and the overall source structure, see [Language Reference](language-reference.md).

## Arithmetic Operators

Supported arithmetic operators:

- `+`
- `-`
- `*`
- `/`
- `//`
- `%`
- `**`
- postfix `!`

### Operator Notes

`/`
: division, produces a float result

`//`
: floor division

`%`
: modulo

`**`
: power, right-associative

`!`
: postfix factorial, valid for non-negative integers only

Unary minus is also supported on variables and grouped expressions, for example:

```gion
count = 5
neg_count = -count
neg_group = -(1 + 2)
```

Factorial is also supported as a postfix operator:

```gion
count = 5!
grouped = (1 + 2)!
```

These are currently runtime errors:

- `(-1)!`
- `1.5!`

## Comparison Operators

Currently supported comparison operators:

- `==`
- `!=`
- `<`
- `<=`
- `>`
- `>=`

`==`, `!=`, `<`, `<=`, `>`, and `>=` return a `bool`.

Examples:

```gion
same_int = 42 == 42
same_number = 42 == 42.0
same_bool_bridge = 1 == true
same_false_bridge = 0 == false
same_text = "ok" == "ok"
different_int = 42 != 41
different_text = "ok" != "no"
smaller_number = 2 < 3
same_or_smaller = 3 <= 3
greater_number = 4 > 3
same_or_greater = 4 >= 4
```

Current comparison semantics:

- `int == int`
- `int == float`
- `float == float`
- `int == bool` only when the integer is `0` or `1`
- `bool == int` only when the integer is `0` or `1`
- `bool == bool`
- `string == string`

`1 == true`, `true == 1`, `0 == false`, and `false == 0` currently return `true`.

These are currently runtime errors:

- `2 == true`
- `true == 2`
- `1.0 == true`
- `"1" == 1`
- `"true" == true`

`!=` follows the same type rules as `==`, but negates the final boolean result.

`<`, `<=`, `>`, and `>=` currently support numeric comparisons only:

- `int < int`
- `int < float`
- `float < float`

`<=`, `>`, and `>=` follow the same numeric-only rule.

Using `<`, `<=`, `>`, or `>=` with `bool` or `string` currently raises a runtime error.

## Boolean Logic

Currently supported boolean logic operators:

- `and`
- `nand`
- `or`
- `nor`
- `not`

`and`, `nand`, `or`, `nor`, and `not` return a `bool`.

### Truth Rules

Graphion currently uses a strict boolean subset for boolean logic and conditions:

- `true` is true
- `false` is false
- integer `1` is treated as true
- integer `0` is treated as false
- other integers are rejected
- `float` values are rejected
- `string` values are rejected

This rule currently applies to:

- `if` / `elif` conditions
- ternary conditions
- `not`
- `and` / `nand`
- `or` / `nor`

Examples:

```gion
if 1:
    print("true")

if 0:
    print("bad")
else:
    print("false")

print(not 1)
print(false nor 0)
```

#### Truth Tables

`and`

| left | right | result |
| --- | --- | --- |
| `true` | `true` | `true` |
| `true` | `false` | `false` |
| `false` | `true` | `false` |
| `false` | `false` | `false` |

`nand`

| left | right | result |
| --- | --- | --- |
| `true` | `true` | `false` |
| `true` | `false` | `true` |
| `false` | `true` | `true` |
| `false` | `false` | `true` |

`or`

| left | right | result |
| --- | --- | --- |
| `true` | `true` | `true` |
| `true` | `false` | `true` |
| `false` | `true` | `true` |
| `false` | `false` | `false` |

`nor`

| left | right | result |
| --- | --- | --- |
| `true` | `true` | `false` |
| `true` | `false` | `false` |
| `false` | `true` | `false` |
| `false` | `false` | `true` |

`not`

| value | result |
| --- | --- |
| `true` | `false` |
| `false` | `true` |

Examples:

```gion
both_true = true and true
bridge_true = 1 and true
bridge_false = false and 1
all_ready = true and 1 and 2 < 3
not_both_ready = true nand 1
any_ready = false or 1
none_ready = false nor 0
any_path = false or 1 == 1 or false
inverted_ready = not false
inverted_path = not (false or 0)
```

`and` can be chained multiple times.

This:

```gion
true and 1 and 2 < 3
```

is currently evaluated left to right as repeated `and` operations, with comparisons evaluated before `and`.

`nand` currently follows the same precedence and type rules as `and`, but inverts the final boolean result.

`or` can also be chained multiple times.

This:

```gion
false or 1 == 1 or false
```

is currently evaluated left to right as repeated `or` operations, with comparisons evaluated before `or`.

`nor` currently follows the same precedence and type rules as `or`, but inverts the final boolean result.

When `and` / `nand` and `or` / `nor` are mixed, `and` / `nand` currently bind tighter than `or` / `nor`.

So:

```gion
true or true nand true
```

is currently interpreted as:

```gion
true or (true nand true)
```

`not` currently binds tighter than both `and` and `or`.

```gion
not false and false
```

is currently interpreted as:

```gion
(not false) and false
```

Current `and` rules:

- `bool and bool`
- `int and bool` only when the integer is `0` or `1`
- `bool and int` only when the integer is `0` or `1`
- `int and int` only when both integers are `0` or `1`

These are currently runtime errors when the invalid side must actually be evaluated:

- `2 and true`
- `true and 2`
- `1.0 and true`
- `"x" and true`

These currently succeed because the left side short-circuits first:

- `false and 2`
- `0 and "x"`

`or` currently follows the same type rules as `and`, but returns true when either side is true.

`nand` currently follows the same type rules as `and`, but returns the negation of `and`.

These are currently runtime errors when the invalid side must actually be evaluated:

- `2 nand true`
- `true nand 2`
- `1.0 nand true`
- `"x" nand true`

These currently succeed because the left side short-circuits first:

- `false nand 2`
- `0 nand "x"`

These are currently runtime errors when the invalid side must actually be evaluated:

- `2 or true`
- `false or 2`
- `1.0 or true`
- `"x" or true`

These currently succeed because the left side short-circuits first:

- `true or 2`
- `1 or "x"`

`nor` currently follows the same type rules as `or`, but returns the negation of `or`.

These are currently runtime errors when the invalid side must actually be evaluated:

- `2 nor false`
- `false nor 2`
- `1.0 nor false`
- `"x" nor false`

These currently succeed because the left side short-circuits first:

- `true nor 2`
- `1 nor "x"`

`not` currently accepts:

- `bool`
- integer `0` / `1`

These are currently runtime errors:

- `not 2`
- `not 1.0`
- `not "x"`

Current evaluation note:

- `not` evaluates its operand and negates the resulting boolean value
- `and` short-circuits when the left side is `false` or `0`
- `nand` short-circuits when the left side is `false` or `0`
- `or` short-circuits when the left side is `true` or `1`
- `nor` short-circuits when the left side is `true` or `1`

## Precedence And Grouping

Current precedence order:

1. grouped expressions with parentheses
2. `abs(...)`, `min(...)`, `max(...)`, `clamp(...)`, `sqrt(...)`, `cbrt(...)`, `sin(...)`, `csc(...)`, `sec(...)`, `cot(...)`, `sinh(...)`, `asinh(...)`, `acosh(...)`, `cos(...)`, `cosh(...)`, `tan(...)`, `tanh(...)`, `atanh(...)`, `asin(...)`, `acos(...)`, `atan(...)`, `atan2(...)`, `hypot(...)`, `copysign(...)`, `fma(...)`, `fdim(...)`, `remainder(...)`, `rint(...)`, `exp(...)`, `exp2(...)`, `expm1(...)`, `log1p(...)`, `erf(...)`, `erfc(...)`, `gamma(...)`, `lgamma(...)`, `ln(...)`, `log(...)`, `log10(...)`, `log2(...)`, `floor(...)`, `ceil(...)`, `round(...)`, `trunc(...)`, `sign(...)`, `len(...)`
3. postfix factorial `!`
4. `**`
5. `*`, `/`, `//`, `%`
6. `+`, `-`
7. `==`, `!=`, `<`, `<=`, `>`, `>=`
8. `not`
9. `and`, `nand`
10. `or`, `nor`

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

And:

```gion
value = 2 ** 3!
```

is interpreted as:

```gion
value = 2 ** (3!)
```

### Parentheses

Parentheses currently group arithmetic expressions:

```gion
grouped = (1 + 2) * 3
```

This is the current implemented behavior.

Tuple syntax is not part of the current documented language subset.

## Bits Operators

Current `bits` support includes:

- literals written as `0b...`
- preserved width from literal spelling
- `==`
- `!=`
- `&`
- `|`
- `^`
- `~`
- `<<` with an integer shift count
- `>>` with an integer shift count
- `&=` with another `bits` value of the same stored width
- `|=` with another `bits` value of the same stored width
- `^=` with another `bits` value of the same stored width
- `<<=` with a non-negative `int` shift count
- `>>=` with a non-negative `int` shift count

Examples:

```gion
short_bits = 0b10
wide_bits = 0b0010
same_bits = 0b10 == 0b0010
shifted_bits = 0b0011 << 1
right_shifted_bits = 0b1010 >> 1
```

Current behavior:

- `0b10` and `0b0010` print differently because width is preserved
- `==` and `!=` compare normalized values, so leading zeroes do not affect equality
- `&`, `|`, and `^` require both operands to be `bits`
- `&`, `|`, and `^` currently require matching widths
- `&=` follows the same width and type rules as `&`
- `|=` follows the same width and type rules as `|`
- `^=` follows the same width and type rules as `^`
- `<<=` follows the same width, truncation, and shift-count rules as `<<`
- `>>=` follows the same width, zero-fill, and shift-count rules as `>>`
- `~` keeps the stored width
- `<<` takes a `bits` value on the left and a non-negative `int` shift count on the right
- `<<` keeps the stored width and truncates overflow back to that width
- `>>` takes a `bits` value on the left and a non-negative `int` shift count on the right
- `>>` keeps the stored width and shifts in zeroes from the left

Current precedence for `bits` operators:

1. grouping parentheses
2. `~`
3. `+` / `-` inside shift counts
4. `<<` / `>>`
5. `&`
6. `|` / `^`
7. comparisons

Examples:

```gion
~0b0011 & 0b1111
```

is read as:

```gion
(~0b0011) & 0b1111
```

```gion
0b0011 << 1 + 1
```

is read as:

```gion
0b0011 << (1 + 1)
```

```gion
0b1111 >> 1 & 0b0111
```

is read as:

```gion
(0b1111 >> 1) & 0b0111
```

Current restrictions:

- non-bitwise arithmetic on `bits` is rejected:
  - `+`
  - `-`
  - `*`
  - `/`
  - `//`
  - `%`
  - `**`
- ordered comparisons on `bits` are rejected:
  - `<`
  - `<=`
  - `>`
  - `>=`
- boolean logic on `bits` is rejected:
  - `and`
  - `nand`
  - `or`
  - `nor`
  - `not`
- direct use of `bits` as an `if` condition is rejected
- direct use of `bits` as a ternary condition is rejected

Examples:

```gion
0b1100 & 0b1010
```

produces:

```text
0b1000
```

```gion
0b0011 << 1
```

produces:

```text
0b0110
```

```gion
0b1111 << 1
```

produces:

```text
0b1110
```

```gion
0b1010 >> 1
```

produces:

```text
0b0101
```

These are currently runtime errors:

- `0b10 & 0b0010`
- `mask = 0b10` followed by `mask &= 0b0010`
- `merge = 0b10` followed by `merge |= 0b0010`
- `flip = 0b10` followed by `flip ^= 0b0010`
- `shift = 0b10` followed by `shift <<= 0b0010`
- `shift = 0b10` followed by `shift <<= 1.0`
- `shift = 0b10` followed by `shift <<= -1`
- `shift = 0b10` followed by `shift >>= 0b0010`
- `shift = 0b10` followed by `shift >>= 1.0`
- `shift = 0b10` followed by `shift >>= -1`
- `0b10 | 0b0010`
- `0b10 ^ 0b0010`
- `0b10 << 0b0010`
- `0b10 << 1.0`
- `0b10 << -1`
- `0b10 >> 0b0010`
- `0b10 >> 1.0`
- `0b10 >> -1`
- `0b10 + 1`
- `0b10 < 0b0010`
- `0b10 and true`
- `if 0b10:`

Error wording:

- invalid `bits` operators currently report `incompatible operand types`
- invalid direct `if` conditions currently report `if condition must be boolean or 0/1`

## Strings And `print(...)`

### Strings

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

### `print(...)`

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
