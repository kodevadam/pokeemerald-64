/*
 * src/n64/scanline_effect_n64.c
 *
 * N64 port — per-scanline register update bridge
 *
 * On GBA, the scanline effect system uses HBlank DMA to write one value
 * per scanline from gScanlineEffectRegBuffers[] to a hardware IO register
 * (e.g. REG_BG0HOFS) — producing wave distortion effects in battles.
 *
 * On N64 the software compositor in tile_renderer.c calls
 * ScanlineEffect_ApplyLine(y) before compositing each scanline.  This
 * function overrides the weak stub in tile_renderer.c and performs the
 * equivalent of the HBlank DMA write: it reads the per-line value from
 * gScanlineEffectRegBuffers and writes it to the destination IO register
 * pointer (which, on N64, points into our software gN64IoRegs buffer).
 *
 * Since REG_* macros on N64 expand to pointers into gN64IoRegs[], the
 * gScanlineEffect.dmaDest pointer set by ScanlineEffect_Start() already
 * points to the correct location in the software register file.
 * No address translation is needed.
 */

#include "global.h"
#include "scanline_effect.h"

extern struct ScanlineEffect gScanlineEffect;
extern u16 gScanlineEffectRegBuffers[2][0x3C0];

/*
 * ScanlineEffect_ApplyLine — overrides the weak stub in tile_renderer.c
 *
 * Writes the per-scanline register value to gScanlineEffect.dmaDest
 * before the compositor renders that scanline.
 */
void ScanlineEffect_ApplyLine(int line)
{
    if (gScanlineEffect.state == 0 || gScanlineEffect.dmaDest == NULL)
        return;

    if (line < 0 || line >= DISPLAY_HEIGHT)
        return;

    /* The 16-bit value for this scanline */
    u16 val;
    if (gScanlineEffect.dmaControl & (DMA_32BIT << 16)) {
        /* 32-bit mode: not typical for scroll effects, handle as 16-bit */
        val = (u16)(*(u32 *)(&gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer][line]));
    } else {
        val = gScanlineEffectRegBuffers[gScanlineEffect.srcBuffer][line];
    }

    /* Write to the destination register (in gN64IoRegs[]) */
    *(volatile u16 *)gScanlineEffect.dmaDest = val;
}
