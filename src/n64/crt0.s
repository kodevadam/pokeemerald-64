/*
 * src/n64/crt0.s
 *
 * N64 boot stub — MIPS assembly entry point
 *
 * Execution flow:
 *   1. libdragon IPL3 reads ROM[0x1040] = 0x4000 (16 KB boot size).
 *      It DMA's ROM[0x1048..0x5048] → RDRAM[RDRAM_SIZE-0x4000..RDRAM_SIZE-1]
 *      then jumps to 0x80000000 + RDRAM_SIZE - 0x4000.
 *      On 8 MB RDRAM (expansion pak): runs from 0x807FC000.
 *   2. This stub runs from the top of RDRAM (0x807FC000 with 8 MB RDRAM).
 *      It PI-DMA's .text from ROM → RDRAM[0x80000400..] and .data from
 *      ROM → its RDRAM VMA, then jumps to N64Main() in platform.c.
 *   3. Expansion pak (8 MB RDRAM) is REQUIRED for the VMA to match.
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
    /* -----------------------------------------------------------------------
     * DIAG-RED: Pure-assembly VI init at the very first instruction of
     * __n64_boot.  If RED appears, the CPU reached our code.
     * Framebuffer at 0x807B0000 (physical 0x7B0000) -- the same buffer the
     * VI uses once the game is running, so it does not collide with the
     * RDRAM window the const data is copied into.
     * VI registers at 0xA4400000 (KSEG1 uncached).
     * --------------------------------------------------------------------- */
    li      $t0, 0xA4400000         /* VI base (KSEG1 uncached)              */
    li      $t1, 0x00003202         /* VI_STATUS: 16bpp RGBA5551             */
    sw      $t1, 0x00($t0)          /* VI_STATUS                              */
    li      $t1, 0x007B0000         /* VI_ORIGIN: physical 0x7B0000           */
    sw      $t1, 0x04($t0)          /* VI_ORIGIN                              */
    li      $t1, 320
    sw      $t1, 0x08($t0)          /* VI_WIDTH                               */
    li      $t1, 0x00000002
    sw      $t1, 0x0C($t0)          /* VI_INTR                                */
    /* VI_CURRENT (0x10) is read-only — skip                                  */
    li      $t1, 0x03E52239
    sw      $t1, 0x14($t0)          /* VI_BURST  (timing)                     */
    li      $t1, 0x0000020D
    sw      $t1, 0x18($t0)          /* VI_V_SYNC                              */
    li      $t1, 0x00000C15
    sw      $t1, 0x1C($t0)          /* VI_H_SYNC                              */
    li      $t1, 0x0C150C15
    sw      $t1, 0x20($t0)          /* VI_LEAP   (VI_H_SYNC_LEAP)             */
    li      $t1, 0x006C02EC
    sw      $t1, 0x24($t0)          /* VI_H_START                             */
    li      $t1, 0x002501FF
    sw      $t1, 0x28($t0)          /* VI_V_START                             */
    li      $t1, 0x000E0204
    sw      $t1, 0x2C($t0)          /* VI_V_BURST                             */
    li      $t1, 0x00000200
    sw      $t1, 0x30($t0)          /* VI_X_SCALE                             */
    li      $t1, 0x00000400
    sw      $t1, 0x34($t0)          /* VI_Y_SCALE                             */
    /* Fill framebuffer with RED (RGBA5551: R=31, G=0, B=0, A=1 = 0xF801)
     * Use KSEG1 (uncached, 0xA0000000+) so writes go directly to RDRAM —
     * VI reads RDRAM directly and won't see cached-but-not-written-back data */
    lui     $t2, 0xA07B             /* $t2 = 0xA07B0000 (KSEG1 uncached)     */
    lui     $t3, 0xA07D
    ori     $t3, $t3, 0x5800        /* $t3 = 0xA07D5800 (end: 0xA07B0000+320*240*2) */
    li      $t1, 0xF801F801         /* two red pixels packed into one word    */
