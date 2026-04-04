/*
 * tools/ipl3.s — Minimal open-source N64 IPL3 boot code (CPU-copy variant)
 *
 * Purpose: the N64 PIF ROM copies bytes 0x040–0x0FFF of the cart ROM into
 * RDRAM at 0xA0000040 and jumps there.  This stub then:
 *
 *   1. Copies ROM[0x1000..0x101000] into RDRAM[0x80000000..0x80100000]
 *      using CPU word reads from KSEG1 (uncached cart PI bus).
 *      We deliberately avoid PI DMA here: SC64 PI DMA appears to hang in
 *      this context (same symptom seen in crt0.s DMA wait loops).
 *      CPU copy at ~200ns/word for 1MB ≈ 50-200ms — acceptable for boot.
 *
 *   2. Invalidates the I-cache over the copied region so the CPU sees
 *      the fresh code.
 *
 *   3. Jumps to the game entry point 0x80000400 (__n64_boot in crt0.s).
 *
 * No CRC check — skip it entirely.
 *
 * RDRAM is already initialized by the PIF ROM before our IPL3 runs
 * (the PIF ROM must write IPL3 into RDRAM to execute it).
 *
 * Register use (no ABI constraints):
 *   $t0  cart source pointer (KSEG1 0xB0001000 → 0xB0101000)
 *   $t1  RDRAM dest pointer  (KSEG0 0x80000000 → 0x80100000)
 *   $t2  RDRAM end address
 *   $t3  word scratch
 *   $t4  icache loop counter
 *   $t5  icache end
 */

    .section .text
    .set    noreorder
    .set    noat
    .globl  _start
    .align  2

_start:
    /* ---------------------------------------------------------------
     * Step 1: CPU word copy ROM[0x1000..0x101000] → RDRAM[0..0x100000]
     *
     * Source: KSEG1 cart address 0xB0001000 (ROM file offset 0x1000)
     * Dest:   KSEG0 RDRAM        0x80000000 (physical 0x00000000)
     * Length: 1 MB = 0x100000 bytes = 0x40000 words
     * --------------------------------------------------------------- */
    lui     $t0, 0xB000
    ori     $t0, $t0, 0x1000    /* $t0 = 0xB0001000 (cart, ROM+0x1000)  */
    lui     $t1, 0x8000          /* $t1 = 0x80000000 (RDRAM dest)         */
    lui     $t2, 0x8010          /* $t2 = 0x80100000 (RDRAM end)          */
.Lipl3_copy:
    lw      $t3, 0($t0)          /* word read from cart (uncached KSEG1)  */
    sw      $t3, 0($t1)          /* word write to RDRAM (cached KSEG0)    */
    addiu   $t0, $t0, 4
    addiu   $t1, $t1, 4
    bne     $t1, $t2, .Lipl3_copy
    nop

    /* ---------------------------------------------------------------
     * Step 2: Writeback D-cache over the written RDRAM range so that
     * the VI (which reads RDRAM directly) sees the correct bytes.
     * CACHE 0x15 = Hit_Writeback_Invalidate_D
     * --------------------------------------------------------------- */
    lui     $t4, 0x8000          /* $t4 = 0x80000000                      */
    lui     $t5, 0x8010          /* $t5 = 0x80100000                      */
.Lipl3_dcache:
    cache   0x15, 0($t4)
    addiu   $t4, $t4, 32
    bne     $t4, $t5, .Lipl3_dcache
    nop

    /* ---------------------------------------------------------------
     * Step 3: Invalidate I-cache over the copied region.
     * CACHE 0x10 = Hit_Invalidate_I
     * --------------------------------------------------------------- */
    lui     $t4, 0x8000
    lui     $t5, 0x8010
.Lipl3_icache:
    cache   0x10, 0($t4)
    addiu   $t4, $t4, 32
    bne     $t4, $t5, .Lipl3_icache
    nop

    /* ---------------------------------------------------------------
     * Step 4: Jump to game entry point 0x80000400 (__n64_boot)
     * --------------------------------------------------------------- */
    lui     $t0, 0x8000
    ori     $t0, $t0, 0x0400
    jr      $t0
    nop
