# Tutorial

This tutorial is a guided introduction to Graphion.

The goal is simple: write a small script, understand what each line does, and learn the core habits of the language without having to read the whole reference first.

If you want the full rules, edge cases, or the complete list of builtins, use:

- [Language Reference](language-reference.md)
- [Builtins](builtins.md)

## What You Will Learn

By the end of this tutorial, you will know how to:

- store values in variables
- print results
- write arithmetic expressions
- make decisions with `if` and `match`
- use strings and comments
- work with `bits` values
- call a few useful builtins

## Your First Script

Start with this:

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

What is happening here:

- `count`, `name`, and `ready` are variables
- `=` stores a value in a variable
- `print(...)` displays a value
- Graphion already knows how to print numbers, text, and booleans

That is the basic rhythm of the language: compute values, store them, then print or reuse them.

## Variables And Values

Graphion currently works mainly with scalar values.

```gion
count = 42
ratio = 3.5
name = "graphion"
ready = true
circle = pi
growth = e
```

This gives you several useful value kinds:

- `int` for whole numbers
- `float` for decimal numbers
- `string` for text
- `bool` for `true` and `false`
- numeric constants like `pi` and `e`

You can also copy an existing value:

```gion
count = 42
copy = count

print(copy)
```

Expected output:

```text
42
```

A good mental model is: the right side is evaluated first, then its result is stored on the left.

## Doing Arithmetic

Now let's compute a few values.

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

What to notice:

- `*` binds tighter than `+`
- parentheses change the order explicitly
- `/` gives a floating-point result
- `//` is floor division
- `%` is modulo
- `**` is exponentiation

If you already know arithmetic from Python, most of this will feel familiar.

## Negative Values

Unary minus works on literals, variables, and grouped expressions.

```gion
count = 5
neg_count = -count
neg_group = -(1 + 2)
negative_div = -7 / 2

print(neg_count)
print(neg_group)
print(negative_div)
```

Expected output:

```text
-5
-3
-3.5
```

## Updating A Variable

Once a variable exists, you can update it with compound assignment.

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

This is just a compact way to say "take the old value, combine it with something, then store the result back".

## Calling Builtins

Graphion includes a growing set of builtins for common numeric work.

```gion
small = min(7, 3)
clamped = clamp(17, 0, 10)
root = sqrt(9)
rounded = round(7.5)
length = len("graphion")

print(small)
print(clamped)
print(root)
print(rounded)
print(length)
```

Expected output:

```text
3
10
3
8
8
```

For now, the important idea is simple:

- builtins look like regular function calls
- they evaluate their arguments first
- they return a value you can print or store

Use the builtins reference when you need the full catalog.

## Strings

Strings are written with double quotes.

```gion
message = "graph" + "ion"
message += "!"

print(message)
```

Expected output:

```text
graphion!
```

One special rule is worth knowing early:

inside `print(...)`, Graphion currently allows string concatenation with non-string scalar values.

```gion
count = 7
print("count=" + count)
```

Expected output:

```text
count=7
```

But outside `print(...)`, mixed string-plus-number expressions are still rejected.

## Making Decisions With `if`

You can branch with indentation-based blocks.

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

Important habits:

- put `:` at the end of the condition line
- indent the block below it
- keep branches aligned with each other

You can also nest blocks:

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

For now, conditions are intentionally strict:

- `true` and `false` are valid
- `1` and `0` are also accepted
- expressions like `1 + 1 == 2` are valid because they produce a boolean result
- values like `2` or `"x"` are not accepted directly as conditions

## Choosing A Value With A Ternary

When you want a small inline choice, use a ternary expression.

```gion
ready = true
label = "ready" if ready else "not ready"
print(label)
```

Expected output:

```text
ready
```

Use this when the choice is short and obvious. If it starts becoming dense, switch back to a normal `if` block.

## Matching Several Cases

When you want to branch on a value, `match` is often clearer than stacking many `if` tests.

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

Expected output:

```text
go
```

This is a good fit when you are comparing one value against several known cases.

## Comments

Graphion supports both line comments and block comments.

```gion
# initialize the counter
count = 40 # base value
count += 2

/*
this is a block comment
that can span several lines
*/

print(count)
```

Expected output:

```text
42
```

Use comments to explain intent, not to restate obvious code.

## Working With `bits`

Graphion also supports `bits` literals written with `0b...`.

```gion
mask = 0b1100
value = 0b1010
masked = mask & value
shifted = value >> 1

print(masked)
print(shifted)
```

Expected output:

```text
0b1000
0b0101
```

This first mental model is enough to get started:

- `0b...` creates a `bits` value
- `&`, `|`, and `^` combine `bits`
- `~` flips bits
- `<<` and `>>` shift them
- printed width is preserved from how the literal was written

That last point matters:

```gion
print(0b10)
print(0b0010)
print(0b10 == 0b0010)
```

Expected output:

```text
0b10
0b0010
true
```

So:

- the display keeps the written width
- equality compares the normalized bit value

If you plan to use `bits` seriously, read the reference after this tutorial. That part of the language has more precise rules than the rest of this introduction needs.

## A Small Combined Example

Here is a short script that combines the main ideas.

```gion
radius = 2
ready = true
mask = 0b1111
mask &= 0b1010
circumference = 2 * pi * radius
label = "ready" if ready else "waiting"

print("label=" + label)
print("circumference=" + circumference)
print(mask)
```

Expected output:

```text
label=ready
circumference=12.5664
0b1010
```

This is a good example of how Graphion currently feels:

- scalar values
- expressions
- a bit of branching
- a bit of formatting through `print(...)`
- optional `bits` manipulation when needed

## What To Read Next

Once this tutorial feels comfortable, the best next steps are:

1. read the [Language Reference](language-reference.md) for exact rules
2. read [Builtins](builtins.md) when you need a specific function
3. open the examples in `examples/` and run them

The tutorial should get you moving.
The reference should answer the precise questions you hit after that.
