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
circle = pi
growth = e
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
- postfix `!`

Example:

```gion
sum = 40 + 2
mixed = 1 + 2 * 3
grouped = (1 + 2) * 3
half = 7 / 2
floor_half = 7 // 2
power = 2 ** 3
factorial_zero = 0!
factorial_int = 5!
factorial_group = (1 + 2)!
remainder = 10 % 4

print(sum)
print(mixed)
print(grouped)
print(half)
print(floor_half)
print(power)
print(factorial_zero)
print(factorial_int)
print(factorial_group)
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
1
120
6
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

## Bits Literals

Graphion also supports `bits` literals with a `0b...` prefix.

```gion
short_bits = 0b10
wide_bits = 0b0010
copied_bits = wide_bits
same_bits = 0b10 == 0b0010
different_bits = 0b10 != 0b0011
masked_bits = 0b1100 & 0b1010
merged_bits = 0b1100 | 0b1010
xor_bits = 0b1100 ^ 0b1010
not_wide_bits = ~0b0010
not_short_bits = ~0b10
shifted_bits = 0b0011 << 1
truncated_shift_bits = 0b1111 << 1
right_shifted_bits = 0b1010 >> 1
cleared_right_shift_bits = 0b1010 >> 4
grouped_mask_then_shift_bits = (0b1100 & 0b1010) >> 1
not_then_mask_bits = ~0b0011 & 0b1111
shift_count_expression_bits = 0b0011 << (1 + 1)
compound_mask_bits = 0b1100
compound_mask_bits &= 0b1010
compound_merge_bits = 0b1100
compound_merge_bits |= 0b0011
compound_flip_bits = 0b1100
compound_flip_bits ^= 0b1010
compound_shift_bits = 0b0011
compound_shift_bits <<= 1
compound_shift_overflow_bits = 0b1111
compound_shift_overflow_bits <<= 1
compound_right_shift_bits = 0b1010
compound_right_shift_bits >>= 1
compound_right_shift_clear_bits = 0b1010
compound_right_shift_clear_bits >>= 4

