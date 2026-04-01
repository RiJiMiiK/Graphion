# Roadmap

> The rebuild is governed by [docs/runtime/core/REBUILD_CHARTER.md](docs/runtime/core/REBUILD_CHARTER.md).

This roadmap is intentionally narrowed to the current feature lane.
Completed work in this lane stays listed so we keep visible traceability.

## Current focus

The current active lane is conditional logic in the `.gion` scalar language.

## Conditionals

### Control flow

- [x] `if`
- [x] `elif`
- [x] `else`
- [x] support multiple `elif` clauses
- [x] support nested `if` blocks
- [x] accept strict boolean conditions plus integer `0` / `1`

### Comparisons

- [x] `==`
- [x] `!=`
- [x] `<`
- [x] `<=`
- [x] `>`
- [x] `>=`
- [x] finish and document comparison semantics across `int`, `float`, `bool`, and `string`
  - current `==` and `!=` support numeric comparison, same-kind `bool`, same-kind `string`, and a strict `int`/`bool` bridge only for `0` and `1`
  - current `==` and `!=` reject `float`/`bool`, `string`/non-`string`, and `int`/`bool` comparisons when the integer is outside `0` or `1`
  - current `<`, `<=`, `>`, and `>=` support numeric comparison only and reject `bool` / `string`

### Boolean logic

- [x] `and`
- [x] `or`
- [x] `not`
- [x] `nand`
- [x] `nor`
- [x] documented truth rules
- [x] fix boolean precedence so `and` / `nand` bind tighter than `or` / `nor`
- [x] decide and implement short-circuit behavior for `and` / `or`
- [x] tests for precedence and short-circuit behavior after it exists

### Documentation and examples

- [x] document nested `if` blocks explicitly
- [x] add a dedicated nested-`if` example in [examples/04_conditionals.gion](examples/04_conditionals.gion)
- [x] clarify in the docs that `else` binds to the `if` at the same indentation level

### Future condition features

- [x] multiline conditions with required grouping parentheses
  - target shape:
    ```gion
    if (
        ready and
        has_token and
        level >= 3 and
        not blocked
    ):
        print("ok")
    ```
  - multiline conditions without parentheses should remain invalid
- [x] ternary conditional expressions
  - target shape:
    ```gion
    label = "ready" if ready else "not ready"
    ```
- [x] multiline ternary expressions with required grouping parentheses
  - target shape:
    ```gion
    label = (
        "ready"
        if ready
        else "not ready"
    )
    ```
- [ ] pre-execution warnings with file-level opt-out
  - warnings should be collected and emitted before execution starts
  - a file-level directive should disable them globally:
    ```gion
    # graphion: warnings=off
    ```
  - errors must remain blocking even when warnings are disabled
- [ ] value-based branching with `match` / `switch`
