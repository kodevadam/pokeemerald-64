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
 */

#include <string.h>
#include "global.h"
#include "n64/asm_defs.h"
#include "n64/defines.h"

/* -----------------------------------------------------------------------
 * VI register access
 * --------------------------------------------------------------------- */
#define VI_REG(off) (*(volatile u32 *)(N64_VI_BASE_REG + (off)))

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

/* 320×240 stride in 16-bit pixels */
#define FB_STRIDE  N64_VI_WIDTH

/* -----------------------------------------------------------------------
 * N64_InitVI — configure the Video Interface for 320×240 16-bit NTSC
 *
 * Register values from the N64 Programming Manual and libdragon source.
 * --------------------------------------------------------------------- */
void N64_InitVI(void)
{
    gN64FrontBuffer = (u16 *)__fb0_start;
    gN64BackBuffer  = (u16 *)__fb1_start;
    sDisplayFB      = 0;

    /* Clear both framebuffers to black */
    memset(__fb0_start, 0, N64_VI_WIDTH * N64_VI_HEIGHT * 2);
    memset(__fb1_start, 0, N64_VI_WIDTH * N64_VI_HEIGHT * 2);

    /*
     * VI_STATUS:
     *   bits 1-0:  colour depth (3 = 16-bit)
     *   bit  2:    gamma dither enable
     *   bit  3:    gamma enable
     *   bit  4:    divot enable (anti-aliasing)
     *   bit  6:    serrate (interlace)
     *   bits 9-8:  anti-alias mode (3 = AA + resample, 1 = resample only)
     *
     * We use 16-bit, no gamma, no divot, no AA (for pixel-perfect output),
     * progressive (non-interlaced).
     */
    VI_REG(VI_STATUS_REG) = 0x0003;   /* 16-bit, no filters */

    /* VI_ORIGIN: physical RDRAM address of front buffer (strip KSEG0 bit) */
    VI_REG(VI_ORIGIN_REG) = (u32)((uintptr_t)__fb0_start & 0x00FFFFFF)
                           | ((u32)__fb0_start & 0x0FFFFFFF);

    /* VI_WIDTH: line width in pixels */
    VI_REG(VI_WIDTH_REG) = N64_VI_WIDTH;

    /* VI_INTR: fire VI interrupt at scanline 0 (VBlank) */
    VI_REG(VI_INTR_REG) = 0x00000200;   /* scanline 0 */

    /* VI_BURST: colour burst signal — standard NTSC values */
    VI_REG(VI_BURST_REG) = 0x03E52239;

    /* VI_V_SYNC: NTSC — 525 half-lines (262.5 lines) = 0x20C */
    VI_REG(VI_V_SYNC_REG) = 0x0000020D;

    /* VI_H_SYNC: NTSC — 3093 pixels per line */
    VI_REG(VI_H_SYNC_REG) = 0x00000C15;

    /* VI_LEAP: equalisation/leap */
    VI_REG(VI_LEAP_REG) = 0x0C150C15;

    /* VI_H_START: horizontal active area [109, 749] (320-pixel output) */
    VI_REG(VI_H_START_REG) = 0x006C02EC;

    /* VI_V_START: vertical active area [37, 511] in half-lines */
    VI_REG(VI_V_START_REG) = 0x002501FF;

    /* VI_V_BURST: colour burst timing */
    VI_REG(VI_V_BURST_REG) = 0x000E0204;

    /* VI_X_SCALE: horizontal scale factor (320 pixels from 640 pixels)
     * Format: 0xABB where BB is 1/scale * 1024 (0x200 = ×1.0, no scale) */
    VI_REG(VI_X_SCALE_REG) = 0x00000200;   /* 1:1 (no horizontal scaling) */

    /* VI_Y_SCALE: vertical scale factor (240 → 240, no scaling) */
    VI_REG(VI_Y_SCALE_REG) = 0x00000400;   /* 1:1 vertical */
}

/* -----------------------------------------------------------------------
 * N64_VISwapBuffers — flip front/back buffers at VBlank
 *
 * Called at the start of each VBlank from interrupt.c after the compositor
 * has finished writing the back buffer.
 * --------------------------------------------------------------------- */
void N64_VISwapBuffers(void)
{
    /* Point VI_ORIGIN to the newly completed back buffer */
    u32 physAddr = (u32)((uintptr_t)(sDisplayFB == 0 ? __fb1_start : __fb0_start)
                         & 0x0FFFFFFF);
    VI_REG(VI_ORIGIN_REG) = physAddr;

    /* Swap buffer roles */
    if (sDisplayFB == 0) {
        sDisplayFB     = 1;
        gN64FrontBuffer = (u16 *)__fb1_start;
        gN64BackBuffer  = (u16 *)__fb0_start;
    } else {
        sDisplayFB     = 0;
        gN64FrontBuffer = (u16 *)__fb0_start;
        gN64BackBuffer  = (u16 *)__fb1_start;
    }
}

/* -----------------------------------------------------------------------
 * N64_VISetVCountLine — configure VI to interrupt at a given scanline
 * Used by EnableVCountIntrAtLine150() in main.c.
 * --------------------------------------------------------------------- */
void N64_VISetVCountLine(u16 line)
{
    VI_REG(VI_INTR_REG) = (u32)line;
}

/* -----------------------------------------------------------------------
 * N64_BlitGBAFrame — copy the 240×160 compositor output to the back buffer
 *
 * The compositor writes to gN64GBAFramebuffer (240×160 RGBA5551).
 * This function centres it in the 320×240 back buffer with black borders.
 *
 * Called by the tile_renderer after compositing all layers.
 * --------------------------------------------------------------------- */
u16 gN64GBAFramebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];  /* 240×160 */

void N64_BlitGBAFrame(void)
{
    u16 *src = gN64GBAFramebuffer;
    u16 *dst = gN64BackBuffer;

    /* Fill top border rows with black */
    memset(dst, 0, N64_FB_Y_OFFSET * N64_VI_WIDTH * sizeof(u16));
    dst += N64_FB_Y_OFFSET * N64_VI_WIDTH;

    /* Copy 240×160 with left/right borders */
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        /* Left border */
        memset(dst, 0, N64_FB_X_OFFSET * sizeof(u16));
        dst += N64_FB_X_OFFSET;

        /* GBA line */
        memcpy(dst, src, DISPLAY_WIDTH * sizeof(u16));
        dst += DISPLAY_WIDTH;
        src += DISPLAY_WIDTH;

        /* Right border */
        memset(dst, 0, N64_FB_X_OFFSET * sizeof(u16));
        dst += N64_FB_X_OFFSET;
    }

    /* Fill bottom border rows with black */
    memset(dst, 0, N64_FB_Y_OFFSET * N64_VI_WIDTH * sizeof(u16));
}
