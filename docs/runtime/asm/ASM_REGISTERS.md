# ASM Register And ABI Notes

This project currently ships one x86_64 assembly hotpath:

- Source: `src/vm/hotpaths.s`
- Symbol: `graphion_vm_run_hotpath_arith_asm`
- ABI: x86_64 SysV

Function contract:

```c
size_t graphion_vm_run_hotpath_arith_asm(
  int64_t *regs,
  const graphion_insn *program,
  size_t program_len,
  int *halted
);
```

Argument registers (SysV):

- `rdi`: `regs`
- `rsi`: `program`
- `rdx`: `program_len`
- `rcx`: `halted`
- `rax`: return value (`pc`)

Working registers used by current hotpath:

- `r8`: program counter index
- `r9`: instruction pointer (`program + pc * 8`)
- `r10`: decoded register index `a`
- `r11`: decoded register index `b` or sign-extended immediate
- `eax`: decoded opcode byte

Behavior:

- Supports only arithmetic subset: `NOP`, `HALT`, `MOV_IMM`, `ADD`.
- Writes `*halted = 1` on `HALT`, otherwise leaves `0`.
- Returns next program counter (`pc`) in `rax`.

Safety:

- Keep changes small and benchmark-backed.
- Run `python scripts/quality/check_asm_safety.py`.
- Do not introduce privileged/system instructions.
