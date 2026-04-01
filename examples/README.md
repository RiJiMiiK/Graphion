# Examples

This directory contains focused `.gion` examples for the currently implemented language subset.

Files:

- `sample_test.gion`
  - quick overview script
- `01_scalars_and_print.gion`
  - scalar assignments, variable copies, and `print(...)`
- `02_arithmetic.gion`
  - arithmetic operators, precedence, negative operands, unary minus, and `abs(...)`
- `03_compound_assignments.gion`
  - `+=`, `-=`, `*=`, `/=`, `//=`, `%=`, `**=`
- `04_conditionals.gion`
  - `if / elif / else`, nested conditionals, grouped multiline conditions, single-line and multiline ternary expressions, value-based `match` branching, equality/ordering conditions (`==`, `!=`, `<`, `<=`, `>`, `>=`), `and` / `nand` / `or` / `nor` / `not`, and accepted `0` / `1` conditions
- `05_comments.gion`
  - line comments with `#` and block comments with `/* ... */`
- `06_bits_literals.gion`
  - `bits` literals with `0b...`, preserved width, normalized equality, `&`, `|`, and `^`

Run any example with:

```powershell
.\build\Release\graphion.exe .\examples\01_scalars_and_print.gion
```