print(short_bits)
print(wide_bits)
print(copied_bits)
print(same_bits)
print(different_bits)
print(masked_bits)
print(merged_bits)
print(xor_bits)
print(not_wide_bits)
print(not_short_bits)
print(shifted_bits)
print(truncated_shift_bits)
print(right_shifted_bits)
print(cleared_right_shift_bits)
print(grouped_mask_then_shift_bits)
print(not_then_mask_bits)
print(shift_count_expression_bits)
print(compound_mask_bits)
print(compound_merge_bits)
print(compound_flip_bits)
print(compound_shift_bits)
print(compound_shift_overflow_bits)
print(compound_right_shift_bits)
print(compound_right_shift_clear_bits)
```

Expected output:

```text
0b10
0b0010
0b0010
true
true
0b1000
0b1110
0b0110
0b1101
0b01
0b0110
0b1110
0b0101
0b0000
0b0100
0b1100
0b1100
0b1000
0b1111
0b0110
0b0110
0b1110
0b0101
0b0000
```

Current behavior:

- width is preserved from the literal spelling
- `0b10` and `0b0010` therefore print differently
- `==` compares normalized bit values, so `0b10 == 0b0010` is `true`
- `!=` follows the same normalized-value rule
- `&` works between `bits` values with the same stored width
- `0b1100 & 0b1010` therefore produces `0b1000`
- `|` works under the same width rule, so `0b1100 | 0b1010` produces `0b1110`
- `^` works under the same width rule, so `0b1100 ^ 0b1010` produces `0b0110`
- `~` inverts bits within the stored width, so `~0b0010` becomes `0b1101`
- `<<` keeps the stored width and truncates overflow back to that width
- `0b0011 << 1` therefore produces `0b0110`
- `0b1111 << 1` therefore produces `0b1110`
- `>>` keeps the stored width and shifts in zeroes from the left
- `0b1010 >> 1` therefore produces `0b0101`
- `0b1010 >> 4` therefore produces `0b0000`
- `&=` follows the same rule, so `mask = 0b1100` then `mask &= 0b1010` produces `0b1000`
- `|=` follows the same rule, so `merge = 0b1100` then `merge |= 0b0011` produces `0b1111`
- `^=` follows the same rule, so `flip = 0b1100` then `flip ^= 0b1010` produces `0b0110`
- `<<=` follows the same rule, so `shift = 0b0011` then `shift <<= 1` produces `0b0110`
- `<<=` also keeps truncation, so `shift_overflow = 0b1111` then `shift_overflow <<= 1` produces `0b1110`
- `>>=` follows the same rule, so `shift_right = 0b1010` then `shift_right >>= 1` produces `0b0101`
- `>>=` also keeps zero-fill, so `shift_right_clear = 0b1010` then `shift_right_clear >>= 4` produces `0b0000`
- current operator order for `bits` reads as:
  - parentheses
  - `~`
  - `<<` / `>>`
  - `&`
  - `|` / `^`
- `~0b0011 & 0b1111` therefore reads as `(~0b0011) & 0b1111`
- `0b0011 << 1 + 1` therefore reads as `0b0011 << (1 + 1)`
- `&`, `|`, and `^` reject non-`bits` operands
- `<<` and `>>` accept a non-negative `int` shift count on the right
- other `bits` / `int` mixes are still rejected
- ordered comparisons on `bits` are rejected
- boolean logic on `bits` is rejected
- `bits` cannot be used directly as an `if` condition or ternary condition
- invalid `bits` operators currently report `incompatible operand types`
- this first step covers literal creation, copying, printing, `==`, `!=`, `&`, `|`, `^`, `~`, `<<`, and `>>`

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

## Numeric Builtins

Graphion currently provides postfix factorial `!`, `abs(...)`, `min(a, b)`, `max(a, b)`, `clamp(x, lo, hi)`, `sqrt(x)`, `cbrt(x)`, `sin(x)`, `sinh(x)`, `cos(x)`, `tan(x)`, `asin(x)`, `acos(x)`, `atan(x)`, `atan2(y, x)`, `hypot(x, y)`, `exp(x)`, `ln(x)`, `log(x, base)`, `log10(x)`, `log2(x)`, `floor(x)`, `ceil(x)`, `round(x)`, `trunc(x)`, and `sign(x)` for numeric values, plus `len(x)` for strings.

```gion
abs_int = abs(-42)
abs_float = abs(-3.5)
abs_expr = abs(-5 + 2)
min_int = min(7, 3)
min_float = min(3.5, 2)
min_expr = min(10 - 2, 3 * 3)
max_int = max(7, 3)
max_float = max(3.5, 2)
max_expr = max(10 - 2, 3 * 3)
clamp_low = clamp(-2, 0, 10)
clamp_mid = clamp(5, 0, 10)
clamp_high = clamp(17, 0, 10)
clamp_float = clamp(12.5, 0, 10)
sqrt_int = sqrt(9)
sqrt_float = sqrt(2.25)
sqrt_expr = sqrt(1 + 8)
cbrt_int = cbrt(27)
cbrt_negative = cbrt(-8)
cbrt_expr = cbrt(1 + 26)
sin_zero = sin(0)
sin_half_turn = sin(pi / 2)
sin_expr = sin(1.5707963267948966)
sinh_zero = sinh(0)
sinh_one = sinh(1)
sinh_negative = sinh(-1)
cos_zero = cos(0)
cos_pi = cos(pi)
cos_expr = cos(3.14159265358979323846)
tan_zero = tan(0)
tan_quarter_turn = tan(pi / 4)
tan_expr = tan(0.7853981633974483)
asin_zero = asin(0)
asin_one = asin(1)
asin_half = asin(0.5)
acos_one = acos(1)
acos_zero = acos(0)
acos_half = acos(0.5)
atan_zero = atan(0)
atan_one = atan(1)
atan_negative_one = atan(-1)
atan2_diag = atan2(1, 1)
atan2_quadrant_two = atan2(1, -1)
atan2_quadrant_three = atan2(-1, -1)
hypot_diag = hypot(3, 4)
hypot_large = hypot(5, 12)
hypot_negative = hypot(-3, 4)
exp_int = exp(1)
exp_float = exp(0.0)
exp_expr = exp(1 + 1)
ln_int = ln(1)
ln_float = ln(e)
ln_expr = ln(e ** 2)
log_int = log(8, 2)
log_float = log(100, 10)
log_expr = log(2 ** 5, 2)
log10_int = log10(1000)
log10_float = log10(10.0)
log10_expr = log10(10 ** 4)
log2_int = log2(8)
log2_float = log2(2.0)
log2_expr = log2(2 ** 6)
floor_int = floor(7)
floor_float = floor(7.5)
floor_negative = floor(-3.2)
ceil_int = ceil(7)
ceil_float = ceil(7.5)
ceil_negative = ceil(-3.2)
round_int = round(7)
round_float = round(7.4)
round_half = round(7.5)
round_negative = round(-3.2)
round_negative_half = round(-3.5)
trunc_int = trunc(7)
trunc_float = trunc(7.9)
trunc_negative = trunc(-3.9)
trunc_small_negative = trunc(-0.4)
sign_positive = sign(7)
sign_negative = sign(-3.9)
sign_zero = sign(0)
pi_value = pi
e_value = e
factorial_zero = 0!
factorial_int = 5!
factorial_group = (1 + 2)!
len_text = len("graphion")
len_concat = len("graph" + "ion")

