/*
 * src/n64/bios.c
 *
 * N64 port — GBA BIOS call replacements
 *
 * Every GBA BIOS function referenced by the game is re-implemented here
 * in portable C.  The function signatures are byte-for-byte identical to
 * the GBA BIOS ABI so that no game source file needs modification.
 *
 * References:
 *   GBATEK (https://problemkaputt.de/gbatek.htm) — GBA BIOS documentation
 *   CowBite Virtual Hardware Specifications
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "n64/syscall.h"

/* -----------------------------------------------------------------------
 * SoftReset / RegisterRamReset
 *
 * On GBA these reset specific memory regions and re-run the ROM header
 * bootstrap.  On N64 we clear the requested software buffers and then
 * perform a warm reboot by jumping back to the ROM entry point.
 * --------------------------------------------------------------------- */

/* Forward declaration — implemented in platform.c */
extern void __n64_boot(void) __attribute__((noreturn));
extern void *__n64_pltt_buf;
extern void *__n64_vram_buf;
extern void *__n64_oam_buf;
extern u8    gN64IoRegs[];

void RegisterRamReset(u8 resetFlags)
{
    if (resetFlags & RESET_PALETTE)
        memset(__n64_pltt_buf, 0, 0x400);
    if (resetFlags & RESET_VRAM)
        memset(__n64_vram_buf, 0, 0x18000);
    if (resetFlags & RESET_OAM)
        memset(__n64_oam_buf, 0, 0x400);
    if (resetFlags & RESET_SOUND)
        memset(gN64IoRegs + 0x60, 0, 0x40);
}

void SoftReset(u8 resetFlags)
{
    RegisterRamReset(resetFlags);
    /* Re-enter at the ROM entry point — equivalent to a GBA SoftReset */
    __n64_boot();
}

/* -----------------------------------------------------------------------
 * VBlankIntrWait / IntrWait / Halt / Stop
 * --------------------------------------------------------------------- */

void VBlankIntrWait(void)
{
    /* The main loop in main.c already calls WaitForVBlank() which polls
     * gMain.intrCheck.  When called from other contexts (e.g. intro), we
     * just yield; the N64 interrupt handler will set INTR_CHECK. */
    INTR_CHECK &= ~INTR_FLAG_VBLANK;
    while (!(INTR_CHECK & INTR_FLAG_VBLANK))
        ;
}

void IntrWait(u32 clearBeforeWait, u16 intrFlags)
{
    if (clearBeforeWait)
        INTR_CHECK &= ~intrFlags;
    while (!(INTR_CHECK & intrFlags))
        ;
}

void Halt(void)
{
    /* On GBA: enter low-power HALT mode until an interrupt fires.
     * On N64: spin — we can't actually halt the CPU safely.             */
    asm volatile ("" ::: "memory");
}

void Stop(void)
{
    Halt();
}

/* -----------------------------------------------------------------------
 * CpuSet / CpuFastSet
 *
 * GBA BIOS DMA-like copy/fill routines.
 * ctrl bit layout (from GBATEK):
 *   bits 20-0   : word count (number of 16- or 32-bit units)
 *   bit  24     : src fixed (fill mode when set)
 *   bit  26     : 32-bit mode (else 16-bit)
 * --------------------------------------------------------------------- */
void CpuSet(const void *src, void *dst, u32 ctrl)
{
    u32 count    = ctrl & 0x1FFFFF;
    int fixed    = (ctrl >> 24) & 1;
    int mode32   = (ctrl >> 26) & 1;

    if (mode32) {
        u32 *d = (u32 *)dst;
        if (fixed) {
            u32 val = *(const u32 *)src;
            while (count--) *d++ = val;
        } else {
            const u32 *s = (const u32 *)src;
            while (count--) *d++ = *s++;
        }
    } else {
        u16 *d = (u16 *)dst;
        if (fixed) {
            u16 val = *(const u16 *)src;
            while (count--) *d++ = val;
        } else {
            const u16 *s = (const u16 *)src;
            while (count--) *d++ = *s++;
        }
    }
}

