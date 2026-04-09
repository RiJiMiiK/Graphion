# Examples

This directory contains focused `.gion` examples for the currently implemented language subset.

Files:

- `01_scalars_and_print.gion`
  - scalar assignments, variable copies, and `print(...)`
- `02_arithmetic.gion`
  - arithmetic operators, precedence, negative operands, unary minus, postfix factorial `!`, the `pi`, `e`, `nan`, and `inf` constants, numeric builtins like `abs(...)`, `min(...)`, `max(...)`, `clamp(...)`, `sqrt(...)`, `cbrt(...)`, `sin(...)`, `sinh(...)`, `asinh(...)`, `acosh(...)`, `cos(...)`, `cosh(...)`, `tan(...)`, `tanh(...)`, `atanh(...)`, `asin(...)`, `acos(...)`, `atan(...)`, `atan2(...)`, `hypot(...)`, `degrees(...)`, `radians(...)`, `isnan(...)`, `isinf(...)`, `isfinite(...)`, `exp(...)`, `ln(...)`, `log(...)`, `log10(...)`, `log2(...)`, `floor(...)`, `ceil(...)`, `round(...)`, `trunc(...)`, `fract(...)`, `sign(...)`, and the string builtin `len(...)`
- `03_compound_assignments.gion`
  - `+=`, `-=`, `*=`, `/=`, `//=`, `%=`, `**=`
- `04_conditionals.gion`
  - `if / elif / else`, nested conditionals, grouped multiline conditions, single-line and multiline ternary expressions, value-based `match` branching, equality/ordering conditions (`==`, `!=`, `<`, `<=`, `>`, `>=`), `and` / `nand` / `or` / `nor` / `not`, and accepted `0` / `1` conditions
- `05_comments.gion`
  - line comments with `#` and block comments with `/* ... */`
- `06_bits_literals.gion`
  - `bits` literals with `0b...`, preserved width, normalized equality, `&`, `|`, `^`, `~`, `<<`, and `>>`

Run any example with:

```powershell
.\build\Release\graphion.exe .\examples\01_scalars_and_print.gion
```
