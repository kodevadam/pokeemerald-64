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
 *
 * The full software compositor pipeline is:
 *   1. N64_CompositeFrame()   — tile renderer (tile_renderer.c)
 *   2. N64_CompositeSprites() — sprite renderer (sprite_renderer.c)
 *   3. N64_BlitGBAFrame()     — scale/centre into VI framebuffer (vi.c)
 *   4. N64_VISwapBuffers()    — flip front/back framebuffers
 *
 * CopyBufferedValuesToGpuRegs() is called from VBlankIntr(), which runs in
 * interrupt context (see interrupt.c's N64_DispatchIntr).  On real GBA
 * hardware this function is a handful of register writes -- microseconds.
 * Here it is a full software render of a 240x160 frame plus up to 128
 * sprites, measured at up to ~3.7x N64_COUNTS_PER_FRAME (the entire CPU
 * cycle budget of one 60 Hz VBlank period) once real tile/sprite content
 * is loaded.  Running that synchronously inside the interrupt handler is
 * fatal: MI's VI interrupt is level-triggered, so if servicing it takes
 * longer than one VBlank period, the next VBlank is already pending the
 * instant this one returns, and the main thread never regains the CPU --
 * an unbreakable back-to-back interrupt chain indistinguishable from an
 * infinite loop anywhere else in the game (this is exactly how Pokemon
 * Emerald 64's boot sequence stalled forever partway through the
 * copyright screen, once LoadCopyrightGraphics() populated real VRAM
 * content for the compositor to render).
 *
 * So CopyBufferedValuesToGpuRegs() itself just raises a flag (sGN64RenderPending)
 * and returns immediately, keeping the interrupt handler fast regardless of
 * compositor cost.  N64_RunDeferredCompositor(), called once per iteration
 * from WaitForVBlank() in main.c (outside interrupt context), does the
 * actual work.  This decouples "how long rendering takes" from "whether
 * interrupts can be serviced": other interrupts (and the exception
 * mechanism generally) keep working normally while a render is in
 * progress, so the game runs at whatever framerate the compositor can
 * sustain instead of hanging outright once it can't keep up with 60 Hz.
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
 * CopyBufferedValuesToGpuRegs — called each VBlank from main.c's
 * VBlankIntr(), i.e. from interrupt context.
 *
 * On GBA this flushes the pending register write queue to MMIO -- cheap.
 * On N64 it must NOT run the compositor directly (see the file comment
 * above); it just raises sGN64RenderPending for N64_RunDeferredCompositor()
 * to pick up outside interrupt context.
 * --------------------------------------------------------------------- */
static volatile u8 sGN64RenderPending = 0;

void CopyBufferedValuesToGpuRegs(void)
{
    sGN64RenderPending = 1;
}

/* -----------------------------------------------------------------------
 * N64_RunDeferredCompositor — the actual compositor pipeline, run from
 * WaitForVBlank() in main.c, outside interrupt context.  See the file
 * comment above for why this is split out of CopyBufferedValuesToGpuRegs().
 * --------------------------------------------------------------------- */
extern void N64_CompositeFrame(void);      /* tile_renderer.c   */
extern void N64_CompositeSprites(void);    /* sprite_renderer.c */
extern void N64_BlitGBAFrame(void);        /* vi.c              */
extern void N64_VISwapBuffers(void);       /* vi.c              */

void N64_RunDeferredCompositor(void)
{
    if (!sGN64RenderPending)
        return;
    sGN64RenderPending = 0;

    /* Run the full software compositor */
    N64_CompositeFrame();
    N64_CompositeSprites();
    N64_BlitGBAFrame();
    N64_VISwapBuffers();

    /* Update VCOUNT to match current VI line */
    extern volatile u16 gN64CurrentLine;
    _REG16(REG_OFFSET_VCOUNT) = gN64CurrentLine;
}

/* -----------------------------------------------------------------------
 * EnableInterrupts / DisableInterrupts
 * Declared in gpu_regs.h; implemented here for N64.
 * --------------------------------------------------------------------- */
void EnableInterrupts(u16 mask)
{
    N64_IntrEnable(mask);
}

void DisableInterrupts(u16 mask)
{
    /* On N64 we don't currently disable individual interrupt sources;
     * the game uses this for temporary critical sections which are
     * short enough that we can ignore. */
    (void)mask;
}

/* -----------------------------------------------------------------------
 * SetGpuReg_ForcedBlank — sets a register while also enabling forced blank.
 * On N64, forced blank just means we clear screen (the compositor handles it).
 * --------------------------------------------------------------------- */
void SetGpuReg_ForcedBlank(u8 regOffset, u16 value)
{
    /* Enable forced blank in DISPCNT */
    _REG16(REG_OFFSET_DISPCNT) |= DISPCNT_FORCED_BLANK;
    /* Write the target register */
    SetGpuReg(regOffset, value);
}
