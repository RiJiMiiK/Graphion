# Types

This page documents the currently implemented value kinds in Graphion.

Use it when you need:

- the current scalar and non-scalar types
- literal forms
- built-in numeric constants
- the current `bits` value model

For syntax and statement rules, see [Language Reference](language-reference.md).

## Current Types

Graphion currently exposes these value kinds:

- `int`
- `float`
- `bool`
- `string`
- `bits`
- `list`
- `dict`
- `tuple`
- `set`
- `graph`

## Built-In Numeric Constants

Graphion currently provides six built-in numeric constants:

- `pi`
- `tau`
- `phi`
- `e`
- `nan`
- `inf`

Current values:

- `pi = 3.141592653589793`
- `tau = 6.283185307179586`
- `phi = 1.618033988749895`
- `e = 2.718281828459045`
- `nan = not-a-number`
- `inf = positive infinity`

These names are reserved and cannot be reassigned.

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
circle = pi
turn = tau
golden = phi
growth = e
unknown = nan
limit = inf
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

### Lists

```gion
values = [1, 2, 3]
mixed = [1, true, "graphion"]
nested = [values, [4, 5], []]
```

Current `list` literals:

- start with `[`
- end with `]`
- use commas between elements
- may contain nested lists
- currently reject trailing commas

### Dictionaries

```gion
weights = {"a": 1, "b": 2}
nested = {"inner": weights, "empty": {}}
```

Current `dict` literals:

- start with `{`
- end with `}`
- use commas between entries
- require `string` literal keys in the form `"key": value`
- may contain nested `dict` and `list` values
- currently reject trailing commas

### Tuples

```gion
pair = (1, 2)
mixed = (pair, "graphion", true)
```

Current `tuple` literals:

- start with `(`
- end with `)`
- use commas between elements
- may contain nested tuples
- currently require at least two elements
- currently reject trailing commas

### Sets

```gion
frontier = set(1, 2, 2, "a")
empty = set()
```

Current `set` literals:

- use `set(...)`
- use commas between elements
- remove duplicate elements during construction
- may contain nested container values
- currently reject trailing commas

### Graphs

```gion
graph G;

alice = "Alice"
bob = "Bob"

graph H:
    defaults node {"label": "unknown", "score": 0}
    alice {"label": "start", "score": 1}
    2 {"score": 2}
    bob

graph I:
    defaults edge {"kind": "normal", "weight": 1}
    1-2 15
    3 - 2 {"kind": "shortcut", "weight": 2.5}

graph J:
    defaults edge {"kind": "directed", "weight": 1}
    1 -> 2 {"weight": 1}
    3 <-> 4 {"kind": "bidirectional"}
```

Current `graph` declarations:

- use `graph Name;`
- create an empty first-class graph value
- use `graph Name:` followed by an indented node block to create a graph with nodes
- require a valid identifier after `graph`
- require either the trailing semicolon for an empty graph or the trailing colon for a node block

Current graph node block entries:

- may be string literals, such as `"Alice"`
- may be non-negative integer IDs, such as `2`
- may be variables that contain a `string` node name or an `int` node ID
- treat unquoted identifiers as variables, so literal string node names must use quotes
- may mix names and integer IDs in the same block
- may attach node attributes with `node {"key": value}`
- store node attributes as dict values whose entries may contain any supported Graphion value type
- may declare node attribute defaults with `defaults node {"key": value}`
- use node defaults for nodes that omit attributes or omit specific keys
- require all node attributes to share the same keys when no node defaults are declared
- reject node attribute keys outside the declared node defaults
- reject duplicate explicit integer IDs
- may define undirected edges with `node - node`
- may define directed edges with `node -> node`
- may define bidirectional directed edges with `node <-> node`
- may attach edge attributes with `edge {"key": value}`
- may use the compact edge weight form `node - node 15`
- may use variables or expressions after an edge when they evaluate to `int`, `float`, or `dict`
- may declare edge attribute defaults with `defaults edge {"key": value}`
- use edge defaults for edges that omit attributes or omit specific keys
- require all edge attributes to share the same keys when no edge defaults are declared
- reject edge attribute keys outside the declared edge defaults
- reserve `weight` as an edge attribute key whose value must be `int` or `float`
- cannot mix `node - node` with directed edge syntax in the same graph
- create missing endpoint nodes before creating the edge
- warn when explicit numeric IDs have gaps after generated named-node IDs are assigned

