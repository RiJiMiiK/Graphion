# Language Reference

This reference describes the currently implemented `.gion` language subset.

## Statements

Current supported top-level statements:

- assignment
- compound assignment
- `if` / `elif` / `else`
- `print(...)`

Examples:

```gion
count = 42
count += 1

if true:
    count += 1
else:
    count -= 1

print(count)
```

Unsupported statements are parse errors in the current `.gion` frontend path.

## Values

Current scalar value kinds:

- `int`
- `float`
- `bool`
- `string`
- `bits`

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
- `if`
- `elif`
- `else`
- `and`
- `or`
- `not`
- `match`
- `default`

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

## Conditional Statements

Graphion currently supports Python-style conditional blocks:

- `if`
- `elif`
- `else`

Valid shapes:

- `if`
- `if` + one or more `elif`
- `if` + `else`
- `if` + one or more `elif` + `else`

Examples:

```gion
ready = true
fallback = false

if ready:
    message = "ready"
elif fallback:
    message = "fallback"
else:
    message = "other"
```

### Condition Rules

Current conditions must evaluate to:

- `true`
- `false`
- `1`
- `0`
- an expression that evaluates to `bool`, such as `1 + 1 == 2`

Valid condition examples:

```gion
flag = true

if flag:
    print("ok")
```

```gion
if false:
    print("never")
else:
    print("taken")
```

```gion
if 1:
    print("taken")

if 0:
    print("never")
```

```gion
if 1 + 1 == 2:
    print("taken")
```

Invalid examples:

```gion
if 2:
    print("bad")
```

```gion
if "x":
    print("bad")
```

These currently fail with:

`if condition must be boolean or 0/1`
: runtime error for conditions outside the current accepted boolean subset

### Block Rules

Conditional blocks currently use indentation-significant syntax.

Rules:

- a block line must end with `:`
- the following block must be indented
- `elif` can appear multiple times
- `else` is optional
- `else` must be last
- nested `if` blocks are allowed
- an `else` always binds to the `if` at the same indentation level
- indentation decides the binding, not the nearest visible `if` token on the page

Invalid examples:

```gion
if true
    print("bad")
```

```gion
if true:
print("bad")
```

```gion
else:
    print("bad")
```

### Nested Conditionals

You can place an `if` block inside another `if` block.

```gion
ready = true
admin = false

if ready:
    if admin:
        label = "admin"
    else:
        label = "user"
else:
    label = "offline"
```

In this example:

- the inner `else` binds to `if admin:`
- the outer `else` binds to `if ready:`

That binding is determined by indentation, not by the nearest visible `if` keyword alone.

### Multiline Conditions

Long conditions can span multiple physical lines, but only when the full condition is wrapped in grouping parentheses.

```gion
if (
    ready and
    has_token and
    level >= 3 and
    not blocked
):
    print("ok")
```

Rules:

- multiline conditions require outer grouping parentheses
- the closing `)` must appear before the final `:`
- line breaks inside the grouped condition are treated like spaces
- multiline conditions without grouping parentheses are invalid

Invalid example:

```gion
if ready and
   has_token:
    print("bad")
```

### Ternary Expressions

Graphion also supports inline conditional expressions in the form:

```gion
result = "ready" if ready else "not ready"
```

Long ternary expressions can also span multiple physical lines when the whole expression is wrapped in grouping parentheses:

```gion
label = (
    "ready"
    if ready
    else "not ready"
)
```

Rules:

- the shape is `true_value if condition else false_value`
- the condition follows the same truth rules as `if` / `elif`
- the whole ternary expression produces a single scalar value
- nested ternary expressions are allowed, but become harder to read quickly
- multiline ternary expressions require outer grouping parentheses

Examples:

```gion
label = "ready" if ready else "not ready"
```

```gion
label = "outer" if ready else "inner" if fallback else "none"
```

Invalid example:

```gion
label = "ready"
if ready
else "not ready"
```

### Reading Tips

To keep conditions and ternary expressions readable:

- prefer grouping parentheses when mixing several boolean operators
- prefer multiline grouped conditions once a single line starts to feel dense
- keep nested ternary expressions short
- switch back to a full `if` / `elif` / `else` block when the ternary stops being immediately obvious

Examples:

```gion
if (ready and has_token) or fallback:
    print("ok")
```

```gion
label = "ready" if ready else "not ready"
```

```gion
label = "outer" if ready else "inner" if fallback else "none"
```

The last form is valid, but a block is usually easier to read once nested ternary logic grows.

### Conditional Precedence

When several conditional operators appear in the same expression, Graphion reads them in this order:

