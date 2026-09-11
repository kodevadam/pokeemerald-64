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
    /* CPU and VI framebuffer DMA are both big-endian (-EB); no byte-swap. */
    return (u16)((r << 11) | (g << 6) | (b << 1) | 1);
}

/* Palette entries are stored native (LoadPalette byte-swaps the
 * little-endian GBA asset once on the way in), so they read back
 * directly as RGB555 -- the same values the game's own fade/blend code
 * operates on. Tilemap entries and bitmap-mode pixels below are *not*
 * native: nothing but this renderer ever reads them, so they stay as
 * the raw little-endian bytes the assets ship with and get swapped at
 * the point of use. */
static inline u16 PlttRead(const u16 *p, int i)
{
    return p[i];
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
        /* BG2X/BG2Y are 32-bit registers the game writes as two halves
         * (BG2X_L then BG2X_H). The software register file is big-endian,
         * so a plain 32-bit read would put the _L half in the high word --
         * reassemble them explicitly instead. */
        bg->refX = (s32)((_REG16(refXOffsets[ai] + 2) << 16) | _REG16(refXOffsets[ai] + 0));
        bg->refY = (s32)((_REG16(refXOffsets[ai] + 6) << 16) | _REG16(refXOffsets[ai] + 4));
        /* Sign-extend 28-bit value */
        if (bg->refX & 0x08000000) bg->refX |= 0xF0000000;
        if (bg->refY & 0x08000000) bg->refY |= 0xF0000000;
    }
}

/* -----------------------------------------------------------------------
 * Per-scanline layer buffers
 *
 * The compositor used to look each pixel up individually, which meant
 * re-reading the tilemap entry and re-deriving the character address
 * eight times per tile -- around 500 CPU cycles per output pixel once
 * the cache misses were counted, or ~0.2s per frame.  Instead each
 * enabled layer now renders a whole scanline in 8-pixel tile runs, and
 * the compositing pass just walks the resulting line buffers.
 * --------------------------------------------------------------------- */


/* A layer's scanline, already converted to the framebuffer's RGBA5551.
 * RGB555toRGBA5551() always sets the alpha bit, so a zero entry can stand
 * for "transparent" and no separate coverage array is needed. */
static u16 sLine[4][DISPLAY_WIDTH];

/* Palette pre-converted to RGBA5551 once per frame, so the inner loops
 * never convert and never branch on the index:
 *   sPal4 zeroes index 0 of each 16-colour bank (4bpp transparency),
 *   sPal8 zeroes only index 0 (256-colour transparency). */
static u16 sPal4[256];
static u16 sPal8[256];

static void BuildPaletteCache(const u16 *pltt)
{
    for (int i = 0; i < 256; i++) {
        u16 c = RGB555toRGBA5551(PlttRead(pltt, i));
        sPal8[i] = c;
        sPal4[i] = (i & 0xF) ? c : 0;
    }
    sPal8[0] = 0;
}

/* -----------------------------------------------------------------------
 * Text-mode (modes 0/1) scanline renderer
 * --------------------------------------------------------------------- */
static void RenderTextLine(const BgDesc *bg, int y, u16 *out)
{
    const u8 *vram = VramBuf();

    int mapW = 256 << (bg->screenSize & 1);   /* 256 or 512 px wide      */
    int mapH = 256 << (bg->screenSize >> 1);  /* 256 or 512 px tall      */

    int ty  = (y + bg->vOfs) & (mapH - 1);
    int sby = ty >> 8;
    int tileY = (ty & 0xFF) >> 3;
    int rowInTile = ty & 7;

    int x = 0;
    while (x < DISPLAY_WIDTH)
    {
        int tx  = (x + bg->hOfs) & (mapW - 1);
        int sbx = tx >> 8;

        /* Which screenblock does this tile fall in?
         * 512-wide maps put screenblocks 0,1 side-by-side; 512-tall maps
         * stack them; 512x512 uses 0 top-left, 1 top-right, 2 bottom-left,
         * 3 bottom-right. */
        int sbIndex;
        switch (bg->screenSize) {
            case 1:  sbIndex = sbx;          break;
            case 2:  sbIndex = sby;          break;
            case 3:  sbIndex = sby * 2 + sbx; break;
            default: sbIndex = 0;            break;
        }

        int tileX = (tx & 0xFF) >> 3;
        int entryOffset = bg->screenBase + sbIndex * BG_SCREEN_SIZE
                        + (tileY * 32 + tileX) * 2;
        u16 entry = __builtin_bswap16(*(const u16 *)(vram + entryOffset));

        int tileNum = entry & 0x3FF;
        int hFlip   = (entry >> 10) & 1;
        int vFlip   = (entry >> 11) & 1;
        int py      = vFlip ? 7 - rowInTile : rowInTile;

        int first = tx & 7;                 /* first pixel of this tile   */
        int n     = 8 - first;              /* pixels left in the tile    */
        if (x + n > DISPLAY_WIDTH)
            n = DISPLAY_WIDTH - x;

        if (bg->bpp8)
        {
            const u8 *row = vram + bg->charBase + tileNum * TILE_SIZE_8BPP + py * 8;
            for (int i = 0; i < n; i++) {
                int sx = first + i;
                if (hFlip) sx = 7 - sx;
                out[x + i] = sPal8[row[sx]];
            }
        }
        else
        {
            /* Tile rows are 4 bytes and always 4-byte aligned, so the whole
             * row comes in with one load. Big-endian: byte 0 is the top
             * byte, and within a byte the low nibble is the left pixel. */
            const u32 *row = (const u32 *)(vram + bg->charBase
                                           + tileNum * TILE_SIZE_4BPP + py * 4);
            const u16 *pal = sPal4 + ((entry >> 12) & 0xF) * 16;
            u32 w = *row;

            if (n == 8 && !hFlip) {
                /* The common case: a whole unflipped tile */
                out[x + 0] = pal[(w >> 24) & 0xF];
                out[x + 1] = pal[(w >> 28) & 0xF];
                out[x + 2] = pal[(w >> 16) & 0xF];
                out[x + 3] = pal[(w >> 20) & 0xF];
                out[x + 4] = pal[(w >>  8) & 0xF];
                out[x + 5] = pal[(w >> 12) & 0xF];
                out[x + 6] = pal[(w >>  0) & 0xF];
                out[x + 7] = pal[(w >>  4) & 0xF];
            } else {
                for (int i = 0; i < n; i++) {
                    int sx = first + i;
                    if (hFlip) sx = 7 - sx;
                    int shift = (sx & 1) * 4 + (3 - (sx >> 1)) * 8;
                    out[x + i] = pal[(w >> shift) & 0xF];
                }
            }
        }

        x += n;
    }
}

