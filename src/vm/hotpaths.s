/* SPDX-License-Identifier: MIT */

/* x86_64 SysV ABI
 * size_t graphion_vm_run_hotpath_arith_asm(
 *   int64_t *regs,               // rdi
 *   const graphion_insn *prog,   // rsi
 *   size_t prog_len,             // rdx
 *   int *halted                  // rcx
 * )
 */

    .text
    .intel_syntax noprefix
    .globl graphion_vm_run_hotpath_arith_asm
graphion_vm_run_hotpath_arith_asm:
    xor r8, r8
    mov dword ptr [rcx], 0

.L_loop:
    cmp r8, rdx
    jae .L_done

    lea r9, [rsi + r8 * 8]
    movzx eax, byte ptr [r9]

    cmp eax, 0
    je .L_next
    cmp eax, 1
    je .L_halt
    cmp eax, 2
    je .L_mov_imm
    cmp eax, 3
    je .L_add
    jmp .L_done

.L_mov_imm:
    movzx r10d, byte ptr [r9 + 1]
    movsxd r11, dword ptr [r9 + 4]
    mov qword ptr [rdi + r10 * 8], r11
    jmp .L_next

.L_add:
    movzx r10d, byte ptr [r9 + 1]
    movzx r11d, byte ptr [r9 + 2]
    mov rax, qword ptr [rdi + r10 * 8]
    add rax, qword ptr [rdi + r11 * 8]
    mov qword ptr [rdi + r10 * 8], rax
    jmp .L_next

.L_halt:
    mov dword ptr [rcx], 1
    inc r8
    mov rax, r8
    ret

.L_next:
    inc r8
    jmp .L_loop

.L_done:
    mov rax, r8
    ret

    .section .note.GNU-stack,"",@progbits
