/*
 * src/n64/tile_renderer.c
 *
 * N64 port — software 2D tile/background compositor
 *
 * Replaces the GBA hardware tile engine with a software renderer that:
 *   1. Reads the same tilemap / charblock data the game already builds in
 *      the software VRAM buffer (gN64IoRegs, sw_vram, sw_palette).
 *   2. Composites 4 BG layers in priority order.
 *   3. Handles text-mode BGs (modes 0/1), affine BGs (modes 1/2), and
 *      bitmap mode (mode 3).
 *   4. Applies scroll offsets, tile flipping, 4bpp/8bpp colour depth.
 *   5. Supports affine transformation (PA/PB/PC/PD + reference point).
 *   6. Applies window clipping (WIN0/WIN1/WINOUT/WININ).
 *   7. Applies colour blending (BLDCNT / BLDALPHA / BLDY).
 *   8. Runs the scanline effect array (per-line scroll changes).
 *   9. Writes the final RGBA5551 pixel stream to gN64GBAFramebuffer.
 *
 * Performance: the N64 VR4300 at 93.75 MHz is ~5.6× faster than the GBA
 * ARM7TDMI at 16.78 MHz.  A 240×160 frame with 4 BG layers needs roughly
 * 240×160×4 = 153,600 tile lookups per frame.  At 93 MIPS this is about
 * 1.65 ms of budget — comfortable within the 16.67 ms frame time.
 */

#include <string.h>
#include "global.h"
#include "n64/defines.h"

/* -----------------------------------------------------------------------
 * External framebuffer (declared in vi.c)
 * --------------------------------------------------------------------- */
extern u16 gN64GBAFramebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

/* -----------------------------------------------------------------------
 * Palette helpers
 *
 * GBA palette format: RGB555 (bits 14-10 = B, bits 9-5 = G, bits 4-0 = R)
 * N64 framebuffer: RGBA5551 (bits 15-11 = R, bits 10-6 = G, bits 5-1 = B,
 *                             bit 0 = alpha)
 *
 * Conversion: swap R and B channels, set alpha bit to 1.
 * --------------------------------------------------------------------- */
static inline u16 RGB555toRGBA5551(u16 gba)
{
    u16 r = (gba >>  0) & 0x1F;
    u16 g = (gba >>  5) & 0x1F;
    u16 b = (gba >> 10) & 0x1F;
    u16 px = (u16)((r << 11) | (g << 6) | (b << 1) | 1);
    /* N64 VI reads framebuffer in big-endian; CPU is little-endian (-EL).
     * Byte-swap so the VI sees the correct channel layout.               */
    return __builtin_bswap16(px);
}

/* -----------------------------------------------------------------------
 * Software memory buffer pointers
 * --------------------------------------------------------------------- */
static inline u16 *PlttBuf(void)   { return (u16 *)__n64_pltt_buf; }
static inline u8  *VramBuf(void)   { return (u8  *)__n64_vram_buf; }
static inline u8  *OamBuf(void)    { return (u8  *)__n64_oam_buf;  }

/* -----------------------------------------------------------------------
 * BG layer descriptor — built each frame from the software IO registers
 * --------------------------------------------------------------------- */
typedef struct {
    int     enabled;
    int     priority;    /* 0 = front, 3 = back */
    int     charBase;    /* charblock base (0-3) → byte offset in VRAM   */
    int     screenBase;  /* screenblock base (0-31) → byte offset         */
    int     bpp8;        /* 0 = 4bpp, 1 = 8bpp                           */
    int     mosaic;
    int     affine;      /* this BG uses affine transform                 */
    int     areaOverflow;/* affine area overflow wrap                     */
    int     screenSize;  /* 0-3: text 256/512 wide/tall, affine 128-1024 */
    int     hOfs;
    int     vOfs;
    s32     pa, pb, pc, pd;  /* affine matrix (8.8 fixed-point)          */
    s32     refX, refY;      /* affine reference point (8.8 fixed-point)  */
} BgDesc;

/* -----------------------------------------------------------------------
 * Parse BG layer descriptors from software IO registers
 * --------------------------------------------------------------------- */
