/*
 * src/n64/sprite_renderer.c
 *
 * N64 port — software sprite (OBJ) compositor
 *
 * Reads the 128-entry OAM buffer that the game builds via BuildOamBuffer()
 * and composites sprites over the BG layers in gN64GBAFramebuffer.
 *
 * OAM entry format (from include/gba/types.h struct OamData):
 *   Word 0 (lo 16): y[7:0], affineMode[1:0], objMode[1:0], mosaic, bpp,
 *                   shape[1:0]
 *   Word 0 (hi 16): x[8:0], matrixNum[4:0], size[1:0]
 *   Word 1 (lo 16): tileNum[9:0], priority[1:0], paletteNum[3:0]
 *   Word 1 (hi 16): affineParam (only used in affine matrix slots)
 *
 * Sprite sizes (shape × size from GBATEK):
 *   Square:      8, 16, 32, 64
 *   Horizontal:  16×8, 32×8, 32×16, 64×32
 *   Vertical:    8×16, 8×32, 16×32, 32×64
 *
 * The sprite is composited after all BG layers: for each sprite pixel,
 * if opaque and within window bounds, it overwrites or blends with the
 * background pixel according to OBJ priority and BLDCNT settings.
 */

#include <string.h>
#include "global.h"
#include "n64/defines.h"

extern u16 gN64GBAFramebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
extern void *__n64_oam_buf;
static inline u8 *OamBuf(void) { return (u8 *)__n64_oam_buf; }

/* Sprite size table: [shape][size] = {width, height} */
static const int sSpriteSize[3][4][2] = {
    /* Square */  {{8,8},  {16,16}, {32,32}, {64,64}},
    /* H-rect */  {{16,8}, {32,8},  {32,16}, {64,32}},
    /* V-rect */  {{8,16}, {8,32},  {16,32}, {32,64}},
};

/* Affine parameter matrix — 32 matrices × 4 params (pa,pb,pc,pd), each s16.
 * In the OAM buffer the affine params are interleaved with OBJ attributes
 * at offsets 6, 14, 22, 30 bytes within each 32-byte affine group. */
static inline s16 GetAffineParam(const u8 *oam, int matrixNum, int param)
{
    /* Each affine matrix occupies 4 consecutive OAM entries (32 bytes).
     * param 0-3 = pa,pb,pc,pd stored at byte offset 6 of entries 0-3. */
    int byteOffset = matrixNum * 32 + param * 8 + 6;
    return (s16)(*(u16 *)(oam + byteOffset));
}

/* Convert GBA RGB555 to RGBA5551 (same helper as in tile_renderer.c) */
static inline u16 RGB555toRGBA5551_spr(u16 gba)
{
    u16 r = (gba >>  0) & 0x1F;
    u16 g = (gba >>  5) & 0x1F;
    u16 b = (gba >> 10) & 0x1F;
    /* CPU and VI framebuffer DMA are both big-endian (-EB); no byte-swap. */
    return (u16)((r << 11) | (g << 6) | (b << 1) | 1);
}

/* OBJ palette entries are stored native -- LoadPalette byte-swaps the
 * little-endian GBA asset once on the way in. */
static inline u16 SpritePlttRead(const u16 *p, int i)
{
    return p[i];
}

/* Window mask (also in tile_renderer.c — declared extern here) */
extern u16 GetWindowMask(int x, int y) __attribute__((weak));
static inline u16 LocalGetWindowMask(int x, int y)
{
    u16 dispcnt = _REG16(REG_OFFSET_DISPCNT);
    if (!((dispcnt >> 13) & 3))
        return 0x3F;
    /* Reuse the same logic */
    u16 winin  = _REG16(REG_OFFSET_WININ);
    u16 winout = _REG16(REG_OFFSET_WINOUT);

    if ((dispcnt >> 13) & 1) {
        u16 win0h = _REG16(REG_OFFSET_WIN0H);
        u16 win0v = _REG16(REG_OFFSET_WIN0V);
        int x1 = (win0h >> 8) & 0xFF, x2 = win0h & 0xFF;
        int y1 = (win0v >> 8) & 0xFF, y2 = win0v & 0xFF;
        if (x >= x1 && x < x2 && y >= y1 && y < y2)
            return winin & 0x3F;
    }
    if ((dispcnt >> 14) & 1) {
        u16 win1h = _REG16(REG_OFFSET_WIN1H);
        u16 win1v = _REG16(REG_OFFSET_WIN1V);
        int x1 = (win1h >> 8) & 0xFF, x2 = win1h & 0xFF;
        int y1 = (win1v >> 8) & 0xFF, y2 = win1v & 0xFF;
        if (x >= x1 && x < x2 && y >= y1 && y < y2)
            return (winin >> 8) & 0x3F;
    }
    return winout & 0x3F;
}

