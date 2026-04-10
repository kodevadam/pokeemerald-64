/*
 * tools/ipl3.s — N64 IPL3 boot stub (CPU-copy, libdragon-compatible VMA layout)
 *
 * The N64 PIF ROM copies bytes 0x040–0x0FFF of the cart ROM into
 * RSP IMEM (or RDRAM 0xA0000040 on some revisions) and executes it.
 * This stub:
 *
 *   1. CPU-copies ROM[0x1048 .. 0x5048] (16 KB) → RDRAM[0x807FC000 .. 0x80800000]
 *      using word reads from KSEG1 (uncached PI bus).
 *      This places the .boot section (__n64_boot in crt0.s) at its linker VMA
 *      of 0x807FC000 so all absolute addresses and J-targets are correct.
 *
 *      PI DMA is deliberately avoided: SC64 PI DMA hangs in this early-boot
 *      context (the same symptom appears in crt0.s DMA wait loops).
 *      CPU copy at ~400 ns/word for 16 KB ≈ 6 ms — negligible.
 *
 *   2. Writes back D-cache and invalidates I-cache over [0x807FC000..0x80800000]
 *      so the CPU sees the freshly written code.
 *
 *   3. Jumps to 0x807FC000 (__n64_boot entry, VMA matches actual RDRAM location).
 *
 * Expansion pak (8 MB RDRAM) is REQUIRED: 0x807FC000 = physical 0x7FC000 = 7.98 MB.
 *
 * Register use (no ABI constraints here):
 *   $t0  cart source pointer  (KSEG1 0xB0001048 → 0xB0005048)
 *   $t1  RDRAM dest pointer   (KSEG0 0x807FC000 → 0x80800000)
 *   $t2  RDRAM end address    (0x80800000, reused for all three loops)
 *   $t3  word scratch
 */

    .section .text
    .set    noreorder
    .set    noat
    .globl  _start
    .align  2

_start:
    /* ---------------------------------------------------------------
     * Step 1: CPU word copy ROM[0xB0001048..0xB0005048] → RDRAM[0x807FC000..0x80800000]
     *
     * Source: KSEG1 cart address 0xB0001048 (ROM file offset 0x1048 = .boot LMA)
     * Dest:   KSEG0 RDRAM        0x807FC000 (= .boot section VMA)
     * Length: 16 KB = 0x4000 bytes
     * --------------------------------------------------------------- */
    lui     $t0, 0xB000
    ori     $t0, $t0, 0x1048    /* $t0 = 0xB0001048 (ROM .boot LMA, KSEG1) */
    lui     $t1, 0x807F
    ori     $t1, $t1, 0xC000    /* $t1 = 0x807FC000 (RDRAM .boot VMA)      */
    lui     $t2, 0x8080          /* $t2 = 0x80800000 (end = VMA + 16KB)     */
.Lipl3_copy:
    lw      $t3, 0($t0)          /* read word from cart ROM (KSEG1 PI bus)  */
    sw      $t3, 0($t1)          /* write word to RDRAM (KSEG0 cached)      */
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    bne     $t1, $t2, .Lipl3_copy
    nop

    /* ---------------------------------------------------------------
     * Step 2: Write back D-cache over the written RDRAM region so that
     * RDRAM (read by the I-cache refill) contains the correct bytes.
     * CACHE 0x15 = Hit_Writeback_Invalidate_D
     * --------------------------------------------------------------- */
    lui     $t1, 0x807F
    ori     $t1, $t1, 0xC000    /* 0x807FC000                              */
    /* $t2 still = 0x80800000 */
.Lipl3_dcache:
    cache   0x15, 0($t1)
    addiu   $t1, $t1, 32
    bne     $t1, $t2, .Lipl3_dcache
    nop

    /* ---------------------------------------------------------------
     * Step 3: Invalidate I-cache over the copied region so the CPU
     * fetches fresh instructions from RDRAM on first execution.
     * CACHE 0x10 = Hit_Invalidate_I
     * --------------------------------------------------------------- */
    lui     $t1, 0x807F
    ori     $t1, $t1, 0xC000
    /* $t2 still = 0x80800000 */
.Lipl3_icache:
    cache   0x10, 0($t1)
    addiu   $t1, $t1, 32
    bne     $t1, $t2, .Lipl3_icache
    nop

    /* ---------------------------------------------------------------
     * Step 4: Jump to __n64_boot at its VMA 0x807FC000
     * --------------------------------------------------------------- */
    lui     $t0, 0x807F
    ori     $t0, $t0, 0xC000    /* 0x807FC000 */
    jr      $t0
    nop
