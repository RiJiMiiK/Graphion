# VM ISA (v0)

See also:

- `docs/ISA_VERSIONING.md` for version and compatibility policy
- `docs/IR.md` for frontend-to-bytecode bridge contract
- `docs/VM_ERRORS.md` for structured runtime error interpretation
- `docs/ISA_FIXTURES.md` for golden fixture format and expansion policy
- `docs/VM_SNAPSHOT.md` for deterministic snapshot/debug dump format

## Encoding

Instruction binary encoding is fixed to 7 bytes:

1. `op` (1 byte)
2. `a` (1 byte)
3. `b` (1 byte)
4. `imm` (4 bytes, little-endian signed int32)

## Register model

- 16 general-purpose integer registers: `r0..r15`
- Register width: signed 64-bit

## Opcodes

- `GVM_OP_NOP (0)`: no operation
- `GVM_OP_HALT (1)`: stop execution
- `GVM_OP_MOV_IMM (2)`: `r[a] = imm`
- `GVM_OP_ADD (3)`: `r[a] += r[b]` with two's-complement wraparound semantics
- `GVM_OP_BFS_LEVELS (16)`: source node in `r[a]`, visited count written to `r[b]`
- `GVM_OP_INCIDENT_COUNT (17)`: node id in `r[a]`, incident hyperedge count to `r[b]`
- `GVM_OP_HYPEREDGE_SIZE (18)`: hyperedge id in `r[a]`, size to `r[b]`
- `GVM_OP_INCIDENT_SUM (19)`: node id in `r[a]`, sum of incident hyperedge ids to `r[b]`
- `GVM_OP_HYPEREDGE_NODE_SUM (20)`: hyperedge id in `r[a]`, sum of node ids to `r[b]`

## Opcode semantics tables

### `GVM_OP_NOP (0)`

| Field | Value |
| --- | --- |
| Inputs | none |
| Outputs | none |
| State changes | `pc` advances by one instruction |
| Failure cases | none |

### `GVM_OP_HALT (1)`

| Field | Value |
| --- | --- |
| Inputs | none |
| Outputs | `vm.halted = true` |
| State changes | `pc` advances by one instruction, execution stops |
| Failure cases | none |

### `GVM_OP_MOV_IMM (2)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a`, signed `imm` |
| Outputs | `r[a] = sign_extend_i32_to_i64(imm)` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-2` if `a` is not a valid VM register |

### `GVM_OP_ADD (3)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a`, source register `b` |
| Outputs | `r[a] = r[a] + r[b]` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-3` if `a` or `b` is not a valid VM register |

Notes:

- arithmetic uses explicit two's-complement wraparound semantics

### `GVM_OP_BFS_LEVELS (16)`

| Field | Value |
| --- | --- |
| Inputs | source node id in `r[a]`, destination register `b` |
| Outputs | visited-node count written to `r[b]` |
| State changes | `bfs_levels` and `bfs_queue` scratch buffers are used; `pc` advances by one instruction |
| Failure cases | `-3` invalid register, `-5` missing CSR/BFS bindings, `-6` invalid source node id, `-7` CSR BFS kernel failure |

### `GVM_OP_INCIDENT_COUNT (17)`

| Field | Value |
| --- | --- |
| Inputs | node id in `r[a]`, destination register `b` |
| Outputs | incident hyperedge count written to `r[b]` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-3` invalid register, `-8` missing hypergraph binding, `-9` invalid node id |

### `GVM_OP_HYPEREDGE_SIZE (18)`

| Field | Value |
| --- | --- |
| Inputs | hyperedge id in `r[a]`, destination register `b` |
| Outputs | hyperedge size written to `r[b]` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-3` invalid register, `-8` missing hypergraph binding, `-10` invalid hyperedge id |

### `GVM_OP_INCIDENT_SUM (19)`

| Field | Value |
| --- | --- |
| Inputs | node id in `r[a]`, destination register `b` |
| Outputs | sum of incident hyperedge ids written to `r[b]` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-3` invalid register, `-8` missing hypergraph binding, `-9` invalid node id |

### `GVM_OP_HYPEREDGE_NODE_SUM (20)`

| Field | Value |
| --- | --- |
| Inputs | hyperedge id in `r[a]`, destination register `b` |
| Outputs | sum of node ids in the hyperedge written to `r[b]` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-3` invalid register, `-8` missing hypergraph binding, `-10` invalid hyperedge id |

## Error behavior

- `GVM_ERR_INVALID_ARG (-1)`: invalid VM/program pointer or entry-point argument
- `GVM_ERR_INVALID_MOV_IMM_REG (-2)`: invalid register in `MOV_IMM`
- `GVM_ERR_INVALID_REG (-3)`: invalid register in register-based VM opcodes
- `GVM_ERR_UNKNOWN_OPCODE (-4)`: unknown opcode
- Full layer-scoped error model and subsystem interpretation rules are defined in
  `docs/VM_ERRORS.md`.

## Compatibility policy

- Encoding changes must update:
  - `docs/ISA.md`
  - `src/parser/bytecode.*`
  - tests and fuzz harnesses
- Backward-incompatible changes must be called out in `CHANGELOG.md`.
- `v0.x` bytecode compatibility is not guaranteed across revisions.
- `v1.0` will be the first compatibility-frozen ISA line.

## Deterministic execution mode

- The VM exposes `graphion_vm_set_deterministic(vm, true)` for reproducible
  debugging runs.
- In deterministic mode, execution uses the portable switch-dispatch path.
- Deterministic mode bypasses fast arithmetic specialization and any asm-backed
  execution path.
- Instruction semantics and observable register / `pc` results remain the same.
- `graphion_vm_write_snapshot(...)` provides a versioned state dump for
  deterministic repro capture.

## Golden conformance coverage

Golden ISA fixtures live in:

- `tests/test_isa.c`
- `docs/ISA_FIXTURES.md`

Current fixture coverage includes:

- bytecode decode success and failure cases
- arithmetic execution semantics
- unknown-opcode rejection
- graph and hypergraph execution fixtures for currently documented opcodes

## Overflow policy

- `GVM_OP_MOV_IMM`: exact sign-extension from `int32` immediate to `int64` register.
- `GVM_OP_ADD`: two's-complement wraparound on overflow.
- `GVM_OP_BFS_LEVELS`: exact non-negative visited-count result with current graph
  storage bounds.
- `GVM_OP_INCIDENT_COUNT`: exact non-negative result with current hypergraph
  storage bounds.
- `GVM_OP_HYPEREDGE_SIZE`: exact non-negative result with current hypergraph
  storage bounds.
- `GVM_OP_INCIDENT_SUM`: exact result for current `uint32_t`-bounded hypergraph
  inputs, then stored in `int64`.
- `GVM_OP_HYPEREDGE_NODE_SUM`: exact result for current `uint32_t`-bounded
  hypergraph inputs, then stored in `int64`.
