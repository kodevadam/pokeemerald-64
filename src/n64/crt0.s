/*
 * src/n64/crt0.s
 *
 * N64 boot stub — MIPS assembly entry point
 *
 * Execution flow:
 *   1. IPL3 (in the ROM header) copies the first ~1 MB of ROM into RDRAM
 *      and jumps to the entry point defined in the ROM header (0x80000400).
 *   2. This stub runs in RDRAM at 0x80000400 (cached KSEG0).
 *   3. We set up the stack pointer, clear BSS, initialise the software
 *      hardware buffers, then jump to N64Main() in platform.c.
 *
 * Register conventions (MIPS o32 ABI):
 *   $zero / $0    — always zero
 *   $at  / $1     — assembler temporary
 *   $v0-$v1       — return values
 *   $a0-$a3       — function arguments
 *   $t0-$t9       — caller-saved temporaries
 *   $s0-$s7       — callee-saved
 *   $gp           — global pointer
 *   $sp           — stack pointer
 *   $fp / $s8     — frame pointer
 *   $ra           — return address
 *
 * CP0 registers used here:
 *   $12 (Status)  — CPU status / interrupt enable
 *   $13 (Cause)   — exception cause
 */

#include "n64/asm_defs.h"

    .section .boot, "ax", @progbits
    .set    noreorder
    .set    noat
    .align  2

/* -----------------------------------------------------------------------
 * __n64_boot — ROM entry point
 * IPL3 jumps here after copying the ROM image into RDRAM.
 * --------------------------------------------------------------------- */
    .globl  __n64_boot
    .type   __n64_boot, @function
__n64_boot:
    /* Disable all interrupts and clear BEV (use normal exception vectors) */
    mfc0    $t0, $12            /* read CP0 Status                        */
    li      $t1, ~0x00010001   /* clear IE (bit 0) and EXL (not needed)  */
    and     $t0, $t0, $t1
    ori     $t0, $t0, 0x0400   /* CP0 usable                              */
    mtc0    $t0, $12

    /* Set up stack — __stack_top is exported by the linker script */
    la      $sp, __stack_top
    addiu   $sp, $sp, -8       /* ABI: maintain 8-byte alignment          */

    /* Set global pointer */
    la      $gp, _gp

    /* -----------------------------------------------------------------------
     * Copy the .text section from ROM (LMA) to RDRAM (VMA).
     *
     * The N64 IPL3 only copies the first 1MB of ROM to RDRAM[0x80000000].
     * Our .text section is ~2.9MB.  Functions past the 1MB mark (e.g.
     * AgbMain at 0x801c4f18) are NOT in RDRAM after IPL3.  We must copy
     * the full section here before calling any C function.
     *
     * We copy word-by-word from the KSEG1 uncached ROM PI bus address
     * (__text_lma = 0xB0001700) to the RDRAM VMA (__text_start = 0x80000700).
     * This overwrites the 1MB already copied by IPL3 with identical data —
     * harmless but correct.
     * --------------------------------------------------------------------- */
    la      $t0, __text_lma     /* ROM source (KSEG1 uncached PI bus)       */
    la      $t1, __text_start   /* RDRAM destination VMA                    */
    la      $t2, __text_end
    beq     $t1, $t2, .Ltext_done
    nop
.Ltext_copy:
    lw      $t3, 0($t0)
    sw      $t3, 0($t1)
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    bne     $t1, $t2, .Ltext_copy
    nop
.Ltext_done:

    /* Writeback and invalidate dcache for the written .text range.
     * CACHE 0x15 = HIT_WRITEBACK_INVALIDATE_D: flushes dirty dcache lines
     * to RDRAM so the icache can read the correct code bytes.              */
    sync
    la      $t0, __text_start
    la      $t1, __text_end
.Ltext_dcache_flush:
    cache   0x15, 0($t0)
    addiu   $t0, $t0, 32
    bne     $t0, $t1, .Ltext_dcache_flush
    nop

    /* Invalidate instruction cache over the full .text range.
     * CACHE 0x10 = HIT_INVALIDATE_I: forces the icache to re-fill from
     * RDRAM, which now has the correct code bytes.                         */
    la      $t0, __text_start
    la      $t1, __text_end