| Level | Operators | Notes |
| --- | --- | --- |
| 1 | `(...)` | Explicit grouping always wins |
| 2 | `==`, `!=`, `<`, `<=`, `>`, `>=` | Comparisons produce boolean results |
| 3 | `not` | Unary boolean negation |
| 4 | `and`, `nand` | `nand` shares the same level as `and` |
| 5 | `or`, `nor` | `nor` shares the same level as `or` |
| 6 | `value_if_true if condition else value_if_false` | Ternary expressions bind last among conditional forms |

Examples:

```gion
true or false and false
```

is read as:

```gion
true or (false and false)
```

```gion
not 1 == 2 and true
```

is read as:

```gion
(not (1 == 2)) and true
```

```gion
"ready" if true or false and false else "fallback"
```

is read as:

```gion
"ready" if (true or (false and false)) else "fallback"
```

### Match Blocks

Graphion also supports value-based branching with `match`:

```gion
match status:
    "ready":
        print("go")
    "waiting":
        print("hold")
    default:
        print("unknown")
```

Rules:

- `match` is a statement, not an expression
- each non-`default` branch starts with a scalar literal followed by `:`
- supported case literals are `int`, `float`, `bool`, and `string`
- `default:` is optional, may appear at most once, and must be last
- grouped cases are allowed by stacking labels above the same block
- the matched expression is evaluated once, then branches are tested from top to bottom
- the first matching branch wins
- incompatible case types do not raise an error during execution; they simply do not match
- if the matched expression is a scalar literal and a case can never match it, Graphion emits a pre-execution warning unless warnings are disabled

Grouped cases:

```gion
match level:
    1:
    2:
        print("small")
    default:
        print("other")
```

Invalid examples:

```gion
match value:
    default:
        print("x")
    1:
        print("y")
```

```gion
match value:
    1:
        print("x")
    1.0:
        print("y")
```

## Comments

Graphion currently supports two comment forms:

- `#` for line comments
- `/* ... */` for block comments

At the top of a file, `#` also supports a reserved Graphion directive form:

```gion
# graphion: warnings=off
```

When this directive appears before the first real statement, pre-execution warnings are suppressed for the file. It
does not suppress parse errors or runtime errors.

### Line Comments

`#` ignores the rest of the current line.

Examples:

```gion
# full-line comment
count = 40 # inline comment
count += 2
```

### Block Comments

`/* ... */` ignores everything until the closing `*/`.

Examples:

```gion
/*
multi-line note
about the next assignment
*/
message = "graphion"
```

```gion
ratio = /* inline block comment */ 7 / 2
```

Current rules:

- block comments can span multiple lines
- block comments are not nested
- comment markers inside string literals are treated as string text

Example:

```gion
message = "/* not a comment */"
```

### Comment Errors

This is currently a parse error:

```gion
/*
unterminated comment
count = 42
```

The current diagnostic is:

`unterminated block comment`
: parse error when `/*` does not have a matching closing `*/`

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

Unary minus is also supported on variables and grouped expressions, for example:

```gion
count = 5
neg_count = -count
neg_group = -(1 + 2)
```

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

## Truth Rules

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

### Truth Tables

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

print(both_true)
print(bridge_true)
print(bridge_false)
print(all_ready)
print(not_both_ready)
print(any_ready)
print(none_ready)
print(any_path)
print(inverted_ready)
print(inverted_path)
```

`and` can be chained multiple times.

This:

```gion
true and 1 and 2 < 3
```

is currently evaluated left to right as repeated `and` operations, with comparisons evaluated before `and`.

`nand` currently follows the same precedence and type rules as `and`, but inverts the final boolean result.

```gion
true nand 1
```

currently evaluates to:

```gion
false
```

`or` can also be chained multiple times.

This:

```gion
false or 1 == 1 or false
```

is currently evaluated left to right as repeated `or` operations, with comparisons evaluated before `or`.

`nor` currently follows the same precedence and type rules as `or`, but inverts the final boolean result.

```gion
false nor 0
```

currently evaluates to:

```gion
true
```

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

## Precedence

Current precedence order:

1. grouped expressions with parentheses
2. `abs(...)`
3. `**`
4. `*`, `/`, `//`, `%`
5. `+`, `-`
6. `==`, `!=`, `<`, `<=`, `>`, `>=`
7. `not`
8. `and`, `nand`
9. `or`, `nor`

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

## Bits

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

```gion
0b1000
```

```gion
0b0011 << 1
```

produces:

```gion
0b0110
```

```gion
0b1111 << 1
```

produces:

```gion
0b1110
```

```gion
0b1010 >> 1
```

produces:

```gion
0b0101
```

These are currently runtime errors:

- `0b10 & 0b0010`
- `mask = 0b10` followed by `mask &= 0b0010`
- `merge = 0b10` followed by `merge |= 0b0010`
- `flip = 0b10` followed by `flip ^= 0b0010`
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

`incompatible operand types`
: runtime type error for numeric operators