/* -----------------------------------------------------------------------
 * Affine (modes 1/2) scanline renderer
 *
 * Steps the texture coordinate by pa/pc across the line instead of
 * recomputing the full matrix product per pixel, and masks instead of
 * dividing -- affine map sizes are always powers of two.
 * --------------------------------------------------------------------- */
static void RenderAffineLine(const BgDesc *bg, int y, u16 *out)
{
    const u8 *vram = VramBuf();

    static const int affineMapSizes[4] = {128, 256, 512, 1024};
    int mapSize  = affineMapSizes[bg->screenSize];
    int mapMask  = mapSize - 1;
    int mapTiles = mapSize >> 3;

    s32 texX = bg->refX + bg->pb * y;
    s32 texY = bg->refY + bg->pd * y;

    for (int x = 0; x < DISPLAY_WIDTH; x++, texX += bg->pa, texY += bg->pc)
    {
        int px = texX >> 8;
        int py = texY >> 8;

        if (bg->areaOverflow) {
            px &= mapMask;
            py &= mapMask;
        } else if (px < 0 || px >= mapSize || py < 0 || py >= mapSize) {
            out[x] = 0;
            continue;
        }

        /* Affine screenblocks hold 1-byte entries and are always 8bpp */
        u8 tileNum = vram[bg->screenBase + (py >> 3) * mapTiles + (px >> 3)];
        out[x] = sPal8[vram[bg->charBase + tileNum * TILE_SIZE_8BPP
                            + (py & 7) * 8 + (px & 7)]];
    }
}

/* RGBA5551 back to the GBA's RGB555, for the blending paths which work in
 * the palette's own colour space. */
static inline u16 RGBA5551toRGB555(u16 c)
{
    return (u16)(((c >> 11) & 0x1F) | (((c >> 6) & 0x1F) << 5) | (((c >> 1) & 0x1F) << 10));
}

/* -----------------------------------------------------------------------
 * Window mask for a pixel
 *
 * Returns the layer enable bits for a given pixel position.
 * Bit layout mirrors GBA WININ/WINOUT: bits 5-0 = BG0-BG3, OBJ, effects.
 * --------------------------------------------------------------------- */
static u8 sWinMaskRow[DISPLAY_WIDTH];

/* Fills sWinMaskRow for one scanline. Returns 0 if no window is enabled,
 * in which case the row is left untouched and every pixel is 0x3F. */