static void ParseBgDesc(int bgNum, BgDesc *bg, int mode)
{
    static const int bgCntOffsets[4] = {
        REG_OFFSET_BG0CNT, REG_OFFSET_BG1CNT,
        REG_OFFSET_BG2CNT, REG_OFFSET_BG3CNT
    };
    static const int bgHOfsOffsets[4] = {
        REG_OFFSET_BG0HOFS, REG_OFFSET_BG1HOFS,
        REG_OFFSET_BG2HOFS, REG_OFFSET_BG3HOFS
    };
    static const int bgVOfsOffsets[4] = {
        REG_OFFSET_BG0VOFS, REG_OFFSET_BG1VOFS,
        REG_OFFSET_BG2VOFS, REG_OFFSET_BG3VOFS
    };

    u16 dispcnt = _REG16(REG_OFFSET_DISPCNT);
    u16 cnt     = _REG16(bgCntOffsets[bgNum]);

    bg->enabled    = (dispcnt >> (8 + bgNum)) & 1;
    bg->priority   = cnt & 3;
    bg->charBase   = ((cnt >> 2) & 3) * BG_CHAR_SIZE;
    bg->screenBase = ((cnt >> 8) & 0x1F) * BG_SCREEN_SIZE;
    bg->bpp8       = (cnt >> 7) & 1;
    bg->mosaic     = (cnt >> 6) & 1;
    bg->screenSize = (cnt >> 14) & 3;
    bg->areaOverflow = (cnt >> 13) & 1;

    bg->affine = (mode == 1 && bgNum == 2) ||
                 (mode == 2 && (bgNum == 2 || bgNum == 3));

    bg->hOfs = _REG16(bgHOfsOffsets[bgNum]) & 0x1FF;
    bg->vOfs = _REG16(bgVOfsOffsets[bgNum]) & 0x1FF;

    if (bg->affine) {
        static const int paOffsets[2] = {REG_OFFSET_BG2PA, REG_OFFSET_BG3PA};
        static const int refXOffsets[2] = {REG_OFFSET_BG2X, REG_OFFSET_BG3X};
        int ai = bgNum - 2;
        bg->pa   = (s32)(s16)_REG16(paOffsets[ai] + 0);
        bg->pb   = (s32)(s16)_REG16(paOffsets[ai] + 2);
        bg->pc   = (s32)(s16)_REG16(paOffsets[ai] + 4);
        bg->pd   = (s32)(s16)_REG16(paOffsets[ai] + 6);
        bg->refX = (s32)_REG32(refXOffsets[ai] + 0);
        bg->refY = (s32)_REG32(refXOffsets[ai] + 4);
        /* Sign-extend 28-bit value */
        if (bg->refX & 0x08000000) bg->refX |= 0xF0000000;
        if (bg->refY & 0x08000000) bg->refY |= 0xF0000000;
    }
}

/* -----------------------------------------------------------------------
 * Text-mode tile lookup for one pixel
 *
 * Returns the palette colour index (0 = transparent) and the palette
 * number in *palNum (4bpp mode: 16-colour palette 0-15).
 * --------------------------------------------------------------------- */
static int TextTilePixel(const BgDesc *bg, int px, int py, int *palNum)
{
    u8 *vram = VramBuf();

    /* Tile map dimensions */
    int mapW = 256 << (bg->screenSize & 1);   /* 256 or 512 px wide      */
    int mapH = 256 << (bg->screenSize >> 1);  /* 256 or 512 px tall      */

    /* Apply scroll (wrap around map boundaries) */
    int tx = ((px + bg->hOfs) & (mapW - 1));
    int ty = ((py + bg->vOfs) & (mapH - 1));

    /* Which screenblock does this tile fall in?
     * For 512-wide maps: screenblocks 0,1 side-by-side.
     * For 512-tall  maps: screenblocks 0,1 stacked.
     * For 512×512   maps: 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right */
    int sbx = tx / 256;
    int sby = ty / 256;
    int sbIndex = 0;
    switch (bg->screenSize) {
        case 1: sbIndex = sbx;          break;  /* 512×256 */
        case 2: sbIndex = sby;          break;  /* 256×512 */
        case 3: sbIndex = sby*2 + sbx;  break;  /* 512×512 */
        default: sbIndex = 0;           break;  /* 256×256 */
    }

    /* Tile coordinate within its screenblock */
    int tileX = (tx % 256) / TILE_WIDTH;
    int tileY = (ty % 256) / TILE_HEIGHT;

    /* Screenblock entry offset: 32×32 tiles, 2 bytes each */
    int sbOffset = bg->screenBase + sbIndex * BG_SCREEN_SIZE;
    int entryOffset = sbOffset + (tileY * 32 + tileX) * 2;
    u16 entry = *(u16 *)(vram + entryOffset);

    int tileNum = entry & 0x3FF;
    int hFlip   = (entry >> 10) & 1;
    int vFlip   = (entry >> 11) & 1;
    *palNum     = (entry >> 12) & 0xF;

    /* Sub-pixel within tile */
    int pixX = (tx % TILE_WIDTH);
    int pixY = (ty % TILE_HEIGHT);
    if (hFlip) pixX = 7 - pixX;
    if (vFlip) pixY = 7 - pixY;

    /* Read tile data */
    int charOffset = bg->charBase + tileNum * (bg->bpp8 ? TILE_SIZE_8BPP : TILE_SIZE_4BPP);
    if (bg->bpp8) {
        *palNum = 0;
        return vram[charOffset + pixY * 8 + pixX];
    } else {
        u8 byte = vram[charOffset + pixY * 4 + pixX / 2];
        return (pixX & 1) ? (byte >> 4) : (byte & 0xF);
    }
}

