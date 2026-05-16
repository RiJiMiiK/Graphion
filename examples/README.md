# Examples

This directory contains focused `.gion` examples for the current documented frontend.

Current project state:

- these examples track the implemented `.gion` subset
- the examples include scalar values, non-scalar containers, structs, graphs, and hypergraphs

Files:

- `01_scalars_and_print.gion`
  - scalar assignments, variable copies, and `print(...)`
- `02_arithmetic.gion`
  - arithmetic operators, precedence, negative operands, unary minus, postfix factorial `!`, the `pi`, `tau`, `phi`, `e`, `nan`, and `inf` constants, numeric builtins like `abs(...)`, `min(...)`, `max(...)`, `clamp(...)`, `sqrt(...)`, `cbrt(...)`, `sin(...)`, `csc(...)`, `sec(...)`, `cot(...)`, `acsc(...)`, `asec(...)`, `acot(...)`, `sech(...)`, `csch(...)`, `coth(...)`, `sinh(...)`, `asinh(...)`, `acosh(...)`, `cos(...)`, `cosh(...)`, `tan(...)`, `tanh(...)`, `atanh(...)`, `asin(...)`, `acos(...)`, `atan(...)`, `atan2(...)`, `hypot(...)`, `copysign(...)`, `fma(...)`, `fdim(...)`, `remainder(...)`, `rint(...)`, `degrees(...)`, `radians(...)`, `isnan(...)`, `isinf(...)`, `isfinite(...)`, `exp(...)`, `exp2(...)`, `expm1(...)`, `log1p(...)`, `erf(...)`, `erfc(...)`, `gamma(...)`, `lgamma(...)`, `ln(...)`, `log(...)`, `log10(...)`, `log2(...)`, `floor(...)`, `ceil(...)`, `round(...)`, `trunc(...)`, `fract(...)`, `sign(...)`, and the string builtin `len(...)`
- `03_compound_assignments.gion`
  - `+=`, `-=`, `*=`, `/=`, `//=`, `%=`, `**=`
- `04_conditionals.gion`
  - `if / elif / else`, nested conditionals, grouped multiline conditions, single-line and multiline ternary expressions, value-based `match` branching, equality/ordering conditions (`==`, `!=`, `<`, `<=`, `>`, `>=`), `and` / `nand` / `or` / `nor` / `not`, and accepted `0` / `1` conditions
- `05_comments.gion`
  - line comments with `#` and block comments with `/* ... */`
- `06_bits_literals.gion`
  - `bits` literals with `0b...`, preserved width, normalized equality, `&`, `|`, `^`, `~`, `<<`, and `>>`
- `07_lists.gion`
  - list literals, nested lists, indexing, equality, printing, and `len(...)`
- `08_dicts.gion`
  - dict literals with `string` keys, nested dict values, lookup, equality, printing, and `len(...)`
- `09_tuples.gion`
  - tuple literals, indexing, equality, printing, and `len(...)`
- `10_sets.gion`
  - set literals, duplicate removal, membership with `contains(...)`, equality, printing, and `len(...)`
- `11_graphs.gion`
  - first-class graph declarations with `graph Name;`, node blocks, node/edge attributes, undirected/directed edges, graph inspection/listing builtins, graph attribute lookup builtins, basic membership/neighbor queries, structural graph mutation including removal, and graph attribute mutation
- `12_hypergraphs.gion`
  - first-class hypergraph declarations with `hypergraph Name;`, vertex blocks, vertex/hyperedge attributes, hyperedge lists, hypergraph inspection/listing builtins, membership/incidence queries, structural mutation including removal, and hypergraph attribute mutation
- `13_structs.gion`
  - first-class `struct Name:` declarations, required/defaulted typed fields, `Name {"field": value}` instances, field lookup, printing, and `len(...)`

Run any example with:

```powershell
.\build\Release\graphion.exe .\examples\01_scalars_and_print.gion
```

Use `-d` to print debug warnings before execution:

```powershell
.\build\Release\graphion.exe -d .\examples\11_graphs.gion
```