.Ldiag_red_fill:
    sw      $t1, 0($t2)
    addiu   $t2, $t2, 4
    bne     $t2, $t3, .Ldiag_red_fill
    nop
    /* END DIAG-RED -------------------------------------------------------- */

    /* -----------------------------------------------------------------------
     * Tell the PIF the boot process is over. The PIF gives the CPU five
     * seconds to send this and halts it otherwise, and the ROM-to-RDRAM
     * copies below take longer than that, so it has to go out before them
     * rather than from N64Main().
     * PIF RAM byte 0x3F, bit 3 = "boot terminated"; the containing word is
     * at 0xBFC007FC.
     * --------------------------------------------------------------------- */
    li      $t0, 0xBFC007FC
    li      $t1, 0x00000008
    sw      $t1, 0($t0)

    /* Disable all interrupts and clear BEV (use normal exception vectors).
     * BEV = bit 22 (0x00400000); IE = bit 0; EXL = bit 1; ERL = bit 2.
     * Must clear BEV so that when IE is set later, exceptions/interrupts
     * vector to our handlers at 0x80000000 instead of PIF ROM 0xBFC00380. */
    mfc0    $t0, $12            /* read CP0 Status                        */
    li      $t1, ~0x00400007   /* clear BEV (bit22), ERL (bit2), EXL (bit1), IE (bit0) */
    and     $t0, $t0, $t1
    ori     $t0, $t0, 0x0400   /* set IM2 (bit10) — enable RCP interrupt mask */
    mtc0    $t0, $12

    /* -----------------------------------------------------------------------
     * Point the low 32 KB of user space at a scratch page.
     *
     * The GBA never faults on a stray pointer: a read through NULL lands in
     * BIOS space and quietly returns junk, so code that dereferences a
     * freed-and-nulled pointer for one more frame -- which Pokemon Emerald
     * does, e.g. SpriteCB_Cursor running once more after the naming screen
     * frees sNamingScreen -- simply works. On N64 the same access is a TLB
     * miss in unmapped user space, and the refill handler has nowhere to go
     * but a halt: the game freezes with no other symptom.
     *
     * Mapping virtual 0x00000000-0x00007FFF onto one scratch page restores
     * the GBA's forgiveness. It does mean a NULL write silently scribbles on
     * that page rather than announcing itself, which is the same trade the
     * original hardware makes.
     * --------------------------------------------------------------------- */
    mtc0    $zero, $6           /* Wired = 0: all entries usable            */

    /* Invalidate every entry first. EntryHi is pointed at a distinct KSEG0
     * address each time so that no two entries can ever match the same VPN
     * (which the CPU treats as a machine check). */
    mtc0    $zero, $5           /* PageMask = 4 KB                          */
    move    $t2, $zero
.Ltlb_invalidate:
    lui     $t0, 0x8000
    sll     $t1, $t2, 13
    addu    $t0, $t0, $t1
    mtc0    $t0, $10            /* EntryHi                                  */
    mtc0    $zero, $2           /* EntryLo0 = invalid                       */
    mtc0    $zero, $3           /* EntryLo1 = invalid                       */
    mtc0    $t2, $0             /* Index                                    */
    nop
    nop
    tlbwi
    nop
    nop
    addiu   $t2, $t2, 1
    li      $t3, 32
    bne     $t2, $t3, .Ltlb_invalidate
    nop

    li      $t0, 0x6000         /* PageMask = 16 KB pages                   */
    mtc0    $t0, $5
    mtc0    $zero, $10          /* EntryHi: VPN2 = 0, ASID 0                */
    la      $t0, __null_page
    li      $t1, 0x1FFFFFFF
    and     $t0, $t0, $t1       /* physical address                          */
    srl     $t0, $t0, 12        /* PFN                                       */
    sll     $t0, $t0, 6         /* into EntryLo's PFN field                  */
    ori     $t0, $t0, 0x1F      /* cached, dirty (writable), valid, global   */
    mtc0    $t0, $2             /* EntryLo0: virtual 0x0000-0x3FFF           */
    mtc0    $t0, $3             /* EntryLo1: virtual 0x4000-0x7FFF           */
    mtc0    $zero, $0           /* Index = 0                                 */
    nop
    nop
    tlbwi
    nop
    nop

    /* Set up stack. __stack_top is reserved by n64.ld just above .bss, so
     * it stays clear of the RDRAM window the const data is copied into. */
    la      $sp, __stack_top
    addiu   $sp, $sp, -8       /* ABI: maintain 8-byte alignment          */

    /* Set global pointer */
    la      $gp, _gp

    /* -----------------------------------------------------------------------
     * Copy .text from ROM (KSEG1) to RDRAM (KSEG0) word by word.
     *
     * PI DMA hangs on SC64 hardware in this early-boot context (same symptom
     * observed in ipl3.s development).  Use a simple CPU word copy instead:
     * __text_lma is already a KSEG1 address (0xB0000000+), so each LW goes
     * directly to the PI cart bus without any DMA setup.
     *
     * Speed: ~400 ns/word → 2.85 MB ≈ 300 ms.  Acceptable for a one-time
     * boot screen — the DIAG-RED fill is visible during the copy.
     * --------------------------------------------------------------------- */
    la      $t0, __text_lma    /* KSEG1 ROM source  (e.g. 0xB0001890)      */
    la      $t1, __text_start  /* KSEG0 RDRAM dest  = 0x80000400           */
    la      $t2, __text_end    /* RDRAM end         = __text_end           */
