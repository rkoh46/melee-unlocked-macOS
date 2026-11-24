// Linux AArch64 (AAPCS64) variadic entry points for the C library functions the game image imports.
// The image is a Linux ELF: it calls printf-like functions with the variadic arguments in x0-x7 and
// v0-v7 and then on the stack, and passes a va_list as a pointer to the AAPCS64 structure. Apple's
// arm64 ABI puts every variadic argument on the stack and its va_list is a plain pointer, so the game
// cannot call macOS's own printf family. Each entry here builds the AAPCS64 va_list exactly as a
// Linux callee would (va_start) and calls a C function that reads it (mu_imports_macos.c).
// SPDX-License-Identifier: GPL-3.0-or-later

// struct lx_va { void* stack; void* gr_top; void* vr_top; int gr_offs; int vr_offs; }
.macro LXVA name, target, named, vareg
    .text
    .p2align 2
    .globl \name
\name:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    sub     sp, sp, #224                 // 64 bytes general registers, 128 vector registers, 32 the va_list
    stp     x0, x1, [x29, #-64]
    stp     x2, x3, [x29, #-48]
    stp     x4, x5, [x29, #-32]
    stp     x6, x7, [x29, #-16]
    stp     q0, q1, [x29, #-192]
    stp     q2, q3, [x29, #-160]
    stp     q4, q5, [x29, #-128]
    stp     q6, q7, [x29, #-96]
    add     x9, x29, #16
    str     x9, [sp, #0]                 // __stack: the caller's stack arguments
    str     x29, [sp, #8]                // __gr_top: end of the general register area
    sub     x9, x29, #64
    str     x9, [sp, #16]                // __vr_top: end of the vector register area
    mov     w9, #(-(8 - \named) * 8)
    str     w9, [sp, #24]                // __gr_offs
    mov     w9, #-128
    str     w9, [sp, #28]                // __vr_offs (no named floating-point arguments)
    mov     \vareg, sp
    bl      \target
    mov     sp, x29
    ldp     x29, x30, [sp], #16
    ret
.endm

LXVA _mu_lx_printf,   _mu_vprintf_lx,   1, x1
LXVA _mu_lx_fprintf,  _mu_vfprintf_lx,  2, x2
LXVA _mu_lx_sprintf,  _mu_vsprintf_lx,  2, x2
LXVA _mu_lx_snprintf, _mu_vsnprintf_lx, 3, x3
