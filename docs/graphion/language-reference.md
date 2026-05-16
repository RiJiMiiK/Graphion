# Language Reference

This reference describes the currently implemented `.gion` language subset.

Use this page when you need exact rules rather than a guided introduction.

Quick orientation:

- source structure: statements, names, assignment
- control flow: `if`, ternary expressions, `match`
- expressions and operators: see [Operators](operators.md)
- runtime library: builtins and constants
- errors: current high-level user-visible error classes

## Source Structure

### Statements

Current supported top-level statements:

- assignment
- compound assignment
- graph declaration
- hypergraph declaration
- struct declaration
- `if` / `elif` / `else`
- `match`
- `print(...)`
- graph mutation with `add_node(...)` and `add_edge(...)`
- hypergraph mutation with `add_vertex(...)` and `add_hyperedge(...)`

Examples:

```gion
count = 42
count += 1
graph G;
hypergraph H;
struct Player:
    id: int
    name: string = "unknown"

if true:
    count += 1
else:
    count -= 1

print(count)
```

Unsupported statements are parse errors in the current `.gion` frontend path.

### Values

Graphion currently exposes scalar values plus `list`, `dict`, `tuple`, `set`, `struct`, `graph`, and `hypergraph`.

For the current value kinds, literal forms, and built-in numeric constants, see [Types](types.md).

### Identifiers

Identifiers:

- must start with a letter or `_`
- can then contain letters, digits, or `_`

Examples:

- `count`
- `_tmp`
- `alpha_1`

### Reserved Names

These names are currently reserved and cannot be assigned:

- `print`
- `true`
- `false`
- `pi`
- `tau`
- `phi`
- `e`
- `nan`
- `inf`
- `abs`
- `min`
- `max`
- `clamp`
- `sqrt`
- `cbrt`
- `sin`
- `csc`
- `sec`
- `cot`
- `acsc`
- `asec`
- `acot`
- `sinh`
- `asinh`
- `acosh`
- `cosh`
- `cos`
- `tan`
- `tanh`
- `atanh`
- `asin`
- `acos`
- `atan`
- `atan2`
- `hypot`
- `copysign`
- `fma`
- `fdim`
- `remainder`
- `rint`
- `degrees`
- `radians`
- `isnan`
- `isinf`
- `isfinite`
- `expm1`
- `exp2`
- `log1p`
- `erf`
- `erfc`
- `gamma`
- `lgamma`
- `fract`
- `exp`
- `ln`
- `log`
- `log10`
- `log2`
- `floor`
- `ceil`
- `round`
- `trunc`
- `sign`
- `len`
- `contains`
- `set`
- `graph`
- `if`
- `elif`
- `else`
- `and`
- `nand`
- `or`
- `nor`
- `not`
- `match`
- `default`

### Literals

Graphion currently supports literals for:

- `int`
- `float`
- `bool`
- `string`
- `bits`
- `list`
- `dict`
- `tuple`
- `set`
- `struct`
- `graph`

For the exact literal forms and examples, see [Types](types.md).

### Assignment

Simple assignment:

```gion
count = 42
copy = count
ratio = 7 / 2
weights = {"a": 1}
weights["b"] = 2
```

Graph declaration:

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

Current graph declaration rules:

- the empty shape is `graph Name;`
- the node-block shape is `graph Name:` followed by an indented block
- `Name` must be a valid identifier
- `graph Name;` creates an empty graph value
- graph node blocks create graph values with nodes and optional edges
- node entries may be string literals, non-negative integer IDs, or variables that contain a `string` or `int`
- string node names must use quotes, such as `"Alice"`; an unquoted identifier is resolved as a variable
- node entries may attach attributes with `node {"key": value}`
- node attributes are `dict` values and their values may use any supported Graphion value type
- `defaults node {"key": value}` declares node attribute defaults for the graph
- with node defaults, node entries may omit attributes or override only some keys
- without node defaults, either no node has attributes or every node must define attributes with the same keys
- node attributes cannot introduce keys outside the declared node defaults
- undirected edge entries use `node - node`
- directed edge entries use `node -> node`
- bidirectional directed edge entries use `node <-> node`
- edge entries may attach attributes with `edge {"key": value}`
- the compact form `node - node 15` means `node - node {"weight": 15}`
- the compact edge form also accepts variables or expressions that evaluate to `int`, `float`, or `dict`
- `defaults edge {"key": value}` declares edge attribute defaults for the graph
- with edge defaults, edge entries may omit attributes or override only some keys
- without edge defaults, either no edge has attributes or every edge must define attributes with the same keys
- edge attributes cannot introduce keys outside the declared edge defaults
- `weight` is a reserved edge attribute key and must be `int` or `float` when present
- graphs that use directed edge syntax cannot also use undirected `node - node` entries
- missing edge endpoint nodes are created before the edge is created
- explicit integer IDs are reserved first and cannot be duplicated
- explicit numeric IDs with gaps emit a warning after generated named-node IDs are assigned
- named nodes receive generated IDs after explicit IDs are reserved
- the semicolon is required for empty declarations
- the colon is required for node-block declarations

Hypergraph declaration:

```gion
hypergraph H;

label = "Alice"

hypergraph HG:
    defaults vertex {"label": "unknown", "score": 0}
    defaults hyperedge {"kind": "group", "color": "blue"}
    label
    2 {"label": "explicit id", "score": 2}
    "Bob" {"score": 1}
    [label, 2, "Bob"] {"kind": "team"}
```

Current hypergraph declaration rules:

- the empty shape is `hypergraph Name;`
- the vertex-block shape is `hypergraph Name:` followed by an indented block
- `Name` must be a valid identifier
- `hypergraph Name;` creates an empty first-class hypergraph value
- hypergraph vertex blocks create hypergraph values with vertices and no hyperedges yet
- vertex entries may be string literals, non-negative integer IDs, or variables that contain a `string` or `int`
- string vertex names must use quotes, such as `"Alice"`; an unquoted identifier is resolved as a variable
- vertex entries may attach attributes with `vertex {"key": value}`
- vertex attributes are `dict` values and their values may use any supported Graphion value type
- `defaults vertex {"key": value}` declares vertex attribute defaults for the hypergraph
- with vertex defaults, vertex entries may omit attributes or override only some keys
- without vertex defaults, either no vertex has attributes or every vertex must define attributes with the same keys
- vertex attributes cannot introduce keys outside the declared vertex defaults
- hyperedge entries use list syntax, such as `[vertex_a, vertex_b, vertex_c]`
- hyperedges receive implicit numeric IDs in declaration order, starting at `0`
- user-provided hyperedge IDs are not part of the first hypergraph version
- hyperedge IDs can be inspected with `hyperedge_vertices(hypergraph, id)` and `hyperedge_attrs(hypergraph, id)`
- hyperedges may reference string literals, non-negative integer IDs, or variables that contain a `string` or `int`
- missing vertices referenced by a hyperedge are created before the hyperedge is created
- hyperedge entries may attach attributes with `[vertex_a, vertex_b] {"key": value}`
- hyperedge attributes are `dict` values and do not reserve a `weight` key yet
- `defaults hyperedge {"key": value}` declares hyperedge attribute defaults for the hypergraph
- with hyperedge defaults, hyperedge entries may omit attributes or override only some keys
- without hyperedge defaults, either no hyperedge has attributes or every hyperedge must define attributes with the same keys
- empty hypergraphs print as `hypergraph()`
- vertex-only hypergraphs print as `hypergraph(vertices=N)`
- vertex-only hypergraphs with attributes print as `hypergraph(vertices=N, vertex_attrs=K)`
- hypergraphs with hyperedges print as `hypergraph(vertices=N, hyperedges=M)`
- hypergraphs with hyperedge attributes also include `hyperedge_attrs=K`
- hypergraph inspection builtins include `vertex_count`, `hyperedge_count`, `vertex_attr_count`, and `hyperedge_attr_count`
- hypergraph attribute lookup builtins include `vertex_attrs(hypergraph, vertex)` and `hyperedge_attrs(hypergraph, id)`
- hypergraph membership/query builtins include `has_vertex`, `has_hyperedge`, `incident_hyperedges`, and `hyperedge_vertices`
- hypergraph listing/query builtins include `vertex_ids`, `vertices`, and `hyperedges`
- hypergraph structure mutation statements include `add_vertex` and `add_hyperedge`
- hypergraph attribute mutation statements include `set_vertex_attrs` and `set_hyperedge_attrs`
- hypergraph removal mutation statements include `remove_vertex` and `remove_hyperedge`