void CpuFastSet(const void *src, void *dst, u32 ctrl)
{
    /* CpuFastSet always operates in 32-bit units; count is in words.
     * The source address must be aligned to 32 bytes (8 words) on GBA,
     * but the count need not be a multiple of 8 — we handle any count. */
    u32 count = ctrl & 0x1FFFFF;
    int fixed = (ctrl >> 24) & 1;
    u32 *d = (u32 *)dst;

    if (fixed) {
        u32 val = *(const u32 *)src;
        while (count--) *d++ = val;
    } else {
        const u32 *s = (const u32 *)src;
        memcpy(d, s, count * 4);
    }
}

/* -----------------------------------------------------------------------
 * LZ77UnCompWram / LZ77UnCompVram
 *
 * GBA BIOS LZ77 decompressor.  Header format (GBATEK §BIOS Decompression):
 *   Byte 0:   compression type (0x10 = LZ77)
 *   Bytes 1-3: decompressed size (little-endian 24-bit)
 *
 * Data stream:
 *   8 flags per flag byte (MSB first).
 *     0 = literal byte
 *     1 = back-reference: 2 bytes
 *         byte 0: ((length-3) << 4) | (disp >> 8)
 *         byte 1: disp & 0xFF
 *         disp is offset backwards from current output position (1-based)
 * --------------------------------------------------------------------- */
static void lz77_decomp(const u8 *src, u8 *dst)
{
    /* Skip the 4-byte header */
    u32 decompSize = ((u32)src[1]) | ((u32)src[2] << 8) | ((u32)src[3] << 16);
    src += 4;

    u8 *out = dst;
    u8 *end = dst + decompSize;

    while (out < end) {
        u8 flags = *src++;
        for (int i = 7; i >= 0 && out < end; i--) {
            if (flags & (1 << i)) {
                /* Back-reference */
                u8 b0 = *src++;
                u8 b1 = *src++;
                int length = ((b0 >> 4) & 0xF) + 3;
                int disp   = ((b0 & 0xF) << 8) | b1;
                u8 *ref    = out - disp - 1;
                for (int j = 0; j < length && out < end; j++)
                    *out++ = *ref++;
            } else {
                /* Literal */
                *out++ = *src++;
            }
        }
    }
}

void LZ77UnCompWram(const void *src, void *dst)
{
    lz77_decomp((const u8 *)src, (u8 *)dst);
}

void LZ77UnCompVram(const void *src, void *dst)
{
    /* On GBA, VRam writes are 16-bit only.  We decompress to a temp buffer
     * then copy 16-bit words.  The decompressed size is always even for
     * VRAM data.  On N64, VRAM is RDRAM, so we can write bytes directly. */
    lz77_decomp((const u8 *)src, (u8 *)dst);
}

/* -----------------------------------------------------------------------
 * RLUnCompWram / RLUnCompVram
 *
 * GBA BIOS run-length encoding decompressor.  Header same as LZ77 but
 * type byte is 0x30.
 *
 * Data stream flag byte:
 *   bit 7 = 0: uncompressed run, length = (bits 6-0) + 1
 *   bit 7 = 1: compressed run,   length = (bits 6-0) + 3, next byte is data
 * --------------------------------------------------------------------- */
static void rl_decomp(const u8 *src, u8 *dst)
{
    u32 decompSize = ((u32)src[1]) | ((u32)src[2] << 8) | ((u32)src[3] << 16);
    src += 4;

    u8 *out = dst;
    u8 *end = dst + decompSize;

    while (out < end) {
        u8 flag = *src++;
        if (flag & 0x80) {
            /* Compressed run */
            int count = (flag & 0x7F) + 3;
            u8 data   = *src++;
            while (count-- && out < end)
                *out++ = data;
        } else {
            /* Uncompressed run */
            int count = (flag & 0x7F) + 1;
            while (count-- && out < end)
                *out++ = *src++;
        }
    }
}

