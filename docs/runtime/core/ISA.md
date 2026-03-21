# VM ISA (v0)

See also:

- `docs/runtime/contracts/ISA_VERSIONING.md` for version and compatibility policy
- `docs/runtime/core/IR.md` for frontend-to-bytecode bridge contract
- `docs/runtime/debugging/VM_ERRORS.md` for structured runtime error interpretation
- `docs/runtime/contracts/ISA_FIXTURES.md` for golden fixture format and expansion policy
- `docs/runtime/debugging/VM_SNAPSHOT.md` for deterministic snapshot/debug dump format
- `docs/runtime/debugging/VM_REPRO.md` for deterministic repro workflow
- `docs/runtime/debugging/REPRO_ARTIFACTS.md` for named repro artifact policy
- `docs/runtime/contracts/ISA_COMPATIBILITY_CHECKLIST.md` for opcode change review requirements
- `docs/runtime/debugging/FAILURE_CLASSIFICATION.md` for decode/load/execute triage

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
- `GVM_OP_FRONTIER_CLEAR (32)`: clear the frontier output buffer, write output length to `r[a]`
- `GVM_OP_FRONTIER_PUSH (33)`: append `r[a]` to the frontier output buffer, write output length to `r[b]`
- `GVM_OP_FRONTIER_FILTER_LT_IMM (34)`: filter input frontier values `< imm`, write output length to `r[a]`
- `GVM_OP_FRONTIER_MAP_ADD_IMM (35)`: map input frontier values with `value + imm`, write output length to `r[a]`
- `GVM_OP_FRONTIER_REDUCE_SUM (36)`: sum input frontier values into `r[a]`
- `GVM_OP_FRONTIER_SWAP (37)`: swap frontier input/output roles, write new input length to `r[a]`
- `GVM_OP_NEIGHBORS_OF (38)`: write the neighbors of node `r[a]` to the frontier output buffer
- `GVM_OP_NEIGHBORS_EXPAND (39)`: append neighbors of every node in the input frontier to the frontier output buffer, write output length to `r[a]`
- `GVM_OP_INCIDENT_OF (40)`: write the incident hyperedges of node `r[a]` to the frontier output buffer
- `GVM_OP_HYPEREDGE_NODES_OF (41)`: write the nodes of hyperedge `r[a]` to the frontier output buffer
- `GVM_OP_NEIGHBOR_WEIGHT_SUM (42)`: node id in `r[a]`, sum of outgoing edge weights to `r[b]`
- `GVM_OP_NEIGHBOR_ATTR_SUM (43)`: node id in `r[a]`, sum of outgoing edge attributes to `r[b]`
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

### `GVM_OP_FRONTIER_CLEAR (32)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a` |
| Outputs | clears the frontier output buffer, writes resulting output length (`0`) to `r[a]` |
| State changes | `frontier_output_len = 0` |
| Failure cases | `-3` invalid register, `-11` frontier buffers not bound |

### `GVM_OP_FRONTIER_PUSH (33)`

| Field | Value |
| --- | --- |
| Inputs | source value in `r[a]`, destination register `b` |
| Outputs | appends the value to the frontier output buffer, writes new output length to `r[b]` |
| State changes | `frontier_output[frontier_output_len++] = (uint32_t)r[a]` |
| Failure cases | `-3` invalid register, `-11` frontier buffers not bound, `-12` frontier capacity exceeded, `-13` negative or out-of-range frontier value |

### `GVM_OP_FRONTIER_FILTER_LT_IMM (34)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a`, signed threshold `imm` |
| Outputs | writes filtered output length to `r[a]` |
| State changes | output frontier becomes all input values where `(int64_t)value < imm`, preserving input order |
| Failure cases | `-3` invalid register, `-11` frontier buffers not bound, `-12` frontier capacity exceeded |

### `GVM_OP_FRONTIER_MAP_ADD_IMM (35)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a`, signed delta `imm` |
| Outputs | writes mapped output length to `r[a]` |
| State changes | output frontier becomes `input[i] + imm` for every input item |
| Failure cases | `-3` invalid register, `-11` frontier buffers not bound, `-12` frontier capacity exceeded, `-13` mapped value outside `[0, UINT32_MAX]` |

### `GVM_OP_FRONTIER_REDUCE_SUM (36)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a` |
| Outputs | writes the sum of input frontier values to `r[a]` |
| State changes | no frontier buffers modified |
| Failure cases | `-3` invalid register, `-11` frontier buffers not bound, `-13` reduction result exceeds `INT64_MAX` |

### `GVM_OP_FRONTIER_SWAP (37)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a` |
| Outputs | writes the new input frontier length to `r[a]` |
| State changes | previous output frontier becomes the next input frontier; new output length resets to `0` |
| Failure cases | `-3` invalid register, `-11` frontier buffers not bound |

### `GVM_OP_NEIGHBORS_OF (38)`