Struct declaration:

```gion
struct Player:
    id: int
    name: string = "unknown"
    score: float = 0.0

alice = Player {"id": 1, "name": "Alice", "score": 42.5}
bob = Player {"id": 2}
```

Current struct declaration rules:

- the declaration shape is `struct Name:` followed by an indented field block
- `Name` must be a valid identifier
- fields use `field: type`
- defaulted fields use `field: type = value`
- supported field types are `int`, `float`, `bool`, `string`, `bits`, `list`, `dict`, `tuple`, `set`, `graph`, `hypergraph`, `any`, and previously declared struct type names
- field defaults are validated against the declared field type
- struct instances use `Name {"field": value}`
- omitted defaulted fields are filled automatically
- missing required fields, unknown fields, and wrong field types are runtime errors
- field lookup uses the same index syntax as dictionaries, such as `player["name"]`
- `len(struct_instance)` returns the number of fields
- struct types print as `struct Name(fields=N)`
- struct instances print as `Name{"field": value}`

Graph mutation statements:

```gion
graph G;

add_node(G, "Alice")
add_edge(G, "Alice", 2)
set_node_attrs(G, "Alice", {"label": "start", "score": 1})
set_node_attrs(G, "Alice", {"score": 10})
set_edge_attrs(G, "Alice", 2, {"kind": "path", "weight": 3})
set_edge_attrs(G, "Alice", 2, {"kind": "shortcut"})
set_edge_weight(G, "Alice", 2, 7)
remove_edge(G, "Alice", 2)
remove_node(G, "Alice")
```

Current graph mutation rules:

- `add_node(graph_variable, node)` mutates the named graph variable in place
- `add_edge(graph_variable, from, to)` mutates the named graph variable in place
- `remove_node(graph_variable, node)` removes a node and all incident edges
- `remove_edge(graph_variable, from, to)` removes one edge direction
- `node`, `from`, and `to` may be integer IDs or string node names
- `add_edge(...)` creates missing endpoint nodes before adding the edge
- `add_edge(...)` currently creates undirected edges
- added nodes receive declared `defaults node` attributes when node defaults exist
- added edges receive declared `defaults edge` attributes when edge defaults exist
- adding an existing node or edge is a no-op
- removing a missing node or edge is a runtime error
- removing a direction from a bidirectional directed edge keeps the reverse direction
- node removal keeps other node IDs stable and can leave numeric ID gaps
- `set_node_attrs(graph_variable, node, attrs)` applies a full or partial dictionary patch to node attributes
- `set_edge_attrs(graph_variable, from, to, attrs)` applies a full or partial dictionary patch to edge attributes
- `set_edge_weight(graph_variable, from, to, weight)` updates the reserved numeric `weight` key
- `set_*_attrs(...)` can establish the initial node or edge attribute schema
- partial `set_*_attrs(...)` patches require the target node or edge to already have an attribute dictionary containing every patched key
- if a schema exists but the target has no attributes yet, `set_*_attrs(...)` must provide the full schema
- nodes and edges added after initialization use defaults first, so partial patches are valid for them when defaults exist
- edge `weight` values must be `int` or `float`
- attribute mutation preserves the shared-key schema once a schema exists

