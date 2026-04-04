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
 * Framebuffer: physical 0x00200000 (2 MB, well above IPL3 DMA region 0–1 MB)
 *   - 640×480×2 = 614 400 bytes → ends at 0x0024B000 (within 4 MB)
 *   - Matches elite_newkind exactly: 640×480 interlaced, VI_CTRL=0x3242
 *   - libdragon vi_ntsc_i preset timing (VI_V_SYNC=0x020C, VI_V_VIDEO=0x002301FD)
 *   - VI disabled first, all registers written, framebuffer filled, then VI enabled
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
     * VI init following libdragon's exact order (vi_ntsc_i preset):
     *   1. Disable VI first (VI_CTRL=0)
     *   2. Write all timing registers
     *   3. Fill framebuffer
     *   4. Enable VI last (VI_CTRL=0x3242)
     *
     * Using 640×480 interlaced to exactly match elite_newkind behaviour.
     * Framebuffer at physical 0x00200000 (2 MB, well clear of DMA region).
     * VI_CTRL=0x3242: 16bpp, serrate, AA_MODE_RESAMPLE, pixel_advance=3.
     * ------------------------------------------------------------------ */
    lui     $t0, 0xA440             /* $t0 = 0xA4400000 (VI base, KSEG1)     */

    /* 1. Disable VI immediately */
    sw      $zero, 0x00($t0)        /* VI_CTRL = 0 (blank)                    */

    /* 2a. Timing registers (vi_ntsc_i preset, indices 5-11) */
    li      $t1, 0x03E52239
    sw      $t1, 0x14($t0)          /* VI_BURST                               */

    li      $t1, 0x0000020C
    sw      $t1, 0x18($t0)          /* VI_V_SYNC  (interlaced NTSC = 524)     */

    li      $t1, 0x00000C15
    sw      $t1, 0x1C($t0)          /* VI_H_SYNC                              */

    li      $t1, 0x0C150C15
    sw      $t1, 0x20($t0)          /* VI_LEAP                                */

    li      $t1, 0x006C02EC
    sw      $t1, 0x24($t0)          /* VI_H_VIDEO                             */

    li      $t1, 0x002301FD
    sw      $t1, 0x28($t0)          /* VI_V_VIDEO (interlaced NTSC)           */

    li      $t1, 0x000E0204
    sw      $t1, 0x2C($t0)          /* VI_V_BURST                             */

    /* 2b. Framebuffer address, width, scale
     * DIAGNOSTIC: Point VI_ORIGIN at 0x000400 (our own boot code in RDRAM).
     * The code bytes (3C08A440 AD000000...) will render as colored pixels.
     * If screen shows ANY non-black content → VI reads correctly,
     *   fill loop is the problem (not writing to 0x200000).
     * If screen stays black → VI_ORIGIN ignored or timing wrong.
     */
    li      $t1, 0x00000400         /* VI_ORIGIN = 0x000400 (boot code bytes) */
    sw      $t1, 0x04($t0)          /* VI_ORIGIN                              */

    li      $t1, 640
    sw      $t1, 0x08($t0)          /* VI_WIDTH   = 640                       */

    li      $t1, 0x00000002
    sw      $t1, 0x0C($t0)          /* VI_V_INTR  = 2                         */

    li      $t1, 0x00000400
    sw      $t1, 0x30($t0)          /* VI_X_SCALE = 0x400 (1:1, 640 wide)    */

    li      $t1, 0x00000800
    sw      $t1, 0x34($t0)          /* VI_Y_SCALE = 0x800 (2:1, 480 tall)    */

    /* 3. Fill 640×480 framebuffer (KSEG1 0xA0200000) with RED 0xF801 */
    lui     $t2, 0xA020             /* $t2 = 0xA0200000                       */
    lui     $t3, 0xA029
    ori     $t3, $t3, 0x6000        /* $t3 = 0xA0200000 + 640*480*2 = 0xA0296000 */
    li      $t1, 0xF801F801         /* two red RGBA5551 pixels                */
.Lfill:
    sw      $t1, 0($t2)
    addiu   $t2, $t2, 4
    bne     $t2, $t3, .Lfill
    nop

    /* 4. Enable VI — must come AFTER framebuffer is filled */
    li      $t1, 0x00003242         /* VI_CTRL: 16bpp, serrate, resample,     */
    sw      $t1, 0x00($t0)          /*          pixel_advance=3 (= elite)     */

    /* Halt — spin forever */
.Lhalt:
    b       .Lhalt
    nop
