# VM ISA (v0)

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
- `GVM_OP_ADD (3)`: `r[a] += r[b]`
- `GVM_OP_BFS_LEVELS (16)`: source node in `r[a]`, visited count written to `r[b]`
- `GVM_OP_INCIDENT_COUNT (17)`: node id in `r[a]`, incident hyperedge count to `r[b]`
- `GVM_OP_HYPEREDGE_SIZE (18)`: hyperedge id in `r[a]`, size to `r[b]`
- `GVM_OP_INCIDENT_SUM (19)`: node id in `r[a]`, sum of incident hyperedge ids to `r[b]`
- `GVM_OP_HYPEREDGE_NODE_SUM (20)`: hyperedge id in `r[a]`, sum of node ids to `r[b]`

## Error behavior

- Invalid VM/program pointer: `-1`
- Invalid register in `MOV_IMM`: `-2`
- Invalid register in `ADD`: `-3`
- Unknown opcode: `-4`

## Compatibility policy

- Encoding changes must update:
  - `docs/ISA.md`
  - `src/parser/bytecode.*`
  - tests and fuzz harnesses
- Backward-incompatible changes must be called out in `CHANGELOG.md`.
