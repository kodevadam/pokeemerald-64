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

    /* Check ExcCode field (bits 6-2): 0 = interrupt */
    if ((cause & CAUSE_EXCCODE) != 0)
        return;  /* Not an interrupt — unexpected exception, ignore */

    /* Read MI interrupt register */
    u32 miIntr = MI_INTR_RD();

    /* ------------------------------------------------------------------
     * VI interrupt — VBlank / VCount / HBlank
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_VI) {
        /* Acknowledge the VI interrupt by re-arming VI_INTR */
        VI_INTR_LINE_WR(2);

        /* The VI interrupt fires at half-line 2 — once per frame.
         * Treat every VI interrupt as the GBA VBlank event.
         * gIntrTable layout (from main.c gIntrTableTemplate):
         *   [0] = VCountIntr, [1] = SerialIntr, [2] = Timer3Intr,
         *   [3] = HBlankIntr, [4] = VBlankIntr                           */
        gN64VBlankCount++;
        gN64CurrentLine = 0;
        _REG16(REG_OFFSET_VCOUNT)   = 0;
        _REG16(REG_OFFSET_DISPSTAT) |= DISPSTAT_VBLANK;

        N64_RtcVBlankTick();

        extern IntrFunc gIntrTable[];
        if (gIntrTable[4])
            gIntrTable[4]();   /* VBlankIntr */

        _REG16(REG_OFFSET_DISPSTAT) &= ~DISPSTAT_VBLANK;

        /* Fire VCount interrupt if the game configured it for line 160   */
        {
            u16 vCountLine = (_REG16(REG_OFFSET_DISPSTAT) >> 8) & 0xFF;
            if ((_REG16(REG_OFFSET_DISPSTAT) & DISPSTAT_VCOUNT_INTR)
                && vCountLine == 160)
            {
                _REG16(REG_OFFSET_DISPSTAT) |= DISPSTAT_VCOUNT_MATCH;
                if (gIntrTable[0])
                    gIntrTable[0]();   /* VCountIntr */
                _REG16(REG_OFFSET_DISPSTAT) &= ~DISPSTAT_VCOUNT_MATCH;
                INTR_CHECK |= INTR_FLAG_VCOUNT;
                gMain.intrCheck |= INTR_FLAG_VCOUNT;
            }
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
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_PI) {
        extern void N64_PiDmaDone(void);
        N64_PiDmaDone();

        /* Acknowledge PI */
        N64_HW_WR(N64_PI_BASE_REG, PI_STATUS_REG, 2);   /* clear interrupt */
    }

    /* ------------------------------------------------------------------
     * SP interrupt — RSP task done (unused for this port)
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_SP) {
        /* Acknowledge SP */
        N64_HW_WR(N64_SP_BASE_REG, 0x10, 1);   /* SP_STATUS: clear halt */
    }

    /* ------------------------------------------------------------------
     * DP interrupt — RDP done (unused for this port)
     * ------------------------------------------------------------------ */
    if (miIntr & MI_INTR_DP) {
        N64_HW_WR(N64_DP_BASE_REG, 0x0C, 0);   /* DPC_STATUS: ack        */
    }
}