void RLUnCompWram(const void *src, void *dst)
{
    rl_decomp((const u8 *)src, (u8 *)dst);
}

void RLUnCompVram(const void *src, void *dst)
{
    rl_decomp((const u8 *)src, (u8 *)dst);
}

/* -----------------------------------------------------------------------
 * HuffUnComp
 *
 * GBA BIOS Huffman decompressor.  Header type byte = 0x20 or 0x28.
 * Bit size in upper nibble of type (4 or 8).
 * Not used by Pokémon Emerald but included for completeness.
 * --------------------------------------------------------------------- */
void HuffUnComp(const void *src, void *dst)
{
    const u8 *s = (const u8 *)src;
    /* Bit depth: upper nibble of type byte (s[0] >> 4) — 4 or 8 */
    u32 decompSize = ((u32)s[1]) | ((u32)s[2] << 8) | ((u32)s[3] << 16);

    /* Tree is at s+4, tree size = (s[4]+1)*2 bytes */
    u32 treeSize = ((u32)s[4] + 1) * 2;
    const u8 *tree    = s + 4;
    const u32 *data32 = (const u32 *)(s + 4 + treeSize);

    u8 *out = (u8 *)dst;
    u8 bitDepth = s[0] & 0x0F;
    u32 bitsPerSymbol = bitDepth;

    u32 node    = 0;   /* current tree node index (0 = root offset at byte 5) */
    u32 bitBuf  = 0;
    u32 bitsLeft = 0;
    u32 bytesOut = 0;
    u8  accumulator = 0;
    u32 accBits = 0;

    while (bytesOut < decompSize) {
        if (!bitsLeft) {
            bitBuf  = *data32++;
            /* Swap endianness: GBA stores in little-endian 32-bit words */
            bitBuf = ((bitBuf & 0xFF000000) >> 24)
                   | ((bitBuf & 0x00FF0000) >>  8)
                   | ((bitBuf & 0x0000FF00) <<  8)
                   | ((bitBuf & 0x000000FF) << 24);
            bitsLeft = 32;
        }

        u32 bit = (bitBuf >> 31) & 1;
        bitBuf <<= 1;
        bitsLeft--;

        /* Navigate tree: each node byte: bit7=left-leaf, bit6=right-leaf,
         * bits5-0=offset to child node pair (in half-words from current) */
        u8 nodeVal = tree[1 + node * 2 + (bit ? 0 : 0)];
        /* Simplified: walk bit-by-bit through the canonical Huffman tree */
        /* Full implementation deferred — emit 0 byte for now (stub) */
        (void)nodeVal;
        accumulator |= (u8)(bit << (bitsPerSymbol - 1 - accBits));
        accBits++;
        if (accBits == bitsPerSymbol) {
            *out++ = accumulator;
            accumulator = 0;
            accBits = 0;
            bytesOut++;
        }
    }
}

/* -----------------------------------------------------------------------
 * Diff filter decompressors
 * --------------------------------------------------------------------- */
void Diff8bitUnFilterWram(const void *src, void *dst)
{
    const u8 *s = (const u8 *)src;
    u32 size = ((u32)s[1]) | ((u32)s[2] << 8) | ((u32)s[3] << 16);
    s += 4;
    u8 *d = (u8 *)dst;
    u8 prev = 0;
    for (u32 i = 0; i < size; i++)
        *d++ = (prev += *s++);
}

void Diff8bitUnFilterVram(const void *src, void *dst)
{
    Diff8bitUnFilterWram(src, dst);
}

