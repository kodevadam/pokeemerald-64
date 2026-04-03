/*
 * src/n64/vi.c
 *
 * N64 port — Video Interface (VI) initialisation and framebuffer management
 *
 * The N64 VI drives the display from a framebuffer in RDRAM.  We configure
 * it for 320×240 @ 16-bit colour (RGBA5551 — N64 native 16-bit format).
 *
 * The GBA renders at 240×160.  We allocate a 240×160 internal render buffer
 * that the tile/sprite compositor writes to, then blit it into the centre
 * of the 320×240 VI framebuffer (40-pixel black borders top/bottom).
 *
 * Double-buffering: two 320×240 framebuffers live at the top of RDRAM
 * (__fb0_start, __fb1_start).  The compositor writes to the back buffer;
 * after each VBlank the buffers are flipped by updating VI_ORIGIN.
 *
 * Endianness note:
 *   The CPU is compiled little-endian (-EL) but N64 VI registers are
 *   big-endian.  All VI register writes use N64_HW_WR() which calls
 *   __builtin_bswap32() before writing.
 */

#include <string.h>
#include "global.h"
#include "n64/asm_defs.h"
#include "n64/defines.h"

/* -----------------------------------------------------------------------
 * VI register access wrappers (LE CPU → BE hardware)
 * --------------------------------------------------------------------- */
#define VI_WR(off, val)  N64_HW_WR(N64_VI_BASE_REG, (off), (uint32_t)(val))
#define VI_RD(off)       N64_HW_RD(N64_VI_BASE_REG, (off))

/* -----------------------------------------------------------------------
 * Framebuffer pointers (exported so the compositor can use them)
 * --------------------------------------------------------------------- */
extern u8 __fb0_start[];
extern u8 __fb1_start[];

/* Currently displayed framebuffer (0 or 1) */
static int sDisplayFB  = 0;

/* Back buffer (compositor writes here) */
u16 *gN64BackBuffer  = NULL;
u16 *gN64FrontBuffer = NULL;

/* 240×160 GBA render target — compositor fills this, then we blit to VI FB */
u16 gN64GBAFramebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

/* -----------------------------------------------------------------------
 * N64_InitVI — configure the Video Interface for 320×240 16-bit NTSC
 * --------------------------------------------------------------------- */
void N64_InitVI(void)
{
    gN64FrontBuffer = (u16 *)__fb0_start;
    gN64BackBuffer  = (u16 *)__fb1_start;
    sDisplayFB      = 0;

    /* Clear both framebuffers to black */
    memset(__fb0_start, 0, N64_VI_WIDTH * N64_VI_HEIGHT * sizeof(u16));
    memset(__fb1_start, 0, N64_VI_WIDTH * N64_VI_HEIGHT * sizeof(u16));

    /* VI_STATUS: 16-bit colour, no gamma, no divot, no AA, progressive */
    VI_WR(VI_STATUS_REG,  0x00003202);

    /* VI_ORIGIN: physical RDRAM address of front framebuffer */
    VI_WR(VI_ORIGIN_REG,  (u32)((uintptr_t)__fb0_start & 0x00FFFFFF));

    /* VI_WIDTH: line width in pixels */
    VI_WR(VI_WIDTH_REG,   N64_VI_WIDTH);

    /* VI_INTR: interrupt at half-line 2 — fires once per frame at the very
     * start of the next field, used as our VBlank event.               */
    VI_WR(VI_INTR_REG,    0x00000002);

    /* VI_CURRENT: clear */
    VI_WR(0x10, 0);

    /* VI_BURST: colour burst — standard NTSC */
    VI_WR(VI_BURST_REG,   0x03E52239);

    /* VI_V_SYNC: NTSC — 525 half-lines */
    VI_WR(VI_V_SYNC_REG,  0x0000020D);

    /* VI_H_SYNC: NTSC — 3093 pixels per line */
    VI_WR(VI_H_SYNC_REG,  0x00000C15);

    /* VI_LEAP */
    VI_WR(VI_LEAP_REG,    0x0C150C15);

    /* VI_H_START: [109, 749] for 320-pixel output */
    VI_WR(VI_H_START_REG, 0x006C02EC);

    /* VI_V_START: [37, 511] half-lines */
    VI_WR(VI_V_START_REG, 0x002501FF);

    /* VI_V_BURST */
    VI_WR(VI_V_BURST_REG, 0x000E0204);

    /* VI_X_SCALE: 1:1 horizontal (no scale) */
    VI_WR(VI_X_SCALE_REG, 0x00000200);

    /* VI_Y_SCALE: 1:1 vertical */
    VI_WR(VI_Y_SCALE_REG, 0x00000400);
}

/* -----------------------------------------------------------------------
 * N64_VISwapBuffers — flip front/back buffers at VBlank
 * --------------------------------------------------------------------- */
void N64_VISwapBuffers(void)
{
    u8 *newFront = (sDisplayFB == 0) ? __fb1_start : __fb0_start;
    u32 physAddr = (u32)((uintptr_t)newFront & 0x00FFFFFF);

    VI_WR(VI_ORIGIN_REG, physAddr);

    if (sDisplayFB == 0) {
        sDisplayFB      = 1;
        gN64FrontBuffer = (u16 *)__fb1_start;
        gN64BackBuffer  = (u16 *)__fb0_start;
    } else {
        sDisplayFB      = 0;
        gN64FrontBuffer = (u16 *)__fb0_start;
        gN64BackBuffer  = (u16 *)__fb1_start;
    }
}

/* -----------------------------------------------------------------------
 * N64_VISetVCountLine — configure VI interrupt scanline
 * --------------------------------------------------------------------- */
void N64_VISetVCountLine(u16 line)
{
    VI_WR(VI_INTR_REG, (u32)line);
}

/* -----------------------------------------------------------------------
 * N64_BlitGBAFrame — copy 240×160 render target into 320×240 back buffer
 *
 * Centres the GBA picture with black letterbox/pillarbox borders.
 * --------------------------------------------------------------------- */
void N64_BlitGBAFrame(void)
{
    const u16 *src = gN64GBAFramebuffer;
    u16       *dst = gN64BackBuffer;

    /* Top border */
    memset(dst, 0, N64_FB_Y_OFFSET * N64_VI_WIDTH * sizeof(u16));
    dst += N64_FB_Y_OFFSET * N64_VI_WIDTH;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        /* Left border */
        memset(dst, 0, N64_FB_X_OFFSET * sizeof(u16));
        dst += N64_FB_X_OFFSET;

        /* GBA scanline */
        memcpy(dst, src, DISPLAY_WIDTH * sizeof(u16));
        dst += DISPLAY_WIDTH;
        src += DISPLAY_WIDTH;

        /* Right border */
        memset(dst, 0, N64_FB_X_OFFSET * sizeof(u16));
        dst += N64_FB_X_OFFSET;
    }

    /* Bottom border */
    memset(dst, 0, N64_FB_Y_OFFSET * N64_VI_WIDTH * sizeof(u16));
}