/* -----------------------------------------------------------------------
 * Affine BG pixel lookup
 * --------------------------------------------------------------------- */
static int AffineTilePixel(const BgDesc *bg, int screenX, int screenY, int line)
{
    u8 *vram = VramBuf();

    /* Map size: 128, 256, 512, 1024 pixels */
    static const int affineMapSizes[4] = {128, 256, 512, 1024};
    int mapSize = affineMapSizes[bg->screenSize];

    /* Apply affine transform (fixed-point 8.8) */
    s32 texX = bg->refX + bg->pa * screenX + bg->pb * line;
    s32 texY = bg->refY + bg->pc * screenX + bg->pd * line;

    /* Convert from 8.8 fixed to pixel coords */
    int px = texX >> 8;
    int py = texY >> 8;

    /* Overflow handling */
    if (bg->areaOverflow) {
        px = ((px % mapSize) + mapSize) % mapSize;
        py = ((py % mapSize) + mapSize) % mapSize;
    } else {
        if (px < 0 || px >= mapSize || py < 0 || py >= mapSize)
            return 0;  /* transparent outside */
    }

    int tileX = px / TILE_WIDTH;
    int tileY = py / TILE_HEIGHT;
    int mapTiles = mapSize / TILE_WIDTH;

    /* Affine screenblock: 1-byte entries */
    u8 tileNum = vram[bg->screenBase + tileY * mapTiles + tileX];

    int pixX = px % TILE_WIDTH;
    int pixY = py % TILE_HEIGHT;

    /* Affine BGs always use 8bpp */
    return vram[bg->charBase + tileNum * TILE_SIZE_8BPP + pixY * 8 + pixX];
}

/* -----------------------------------------------------------------------
 * Window mask for a pixel
 *
 * Returns the layer enable bits for a given pixel position.
 * Bit layout mirrors GBA WININ/WINOUT: bits 5-0 = BG0-BG3, OBJ, effects.
 * --------------------------------------------------------------------- */
static u16 GetWindowMask(int x, int y)
{
    u16 dispcnt = _REG16(REG_OFFSET_DISPCNT);
    int win0en  = (dispcnt >> 13) & 1;
    int win1en  = (dispcnt >> 14) & 1;

    if (!win0en && !win1en)
        return 0x3F;  /* all layers visible, no windowing */

    u16 winin  = _REG16(REG_OFFSET_WININ);
    u16 winout = _REG16(REG_OFFSET_WINOUT);

    if (win0en) {
        u16 win0h = _REG16(REG_OFFSET_WIN0H);
        u16 win0v = _REG16(REG_OFFSET_WIN0V);
        int x1 = (win0h >> 8) & 0xFF,  x2 = win0h & 0xFF;
        int y1 = (win0v >> 8) & 0xFF,  y2 = win0v & 0xFF;
        if (x >= x1 && x < x2 && y >= y1 && y < y2)
            return winin & 0x3F;
    }
    if (win1en) {
        u16 win1h = _REG16(REG_OFFSET_WIN1H);
        u16 win1v = _REG16(REG_OFFSET_WIN1V);
        int x1 = (win1h >> 8) & 0xFF,  x2 = win1h & 0xFF;
        int y1 = (win1v >> 8) & 0xFF,  y2 = win1v & 0xFF;
        if (x >= x1 && x < x2 && y >= y1 && y < y2)
            return (winin >> 8) & 0x3F;
    }
    return (winout & 0x3F);  /* outside both windows */
}