Hypergraph mutation statements:

```gion
hypergraph H;

add_vertex(H, "Alice")
add_hyperedge(H, ["Alice", 2, "Bob"])
set_vertex_attrs(H, "Alice", {"label": "start"})
set_hyperedge_attrs(H, 0, {"kind": "group"})
remove_vertex(H, 2)
remove_hyperedge(H, 0)
```

Current hypergraph mutation rules:

- `add_vertex(hypergraph_variable, vertex)` mutates the named hypergraph variable in place
- `add_hyperedge(hypergraph_variable, vertices)` mutates the named hypergraph variable in place
- `vertex` values may be integer IDs or string vertex names
- `vertices` must be a non-empty list of integer IDs or string vertex names
- `add_hyperedge(...)` creates missing vertices before adding the hyperedge
- added vertices receive declared `defaults vertex` attributes when vertex defaults exist
- added hyperedges receive declared `defaults hyperedge` attributes when hyperedge defaults exist
- adding an existing vertex is a no-op
- hyperedges receive the next implicit numeric hyperedge ID and existing hyperedge IDs stay stable
- `set_vertex_attrs(hypergraph_variable, vertex, attrs)` applies a full or partial dictionary patch to vertex attributes
- `set_hyperedge_attrs(hypergraph_variable, id, attrs)` applies a full or partial dictionary patch to hyperedge attributes
- `set_*_attrs(...)` can establish the initial vertex or hyperedge attribute schema
- partial `set_*_attrs(...)` patches require the target vertex or hyperedge to already have an attribute dictionary containing every patched key
- if a schema exists but the target has no attributes yet, `set_*_attrs(...)` must provide the full schema
- vertices and hyperedges added after initialization use defaults first, so partial patches are valid for them when defaults exist
- attribute mutation preserves the shared-key schema once a schema exists
- `remove_vertex(hypergraph_variable, vertex)` removes a vertex and its attributes
- removing a vertex removes it from every incident hyperedge
- hyperedges that become empty after vertex removal are removed
- non-empty hyperedges remain valid, even if they contain only one vertex
- `remove_hyperedge(hypergraph_variable, id)` removes one hyperedge and its attributes
- removing a hyperedge never removes its vertices
- removed hyperedge IDs are not reused, and later hyperedges keep/newly receive stable IDs

Graph listing and query builtins:

```gion
print(node_ids(G))
print(nodes(G))
print(edges(G))
print(has_node(G, "Alice"))
print(has_edge(G, "Alice", 2))
print(neighbors(G, "Alice"))
print(indegree(G, "Alice"))
print(outdegree(G, "Alice"))
```

- `node_ids(graph)` returns a list of present numeric node IDs
- `nodes(graph)` returns node dictionaries such as `{"id": 0, "name": "Alice"}`
- numeric-only nodes omit the `name` key in `nodes(...)`
- `edges(graph)` returns edge dictionaries with `from`, `to`, `directed`, and `bidirectional`
- bidirectional `<->` edges appear as one logical edge in `edges(...)`
- `neighbors(graph, node)` returns all adjacent node IDs, including incoming and outgoing directed edges
- `indegree(graph, node)` returns incoming adjacent node IDs
- `outdegree(graph, node)` returns outgoing adjacent node IDs
- use `len(indegree(...))` or `len(outdegree(...))` when the count is needed
- undirected `-` and bidirectional `<->` edges appear once in both incoming and outgoing lists for each endpoint

Compound assignment:

```gion
count += 1
count -= 1
count *= 2
count /= 2
count //= 2
count %= 2
count **= 2
```

Compound assignment requires the target variable to already exist.

### Indexing

Graphion currently supports list indexing with `[...]`:

```gion
values = [10, 20, 30]
second = values[1]
print(second)
```

Current indexing rules:

- the left-hand side must evaluate to `list`
- the index must evaluate to `int`
- negative indexes are rejected
- out-of-range indexes are runtime errors

Graphion also supports dictionary lookup and assignment with the same `[...]` syntax:

```gion
weights = {"a": 1, "b": 2}
value = weights["b"]
weights["c"] = 3
print(value)
```

Current dictionary lookup rules:

- the left-hand side must evaluate to `dict`
- literal keys inside `{...}` must be `string` literals
- lookup keys must evaluate to `string`
- assignment keys must evaluate to `string`
- missing keys are runtime errors

Current dictionary assignment notes:

- `dict_expr[key_expr] = value_expr` updates an existing key or creates a new one
- only simple `=` assignment is currently supported on indexed dictionary targets

Graphion also supports tuple indexing with the same `[...]` syntax:

```gion
pair = (10, 20)
second = pair[1]
print(second)
```

Current tuple rules:

- the left-hand side must evaluate to `tuple`
- tuple literals currently require at least two elements
- `(expr)` remains a grouped expression
- indexes must evaluate to `int`
- negative indexes are rejected
- out-of-range indexes are runtime errors

### Set Membership

Graphion supports set membership with `contains(set_expr, value_expr)`:

```gion
frontier = set(1, 2, 2, "a")
print(contains(frontier, 2))
print(contains(frontier, 3))
```

Current set rules:

- set literals use `set(...)`
- duplicate elements are removed
- `set()` is the empty set
- equality between sets ignores insertion order
- `contains(...)` requires the first argument to evaluate to `set`

## Control Flow

### Conditional Statements

Graphion currently supports Python-style conditional blocks:

- `if`
- `elif`
- `else`

Valid shapes:

- `if`
- `if` + one or more `elif`
- `if` + `else`
- `if` + one or more `elif` + `else`

Examples:

```gion
ready = true
fallback = false

if ready:
    message = "ready"
elif fallback:
    message = "fallback"
else:
    message = "other"
```

#### Condition Rules

Current conditions must evaluate to:

- `true`
- `false`
- `1`
- `0`
- an expression that evaluates to `bool`, such as `1 + 1 == 2`

Valid condition examples:

```gion
flag = true

if flag:
    print("ok")
```

```gion
if false:
    print("never")
else:
    print("taken")
```

```gion
if 1:
    print("taken")

if 0:
    print("never")
```

```gion
if 1 + 1 == 2:
    print("taken")
```

Invalid examples:

```gion
if 2:
    print("bad")
```

```gion
if "x":
    print("bad")
```

These currently fail with:

`if condition must be boolean or 0/1`
: runtime error for conditions outside the current accepted boolean subset

#### Block Rules

Conditional blocks currently use indentation-significant syntax.

Rules:

- a block line must end with `:`
- the following block must be indented
- `elif` can appear multiple times
- `else` is optional
- `else` must be last
- nested `if` blocks are allowed
- an `else` always binds to the `if` at the same indentation level
- indentation decides the binding, not the nearest visible `if` token on the page

Invalid examples:

```gion
if true
    print("bad")
```

```gion
if true:
print("bad")
```

```gion
else:
    print("bad")
```

#### Nested Conditionals

You can place an `if` block inside another `if` block.

```gion
ready = true
admin = false

if ready:
    if admin:
        label = "admin"
    else:
        label = "user"
else:
    label = "offline"
```

In this example:

- the inner `else` binds to `if admin:`
- the outer `else` binds to `if ready:`

That binding is determined by indentation, not by the nearest visible `if` keyword alone.

#### Multiline Conditions

Long conditions can span multiple physical lines, but only when the full condition is wrapped in grouping parentheses.

```gion
if (
    ready and
    has_token and
    level >= 3 and
    not blocked
):
    print("ok")
```

Rules:

- multiline conditions require outer grouping parentheses
- the closing `)` must appear before the final `:`
- line breaks inside the grouped condition are treated like spaces
- multiline conditions without grouping parentheses are invalid