| Field | Value |
| --- | --- |
| Inputs | source node id in `r[a]` |
| Outputs | writes all CSR neighbors of the node to the frontier output buffer |
| State changes | output frontier becomes the adjacency list of `r[a]` |
| Failure cases | `-3` invalid register, `-5` missing CSR binding, `-9` invalid node id, `-11` frontier buffers not bound, `-12` frontier capacity exceeded |

### `GVM_OP_NEIGHBORS_EXPAND (39)`

| Field | Value |
| --- | --- |
| Inputs | destination register `a`; input frontier interpreted as node ids |
| Outputs | writes all neighbors of all input frontier nodes to the frontier output buffer and stores output length in `r[a]` |
| State changes | output frontier becomes the concatenated adjacency lists of all input frontier nodes, preserving input-node order and per-node neighbor order |
| Failure cases | `-3` invalid register, `-5` missing CSR binding, `-9` invalid node id in the input frontier, `-11` frontier buffers not bound, `-12` frontier capacity exceeded |

### `GVM_OP_INCIDENT_OF (40)`

| Field | Value |
| --- | --- |
| Inputs | source node id in `r[a]` |
| Outputs | writes all incident hyperedge ids of the node to the frontier output buffer |
| State changes | output frontier becomes the incident hyperedge list of `r[a]` |
| Failure cases | `-3` invalid register, `-8` missing hypergraph binding, `-9` invalid node id, `-11` frontier buffers not bound, `-12` frontier capacity exceeded |

### `GVM_OP_HYPEREDGE_NODES_OF (41)`

| Field | Value |
| --- | --- |
| Inputs | source hyperedge id in `r[a]` |
| Outputs | writes all node ids of the hyperedge to the frontier output buffer |
| State changes | output frontier becomes the node list of hyperedge `r[a]` |
| Failure cases | `-3` invalid register, `-8` missing hypergraph binding, `-10` invalid hyperedge id, `-11` frontier buffers not bound, `-12` frontier capacity exceeded |

### `GVM_OP_NEIGHBOR_WEIGHT_SUM (42)`

| Field | Value |
| --- | --- |
| Inputs | node id in `r[a]`, destination register `b` |
| Outputs | sum of outgoing CSR edge weights written to `r[b]` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-3` invalid register, `-5` missing CSR binding, `-9` invalid node id, `-14` missing CSR weights side data |

### `GVM_OP_NEIGHBOR_ATTR_SUM (43)`

| Field | Value |
| --- | --- |
| Inputs | node id in `r[a]`, destination register `b` |
| Outputs | sum of outgoing CSR edge attributes written to `r[b]` |
| State changes | `pc` advances by one instruction |
| Failure cases | `-3` invalid register, `-5` missing CSR binding, `-9` invalid node id, `-15` missing CSR edge-attribute side data |

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
- `GVM_ERR_FRONTIER_UNBOUND (-11)`: frontier buffers are not bound
- `GVM_ERR_FRONTIER_OVERFLOW (-12)`: frontier operation exceeded configured capacity
- `GVM_ERR_INVALID_FRONTIER_VALUE (-13)`: frontier value or reduction result violated the documented range contract
- `GVM_ERR_CSR_WEIGHTS_UNBOUND (-14)`: weighted graph opcode requires CSR weight side data
- `GVM_ERR_CSR_EDGE_ATTRS_UNBOUND (-15)`: edge-attribute opcode requires CSR edge-attribute side data
- Full layer-scoped error model and subsystem interpretation rules are defined in
  `docs/runtime/debugging/VM_ERRORS.md`.

## Compatibility policy

- Encoding changes must update:
  - `docs/runtime/core/ISA.md`
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
- `docs/runtime/contracts/ISA_FIXTURES.md`

Current fixture coverage includes:

- bytecode decode success and failure cases
- arithmetic execution semantics
- unknown-opcode rejection
- graph and hypergraph execution fixtures for currently documented opcodes
- frontier execution fixtures for the current `push/filter/map/reduce/swap` pipeline

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
- `GVM_OP_FRONTIER_CLEAR`: exact reset of the output frontier length.
- `GVM_OP_FRONTIER_PUSH`: exact append semantics for `uint32_t`-bounded frontier values.
- `GVM_OP_FRONTIER_FILTER_LT_IMM`: stable input-order filter into the output frontier.
- `GVM_OP_FRONTIER_MAP_ADD_IMM`: exact signed-delta map when the result stays inside `[0, UINT32_MAX]`.
- `GVM_OP_FRONTIER_REDUCE_SUM`: exact `int64` reduction while the result stays within the documented bound.
- `GVM_OP_FRONTIER_SWAP`: exact input/output role swap without dynamic allocation.
- `GVM_OP_NEIGHBORS_OF`: exact adjacency-list copy for the source node within the configured frontier capacity.
- `GVM_OP_NEIGHBORS_EXPAND`: exact concatenation of adjacency lists for the input frontier while preserving encounter order.
- `GVM_OP_INCIDENT_OF`: exact incident-hyperedge list copy for the source node within the configured frontier capacity.
- `GVM_OP_HYPEREDGE_NODES_OF`: exact hyperedge-node list copy for the source hyperedge within the configured frontier capacity.