/* -----------------------------------------------------------------------
 * Colour blend / brightness adjustment
 * --------------------------------------------------------------------- */
static u16 BlendColours(u16 top, u16 bot)
{
    u16 bldalpha = _REG16(REG_OFFSET_BLDALPHA);
    int eva = bldalpha & 0x1F;
    int evb = (bldalpha >> 8) & 0x1F;
    if (eva > 16) eva = 16;
    if (evb > 16) evb = 16;

    /* GBA RGB555 format */
    int r = ((top & 0x1F) * eva + (bot & 0x1F) * evb) / 16;
    int g = (((top >> 5) & 0x1F) * eva + ((bot >> 5) & 0x1F) * evb) / 16;
    int b = (((top >> 10) & 0x1F) * eva + ((bot >> 10) & 0x1F) * evb) / 16;
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    return (u16)(r | (g << 5) | (b << 10));
}

static u16 BrightnessIncrease(u16 colour)
{
    u16 bldy = _REG16(REG_OFFSET_BLDY) & 0x1F;
    if (bldy > 16) bldy = 16;
    int r = (colour & 0x1F);
    int g = ((colour >> 5) & 0x1F);
    int b = ((colour >> 10) & 0x1F);
    r += (31 - r) * bldy / 16;
    g += (31 - g) * bldy / 16;
    b += (31 - b) * bldy / 16;
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    return (u16)(r | (g << 5) | (b << 10));
}

static u16 BrightnessDecrease(u16 colour)
{
    u16 bldy = _REG16(REG_OFFSET_BLDY) & 0x1F;
    if (bldy > 16) bldy = 16;
    int r = (colour & 0x1F) - (colour & 0x1F) * bldy / 16;
    int g = ((colour >> 5) & 0x1F) - ((colour >> 5) & 0x1F) * bldy / 16;
    int b = ((colour >> 10) & 0x1F) - ((colour >> 10) & 0x1F) * bldy / 16;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return (u16)(r | (g << 5) | (b << 10));
}

/* -----------------------------------------------------------------------
 * Per-scanline scroll effect support
 *
 * The game uses HBlank DMA to change BG scroll registers per scanline
 * (e.g. battle wave effects).  On GBA this is done in hardware; on N64
 * we snapshot the registers before rendering each line.
 *
 * The game's scanline_effect.c builds gScanlineEffectRegBuffers[][], which
 * contains per-line register values.  We call its update hook before each
 * scanline.
 * --------------------------------------------------------------------- */
extern void ScanlineEffect_ApplyLine(int line);

/* -----------------------------------------------------------------------
 * N64_CompositeFrame — main entry point, called each VBlank
 *
 * Renders one 240×160 frame into gN64GBAFramebuffer.
 * --------------------------------------------------------------------- */
