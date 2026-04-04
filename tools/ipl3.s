/*
 * tools/ipl3.s — Minimal open-source N64 IPL3 boot code
 *
 * Purpose: the N64 PIF ROM copies bytes 0x040–0x0FFF of the cart ROM into
 * RDRAM at 0xA0000040 and then jumps there.  This stub then:
 *
 *   1. Waits for the PI bus to be idle (PIF may have left it busy)
 *   2. Clears any pending PI interrupt
 *   3. Issues a PI Cart→DRAM DMA to copy ROM[0x1000..0x101000] (1 MB)
 *      into RDRAM[0x80000000..0x80100000]
 *   4. Waits for the DMA to complete
 *   5. Invalidates the MIPS I-cache over the copied region
 *   6. Jumps to the game entry point (0x80000400)
 *
 * The PIF ROM already initialises RDRAM (it must, to write the IPL3 there),
 * so we do NOT need to touch the RDRAM Interface registers.
 *
 * No CRC check — we skip the CRC1/CRC2 verification entirely so the
 * computed values in the header do not matter.
 *
 * Built big-endian (the N64 CPU and PI bus are permanently big-endian):
 *   mipsel-linux-gnu-as -EB -mips3 -mabi=32 -G0 tools/ipl3.s -o /tmp/ipl3.o
 *   mipsel-linux-gnu-objcopy -O binary /tmp/ipl3.o tools/ipl3.bin
 *   (pad to exactly 0xFC0 = 4032 bytes with zeros)
 *
 * Register use (no ABI constraints — this is standalone code):
 *   $t0  PI base address  (0xA4600000)
 *   $t1  scratch
 *   $t2  scratch / icache loop
 *   $t3  icache end address
 */

    .section .text
    .set    noreorder
    .set    noat
    .globl  _start
    .align  2

_start:
    /* ---------------------------------------------------------------
     * Step 1: Wait for PI to be idle (DMA_BUSY | IO_BUSY = bits 1:0)
     * --------------------------------------------------------------- */
    lui     $t0, 0xA460          /* $t0 = 0xA4600000 (PI base, KSEG1) */
.Lipl3_pi_wait:
    lw      $t1, 0x10($t0)       /* PI_STATUS                          */
    andi    $t1, $t1, 0x03       /* DMA_BUSY | IO_BUSY                 */
    bnez    $t1, .Lipl3_pi_wait
    nop

    /* ---------------------------------------------------------------
     * Step 2: Clear PI interrupt
     * --------------------------------------------------------------- */
    li      $t1, 0x02
    sw      $t1, 0x10($t0)       /* PI_STATUS = CLR_INTR               */

    /* ---------------------------------------------------------------
     * Step 3: DMA ROM[0x1000..0x101000] → RDRAM[0x00000000..0x100000]
     *
     *   PI_DRAM_ADDR  (+0x00) = 0x00000000  (physical RDRAM base)
     *   PI_CART_ADDR  (+0x04) = 0x10001000  (physical cart ROM offset 0x1000)
     *   PI_RD_LEN     (+0x08) = 0x000FFFFF  (1 MB - 1; write triggers DMA)
     * --------------------------------------------------------------- */
    sw      $zero, 0x00($t0)     /* PI_DRAM_ADDR = 0                   */
    lui     $t1, 0x1000
    ori     $t1, $t1, 0x1000     /* $t1 = 0x10001000                   */
    sw      $t1, 0x04($t0)       /* PI_CART_ADDR                       */
    lui     $t1, 0x000F
    ori     $t1, $t1, 0xFFFF     /* $t1 = 0x000FFFFF (1MB - 1)         */
    sw      $t1, 0x08($t0)       /* PI_RD_LEN — DMA starts now         */

    /* ---------------------------------------------------------------
     * Step 4: Wait for DMA completion (DMA_BUSY = bit 0)
     * --------------------------------------------------------------- */
.Lipl3_dma_wait:
    lw      $t1, 0x10($t0)       /* PI_STATUS                          */
    andi    $t1, $t1, 0x01       /* DMA_BUSY                           */
    bnez    $t1, .Lipl3_dma_wait
    nop

    /* ---------------------------------------------------------------
     * Step 5: Invalidate I-cache over [0x80000400..0x80100000]
     *         so the CPU sees the freshly-DMA'd code.
     *         CACHE 0x10 = Hit_Invalidate_I (MIPS III, VR4300)
     * --------------------------------------------------------------- */
    lui     $t2, 0x8000          /* $t2 = 0x80000400 (entry point)     */
    ori     $t2, $t2, 0x0400
    lui     $t3, 0x8010          /* $t3 = 0x80100000 (end of DMA copy) */
.Lipl3_icache:
    cache   0x10, 0($t2)
    addiu   $t2, $t2, 32
    bne     $t2, $t3, .Lipl3_icache
    nop

    /* ---------------------------------------------------------------
     * Step 6: Jump to game entry point 0x80000400
     * --------------------------------------------------------------- */
    lui     $t0, 0x8000
    ori     $t0, $t0, 0x0400
    jr      $t0
    nop