/* -----------------------------------------------------------------------
 * N64_CompositeSprites — called after N64_CompositeFrame() completes
 *
 * Sprites are drawn front-to-back in OAM order, respecting priority vs BG.
 * We implement a simple "painter" approach: draw all priority-3 sprites
 * first (behind BG layers), then 2, 1, 0 (on top).
 *
 * For correctness with BG priority interaction, a full implementation
 * would interleave sprite and BG rendering per-pixel.  The simplified
 * approach here matches GBA behaviour for the vast majority of cases
 * (sprites in front of or behind all BG layers at a given priority level).
 * --------------------------------------------------------------------- */
void N64_CompositeSprites(void)
{
    u16 dispcnt = _REG16(REG_OFFSET_DISPCNT);
    if (!(dispcnt & DISPCNT_OBJ_ON))
        return;

    u8  *oam    = OamBuf();
    u8  *vram   = (u8 *)__n64_vram_buf;
    u16 *pltt   = (u16 *)__n64_pltt_buf;
    u16 *fb     = gN64GBAFramebuffer;

    int objVram1D = (dispcnt & DISPCNT_OBJ_1D_MAP) != 0;

    u16 bldcnt   = _REG16(REG_OFFSET_BLDCNT);
    int blendEff = (bldcnt >> 6) & 3;
    int objTgt1  = (bldcnt >> 4) & 1;   /* OBJ is blend target 1 */
    int objTgt2  = (bldcnt >> 12) & 1;  /* OBJ is blend target 2 */
    u16 bldalpha = _REG16(REG_OFFSET_BLDALPHA);
    int eva      = bldalpha & 0x1F;
    int evb      = (bldalpha >> 8) & 0x1F;
    if (eva > 16) eva = 16;
    if (evb > 16) evb = 16;

    /* Process all 128 OAM entries */
    for (int s = 0; s < 128; s++) {
        const u8 *entry = oam + s * 8;

        u16 attr0 = *(u16 *)(entry + 0);
        u16 attr1 = *(u16 *)(entry + 2);
        u16 attr2 = *(u16 *)(entry + 4);

        /* Disabled sprites: affineMode == 2 (OAM_AFFINE_ERASE) */
        int affineMode = (attr0 >> 8) & 3;
        if (affineMode == 2)
            continue;

        int objMode  = (attr0 >> 10) & 3;  /* 0=normal,1=blend,2=OBJ win */
        if (objMode == 2) continue;         /* window sprites: skip for now */

        int bpp8  = (attr0 >> 13) & 1;
        int shape = (attr0 >> 14) & 3;
        if (shape == 3) continue;           /* invalid shape */

        int size     = (attr1 >> 14) & 3;
        int spWidth  = sSpriteSize[shape][size][0];
        int spHeight = sSpriteSize[shape][size][1];

        /* Y position (8-bit, wraps at 256; sprites that partially clip at
         * top of screen have Y > 160 — treat as signed offset from 0) */
        int y = attr0 & 0xFF;
        if (y >= 160) y -= 256;  /* wrap for top-clip sprites */

        /* X position (9-bit signed) */
        int x = attr1 & 0x1FF;
        if (x >= 256) x -= 512;

        int priority = (attr2 >> 10) & 3;
        int tileNum  = attr2 & 0x3FF;
        int palNum   = (attr2 >> 12) & 0xF;

        int hFlip = 0, vFlip = 0;
        int matrixNum = 0;
        int doubleSize = 0;

        if (affineMode == 0) {
            /* Standard sprite */
            hFlip = (attr1 >> 12) & 1;
            vFlip = (attr1 >> 13) & 1;
        } else {
            /* Affine sprite */
            matrixNum = (attr1 >> 9) & 0x1F;
            doubleSize = (affineMode == 3) ? 1 : 0;
        }

        /* Affine bounding box is doubled for double-size sprites */
        int bbW = doubleSize ? spWidth  * 2 : spWidth;
        int bbH = doubleSize ? spHeight * 2 : spHeight;

        /* Skip sprites entirely off-screen */
        if (x + bbW <= 0 || x >= DISPLAY_WIDTH)  continue;
        if (y + bbH <= 0 || y >= DISPLAY_HEIGHT) continue;

        /* Affine parameters */
        s16 pa = 0x100, pb = 0, pc = 0, pd = 0x100;
        if (affineMode != 0) {
            pa = GetAffineParam(oam, matrixNum, 0);
            pb = GetAffineParam(oam, matrixNum, 1);
            pc = GetAffineParam(oam, matrixNum, 2);
            pd = GetAffineParam(oam, matrixNum, 3);
        }

        /* Tile stride: 1D mapping packs tiles sequentially;
         * 2D mapping has a fixed 32-tile stride per row.
         *
         * OAM's tile number is always counted in 32-byte units, whatever the
         * colour depth, so an 8bpp tile takes two of them and both the row
         * stride and the step between adjacent tiles double. Treating the
         * number as an 8bpp tile index instead made every 8bpp sprite past
         * its first tile read from the wrong place -- which is why the
         * "EMERALD VERSION" banner came out with its right half as noise. */
        int tileRowStride = objVram1D ? ((spWidth / TILE_WIDTH) << bpp8) : 32;

        for (int sy = 0; sy < bbH; sy++) {
            int fbY = y + sy;
            if (fbY < 0 || fbY >= DISPLAY_HEIGHT) continue;

            for (int sx = 0; sx < bbW; sx++) {
                int fbX = x + sx;
                if (fbX < 0 || fbX >= DISPLAY_WIDTH) continue;

                /* Window check: bit 4 = OBJ visible */
                if (!(LocalGetWindowMask(fbX, fbY) & (1 << 4)))
                    continue;

                /* Compute tile pixel coordinates */
                int pixX, pixY;
                if (affineMode != 0) {
                    /* Affine: transform (sx,sy) relative to sprite centre */
                    s32 cx = bbW / 2, cy = bbH / 2;
                    s32 tx = sx - cx, ty = sy - cy;
                    /* Apply inverse affine matrix (pa,pb,pc,pd in 8.8 f-p) */
                    s32 fpX = (s32)pa * tx + (s32)pb * ty + (cx << 8);
                    s32 fpY = (s32)pc * tx + (s32)pd * ty + (cy << 8);
                    pixX = (int)(fpX >> 8);
                    pixY = (int)(fpY >> 8);
                } else {
                    pixX = hFlip ? (spWidth  - 1 - sx) : sx;
                    pixY = vFlip ? (spHeight - 1 - sy) : sy;
                }

                if (pixX < 0 || pixX >= spWidth || pixY < 0 || pixY >= spHeight)
                    continue;

                /* Tile index within sprite */
                int tileX = pixX / TILE_WIDTH;
                int tileY = pixY / TILE_HEIGHT;
                int tile  = tileNum + tileY * tileRowStride + (tileX << bpp8);

                /* Sub-pixel within tile */
                int subX = pixX % TILE_WIDTH;
                int subY = pixY % TILE_HEIGHT;

                /* OBJ VRAM starts at offset 0x10000 in VRAM buffer */
                u8 *objVram = vram + 0x10000;

                int palIdx;
                if (bpp8) {
                    int charOffset = tile * TILE_SIZE_4BPP;   /* 32-byte units */
                    palIdx = objVram[charOffset + subY * 8 + subX];
                    if (palIdx == 0) continue;  /* transparent */
                } else {
                    int charOffset = tile * TILE_SIZE_4BPP;
                    u8 byte = objVram[charOffset + subY * 4 + subX / 2];
                    palIdx = (subX & 1) ? (byte >> 4) : (byte & 0xF);
                    if (palIdx == 0) continue;  /* transparent */
                }

                /* Look up colour in OBJ palette (starts at entry 256) */
                u16 sprColour;
                if (bpp8)
                    sprColour = SpritePlttRead(pltt, 256 + palIdx);
                else
                    sprColour = SpritePlttRead(pltt, 256 + palNum * 16 + palIdx);

                /* Apply blending if this sprite is in OBJ blend mode */
                u16 finalColour;
                if (objMode == 1 && blendEff == 1 && evb > 0) {
                    /* Semi-transparent sprite: blend with underlying pixel.
                     * Framebuffer pixels are native RGBA5551 (BE CPU, BE VI). */
                    u16 bgRGBA = fb[fbY * DISPLAY_WIDTH + fbX];
                    /* Convert back from RGBA5551 to RGB555 for blending */
                    u16 bgRGB555 = ((bgRGBA >> 11) & 0x1F)
                                 | (((bgRGBA >> 6) & 0x1F) << 5)
                                 | (((bgRGBA >> 1) & 0x1F) << 10);
                    int sr = (sprColour & 0x1F) * eva;
                    int sg = ((sprColour >> 5) & 0x1F) * eva;
                    int sb = ((sprColour >> 10) & 0x1F) * eva;
                    int br = (bgRGB555 & 0x1F) * evb;
                    int bg2 = ((bgRGB555 >> 5) & 0x1F) * evb;
                    int bb = ((bgRGB555 >> 10) & 0x1F) * evb;
                    int nr = (sr + br) / 16; if (nr > 31) nr = 31;
                    int ng = (sg + bg2) / 16; if (ng > 31) ng = 31;
                    int nb = (sb + bb) / 16; if (nb > 31) nb = 31;
                    u16 blended = (u16)(nr | (ng << 5) | (nb << 10));
                    finalColour = RGB555toRGBA5551_spr(blended);
                } else {
                    finalColour = RGB555toRGBA5551_spr(sprColour);
                }

                fb[fbY * DISPLAY_WIDTH + fbX] = finalColour;
            }
        }
    }
}
