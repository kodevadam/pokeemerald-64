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

/* Define the global data here (scanline_effect.c is excluded from N64 build) */
struct ScanlineEffect gScanlineEffect;
u16 gScanlineEffectRegBuffers[2][0x3C0];

/* -----------------------------------------------------------------------
 * ScanlineEffect control functions (scanline_effect.c is excluded on N64)
 * --------------------------------------------------------------------- */

void ScanlineEffect_Stop(void)
{
    gScanlineEffect.state = 0;
}

void ScanlineEffect_Clear(void)
{
    /* Zero the register buffers */
    for (int b = 0; b < 2; b++)
        for (int i = 0; i < 0x3C0; i++)
            gScanlineEffectRegBuffers[b][i] = 0;

    gScanlineEffect.dmaSrcBuffers[0] = NULL;
    gScanlineEffect.dmaSrcBuffers[1] = NULL;
    gScanlineEffect.dmaDest     = NULL;
    gScanlineEffect.dmaControl  = 0;
    gScanlineEffect.srcBuffer   = 0;
    gScanlineEffect.state       = 0;
    gScanlineEffect.unused16    = 0;
    gScanlineEffect.unused17    = 0;
    gScanlineEffect.waveTaskId  = 0xFF; /* TASK_NONE */
}

void ScanlineEffect_SetParams(struct ScanlineEffectParams params)
{
    /* On N64 we just record the parameters; ApplyLine reads them each frame */
    gScanlineEffect.dmaControl  = params.dmaControl;
    gScanlineEffect.dmaDest     = params.dmaDest;
    gScanlineEffect.state       = params.initState;
    gScanlineEffect.unused16    = params.unused9;
    gScanlineEffect.unused17    = params.unused9;
    /* Set src buffer pointers to scanline buffer data */
    gScanlineEffect.dmaSrcBuffers[0] = (u16 *)gScanlineEffectRegBuffers[0];
    gScanlineEffect.dmaSrcBuffers[1] = (u16 *)gScanlineEffectRegBuffers[1];
}

void ScanlineEffect_InitHBlankDmaTransfer(void)
{
    /* On N64, no DMA; effect is applied per-scanline in the software renderer */
    if (gScanlineEffect.state == 3)
        gScanlineEffect.state = 0;
}

u8 ScanlineEffect_InitWave(u8 startLine, u8 endLine, u8 frequency, u8 amplitude, u8 delayInterval, u8 regOffset, bool8 applyBattleBgOffsets)
{
    /* No-op on N64 — wave effects would need custom compositor support */
    (void)startLine; (void)endLine; (void)frequency; (void)amplitude;
    (void)delayInterval; (void)regOffset; (void)applyBattleBgOffsets;
    return 0;
}

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
