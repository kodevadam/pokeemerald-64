/*
 * src/n64/interrupt.c
 *
 * N64 port — interrupt controller
 *
 * The N64 uses a single hardware interrupt line (IP2 in the MIPS Cause
 * register) driven by the RCP Memory Interface (MI).  Six sub-interrupts
 * are multiplexed through MI_INTR_REG:
 *
 *   MI_INTR_SP (0x01)  RSP finished executing
 *   MI_INTR_SI (0x02)  Serial Interface (controller read done)
 *   MI_INTR_AI (0x04)  Audio Interface (audio DMA buffer empty)
 *   MI_INTR_VI (0x08)  Video Interface (VBlank / line)
 *   MI_INTR_PI (0x10)  Peripheral Interface (PI DMA done)
 *   MI_INTR_DP (0x20)  RDP finished
 *
 * N64_DispatchIntr() is called from the MIPS general exception handler in
 * crt0.s whenever the CPU takes an interrupt exception.  It reads MI_INTR
 * and dispatches the appropriate GBA-style callbacks.
 *
 * The GBA had a priority-ordered interrupt table (gIntrTable[] in main.c).
 * We replicate that dispatching logic so that VBlank, HBlank (scanline),
 * VCount, and Serial callbacks fire at the right points.
 */

#include <stdint.h>
#include "global.h"
#include "main.h"
#include "n64/asm_defs.h"

extern void N64_RtcVBlankTick(void);

/* -----------------------------------------------------------------------
 * N64 hardware register access helpers
 * All N64 MMIO registers are 32-bit, big-endian; use byte-swap wrappers.
 * --------------------------------------------------------------------- */
#define MI_INTR_MASK_WR(val)    N64_HW_WR(N64_MI_BASE_REG, MI_INTR_MASK_REG, (val))
#define MI_INTR_RD()            N64_HW_RD(N64_MI_BASE_REG, MI_INTR_REG)
#define VI_CURRENT_RD()         N64_HW_RD(N64_VI_BASE_REG, VI_CURRENT_REG)
#define VI_INTR_LINE_WR(val)    N64_HW_WR(N64_VI_BASE_REG, VI_INTR_REG, (val))

/* -----------------------------------------------------------------------
 * Software VBlank counter and current VI line
 * vi.c updates gN64ViLine each VI interrupt so that REG_VCOUNT reads work.
 * --------------------------------------------------------------------- */
volatile u32 gN64VBlankCount = 0;
volatile u16 gN64CurrentLine = 0;

/* -----------------------------------------------------------------------
 * N64_IntrEnable — called by IntrEnable() macro in macro.h
 *
 * Translates GBA interrupt flag bits into N64 MI interrupt mask writes.
 * --------------------------------------------------------------------- */
void N64_IntrEnable(u16 gbaFlags)
{
    u32 miMask = 0;

    /* VBlank → VI interrupt */
    if (gbaFlags & (INTR_FLAG_VBLANK | INTR_FLAG_VCOUNT | INTR_FLAG_HBLANK))
        miMask |= MI_INTR_VI;

    /* Serial → SI interrupt (controller) */
    if (gbaFlags & INTR_FLAG_SERIAL)
        miMask |= MI_INTR_SI;

    /* Timer3 / DMA → AI / PI interrupts (audio/DMA) */
    if (gbaFlags & (INTR_FLAG_TIMER3 | INTR_FLAG_DMA3))
        miMask |= MI_INTR_AI | MI_INTR_PI;

    /* Write MI_INTR_MASK: set bits by writing 1 to "set" positions.
     * N64 MI mask format: write 0xABCD where each nibble controls one
     * interrupt: bit 0 = clear mask, bit 1 = set mask. */
    if (miMask) {
        /* Each set bit in miMask: write (1 << (bit*2+1)) to set the mask */
        u32 setWord = 0;
        for (int i = 0; i < 6; i++) {
            if (miMask & (1u << i))
                setWord |= (1u << (i * 2 + 1));
        }
        MI_INTR_MASK_WR(setWord);
    }

    /* Enable CPU interrupts (CP0 Status IE bit) */
    u32 sr;
    asm volatile (
        "mfc0 %0, $12\n\t"
        "ori  %0, %0, 0x0401\n\t"  /* set IE and IM2 (RCP interrupt) */
        "mtc0 %0, $12\n\t"
        : "=r"(sr)
    );
}

/* -----------------------------------------------------------------------
 * N64_DispatchIntr — called from crt0.s general exception handler
 *
 * Reads MIPS Cause register to confirm this is an interrupt (not a trap),
 * then reads MI_INTR to find which N64 interrupt fired and dispatches
 * the corresponding GBA-style callback.
 * --------------------------------------------------------------------- */