.Ltext_icache_flush:
    cache   0x10, 0($t0)
    addiu   $t0, $t0, 32
    bne     $t0, $t1, .Ltext_icache_flush
    nop

    /* -----------------------------------------------------------------------
     * Copy initialised data sections from ROM (LMA) to RDRAM (VMA).
     * Sections .data, .ewram_data, .iwram_data, .common_data are stored in
     * ROM but must run from RDRAM.  __data_lma is the ROM source address
     * (0xB0xxxxxx via KSEG1 PI bus), __data_start/__data_end are the RDRAM
     * destination.  We read word-by-word from the uncached ROM address.
     * --------------------------------------------------------------------- */
    la      $t0, __data_lma     /* ROM source (KSEG1 uncached = 0xB0xxxxxx) */
    la      $t1, __data_start   /* RDRAM destination VMA                    */
    la      $t2, __data_end
    beq     $t1, $t2, .Ldata_done
    nop
.Ldata_copy:
    lw      $t3, 0($t0)
    sw      $t3, 0($t1)
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    bne     $t1, $t2, .Ldata_copy
    nop
.Ldata_done:

    /* -----------------------------------------------------------------------
     * Clear BSS
     * Linker exports __bss_start and __bss_end (8-byte aligned)
     * --------------------------------------------------------------------- */
    la      $t0, __bss_start
    la      $t1, __bss_end
    beq     $t0, $t1, .Lbss_done
    nop
.Lbss_loop:
    sw      $zero, 0($t0)
    addiu   $t0, $t0, 4
    bne     $t0, $t1, .Lbss_loop
    nop
.Lbss_done:

    /* -----------------------------------------------------------------------
     * Install N64 exception vectors at 0x80000000.
     * Each vector is 0x80 bytes; we copy 8 × 4-byte words from our handler
     * stubs.  We use cached writes; flush with cache ops afterwards.
     *
     * Exception vector layout (N64 / MIPS III):
     *   0x80000000  TLB Refill (32-bit addressing)
     *   0x80000080  XTLB Refill (64-bit addressing — unused)
     *   0x80000100  Cache Error
     *   0x80000180  General Exception (used for interrupts, syscalls, …)
     * --------------------------------------------------------------------- */
    la      $t0, __n64_exception_vectors
    li      $t1, 0x80000000
    li      $t2, 0x80000200     /* end of vector area                      */
.Lvec_copy:
    lw      $t3, 0($t0)
    sw      $t3, 0($t1)
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    bne     $t1, $t2, .Lvec_copy
    nop

    /* Flush instruction cache to see the new vectors */
    li      $t0, 0x80000000
    li      $t1, 0x80000200
.Licache_flush:
    cache   0x10, 0($t0)        /* Index_Invalidate_I                      */
    addiu   $t0, $t0, 32
    bne     $t0, $t1, .Licache_flush
    nop

    /* -----------------------------------------------------------------------
     * Call N64Main() — does not return
     * --------------------------------------------------------------------- */
    jal     N64Main
    nop

    /* If N64Main ever returns, loop forever */
.Lhalt:
    b       .Lhalt
    nop

    .size   __n64_boot, . - __n64_boot

/* -----------------------------------------------------------------------
 * Exception vector stubs
 * These are copied verbatim to 0x80000000 – 0x800001FF.
 * Each handler jumps to the corresponding C handler in interrupt.c.
 * --------------------------------------------------------------------- */
    .section .boot, "ax", @progbits
    .align  5   /* 32-byte alignment required for cache line copies        */

    .globl  __n64_exception_vectors
__n64_exception_vectors:

/* 0x000: TLB Refill */
    j       __n64_tlb_handler
    nop
    .space  0x80 - 8

/* 0x080: XTLB Refill (64-bit, treated same as TLB on our 32-bit build) */
    j       __n64_tlb_handler
    nop
    .space  0x80 - 8

/* 0x100: Cache Error */
    j       __n64_cache_error_handler
    nop
    .space  0x80 - 8

/* 0x180: General Exception (all interrupts and most exceptions land here) */
    j       __n64_general_exception_handler
    nop
    .space  0x80 - 8

/* -----------------------------------------------------------------------
 * __n64_general_exception_handler
 *
 * Saves the full register context on the stack, calls N64_DispatchIntr()
 * in C, then restores and returns from exception (ERET).
 *
 * Stack frame layout (grows downward, 8-byte aligned per MIPS o32):
 *   +0x00   $at
 *   +0x04   $v0
 *   +0x08   $v1
 *   +0x0C   $a0
 *   +0x10   $a1
 *   +0x14   $a2
 *   +0x18   $a3
 *   +0x1C   $t0
 *   +0x20   $t1
 *   +0x24   $t2
 *   +0x28   $t3
 *   +0x2C   $t4
 *   +0x30   $t5
 *   +0x34   $t6
 *   +0x38   $t7
 *   +0x3C   $t8
 *   +0x40   $t9
 *   +0x44   $gp
 *   +0x48   $ra
 *   +0x4C   EPC (CP0 $14)
 *   +0x50   HI
 *   +0x54   LO
 *   total: 0x58 bytes
 * --------------------------------------------------------------------- */
    .text
    .align  2
    .set    noreorder
    .set    noat

    .globl  __n64_general_exception_handler
    .type   __n64_general_exception_handler, @function