Invalid example:

```gion
if ready and
   has_token:
    print("bad")
```

#### Ternary Expressions

Graphion also supports inline conditional expressions in the form:

```gion
result = "ready" if ready else "not ready"
```

Long ternary expressions can also span multiple physical lines when the whole expression is wrapped in grouping parentheses:

```gion
label = (
    "ready"
    if ready
    else "not ready"
)
```

Rules:

- the shape is `true_value if condition else false_value`
- the condition follows the same truth rules as `if` / `elif`
- the whole ternary expression produces a single scalar value
- nested ternary expressions are allowed, but become harder to read quickly
- multiline ternary expressions require outer grouping parentheses

Examples:

```gion
label = "ready" if ready else "not ready"
```

```gion
label = "outer" if ready else "inner" if fallback else "none"
```

Invalid example:

```gion
label = "ready"
if ready
else "not ready"
```

#### Reading Tips

To keep conditions and ternary expressions readable:

- prefer grouping parentheses when mixing several boolean operators
- prefer multiline grouped conditions once a single line starts to feel dense
- keep nested ternary expressions short
- switch back to a full `if` / `elif` / `else` block when the ternary stops being immediately obvious

Examples:

```gion
if (ready and has_token) or fallback:
    print("ok")
```

```gion
label = "ready" if ready else "not ready"
```

```gion
label = "outer" if ready else "inner" if fallback else "none"
```

The last form is valid, but a block is usually easier to read once nested ternary logic grows.

#### Conditional Precedence

When several conditional operators appear in the same expression, Graphion reads them in this order:

| Level | Operators | Notes |
| --- | --- | --- |
| 1 | `(...)` | Explicit grouping always wins |
| 2 | `==`, `!=`, `<`, `<=`, `>`, `>=` | Comparisons produce boolean results |
| 3 | `not` | Unary boolean negation |
| 4 | `and`, `nand` | `nand` shares the same level as `and` |
| 5 | `or`, `nor` | `nor` shares the same level as `or` |
| 6 | `value_if_true if condition else value_if_false` | Ternary expressions bind last among conditional forms |

Examples:

```gion
true or false and false
```

is read as:

```gion
true or (false and false)
```

```gion
not 1 == 2 and true
```

is read as:

```gion
(not (1 == 2)) and true
```

```gion
"ready" if true or false and false else "fallback"
```

is read as:

```gion
"ready" if (true or (false and false)) else "fallback"
```

#### Match Blocks

Graphion also supports value-based branching with `match`:

```gion
match status:
    "ready":
        print("go")
    "waiting":
        print("hold")
    default:
        print("unknown")
```

Rules:

- `match` is a statement, not an expression
- each non-`default` branch starts with a scalar literal followed by `:`
- supported case literals are `int`, `float`, `bool`, and `string`
- `default:` is optional, may appear at most once, and must be last
- grouped cases are allowed by stacking labels above the same block
- the matched expression is evaluated once, then branches are tested from top to bottom
- the first matching branch wins
- incompatible case types do not raise an error during execution; they simply do not match
- if the matched expression is a scalar literal and a case can never match it, Graphion can emit a pre-execution warning in debug mode

Grouped cases:

```gion
match level:
    1:
    2:
        print("small")
    default:
        print("other")
```

Invalid examples:

```gion
match value:
    default:
        print("x")
    1:
        print("y")
```

```gion
match value:
    1:
        print("x")
    1.0:
        print("y")
```

## Comments And Debug Warnings

Graphion currently supports two comment forms:

- `#` for line comments
- `/* ... */` for block comments

Warnings are not controlled by comments. The CLI prints warnings only in debug mode:

```powershell
.\build\Release\graphion.exe -d .\examples\11_graphs.gion
```

Without `-d`, the program runs normally and warnings are not printed to the terminal.

### Line Comments

`#` ignores the rest of the current line.

