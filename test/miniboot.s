/*
 * test/miniboot.s — Minimal standalone N64 diagnostic ROM
 *
 * Entire ROM in one file: header + (zeroed IPL3 slot) + boot code.
 * The Makefile patches in our CPU-copy IPL3 at offset 0x040 after assembly.
 *
 * Boot code goal: fill the screen RED.  Nothing else.
 * No C, no linker script, no stack, no BSS, no interrupts.
 *
 * Layout after IPL3 patch:
 *   0x0000–0x003F  N64 header  (entry point 0x80000400)
 *   0x0040–0x0FFF  IPL3        (patched in by patch_ipl3.py)
 *   0x1000–0x13FF  zeros       → RDRAM 0x80000000 (exception vector area)
 *   0x1400–        boot code   → RDRAM 0x80000400  ← CPU jumps here
 *
 * Framebuffer: physical 0x00100000 (KSEG0 0x80100000, KSEG1 0xA0100000)
 *   - safely above the 1 MB IPL3 copy (0x80000000–0x80100000)
 *   - 320×240×2 = 153 600 bytes → ends at 0x00125800 (well within 4 MB)
 *   - VI timing uses libdragon's vi_ntsc_i preset (interlaced 480i):
 *     VI_STATUS=0x324E (serrate=1), VI_V_SYNC=0x020C, VI_V_START=0x002301FD
 *     The progressive values (0x020D / 0x002501FF) break VI sync in interlaced mode.
 */

    .set noreorder
    .set noat

/* -----------------------------------------------------------------------
 * Section 1: N64 ROM header — 0x40 bytes at ROM offset 0x0000
 * ----------------------------------------------------------------------- */
    .section .n64hdr, "a"
    /* PI BSD Domain 1 timing — standard cart values */
    .byte 0x80, 0x37, 0x12, 0x40
    /* Clock rate override (0 = default) — must be 0 to match libdragon */
    .byte 0x00, 0x00, 0x00, 0x00
    /* Entry point: 0x80000400 */
    .byte 0x80, 0x00, 0x04, 0x00
    /* Release / OS version */
    .byte 0x00, 0x00, 0x00, 0x00
    /* CRC1 placeholder (patched by n64crc) */
    .byte 0x00, 0x00, 0x00, 0x00
    /* CRC2 placeholder */
    .byte 0x00, 0x00, 0x00, 0x00
    /* Reserved */
    .byte 0x00, 0x00, 0x00, 0x00
    .byte 0x00, 0x00, 0x00, 0x00
    /* Title */
    .ascii "N64 MINIBOOT TEST   "
    /* Reserved */
    .byte 0x00, 0x00, 0x00, 0x00
    /* Game ID */
    .ascii "NMBP"
    /* Destination: E = North America */
    .byte 0x45
    /* Cart ID, reserved, ROM version — 0x22 matches libdragon convention */
    .byte 0x00, 0x00, 0x22

/* -----------------------------------------------------------------------
 * Section 2: IPL3 slot — 0xFC0 bytes at ROM offset 0x0040
 * All zeros here; patch_ipl3.py overwrites this with the real IPL3.
 * ----------------------------------------------------------------------- */
    .section .ipl3slot, "a"
    .space 0xFC0

/* -----------------------------------------------------------------------
 * Section 3: Zero pad — 0x400 bytes at ROM offset 0x1000
 * IPL3 copies ROM[0x1000..] → RDRAM[0x80000000..].
 * This pad maps to RDRAM[0x80000000..0x800003FF] (exception vector area).
 * ----------------------------------------------------------------------- */
    .section .vecpad, "a"
    .space 0x400

/* -----------------------------------------------------------------------
 * Section 4: Boot code — starts at ROM offset 0x1400 → RDRAM 0x80000400
 * ----------------------------------------------------------------------- */
    .section .boot, "ax"
    .globl _start
    .align 2

_start:
    /* ------------------------------------------------------------------
     * Set up VI for 320×240 RGBA5551 NTSC output.
     * Framebuffer at physical 0x00100000 (just above the 1MB DMA window).
     * All writes to VI MMIO at KSEG1 0xA4400000 (uncached, always valid).
     * ------------------------------------------------------------------ */
    lui     $t0, 0xA440             /* $t0 = 0xA4400000  (VI base, KSEG1)    */

    li      $t1, 0x0000324E         /* VI_STATUS: 16bpp, gamma, serrate(480i) — libdragon values */
    sw      $t1, 0x00($t0)

    li      $t1, 0x00100000         /* VI_ORIGIN: physical FB = 0x00100000    */
    sw      $t1, 0x04($t0)

    li      $t1, 320
    sw      $t1, 0x08($t0)          /* VI_WIDTH                               */

    li      $t1, 0x00000002
    sw      $t1, 0x0C($t0)          /* VI_INTR (line 2)                       */

    li      $t1, 0x03E52239
    sw      $t1, 0x14($t0)          /* VI_BURST  (NTSC timing)                */

    li      $t1, 0x0000020C
    sw      $t1, 0x18($t0)          /* VI_V_SYNC (524 half-lines, NTSC interlaced) */

    li      $t1, 0x00000C15
    sw      $t1, 0x1C($t0)          /* VI_H_SYNC                              */

    li      $t1, 0x0C150C15
    sw      $t1, 0x20($t0)          /* VI_LEAP                                */

    li      $t1, 0x006C02EC
    sw      $t1, 0x24($t0)          /* VI_H_START                             */

    li      $t1, 0x002301FD
    sw      $t1, 0x28($t0)          /* VI_V_START (interlaced: 0x23..0x1FD)   */

    li      $t1, 0x000E0204
    sw      $t1, 0x2C($t0)          /* VI_V_BURST                             */

    li      $t1, 0x00000200
    sw      $t1, 0x30($t0)          /* VI_X_SCALE (320→640 2× horiz)         */

    li      $t1, 0x00000400
    sw      $t1, 0x34($t0)          /* VI_Y_SCALE (240→480 2× vert)          */

    /* ------------------------------------------------------------------
     * Fill framebuffer with RED (RGBA5551: R=31, G=0, B=0, A=1 = 0xF801).
     * Write via KSEG1 (0xA0100000) so bytes reach RDRAM immediately —
     * VI reads RDRAM directly and won't see cached-but-unflushed data.
     * ------------------------------------------------------------------ */
    lui     $t2, 0xA010             /* $t2 = 0xA0100000 (KSEG1 framebuffer)  */
    lui     $t3, 0xA012
    ori     $t3, $t3, 0x5800        /* $t3 = 0xA0125800 (end: +320*240*2)    */
    li      $t1, 0xF801F801         /* two red pixels packed                  */
.Lfill:
    sw      $t1, 0($t2)
    addiu   $t2, $t2, 4
    bne     $t2, $t3, .Lfill
    nop

    /* ------------------------------------------------------------------
     * Halt — spin forever.
     * ------------------------------------------------------------------ */
.Lhalt:
    b       .Lhalt
    nop