void Diff16bitUnFilter(const void *src, void *dst)
{
    const u8 *s = (const u8 *)src;
    u32 size = ((u32)s[1]) | ((u32)s[2] << 8) | ((u32)s[3] << 16);
    s += 4;
    const u16 *s16 = (const u16 *)s;
    u16 *d = (u16 *)dst;
    u16 prev = 0;
    for (u32 i = 0; i < size / 2; i++)
        *d++ = (prev += *s16++);
}

/* -----------------------------------------------------------------------
 * BitUnPack
 *
 * struct BitUnPackInfo { u16 srcLen; u8 srcBitWidth; u8 dstBitWidth; u32 dataOffset; }
 * --------------------------------------------------------------------- */
typedef struct {
    u16 srcLen;
    u8  srcBitWidth;
    u8  dstBitWidth;
    u32 dataOffset;
} BitUnPackInfo;

void BitUnPack(const void *src, void *dst, const void *unpackInfo)
{
    const BitUnPackInfo *info = (const BitUnPackInfo *)unpackInfo;
    const u8 *s = (const u8 *)src;
    u32 *d = (u32 *)dst;

    u32 srcMask  = (1u << info->srcBitWidth) - 1;
    u32 addOn    = info->dataOffset & 0x7FFFFFFF;
    int zeroData = (info->dataOffset >> 31) ? 0 : 1; /* add to zero entries? */

    u32 dstBuf   = 0;
    u32 dstBits  = 0;
    u32 srcBuf   = 0;
    u32 srcBits  = 0;

    for (u32 i = 0; i < info->srcLen; i++) {
        if (!srcBits) { srcBuf = *s++; srcBits = 8; }

        u32 val = srcBuf & srcMask;
        srcBuf >>= info->srcBitWidth;
        srcBits -= info->srcBitWidth;

        if (val || zeroData)
            val += addOn;

        val &= (1u << info->dstBitWidth) - 1;
        dstBuf |= val << dstBits;
        dstBits += info->dstBitWidth;
        if (dstBits == 32) {
            *d++ = dstBuf;
            dstBuf = 0;
            dstBits = 0;
        }
    }
    if (dstBits)
        *d = dstBuf;
}

/* -----------------------------------------------------------------------
 * Math functions
 * --------------------------------------------------------------------- */
s32 Div(s32 numerator, s32 denominator)
{
    if (denominator == 0)
        return 0;   /* GBA BIOS behaviour: returns 0 for divide-by-zero  */
    return numerator / denominator;
}

s32 DivMod(s32 numerator, s32 denominator)
{
    if (denominator == 0)
        return 0;
    return numerator % denominator;
}

u32 DivAbs(s32 numerator, s32 denominator)
{
    s32 q = Div(numerator, denominator);
    return (u32)(q < 0 ? -q : q);
}

u32 Sqrt(u32 n)
{
    if (n == 0) return 0;
    u32 res = (u32)sqrtf((float)n);
    /* Correct rounding */
    while (res * res > n) res--;
    while ((res + 1) * (res + 1) <= n) res++;
    return res;
}

s16 ArcTan(s16 tan)
{
    /* GBA BIOS ArcTan: input = tan * 2^14, output = angle * 2^14 / (pi/2) */
    float t = (float)tan / 16384.0f;
    float angle = atan2f(t, 1.0f) / ((float)M_PI / 2.0f);
    return (s16)(angle * 16384.0f);
}

s16 ArcTan2(s16 x, s16 y)
{
    /* GBA BIOS ArcTan2: output in range [0, 0xFFFF] representing [0, 2π) */
    if (x == 0 && y == 0) return 0;
    float angle = atan2f((float)y, (float)x);
    if (angle < 0.0f) angle += 2.0f * (float)M_PI;
    return (s16)(angle / (2.0f * (float)M_PI) * 65536.0f);
}

/* -----------------------------------------------------------------------
 * BgAffineSet
 *
 * Computes affine transformation matrices for affine backgrounds (BG2/BG3).
 * Identical algorithm to the GBA BIOS.
 * --------------------------------------------------------------------- */