.Lcopy_text:
    lw      $t3, 0($t0)        /* read word from cart ROM (uncached PI bus) */
    sw      $t3, 0($t1)        /* write word to RDRAM (cached KSEG0)       */
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    bne     $t1, $t2, .Lcopy_text
    nop
.Ltext_done:

    /* Writeback and invalidate dcache for the written .text range.
     * CACHE 0x15 = HIT_WRITEBACK_INVALIDATE_D: flushes dirty dcache lines
     * to RDRAM so the icache can read the correct code bytes.
     * Use sltu/bnez (not bne) because __text_end may not be 32-byte aligned,
     * so a bne loop would overshoot and spin forever.                      */
    sync
    la      $t0, __text_start
    la      $t1, __text_end
.Ltext_dcache_flush:
    cache   0x15, 0($t0)
    addiu   $t0, $t0, 32
    sltu    $t3, $t0, $t1      /* $t3 = 1 while $t0 < $t1 (unsigned)      */
    bnez    $t3, .Ltext_dcache_flush
    nop

    /* Invalidate instruction cache over the full .text range.
     * CACHE 0x10 = HIT_INVALIDATE_I: forces the icache to re-fill from
     * RDRAM, which now has the correct code bytes.                         */
    la      $t0, __text_start
    la      $t1, __text_end
.Ltext_icache_flush:
    cache   0x10, 0($t0)
    addiu   $t0, $t0, 32
    sltu    $t3, $t0, $t1
    bnez    $t3, .Ltext_icache_flush
    nop


    /* -----------------------------------------------------------------------
     * Copy initialised data sections from ROM (KSEG1) to RDRAM word by word.
     * PI DMA hangs on SC64; use CPU copy (data section is small, ~5 KB).
     * --------------------------------------------------------------------- */
    la      $t0, __data_lma    /* KSEG1 ROM source                          */
    la      $t1, __data_start  /* KSEG0 RDRAM destination                   */
    la      $t2, __data_end    /* RDRAM end                                 */
    beq     $t1, $t2, .Ldata_done  /* skip if no initialised data           */
    nop
.Lcopy_data:
    lw      $t3, 0($t0)
    sw      $t3, 0($t1)
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    bne     $t1, $t2, .Lcopy_data
    nop

    /* Writeback dcache for the written .data range so CPU reads see the
     * new values (SW above wrote to dcache; flush it to RDRAM).           */
    sync
    la      $t0, __data_start
    la      $t1, __data_end
.Ldata_dcache_flush:
    cache   0x15, 0($t0)
    addiu   $t0, $t0, 32
    sltu    $t3, $t0, $t1
    bnez    $t3, .Ldata_dcache_flush
    nop
.Ldata_done:

    /* -----------------------------------------------------------------------
     * Copy the RAM-resident part of .rodata from ROM to RDRAM.
     *
     * The PI bus only answers word-sized reads, so a byte or halfword load
     * from cartridge space returns data from the wrong offset -- see the
     * long note in n64.ld. Every const string and struct table therefore
     * has to be read out of RDRAM instead, which means copying it up front.
     * The bulk graphics blobs stay behind in ROM (.rodata.blobs) since
     * nothing reads those a byte at a time.
     *
     * The destination window starts at 0x80500000, immediately above the
     * 1 MB IPL3 loaded at 0x80400000, so this copy cannot overwrite the
     * code it is running from.
     * --------------------------------------------------------------------- */
    la      $a0, __rodata_start
    la      $a1, __rodata_lma
    la      $a2, __rodata_end
    subu    $a2, $a2, $a0
    beqz    $a2, .Lrodata_done
    nop
    jal     .Lpi_dma_read
    nop

    lui     $t2, 0xA07B
    lui     $t3, 0xA07D
    ori     $t3, $t3, 0x5800
    li      $t1, 0x07C107C1
.Ldiag_green:
    sw      $t1, 0($t2)
    addiu   $t2, $t2, 4
    bne     $t2, $t3, .Ldiag_green
    nop