## Type Notes

### Numeric Values

Graphion currently treats `int` and `float` as the numeric scalar family used by:

- arithmetic expressions
- comparisons
- most numeric builtins

Some operations preserve the input family, while others always return `float`. See [Builtins](builtins.md) for exact per-builtin result rules.

### Booleans

`bool` values are written as:

- `true`
- `false`

Graphion currently uses a strict boolean subset for conditions and boolean logic:

- `true`
- `false`
- `1`
- `0`

Other integers, floats, strings, and `bits` are rejected in boolean contexts.

### Strings

`string` values are scalar text values written with double quotes.

Current string support includes:

- storage in variables
- equality and inequality comparisons with other strings
- concatenation with other strings
- `len(x)`
- print-only scalar coercion inside `print(...)`

### Lists

`list` values are ordered non-scalar containers.

Current list support includes:

- literal construction with `[ ... ]`
- indexing with `list_expr[index_expr]`
- equality and inequality with other lists
- nested list values
- `len(x)`
- printing as bracketed values

Current index rules:

- indexes must be `int`
- indexes must be non-negative
- out-of-range access is a runtime error

### Dictionaries

`dict` values are key-value non-scalar containers.

Current dict support includes:

- literal construction with `{ ... }`
- lookup with `dict_expr[key_expr]`
- key assignment with `dict_expr[key_expr] = value_expr`
- equality and inequality with other dicts
- nested dict values
- `len(x)`
- printing as braced values

Current dict key rules:

- literal keys must be `string` literals
- lookup keys must evaluate to `string`
- missing keys are runtime errors

### Tuples

`tuple` values are ordered non-scalar containers with fixed size semantics.

Current tuple support includes:

- literal construction with `( ... )`
- indexing with `tuple_expr[index_expr]`
- equality and inequality with other tuples
- nested tuple values
- `len(x)`
- printing as parenthesized values

Current tuple rules:

- tuples currently require at least two elements
- `(expr)` remains a grouped expression, not a tuple
- indexes must be `int`
- indexes must be non-negative
- out-of-range access is a runtime error

### Sets

`set` values are non-scalar containers of unique values.

Current set support includes:

- literal construction with `set(...)`
- duplicate removal during construction
- membership checks with `contains(set_expr, value_expr)`
- equality and inequality with other sets
- nested set values
- `len(x)`
- printing as `set(...)`

Current set rules:

- sets are compared without considering insertion order
- printing preserves first-insertion order for deterministic output
- `set()` is the empty set
- trailing commas are currently rejected

### Graphs

`graph` values are first-class graph objects in `.gion`.

Current graph support is intentionally minimal:

- declaration with `graph Name;`
- declaration with `graph Name:` and an indented node block
- empty graph values with zero nodes and zero edges
- node-only and edge-bearing graph values
- explicit integer node IDs reserve their ID first
- named nodes receive generated IDs after explicit IDs are reserved
- storage in variables
- printing as `graph()`
- printing node-only graphs as `graph(nodes=N)`
- printing graph values with edges as `graph(nodes=N, edges=M)`
- inspection with `node_count(graph)`, `edge_count(graph)`, `is_directed(graph)`, `is_weighted(graph)`, and `orientation(graph)`
- attribute lookup with `node_attrs(graph, node)`, `edge_attrs(graph, from, to)`, and `edge_weight(graph, from, to)`
- basic queries with `has_node(graph, node)`, `has_edge(graph, from, to)`, and `neighbors(graph, node)`
- structural mutation with `add_node(graph, node)` and `add_edge(graph, from, to)`

Graph mutation and graph algorithms from `.gion` will be added in later steps.

### Bits

`bits` values are fixed-width binary scalars whose width comes from the literal spelling.

Current `bits` behavior:

- width is preserved for display and bitwise operations
- equality and inequality compare normalized values
- leading zeroes affect width, but not normalized equality

Examples:

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

For the exact operator rules on `bits`, see [Operators](operators.md).