Examples:

```gion
# full-line comment
count = 40 # inline comment
count += 2
```

### Block Comments

`/* ... */` ignores everything until the closing `*/`.

Examples:

```gion
/*
multi-line note
about the next assignment
*/
message = "graphion"
```

```gion
ratio = /* inline block comment */ 7 / 2
```

Current rules:

- block comments can span multiple lines
- block comments are not nested
- comment markers inside string literals are treated as string text

Example:

```gion
message = "/* not a comment */"
```

### Comment Errors

This is currently a parse error:

```gion
/*
unterminated comment
count = 42
```

The current diagnostic is:

`unterminated block comment`
: parse error when `/*` does not have a matching closing `*/`

## Expressions And Operators

See [Operators](operators.md) for:

- arithmetic operators
- comparisons
- boolean logic
- precedence and grouping
- `bits` operator semantics
- string concatenation and `print(...)`

## Runtime Library

### Builtins

Current builtin functions:

- `abs(...)`
- `min(a, b)`
- `max(a, b)`
- `clamp(x, lo, hi)`
- `sqrt(x)`
- `cbrt(x)`
- `sin(x)`
- `csc(x)`
- `sec(x)`
- `cot(x)`
- `acsc(x)`
- `asec(x)`
- `acot(x)`
- `sech(x)`
- `csch(x)`
- `coth(x)`
- `sinh(x)`
- `asinh(x)`
- `acosh(x)`
- `cosh(x)`
- `cos(x)`
- `tan(x)`
- `tanh(x)`
- `atanh(x)`
- `asin(x)`
- `acos(x)`
- `atan(x)`
- `atan2(y, x)`
- `hypot(x, y)`
- `copysign(x, y)`
- `fma(a, b, c)`
- `fdim(x, y)`
- `remainder(x, y)`
- `rint(x)`
- `degrees(x)`
- `radians(x)`
- `isnan(x)`
- `isinf(x)`
- `isfinite(x)`
- `expm1(x)`
- `exp2(x)`
- `log1p(x)`
- `erf(x)`
- `erfc(x)`
- `gamma(x)`
- `lgamma(x)`
- `exp(x)`
- `ln(x)`
- `log(x, base)`
- `log10(x)`
- `log2(x)`
- `floor(x)`
- `ceil(x)`
- `round(x)`
- `trunc(x)`
- `fract(x)`
- `sign(x)`
- `len(x)`
- `contains(set, value)`
- `node_count(graph)`
- `edge_count(graph)`
- `is_directed(graph)`
- `is_weighted(graph)`
- `orientation(graph)`
- `node_ids(graph)`
- `nodes(graph)`
- `edges(graph)`
- `node_attrs(graph, node)`
- `edge_attrs(graph, from, to)`
- `edge_weight(graph, from, to)`
- `has_node(graph, node)`
- `has_edge(graph, from, to)`
- `neighbors(graph, node)`
- `indegree(graph, node)`
- `outdegree(graph, node)`
- `add_node(graph, node)` as a statement
- `add_edge(graph, from, to)` as a statement
- `remove_node(graph, node)` as a statement
- `remove_edge(graph, from, to)` as a statement
- `set_node_attrs(graph, node, attrs)` as a statement
- `set_edge_attrs(graph, from, to, attrs)` as a statement
- `set_edge_weight(graph, from, to, weight)` as a statement

See [Builtins](builtins.md).

### Constants

Current built-in scalar constants:

- `pi`
- `tau`
- `phi`
- `e`
- `nan`
- `inf`

See [Types](types.md) for their current values and basic usage.

## Errors

Current high-level error classes:

- parse errors
- unknown variable errors
- unknown operand errors
- runtime errors

Examples:

`unknown variable`
: compound assignment target does not exist

`unknown operand 'missing'`
: expression references a missing value

`division by zero`
: runtime arithmetic error

`incompatible operand types`
: runtime type error for numeric operators