static int BuildWindowMaskRow(int y)
{
    u16 dispcnt = _REG16(REG_OFFSET_DISPCNT);
    int win0en  = (dispcnt >> 13) & 1;
    int win1en  = (dispcnt >> 14) & 1;

    if (!win0en && !win1en)
        return 0;  /* all layers visible, no windowing */

    u16 winin  = _REG16(REG_OFFSET_WININ);
    u16 winout = _REG16(REG_OFFSET_WINOUT);

    u8 outMask = (u8)(winout & 0x3F);
    u8 in0Mask = (u8)(winin & 0x3F);
    u8 in1Mask = (u8)((winin >> 8) & 0x3F);

    int x0a = 0, x0b = 0, x1a = 0, x1b = 0;

    if (win0en) {
        u16 win0h = _REG16(REG_OFFSET_WIN0H);
        u16 win0v = _REG16(REG_OFFSET_WIN0V);
        int y1 = (win0v >> 8) & 0xFF, y2 = win0v & 0xFF;
        if (y >= y1 && y < y2) {
            x0a = (win0h >> 8) & 0xFF;
            x0b = win0h & 0xFF;
        }
    }
    if (win1en) {
        u16 win1h = _REG16(REG_OFFSET_WIN1H);
        u16 win1v = _REG16(REG_OFFSET_WIN1V);
        int y1 = (win1v >> 8) & 0xFF, y2 = win1v & 0xFF;
        if (y >= y1 && y < y2) {
            x1a = (win1h >> 8) & 0xFF;
            x1b = win1h & 0xFF;
        }
    }

    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        if (x >= x0a && x < x0b)
            sWinMaskRow[x] = in0Mask;
        else if (x >= x1a && x < x1b)
            sWinMaskRow[x] = in1Mask;
        else
            sWinMaskRow[x] = outMask;
    }
    return 1;
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

    BuildPaletteCache(pltt);

    /* Backdrop colour (BG palette entry 0) */
    u16 backdropRGB555 = PlttRead(pltt, 0);
    u16 backdropRGBA   = RGB555toRGBA5551(backdropRGB555);

    /* Fast path: no windows and no colour effects, which is what most of
     * the game runs in. The layer buffers are already in framebuffer
     * format, so compositing is a back-to-front overwrite of non-zero
     * (non-transparent) pixels with no per-pixel conversion at all. */
    int fastPath = (blendEff == 0)
                && !((dispcnt >> 13) & 1) && !((dispcnt >> 14) & 1);

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        /* Apply per-scanline register changes (battle wave effects, etc.) */
        ScanlineEffect_ApplyLine(y);

        /* Re-read scroll registers for this scanline */
        for (int i = 0; i < 4; i++)
            ParseBgDesc(i, &bgs[i], bgMode);

        int windowed = fastPath ? 0 : BuildWindowMaskRow(y);

        /* Render each enabled layer's scanline into its own line buffer */
        for (int bgIdx = 0; bgIdx < 4; bgIdx++) {
            if (!bgs[bgIdx].enabled)
                continue;

            u16 *lb = sLine[bgIdx];

            if (bgMode == 0 || (bgMode == 1 && bgIdx < 2)) {
                RenderTextLine(&bgs[bgIdx], y, lb);
            } else if ((bgMode == 1 && bgIdx == 2) || bgMode == 2) {
                RenderAffineLine(&bgs[bgIdx], y, lb);
            } else if (bgMode == 3 && bgIdx == 2) {
                /* Bitmap mode 3: 240x160 direct-colour (LE bytes from ROM) */
                const u16 *src = (const u16 *)(VramBuf() + y * DISPLAY_WIDTH * 2);
                for (int x = 0; x < DISPLAY_WIDTH; x++)
                    lb[x] = RGB555toRGBA5551(__builtin_bswap16(src[x]));
            } else if (bgMode == 4 && bgIdx == 2) {
                /* Bitmap mode 4: 240x160 8bpp paletted */
                int frame = (dispcnt >> 4) & 1;
                const u8 *src = VramBuf() + frame * 0xA000 + y * DISPLAY_WIDTH;
                for (int x = 0; x < DISPLAY_WIDTH; x++)
                    lb[x] = sPal8[src[x]];
            } else {
                memset(lb, 0, sizeof(sLine[0]));
            }
        }

        u16 *rowOut = gN64GBAFramebuffer + y * DISPLAY_WIDTH;

        if (fastPath) {
            for (int x = 0; x < DISPLAY_WIDTH; x++)
                rowOut[x] = backdropRGBA;

            /* Paint back to front; a zero entry is transparent */
            for (int li = 3; li >= 0; li--) {
                int bgIdx = layerOrder[li];
                if (!bgs[bgIdx].enabled)
                    continue;
                const u16 *lb = sLine[bgIdx];
                for (int x = 0; x < DISPLAY_WIDTH; x++) {
                    u16 v = lb[x];
                    if (v)
                        rowOut[x] = v;
                }
            }
            continue;
        }

        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            u16 winMask = windowed ? sWinMaskRow[x] : 0x3F;

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

                u16 v = sLine[bgIdx][x];
                if (!v) continue;

                u16 colour = RGBA5551toRGB555(v);
                if (!gotTop) {
                    topColour = colour;
                    topLayer  = bgIdx;
                    gotTop    = 1;
                } else {
                    botColour = colour;
                    botLayer  = bgIdx;
                    gotBot    = 1;
                }
            }

            /* Apply colour effects. Bit 5 of the window mask is the
             * colour-special-effect enable: a window can exempt what it
             * covers from the blend or brightness pass. The main menu
             * relies on that to darken everything except the highlighted
             * entry, which without this came out uniformly grey. */
            u16 finalColour = topColour;
            if (!(winMask & 0x20))
            {
                /* effect disabled here */
            } else if (blendEff == 1 && gotTop &&
                (tgt1Mask & (topLayer < 0 ? 0x20 : (1 << topLayer))) &&
                (tgt2Mask & (botLayer < 0 ? 0x20 : (1 << botLayer))))
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
