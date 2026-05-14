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
- `hypergraph`

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
hypergraph HG;

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

Current `hypergraph` declarations:

- use `hypergraph Name;`
- create an empty first-class hypergraph value
- use `hypergraph Name:` followed by an indented vertex block to create a hypergraph with vertices
- require a valid identifier after `hypergraph`
- require either the trailing semicolon for an empty hypergraph or the trailing colon for a vertex block
- accept vertex entries as string literals, non-negative integer IDs, or variables that contain a `string` or `int`
- treat unquoted identifiers as variables, so literal string vertex names must use quotes
- may mix names and integer IDs in the same block
- may attach vertex attributes with `vertex {"key": value}`
- store vertex attributes as dict values whose entries may contain any supported Graphion value type
- may declare vertex attribute defaults with `defaults vertex {"key": value}`
- use vertex defaults for vertices that omit attributes or omit specific keys
- require all vertex attributes to share the same keys when no vertex defaults are declared
- reject vertex attribute keys outside the declared vertex defaults
- reject duplicate explicit integer IDs
- may define hyperedges with `[vertex_a, vertex_b, vertex_c]`
- assign implicit numeric hyperedge IDs in declaration order, starting at `0`
- do not support user-provided hyperedge IDs in the first version
- expose implicit hyperedge IDs through `hyperedge_vertices(hypergraph, id)` and `hyperedge_attrs(hypergraph, id)`
- create missing vertices before creating a hyperedge
- may attach hyperedge attributes with `[vertex_a, vertex_b] {"key": value}`
- may declare hyperedge attribute defaults with `defaults hyperedge {"key": value}`
- use hyperedge defaults for hyperedges that omit attributes or omit specific keys
- require all hyperedge attributes to share the same keys when no hyperedge defaults are declared
- do not reserve a special `weight` key for hyperedges yet
- print empty hypergraphs as `hypergraph()`
- print vertex-only hypergraphs as `hypergraph(vertices=N)`
- print vertex-only hypergraphs with attributes as `hypergraph(vertices=N, vertex_attrs=K)`
- print hypergraphs with hyperedges as `hypergraph(vertices=N, hyperedges=M)`
- print hypergraphs with hyperedge attributes as `hypergraph(vertices=N, hyperedges=M, hyperedge_attrs=K)`
- support inspection with `vertex_count`, `hyperedge_count`, `vertex_attr_count`, and `hyperedge_attr_count`
- support attribute lookup with `vertex_attrs(hypergraph, vertex)` and `hyperedge_attrs(hypergraph, id)`
- support membership/query operations with `has_vertex`, `has_hyperedge`, `incident_hyperedges`, and `hyperedge_vertices`
- support listing/query operations with `vertex_ids`, `vertices`, and `hyperedges`
- support structure mutation after initialization with `add_vertex` and `add_hyperedge`
- support attribute mutation after initialization with `set_vertex_attrs` and `set_hyperedge_attrs`

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
- basic queries with `has_node(graph, node)`, `has_edge(graph, from, to)`, `neighbors(graph, node)`, `indegree(graph, node)`, and `outdegree(graph, node)`
- listing queries with `node_ids(graph)`, `nodes(graph)`, and `edges(graph)`
- structural mutation with `add_node(graph, node)`, `add_edge(graph, from, to)`, `remove_node(graph, node)`, and `remove_edge(graph, from, to)`
- attribute mutation with partial or full `set_node_attrs(...)`, partial or full `set_edge_attrs(...)`, and `set_edge_weight(...)`
- added nodes and edges receive declared defaults before later partial attribute patches

Graph algorithms from `.gion` will be added in later steps.

### Hypergraphs

`hypergraph` values are first-class hypergraph objects in `.gion`.

Current hypergraph support is intentionally minimal:

- declaration with `hypergraph Name;`
- declaration with `hypergraph Name:` and an indented vertex block
- empty hypergraph values with zero nodes, zero hyperedges, and zero incidences
- vertex-only hypergraph values with zero hyperedges and zero incidences
- hyperedges declared as vertex lists
- hyperedge attributes with shared-key schema rules and optional `defaults hyperedge`
- vertex attributes with shared-key schema rules and optional `defaults vertex`
- storage in variables
- printing as `hypergraph()`
- printing vertex-only hypergraphs as `hypergraph(vertices=N)`
- printing vertex-only hypergraphs with attributes as `hypergraph(vertices=N, vertex_attrs=K)`
- printing hypergraphs with hyperedges as `hypergraph(vertices=N, hyperedges=M)`
- printing hypergraphs with hyperedge attributes as `hypergraph(vertices=N, hyperedges=M, hyperedge_attrs=K)`
- inspection with `vertex_count`, `hyperedge_count`, `vertex_attr_count`, and `hyperedge_attr_count`
- attribute lookup with `vertex_attrs(hypergraph, vertex)` and `hyperedge_attrs(hypergraph, id)`
- membership/query operations with `has_vertex`, `has_hyperedge`, `incident_hyperedges`, and `hyperedge_vertices`
- listing/query operations with `vertex_ids`, `vertices`, and `hyperedges`
- structure mutation after initialization with `add_vertex` and `add_hyperedge`
- attribute mutation after initialization with `set_vertex_attrs` and `set_hyperedge_attrs`

Hypergraph removal operations will be added in later steps.

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
