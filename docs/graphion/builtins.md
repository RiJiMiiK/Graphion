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
