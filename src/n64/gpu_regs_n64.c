/*
 * src/n64/gpu_regs_n64.c
 *
 * N64 port — GPU register manager
 *
 * The original src/gpu_regs.c buffers GBA GPU register writes and flushes
 * them to hardware each VBlank.  On N64 the "hardware" is our software IO
 * register file (gN64IoRegs[]), so there is no separate flush step needed.
 *
 * We provide the same public API (SetGpuReg, GetGpuReg, etc.) so that all
 * game code that calls them compiles and links without modification.
 *
 * SetGpuReg() writes directly to gN64IoRegs[].
 * CopyBufferedValuesToGpuRegs() is a no-op (already live).
 *
 * Additionally, after each VBlank we trigger the full compositor pipeline:
 *   1. N64_CompositeFrame()   — tile renderer (tile_renderer.c)
 *   2. N64_CompositeSprites() — sprite renderer (sprite_renderer.c)
 *   3. N64_BlitGBAFrame()     — scale/centre into VI framebuffer (vi.c)
 *   4. N64_VISwapBuffers()    — flip front/back framebuffers
 */

#include "global.h"
#include "gpu_regs.h"

/* -----------------------------------------------------------------------
 * GPU register buffering state
 *
 * The original gpu_regs.c uses a 2-entry pending queue to handle writes
 * that occur during HBlank DMA.  On N64 we simplify: all writes go
 * directly to the software register file.
 * --------------------------------------------------------------------- */

static u8 sGpuRegBufCount = 0;   /* always 0 on N64 — kept for API compat */

void InitGpuRegManager(void)
{
    sGpuRegBufCount = 0;
}

/* Write a 16-bit value to the GPU register at the given offset */
void SetGpuReg(u8 offset, u16 value)
{
    if (offset < N64_IOREGS_SIZE - 1)
        _REG16(offset) = value;
}

/* Read the current value of a GPU register */
u16 GetGpuReg(u8 offset)
{
    if (offset < N64_IOREGS_SIZE - 1)
        return _REG16(offset);
    return 0;
}

/* Bitwise-OR a value into a GPU register */
void SetGpuRegBits(u8 offset, u16 mask)
{
    if (offset < N64_IOREGS_SIZE - 1)
        _REG16(offset) |= mask;
}

/* Bitwise-AND-NOT a mask from a GPU register (clear bits) */
void ClearGpuRegBits(u8 offset, u16 mask)
{
    if (offset < N64_IOREGS_SIZE - 1)
        _REG16(offset) &= ~mask;
}

/* Set the upper byte of a register (used for DISPSTAT VCount line) */
void SetGpuRegByteUpper(u8 offset, u8 value)
{
    if (offset < N64_IOREGS_SIZE - 1) {
        u16 reg = _REG16(offset);
        _REG16(offset) = (reg & 0x00FF) | ((u16)value << 8);
    }
}

/* Set the lower byte of a register */
void SetGpuRegByteLower(u8 offset, u8 value)
{
    if (offset < N64_IOREGS_SIZE - 1) {
        u16 reg = _REG16(offset);
        _REG16(offset) = (reg & 0xFF00) | value;
    }
}

/* -----------------------------------------------------------------------
 * CopyBufferedValuesToGpuRegs — called each VBlank from main.c
 *
 * On GBA this flushes the pending register write queue to MMIO.
 * On N64 all writes are already live in gN64IoRegs[], so we use this
 * callback to trigger the full compositor pipeline.
 * --------------------------------------------------------------------- */
extern void N64_CompositeFrame(void);      /* tile_renderer.c   */
extern void N64_CompositeSprites(void);    /* sprite_renderer.c */
extern void N64_BlitGBAFrame(void);        /* vi.c              */
extern void N64_VISwapBuffers(void);       /* vi.c              */

void CopyBufferedValuesToGpuRegs(void)
{
    /* Run the full software compositor */
    N64_CompositeFrame();
    N64_CompositeSprites();
    N64_BlitGBAFrame();
    N64_VISwapBuffers();

    /* Update VCOUNT to match current VI line */
    extern volatile u16 gN64CurrentLine;
    _REG16(REG_OFFSET_VCOUNT) = gN64CurrentLine;
}