.Lrodata_done:

    /* -----------------------------------------------------------------------
     * Copy the GBA script/text data to RDRAM, for the same reason as
     * .rodata above: the script engine and every string routine walk it a
     * byte at a time, which the PI bus cannot serve correctly.
     * --------------------------------------------------------------------- */
    la      $a0, __script_data_start
    la      $a1, __script_data_lma
    la      $a2, __script_data_end
    subu    $a2, $a2, $a0
    beqz    $a2, .Lscript_done
    nop
    jal     .Lpi_dma_read
    nop

    lui     $t2, 0xA07B
    lui     $t3, 0xA07D
    ori     $t3, $t3, 0x5800
    li      $t1, 0x07FF07FF
.Ldiag_cyan:
    sw      $t1, 0($t2)
    addiu   $t2, $t2, 4
    bne     $t2, $t3, .Ldiag_cyan
    nop
.Lscript_done:

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

    /* Repaint the boot screen blue: red means the ROM-to-RDRAM copies are
     * still running, blue means they finished and the game is starting.
     * Both give way to the first composited frame. */
    lui     $t2, 0xA07B
    lui     $t3, 0xA07D
    ori     $t3, $t3, 0x5800
    li      $t1, 0x003F003F         /* two blue pixels packed into one word  */
.Ldiag_blue_fill:
    sw      $t1, 0($t2)
    addiu   $t2, $t2, 4
    bne     $t2, $t3, .Ldiag_blue_fill
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


/* -----------------------------------------------------------------------
 * .Lpi_dma_read — cartridge to RDRAM over PI DMA.
 *   $a0 = RDRAM destination (KSEG0), $a1 = ROM source (KSEG1), $a2 = bytes
 * Clobbers $a0-$a2, $t4-$t8. Word-copying this much off the PI bus takes
 * minutes; DMA takes a moment.
 * --------------------------------------------------------------------- */
.Lpi_dma_read:
    move    $t8, $ra
    li      $t4, 0xA4600000         /* PI registers (KSEG1 uncached)        */
    li      $t7, 0x1FFFFFFF         /* KSEG0/KSEG1 -> physical              */
    and     $t5, $a0, $t7           /* $t5 = destination, physical          */
    and     $t6, $a1, $t7           /* $t6 = source, physical               */

    /* Drop anything stale the caches hold over the destination: the DMA
     * writes RDRAM behind the CPU's back. */
    move    $a1, $a0
    addu    $a3, $a0, $a2
.Lpi_dma_inval:
    cache   0x15, 0($a1)            /* Hit_Writeback_Invalidate_D           */
    addiu   $a1, $a1, 32
    sltu    $t7, $a1, $a3
    bnez    $t7, .Lpi_dma_inval
    nop
    sync

.Lpi_dma_chunk:
    /* Wait for PI to go idle (bits 0 and 1 = DMA busy / IO busy) */
.Lpi_dma_wait_before:
    lw      $t7, 0x10($t4)
    andi    $t7, $t7, 0x3
    bnez    $t7, .Lpi_dma_wait_before
    nop

    /* One transfer at a time, capped so a single length field never has to
     * describe more than a megabyte. */
    li      $t7, 0x00100000
    sltu    $a3, $a2, $t7
    bnez    $a3, .Lpi_dma_last
    nop
    move    $a3, $t7
    b       .Lpi_dma_go
    nop
.Lpi_dma_last:
    move    $a3, $a2
.Lpi_dma_go:
    sw      $t5, 0x00($t4)          /* PI_DRAM_ADDR                          */
    sw      $t6, 0x04($t4)          /* PI_CART_ADDR                          */
    addiu   $t7, $a3, -1
    sw      $t7, 0x0C($t4)          /* PI_WR_LEN = length - 1, starts the DMA */

.Lpi_dma_wait_after:
    lw      $t7, 0x10($t4)
    andi    $t7, $t7, 0x1
    bnez    $t7, .Lpi_dma_wait_after
    nop

    addu    $t5, $t5, $a3
    addu    $t6, $t6, $a3
    subu    $a2, $a2, $a3
    bnez    $a2, .Lpi_dma_chunk
    nop

    jr      $t8
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

/* 0x000: TLB Refill — routed through the general handler so that an
 * unexpected miss reports itself instead of halting silently. */
    j       __n64_general_exception_handler
    nop
    .space  0x80 - 8

/* 0x080: XTLB Refill (64-bit, treated same as TLB on our 32-bit build) */
    j       __n64_general_exception_handler
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
    nop                         /* CP0 hazard: EPC write takes 1 cycle;    */
                                /* eret must not immediately follow mtc0   */
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
