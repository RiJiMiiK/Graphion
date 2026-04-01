# Tutorial

This tutorial introduces the currently supported Graphion syntax.

## Your First Script

```gion
count = 42
name = "graphion"
ready = true

print(count)
print(name)
print(ready)
```

Expected output:

```text
42
graphion
true
```

## Variables

Graphion currently supports scalar assignments.

```gion
count = 42
ratio = 3.5
name = "graphion"
ready = true
```

You can also copy a variable:

```gion
count = 42
copy = count

print(copy)
```

Expected output:

```text
42
```

## Arithmetic

Supported arithmetic operators:

- `+`
- `-`
- `*`
- `/`
- `//`
- `%`
- `**`

Example:

```gion
sum = 40 + 2
mixed = 1 + 2 * 3
grouped = (1 + 2) * 3
half = 7 / 2
floor_half = 7 // 2
power = 2 ** 3
remainder = 10 % 4

print(sum)
print(mixed)
print(grouped)
print(half)
print(floor_half)
print(power)
print(remainder)
```

Expected output:

```text
42
7
9
3.5
3
8
2
```

## Negative Numbers

Negative values work across arithmetic expressions.

```gion
negative_add = -5 + 2
negative_div = -7 / 2
negative_floor = -7 // 2
count = 5
neg_count = -count
neg_group = -(1 + 2)
negative_power = (-2) ** 3
negative_remainder = -10 % 4

print(negative_add)
print(negative_div)
print(negative_floor)
print(neg_count)
print(neg_group)
print(negative_power)
print(negative_remainder)
```

Expected output:

```text
-3
-3.5
-4
-5
-3
-8
-2
```

## Compound Assignments

Supported compound assignments:

- `+=`
- `-=`
- `*=`
- `/=`
- `//=`
- `%=`
- `**=`

Example:

```gion
count = 10
count += 5
count -= 3
count *= 2
count /= 4

print(count)
```

Expected output:

```text
6
```

## Equality

Graphion currently supports `==`, `!=`, numeric `<` / `<=` / `>` / `>=`, and boolean `and` / `or`.

It returns a boolean result:

```gion
same_int = 42 == 42
same_number = 42 == 42.0
same_bool_bridge = 1 == true
same_false_bridge = 0 == false
same_text = "graphion" == "graphion"
different_number = 42 != 41
different_text = "graphion" != "graph"
smaller_number = 2 < 3
same_or_smaller = 3 <= 3
greater_number = 4 > 3
same_or_greater = 4 >= 4

print(same_int)
print(same_number)
print(same_bool_bridge)
print(same_false_bridge)
print(same_text)
print(different_number)
print(different_text)
print(smaller_number)
print(same_or_smaller)
print(greater_number)
print(same_or_greater)
```

Expected output:

```text
true
true
true
true
true
true
true
true
true
true
true
```

Current behavior:

- `int == int` works
- `int == float` compares numerically
- `1 == true`, `true == 1`, `0 == false`, and `false == 0` work
- `bool == bool` works
- `string == string` works
- `int == bool` is only allowed when the integer is `0` or `1`
- `float == bool` raises a runtime error
- `string` compared to a non-`string` raises a runtime error
- `!=` follows the same type rules and flips the final result
- `<`, `<=`, `>`, and `>=` currently work only on numeric values

## Boolean Logic

Graphion currently supports `and`, `nand`, `or`, `nor`, and `not`.

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

Expected output:

```text
true
true
false
true
false
true
true
true
true
true
```

Current behavior:

- truth rules are strict:
  - `true` and `1` behave as true
  - `false` and `0` behave as false
  - other integers, floats, and strings are rejected in boolean logic
- `and` accepts `bool`
- `and` also accepts integer `0` / `1`
- `and` can be chained multiple times
- `nand` follows the same type rules as `and` and flips the final result
- `or` follows the same type rules and can also be chained
- `nor` follows the same type rules as `or` and flips the final result
- `not` follows the same type rules as a unary operator
- `not` currently binds tighter than `and` / `nand`, and `and` / `nand` bind tighter than `or` / `nor`
- `2 and true` is a runtime error
- `1.0 and true` is a runtime error
- `"x" and true` is a runtime error
- `2 nand true` is a runtime error
- `1.0 nand true` is a runtime error
- `"x" nand true` is a runtime error
- `2 or true` is a runtime error
- `1.0 or true` is a runtime error
- `"x" or true` is a runtime error
- `2 nor false` is a runtime error
- `1.0 nor false` is a runtime error
- `"x" nor false` is a runtime error
- `not 2` is a runtime error
- `not 1.0` is a runtime error
- `not "x"` is a runtime error
- `false and 2` succeeds because `and` short-circuits on the left
- `true or 2` succeeds because `or` short-circuits on the left
- `false nand 2` succeeds because `nand` short-circuits on the left
- `true nor 2` succeeds because `nor` short-circuits on the left

