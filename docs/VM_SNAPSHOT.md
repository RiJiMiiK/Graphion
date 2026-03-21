# VM Snapshot Format

## Goal

Provide a stable, deterministic text dump of VM execution state for debugging and bug reproduction.

## API

Header:

- `src/vm/vm.h`

Function:

- `graphion_vm_write_snapshot(const graphion_vm *vm, char *buffer, size_t buffer_size)`

Behavior:

- returns the full snapshot length, excluding the terminating null byte
- writes a null-terminated string when `buffer_size > 0`
- truncation is allowed, but the returned length still reflects the full snapshot size

## Versioned Format

Current version header:

- `GRAPHION_VM_SNAPSHOT_V1`

Current field order:

1. `pc`
2. `program_bound`
3. `program_len`
4. `halted`
5. `deterministic_mode`
6. `arith_only_fastpath`
7. `arith_only_halt_terminated`
8. `csr_bound`
9. `hypergraph_bound`
10. `regs`

Example:

```text
GRAPHION_VM_SNAPSHOT_V1
pc=4
program_bound=1
program_len=4
halted=1
deterministic_mode=1
arith_only_fastpath=1
arith_only_halt_terminated=1
csr_bound=0
hypergraph_bound=0
regs=[42,35,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
```

## Stability Rules

- field order is part of the format
- field spelling is part of the format
- `regs` always dumps all 16 registers
- boolean fields are encoded as `0` or `1`
- any incompatible change must bump the format header version

## Intended Use

- deterministic repro attachments in bug reports
- parity debugging across dispatch variants
- asm-vs-C investigation when semantic drift is suspected
- local capture of final VM state after a failing fixture