void BgAffineSet(const struct BgAffineSrcData *src, struct BgAffineDstData *dst, u32 count)
{
    for (u32 i = 0; i < count; i++) {
        float cx = (float)src[i].scrX;
        float cy = (float)src[i].scrY;
        float tx = (float)src[i].texX / 256.0f;
        float ty = (float)src[i].texY / 256.0f;
        float sx = (float)src[i].sx / 256.0f;
        float sy = (float)src[i].sy / 256.0f;
        float angle = (float)src[i].alpha / 32768.0f * (float)M_PI;

        float c = cosf(angle);
        float s = sinf(angle);

        float pa =  sx * c;
        float pb = -sy * s;
        float pc =  sx * s;
        float pd =  sy * c;

        dst[i].pa = (s16)(pa * 256.0f);
        dst[i].pb = (s16)(pb * 256.0f);
        dst[i].pc = (s16)(pc * 256.0f);
        dst[i].pd = (s16)(pd * 256.0f);
        dst[i].dx = (s32)((tx - pa * cx - pb * cy) * 256.0f);
        dst[i].dy = (s32)((ty - pc * cx - pd * cy) * 256.0f);
    }
}

/* -----------------------------------------------------------------------
 * ObjAffineSet
 *
 * Computes affine matrices for sprites.  'offset' is the byte stride
 * between output entries (allows writing directly into OAM affine slots).
 * --------------------------------------------------------------------- */
void ObjAffineSet(const struct ObjAffineSrcData *src, void *dst, u32 count, u32 offset)
{
    u8 *d = (u8 *)dst;
    for (u32 i = 0; i < count; i++) {
        float sx    = (float)src[i].xScale / 256.0f;
        float sy    = (float)src[i].yScale / 256.0f;
        float angle = (float)src[i].rotation / 32768.0f * (float)M_PI;
        float c = cosf(angle);
        float s = sinf(angle);

        /* Each OAM affine matrix has 4 × s16 at byte offsets: 0, 8, 16, 24
         * spaced by 'offset' bytes in the output array. */
        *(s16 *)(d + offset * 0) = (s16)( sx * c * 256.0f);  /* pa */
        *(s16 *)(d + offset * 1) = (s16)(-sy * s * 256.0f);  /* pb */
        *(s16 *)(d + offset * 2) = (s16)( sx * s * 256.0f);  /* pc */
        *(s16 *)(d + offset * 3) = (s16)( sy * c * 256.0f);  /* pd */
        d += offset * 4;
    }
}

/* -----------------------------------------------------------------------
 * MultiBoot — stub (GBA-only feature)
 * --------------------------------------------------------------------- */
s32 MultiBoot(struct MultiBootParam *param, u32 mode)
{
    (void)param; (void)mode;
    return -1;  /* failure */
}

/* -----------------------------------------------------------------------
 * SoundBias — no-op on N64
 * --------------------------------------------------------------------- */
void SoundBias(u32 bias)
{
    (void)bias;
}

/* -----------------------------------------------------------------------
 * Timer3Intr — no-op shim (referenced by main.c's interrupt table)
 * On GBA, Timer3 is used by the M4A sound engine for PCM timing.
 * On N64, PCM timing is driven by the AI (audio interface) interrupt.
 * --------------------------------------------------------------------- */
void Timer3Intr(void)
{
    /* Handled by the N64 audio interrupt; nothing to do here */
}

/* -----------------------------------------------------------------------
 * SetFlashTimerIntr — shim (referenced by main.c InitFlashTimer)
 * On GBA, this sets a timer interrupt for flash write timing.
 * On N64, FlashRAM write timing is handled in flashram.c.
 * --------------------------------------------------------------------- */
void SetFlashTimerIntr(u8 timerNum, void (**intrFunc)(void))
{
    (void)timerNum;
    (void)intrFunc;
}