__n64_general_exception_handler:
    addiu   $sp, $sp, -0x58
    sw      $at, 0x00($sp)
    sw      $v0, 0x04($sp)
    sw      $v1, 0x08($sp)
    sw      $a0, 0x0C($sp)
    sw      $a1, 0x10($sp)
    sw      $a2, 0x14($sp)
    sw      $a3, 0x18($sp)
    sw      $t0, 0x1C($sp)
    sw      $t1, 0x20($sp)
    sw      $t2, 0x24($sp)
    sw      $t3, 0x28($sp)
    sw      $t4, 0x2C($sp)
    sw      $t5, 0x30($sp)
    sw      $t6, 0x34($sp)
    sw      $t7, 0x38($sp)
    sw      $t8, 0x3C($sp)
    sw      $t9, 0x40($sp)
    sw      $gp, 0x44($sp)
    sw      $ra, 0x48($sp)
    mfc0    $t0, $14            /* EPC                                     */
    sw      $t0, 0x4C($sp)
    mfhi    $t0
    sw      $t0, 0x50($sp)
    mflo    $t0
    sw      $t0, 0x54($sp)

    /* Restore GP for C code */
    la      $gp, _gp

    /* Call N64_DispatchIntr() */
    jal     N64_DispatchIntr
    nop

    /* Restore registers */
    lw      $at, 0x00($sp)
    lw      $v0, 0x04($sp)
    lw      $v1, 0x08($sp)
    lw      $a0, 0x0C($sp)
    lw      $a1, 0x10($sp)
    lw      $a2, 0x14($sp)
    lw      $a3, 0x18($sp)
    lw      $t0, 0x1C($sp)
    lw      $t1, 0x20($sp)
    lw      $t2, 0x24($sp)
    lw      $t3, 0x28($sp)
    lw      $t4, 0x2C($sp)
    lw      $t5, 0x30($sp)
    lw      $t6, 0x34($sp)
    lw      $t7, 0x38($sp)
    lw      $t8, 0x3C($sp)
    lw      $t9, 0x40($sp)
    lw      $gp, 0x44($sp)
    lw      $ra, 0x48($sp)
    lw      $k0, 0x4C($sp)     /* restore EPC into $k0 (kernel scratch)   */
    lw      $k1, 0x50($sp)     /* HI                                      */
    mthi    $k1
    lw      $k1, 0x54($sp)     /* LO                                      */
    mtlo    $k1
    addiu   $sp, $sp, 0x58
    mtc0    $k0, $14            /* restore EPC                             */
    eret                        /* return from exception, re-enable intrs  */
    .size   __n64_general_exception_handler, . - __n64_general_exception_handler

/* -----------------------------------------------------------------------
 * TLB handler — should not be triggered; just halt
 * --------------------------------------------------------------------- */
    .globl  __n64_tlb_handler
    .type   __n64_tlb_handler, @function
__n64_tlb_handler:
.Ltlb_halt:
    b       .Ltlb_halt
    nop
    .size   __n64_tlb_handler, . - __n64_tlb_handler

/* -----------------------------------------------------------------------
 * Cache error handler — should not be triggered; just halt
 * --------------------------------------------------------------------- */
    .globl  __n64_cache_error_handler
    .type   __n64_cache_error_handler, @function
__n64_cache_error_handler:
.Lcache_halt:
    b       .Lcache_halt
    nop
    .size   __n64_cache_error_handler, . - __n64_cache_error_handler

/* -----------------------------------------------------------------------
 * IntrMain — GBA interrupt dispatcher shim
 *
 * The game copies IntrMain into IntrMain_Buffer and sets INTR_VECTOR to
 * point to it.  On N64, our general exception handler calls N64_DispatchIntr
 * which in turn calls the game's C interrupt handlers (VBlankIntr, etc.),
 * so IntrMain_Buffer is never actually jumped to.  We provide a minimal
 * stub so that the sizeof() and DmaCopy32() calls in main.c compile and
 * run without crashing.
 * --------------------------------------------------------------------- */
    .globl  IntrMain
    .type   IntrMain, @function
    .section .text.IntrMain, "ax"
IntrMain:
    jr      $ra
    nop
    /* Pad to 0x200 words so the DmaCopy32 in InitIntrHandlers doesn't
     * read past the symbol.  IntrMain_Buffer is 0x200 words = 0x800 bytes. */
    .space  0x800 - 8
    .size   IntrMain, . - IntrMain