## Conditional Blocks

Graphion currently supports indentation-based `if / elif / else` with boolean conditions.

```gion
ready = true
fallback = false

if ready:
    print("ready branch")
elif fallback:
    print("fallback branch")
else:
    print("else branch")
```

Expected output:

```text
ready branch
```

You can also omit `elif` or `else`:

```gion
flag = false

if flag:
    print("taken")

print("after if")
```

Expected output:

```text
after if
```

Nested `if` blocks are also supported:

```gion
ready = true
admin = false

if ready:
    if admin:
        print("admin")
    else:
        print("user")
else:
    print("offline")
```

Expected output:

```text
user
```

The important rule is that each `else` attaches to the `if` at the same indentation level.
Indentation decides the binding, not just the nearest visible `if`.

You can also split a longer condition across multiple lines when you wrap it in grouping parentheses:

```gion
if (
    ready and
    has_token and
    level >= 3 and
    not blocked
):
    print("ok")
```

Without the outer parentheses, that multiline form remains invalid.

For simple value selection, you can also use a ternary expression:

```gion
label = "ready" if ready else "not ready"
print(label)
```

Expected output:

```text
ready
```

The ternary condition follows the same truth rules as `if` / `elif`.

If a ternary grows across multiple lines, wrap the whole expression in grouping parentheses:

```gion
label = (
    "ready"
    if ready
    else "not ready"
)
```

Without the outer parentheses, that multiline ternary form remains invalid.

As ternary expressions grow, readability drops quickly. A good rule of thumb is:

- keep simple ternaries on one line
- use grouped multiline conditions for long boolean tests
- switch back to a full block when the ternary starts nesting

For value-based branching, use `match`:

```gion
status = "ready"

match status:
    "waiting":
        print("hold")
    "ready":
        print("go")
    default:
        print("unknown")
```

You can also group several case labels so they share the same block:

```gion
match level:
    1:
    2:
        print("small")
    default:
        print("other")
```

The key rules are:

- cases are scalar literals only in this V1
- `default` is optional
- `default` must be last
- `1` and `1.0` are treated as duplicates because they compare equal in Graphion

When you mix several conditional operators, the practical reading order is:

- parentheses first
- then comparisons like `==` and `<`
- then `not`
- then `and` / `nand`
- then `or` / `nor`
- ternary expressions last

If an expression starts feeling dense, add parentheses instead of relying on memory.

Current conditions currently accept:

- `true`
- `false`
- `1`
- `0`

```gion
if true:
    print("ok")
```

```gion
if 1:
    print("also ok")
```

```gion
if 1 + 1 == 2:
    print("comparison conditions also work")
```

This is currently invalid:

```gion
if 2:
    print("bad")
```

## Comments

Graphion currently supports two comment styles:

- `#` for line comments
- `/* ... */` for block comments

At the top of a file, `#` also supports this Graphion directive:

```gion
# graphion: warnings=off
```

That directive suppresses pre-execution warnings for the file only. Parse errors and runtime errors still stop
execution.

Line comments can appear on their own line or after a statement:

```gion
# initialize the counter
count = 40 # base value
count += 2
```

Block comments can span multiple lines:

```gion
/*
this section demonstrates
multi-line comments
*/
message = "graphion"
```

Inline block comments are also allowed:

```gion
ratio = /* ignore this note */ 7 / 2
```

Comment markers inside strings remain part of the string:

```gion
message = "/* not a comment */"
print(message)
```

Expected output:

```text
/* not a comment */
```

Block comments must be closed. This is currently invalid:

```gion
/*
missing the closing marker
count = 42
```

## Strings

String concatenation is supported with `+`.

```gion
message = "graph" + "ion"
message += "!"

print(message)
```

Expected output:

```text
graphion!
```

## Print-Only String Coercion

Inside `print(...)` only, Graphion currently allows string concatenation with non-string scalar values.

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

This coercion is limited to `print(...)`.

For example, this is still invalid:

```gion
value = "count=" + 7
```

## Absolute Value

Graphion currently provides `abs(...)` as a builtin for numeric values.

```gion
abs_int = abs(-42)
abs_float = abs(-3.5)
abs_expr = abs(-5 + 2)

print(abs_int)
print(abs_float)
print(abs_expr)
```

Expected output:

```text
42
3.5
3
```
