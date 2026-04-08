# Language Reference

This reference describes the currently implemented `.gion` language subset.

Use this page when you need exact rules rather than a guided introduction.

Quick orientation:

- source structure: statements, names, assignment
- control flow: `if`, ternary expressions, `match`
- expressions and operators: see [Operators](operators.md)
- runtime library: builtins and constants
- errors: current high-level user-visible error classes

## Source Structure

### Statements

Current supported top-level statements:

- assignment
- compound assignment
- `if` / `elif` / `else`
- `match`
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

### Values

Graphion currently exposes scalar values only.

For the current scalar types, literal forms, and built-in numeric constants, see [Types](types.md).

### Identifiers

Identifiers:

- must start with a letter or `_`
- can then contain letters, digits, or `_`

Examples:

- `count`
- `_tmp`
- `alpha_1`

### Reserved Names

These names are currently reserved and cannot be assigned:

- `print`
- `true`
- `false`
- `pi`
- `e`
- `abs`
- `min`
- `max`
- `clamp`
- `sqrt`
- `cbrt`
- `sin`
- `sinh`
- `cosh`
- `cos`
- `tan`
- `tanh`
- `asin`
- `acos`
- `atan`
- `atan2`
- `hypot`
- `exp`
- `ln`
- `log`
- `log10`
- `log2`
- `floor`
- `ceil`
- `round`
- `trunc`
- `sign`
- `len`
- `if`
- `elif`
- `else`
- `and`
- `nand`
- `or`
- `nor`
- `not`
- `match`
- `default`

### Literals

Graphion currently supports literals for:

- `int`
- `float`
- `bool`
- `string`
- `bits`

For the exact literal forms and examples, see [Types](types.md).

### Assignment

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

## Control Flow

### Conditional Statements

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

#### Condition Rules

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

#### Block Rules

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

#### Nested Conditionals

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

#### Multiline Conditions

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

#### Ternary Expressions

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

#### Reading Tips

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

#### Conditional Precedence

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

#### Match Blocks

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

## Comments And Directives

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

## Expressions And Operators

See [Operators](operators.md) for:

- arithmetic operators
- comparisons
- boolean logic
- precedence and grouping
- `bits` operator semantics
- string concatenation and `print(...)`

## Runtime Library

### Builtins

Current builtin functions:

- `abs(...)`
- `min(a, b)`
- `max(a, b)`
- `clamp(x, lo, hi)`
- `sqrt(x)`
- `cbrt(x)`
- `sin(x)`
- `sinh(x)`
- `cosh(x)`
- `cos(x)`
- `tan(x)`
- `tanh(x)`
- `asin(x)`
- `acos(x)`
- `atan(x)`
- `atan2(y, x)`
- `hypot(x, y)`
- `exp(x)`
- `ln(x)`
- `log(x, base)`
- `log10(x)`
- `log2(x)`
- `floor(x)`
- `ceil(x)`
- `round(x)`
- `trunc(x)`
- `sign(x)`
- `len(x)`

See [Builtins](builtins.md).

### Constants

Current built-in scalar constants:

- `pi`
- `e`

See [Types](types.md) for their current values and basic usage.

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
