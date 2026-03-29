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
negative_power = (-2) ** 3
negative_remainder = -10 % 4

print(negative_add)
print(negative_div)
print(negative_floor)
print(negative_power)
print(negative_remainder)
```

Expected output:

```text
-3
-3.5
-4
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

This is currently invalid:

```gion
if 2:
    print("bad")
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
