;---------------------------------------------------------------;
;  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  ;
;---------------------------------------------------------------;

; SaveJumpPoint and JumpTo for x64 Windows. The offsets below are JumpPoint's field offsets, which its header
; pins down with static asserts. Only the registers the Win64 ABI calls non-volatile are saved; the stack stays
; the same one, so the TIB's stack and fiber fields are left alone.
;
; Neither function has a prologue, so FRAME with an immediate .endprolog describes both: a stack walk through
; SaveJumpPoint finds its return address at [rsp], and nothing walks through JumpTo, which doesn't return.

Xmm6Offset           equ 000h
Xmm7Offset           equ 010h
Xmm8Offset           equ 020h
Xmm9Offset           equ 030h
Xmm10Offset          equ 040h
Xmm11Offset          equ 050h
Xmm12Offset          equ 060h
Xmm13Offset          equ 070h
Xmm14Offset          equ 080h
Xmm15Offset          equ 090h
MxcsrOffset          equ 0A0h
X87ControlWordOffset equ 0A4h
RbxOffset            equ 0A8h
RbpOffset            equ 0B0h
RdiOffset            equ 0B8h
RsiOffset            equ 0C0h
R12Offset            equ 0C8h
R13Offset            equ 0D0h
R14Offset            equ 0D8h
R15Offset            equ 0E0h
RspOffset            equ 0E8h
RipOffset            equ 0F0h

.code

; int SaveJumpPoint(JumpPoint& point)
;   rcx: the point to fill in
SaveJumpPoint PROC FRAME
    .endprolog

    movaps  [rcx+Xmm6Offset], xmm6
    movaps  [rcx+Xmm7Offset], xmm7
    movaps  [rcx+Xmm8Offset], xmm8
    movaps  [rcx+Xmm9Offset], xmm9
    movaps  [rcx+Xmm10Offset], xmm10
    movaps  [rcx+Xmm11Offset], xmm11
    movaps  [rcx+Xmm12Offset], xmm12
    movaps  [rcx+Xmm13Offset], xmm13
    movaps  [rcx+Xmm14Offset], xmm14
    movaps  [rcx+Xmm15Offset], xmm15
    stmxcsr [rcx+MxcsrOffset]
    fnstcw  [rcx+X87ControlWordOffset]

    mov     [rcx+RbxOffset], rbx
    mov     [rcx+RbpOffset], rbp
    mov     [rcx+RdiOffset], rdi
    mov     [rcx+RsiOffset], rsi
    mov     [rcx+R12Offset], r12
    mov     [rcx+R13Offset], r13
    mov     [rcx+R14Offset], r14
    mov     [rcx+R15Offset], r15

    ; the caller's stack pointer is the one above this call's return address
    lea     rax, [rsp+8]
    mov     [rcx+RspOffset], rax

    ; and the caller resumes at that return address
    mov     rax, [rsp]
    mov     [rcx+RipOffset], rax

    xor     eax, eax
    ret
SaveJumpPoint ENDP

; void JumpTo(JumpPoint& point, int value)
;   rcx: the point to resume at
;   edx: what SaveJumpPoint returns there
JumpTo PROC FRAME
    .endprolog

    movaps  xmm6, [rcx+Xmm6Offset]
    movaps  xmm7, [rcx+Xmm7Offset]
    movaps  xmm8, [rcx+Xmm8Offset]
    movaps  xmm9, [rcx+Xmm9Offset]
    movaps  xmm10, [rcx+Xmm10Offset]
    movaps  xmm11, [rcx+Xmm11Offset]
    movaps  xmm12, [rcx+Xmm12Offset]
    movaps  xmm13, [rcx+Xmm13Offset]
    movaps  xmm14, [rcx+Xmm14Offset]
    movaps  xmm15, [rcx+Xmm15Offset]
    ldmxcsr [rcx+MxcsrOffset]
    fldcw   [rcx+X87ControlWordOffset]

    mov     rbx, [rcx+RbxOffset]
    mov     rbp, [rcx+RbpOffset]
    mov     rdi, [rcx+RdiOffset]
    mov     rsi, [rcx+RsiOffset]
    mov     r12, [rcx+R12Offset]
    mov     r13, [rcx+R13Offset]
    mov     r14, [rcx+R14Offset]
    mov     r15, [rcx+R15Offset]

    ; read both out of the point before the stack moves, since the point may live in a frame being returned to
    mov     r10, [rcx+RipOffset]
    mov     rsp, [rcx+RspOffset]

    mov     eax, edx
    jmp     r10
JumpTo ENDP

END