void N64_DispatchIntr(void)
{
    /* Read CP0 Cause register */
    u32 cause;
    asm volatile ("mfc0 %0, $13" : "=r"(cause));

    /* TEMPORARY DIAGNOSTIC: paint the interrupted PC at the top of the
     * framebuffer. When the main loop stops, the compositor stops painting
     * over it, so the last value left on screen is where the CPU is stuck. */
    {
        extern u16 *gN64FrontBuffer;
        extern u32 gDiagLzSrc, gDiagLzDst, gDiagLzSize;
        u32 epc;
        u32 vals[4];
        asm volatile ("mfc0 %0, $14" : "=r"(epc));
        vals[0] = epc;
        vals[1] = gDiagLzSrc;
        vals[2] = gDiagLzDst;
        vals[3] = gDiagLzSize;
        for (int k = 0; k < 4; k++) {
            for (int b = 0; b < 8; b++) {
                u16 c = (u16)((((vals[k] >> ((7 - b) * 4)) & 0xFu) << 11) | 1);
                for (int y = 30 + k * 12; y < 40 + k * 12; y++)
                    for (int x = 0; x < 14; x++)
                        gN64FrontBuffer[y * 320 + b * 14 + x] = c;
            }
        }
    }

    /* Check ExcCode field (bits 6-2): 0 = interrupt */
    if ((cause & CAUSE_EXCCODE) != 0)
        return;  /* Not an interrupt — unexpected exception, ignore */

    /* Read MI interrupt register.
     * Acknowledge each sub-interrupt BEFORE dispatching game callbacks so
     * that re-entry cannot happen with a stale MI_INTR pending bit. */
    u32 miIntr = MI_INTR_RD();

    /* Acknowledge all hardware interrupts immediately.
     *
     * Every interrupt we unmask in MI MUST be acknowledged here.  The MI
     * interrupt line is level-triggered through CP0 Cause.IP2, so any source
     * left asserted re-enters this handler the instant ERET executes.  That
     * starves the main thread completely: the game never advances a single
     * instruction and the screen freezes on whatever was last drawn, with no
     * crash and no other symptom.
     *
     * AI in particular asserts as soon as the audio DMA runs dry, which
     * happens immediately after N64_InitAI() since nothing is queued yet.
     * Writing AI_STATUS is what clears it — N64_AudioRefill() below only
     * refills the buffer and does not touch the interrupt.  */
    if (miIntr & MI_INTR_VI) N64_HW_WR(N64_VI_BASE_REG, VI_CURRENT_REG, 0);
    if (miIntr & MI_INTR_SI) N64_HW_WR(N64_SI_BASE_REG, SI_STATUS_REG, 0);
    if (miIntr & MI_INTR_AI) N64_HW_WR(N64_AI_BASE_REG, AI_STATUS_REG, 0);
    if (miIntr & MI_INTR_PI) N64_HW_WR(N64_PI_BASE_REG, PI_STATUS_REG, 2);
    if (miIntr & MI_INTR_SP) N64_HW_WR(N64_SP_BASE_REG, 0x10, 0x08);
    if (miIntr & MI_INTR_DP) N64_HW_WR(N64_DP_BASE_REG, 0x0C, 0);

    /* ------------------------------------------------------------------
     * VI interrupt — VBlank / VCount / HBlank
     * (Already acknowledged above via VI_CURRENT write)
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_VI) {
        /* The VI interrupt fires at half-line 2 — once per frame.
         * Treat every VI interrupt as the GBA VBlank event.
         * gIntrTable layout (from main.c gIntrTableTemplate):
         *   [0] = VCountIntr, [1] = SerialIntr, [2] = Timer3Intr,
         *   [3] = HBlankIntr, [4] = VBlankIntr                           */
        gN64VBlankCount++;
        gN64CurrentLine = 0;
        _REG16(REG_OFFSET_VCOUNT)   = 0;
        _REG16(REG_OFFSET_DISPSTAT) |= DISPSTAT_VBLANK;

        /* NOTE: N64_DebugPrint (IS-Viewer at 0xB3FF0014) removed — SC64 does
         * not ACK PI bus write transactions to physical 0x13FF0014 (IS-Viewer
         * range), causing an indefinite PI bus stall on first VBlank. */

        N64_RtcVBlankTick();

        /* Kick off a controller read each VBlank so button state is ready
         * for the next frame.  N64_ControllerReadDone() (SI handler) does
         * NOT restart the read — doing so from the SI handler would cause
         * an immediate SI re-interrupt after every ERET, starving N64Main. */
        extern void N64_InputStartRead(void);
        N64_InputStartRead();

        extern IntrFunc gIntrTable[];
        if (gIntrTable[4])
            gIntrTable[4]();   /* VBlankIntr */

        /* Signal VBlank to BIOS VBlankIntrWait() / IntrWait() */
        INTR_CHECK |= INTR_FLAG_VBLANK;
        gMain.intrCheck |= INTR_FLAG_VBLANK;

        _REG16(REG_OFFSET_DISPSTAT) &= ~DISPSTAT_VBLANK;

        /* Fire VCount interrupt every frame when enabled.
         * N64 has no per-scanline interrupts; fire conceptually "at
         * the configured line" which is always once per frame here.    */
        if (_REG16(REG_OFFSET_DISPSTAT) & DISPSTAT_VCOUNT_INTR) {
            _REG16(REG_OFFSET_DISPSTAT) |= DISPSTAT_VCOUNT_MATCH;
            if (gIntrTable[0])
                gIntrTable[0]();   /* VCountIntr */
            _REG16(REG_OFFSET_DISPSTAT) &= ~DISPSTAT_VCOUNT_MATCH;
            INTR_CHECK |= INTR_FLAG_VCOUNT;
            gMain.intrCheck |= INTR_FLAG_VCOUNT;
        }
    }

    /* ------------------------------------------------------------------
     * AI interrupt — audio buffer empty; refill it
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_AI) {
        extern void N64_AudioRefill(void);
        N64_AudioRefill();
    }

    /* ------------------------------------------------------------------
     * SI interrupt — controller data ready
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_SI) {
        extern void N64_ControllerReadDone(void);
        N64_ControllerReadDone();

        /* Acknowledge SI */
        N64_HW_WR(N64_SI_BASE_REG, SI_STATUS_REG, 0);
    }

    /* ------------------------------------------------------------------
     * PI interrupt — DMA done (e.g. FlashRAM write complete)
     * Acked above; only the application callback is needed here.
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_PI) {
        extern void N64_PiDmaDone(void);
        N64_PiDmaDone();
    }

    /* SP / DP interrupts — unused; acked above, no callbacks needed */
}