print(abs_int)
print(abs_float)
print(abs_expr)
print(min_int)
print(min_float)
print(min_expr)
print(max_int)
print(max_float)
print(max_expr)
print(clamp_low)
print(clamp_mid)
print(clamp_high)
print(clamp_float)
print(sqrt_int)
print(sqrt_float)
print(sqrt_expr)
print(cbrt_int)
print(cbrt_negative)
print(cbrt_expr)
print(sin_zero)
print(sin_half_turn)
print(sin_expr)
print(cos_zero)
print(cos_pi)
print(cos_expr)
print(tan_zero)
print(tan_quarter_turn)
print(tan_expr)
print(asin_zero)
print(asin_one)
print(asin_half)
print(acos_one)
print(acos_zero)
print(acos_half)
print(atan_zero)
print(atan_one)
print(atan_negative_one)
print(atan2_diag)
print(atan2_quadrant_two)
print(atan2_quadrant_three)
print(hypot_diag)
print(hypot_large)
print(hypot_negative)
print(exp_int)
print(exp_float)
print(exp_expr)
print(ln_int)
print(ln_float)
print(ln_expr)
print(log_int)
print(log_float)
print(log_expr)
print(log10_int)
print(log10_float)
print(log10_expr)
print(log2_int)
print(log2_float)
print(log2_expr)
print(floor_int)
print(floor_float)
print(floor_negative)
print(ceil_int)
print(ceil_float)
print(ceil_negative)
print(round_int)
print(round_float)
print(round_half)
print(round_negative)
print(round_negative_half)
print(trunc_int)
print(trunc_float)
print(trunc_negative)
print(trunc_small_negative)
print(sign_positive)
print(sign_negative)
print(sign_zero)
print(pi_value)
print(e_value)
print(factorial_zero)
print(factorial_int)
print(factorial_group)
print(len_text)
print(len_concat)
```

Expected output:

```text
42
3.5
3
3
2
8
7
3.5
9
0
5
10
10
3
1.5
3
3
-2
3
0
1
1
0
1.1752
-1.1752
1
-1
-1
0
1
1
0
1.5708
0.523599
0
1.5708
1.0472
0
0.785398
-0.785398
0.785398
2.35619
-2.35619
5
13
5
2.71828
1
7.38906
0
1
2
3
2
5
3
1
4
3
1
6
7
7
-4
7
8
-3
7
7
8
-3
-4
7
7
-3
0
1
-1
0
3.14159
2.71828
1
120
6
8
8
```

Graphion also exposes `pi` and `e` as built-in numeric constants, so you can write expressions like:

```gion
radius = 2
circumference = 2 * pi * radius
growth = e ** 2
print(circumference)
```