void N64_CompositeFrame(void)
{
    u16 dispcnt = _REG16(REG_OFFSET_DISPCNT);
    int bgMode  = dispcnt & 0x7;

    /* Forced blank: fill with white */
    if (dispcnt & DISPCNT_FORCED_BLANK) {
        memset(gN64GBAFramebuffer, 0xFF, sizeof(gN64GBAFramebuffer));
        return;
    }

    /* Parse BG descriptors for this frame */
    BgDesc bgs[4];
    for (int i = 0; i < 4; i++)
        ParseBgDesc(i, &bgs[i], bgMode);

    /* Sort BG layer indices by priority (stable, highest priority first) */
    int layerOrder[4] = {0, 1, 2, 3};
    /* Insertion sort by priority */
    for (int i = 1; i < 4; i++) {
        int key = layerOrder[i];
        int j = i - 1;
        while (j >= 0 && bgs[layerOrder[j]].priority > bgs[key].priority) {
            layerOrder[j + 1] = layerOrder[j];
            j--;
        }
        layerOrder[j + 1] = key;
    }

    u16 bldcnt  = _REG16(REG_OFFSET_BLDCNT);
    int blendEff = (bldcnt >> 6) & 3;
    u16 tgt1Mask = bldcnt & 0x3F;
    u16 tgt2Mask = (bldcnt >> 8) & 0x3F;
    u16 *pltt    = PlttBuf();

    /* Backdrop colour (BG palette entry 0) */
    u16 backdropRGB555 = pltt[0];

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        /* Apply per-scanline register changes (battle wave effects, etc.) */
        ScanlineEffect_ApplyLine(y);

        /* Re-read scroll registers for this scanline */
        for (int i = 0; i < 4; i++)
            ParseBgDesc(i, &bgs[i], bgMode);

        u16 *rowOut = gN64GBAFramebuffer + y * DISPLAY_WIDTH;

        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            /* Window mask for this pixel */
            u16 winMask = GetWindowMask(x, y);

            /* Composite layers from front to back */
            u16 topColour  = backdropRGB555;  /* fallback = backdrop       */
            u16 botColour  = backdropRGB555;
            int topLayer   = -1;              /* -1 = backdrop              */
            int botLayer   = -1;
            int gotTop     = 0;
            int gotBot     = 0;

            for (int li = 0; li < 4 && !gotBot; li++) {
                int bgIdx = layerOrder[li];

                if (!bgs[bgIdx].enabled) continue;
                if (!(winMask & (1 << bgIdx))) continue;

                u16 colour = 0;
                int transparent = 1;

                if (bgMode == 0 || (bgMode == 1 && bgIdx < 2)) {
                    /* Text mode */
                    int palNum = 0;
                    int palIdx = TextTilePixel(&bgs[bgIdx], x, y, &palNum);
                    if (palIdx != 0) {
                        transparent = 0;
                        if (bgs[bgIdx].bpp8)
                            colour = pltt[palIdx];
                        else
                            colour = pltt[palNum * 16 + palIdx];
                    }
                } else if ((bgMode == 1 && bgIdx == 2) || bgMode == 2) {
                    /* Affine mode */
                    int palIdx = AffineTilePixel(&bgs[bgIdx], x, y, y);
                    if (palIdx != 0) {
                        transparent = 0;
                        colour = pltt[palIdx];
                    }
                } else if (bgMode == 3 && bgIdx == 2) {
                    /* Bitmap mode 3: 240×160 direct-colour in VRAM */
                    colour = *(u16 *)(VramBuf() + (y * 240 + x) * 2);
                    transparent = 0;
                } else if (bgMode == 4 && bgIdx == 2) {
                    /* Bitmap mode 4: 240×160 8bpp paletted */
                    int frame = (dispcnt >> 4) & 1;
                    u8  palIdx8 = VramBuf()[frame * 0xA000 + y * 240 + x];
                    if (palIdx8 != 0) {
                        transparent = 0;
                        colour = pltt[palIdx8];
                    }
                }

                if (!transparent) {
                    if (!gotTop) {
                        topColour = colour;
                        topLayer  = bgIdx;
                        gotTop    = 1;
                    } else if (!gotBot) {
                        botColour = colour;
                        botLayer  = bgIdx;
                        gotBot    = 1;
                    }
                }
            }

            /* Apply colour effects */
            u16 finalColour = topColour;
            if (blendEff == 1 && gotTop &&
                (tgt1Mask & (1 << (topLayer + 5 < 0 ? 5 : topLayer))) &&
                (tgt2Mask & (1 << (botLayer < 0 ? 5 : botLayer))))
            {
                finalColour = BlendColours(topColour, botColour);
            } else if (blendEff == 2 &&
                (tgt1Mask & (topLayer < 0 ? 0x20 : (1 << topLayer))))
            {
                finalColour = BrightnessIncrease(topColour);
            } else if (blendEff == 3 &&
                (tgt1Mask & (topLayer < 0 ? 0x20 : (1 << topLayer))))
            {
                finalColour = BrightnessDecrease(topColour);
            }

            rowOut[x] = RGB555toRGBA5551(finalColour);
        }
    }
}

/* -----------------------------------------------------------------------
 * ScanlineEffect_ApplyLine — stub called before each scanline
 *
 * The real implementation in src/scanline_effect.c manages per-scanline
 * scroll register arrays (gScanlineEffectRegBuffers).  We call it to
 * update the software IO register file before the compositor reads it.
 *
 * This function is declared here as a weak symbol; scanline_effect.c
 * provides the actual implementation.
 * --------------------------------------------------------------------- */
__attribute__((weak)) void ScanlineEffect_ApplyLine(int line)
{
    (void)line;
}
