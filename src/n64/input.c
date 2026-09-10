/*
 * src/n64/input.c
 *
 * N64 port — controller input → GBA key shim
 *
 * The GBA has 10 buttons mapped to REG_KEYINPUT (active-low bitmask).
 * The N64 controller is read via the Serial Interface (SI) using PIF
 * commands.  This module:
 *   1. Initiates a controller read each VBlank.
 *   2. When the SI read completes (SI interrupt), unpacks the button state.
 *   3. Writes the result into REG_KEYINPUT in the software IO register
 *      file (active-LOW, matching GBA convention) so that the game's
 *      ReadKeys() function in main.c works without modification.
 *
 * Button mapping (per the port plan):
 *   GBA A      → N64 A
 *   GBA B      → N64 B
 *   GBA L      → N64 L
 *   GBA R      → N64 R
 *   GBA Start  → N64 Start
 *   GBA Select → N64 Z
 *   GBA D-pad  → N64 D-pad (analog stick as fallback with threshold)
 *
 * REG_KEYINPUT GBA bit layout (active-low):
 *   bit 0: A
 *   bit 1: B
 *   bit 2: Select
 *   bit 3: Start
 *   bit 4: Right
 *   bit 5: Left
 *   bit 6: Up
 *   bit 7: Down
 *   bit 8: R
 *   bit 9: L
 *
 * N64 controller button word (16-bit, big-endian):
 *   bit 15: A
 *   bit 14: B
 *   bit 13: Z
 *   bit 12: Start
 *   bit 11: D-Up
 *   bit 10: D-Down
 *   bit  9: D-Left
 *   bit  8: D-Right
 *   bit  7: (not used / L trigger digital)
 *   bit  6: (not used / R trigger digital)
 *   bit  5: (not used)
 *   bit  4: (not used)
 *   bit  3: C-Up
 *   bit  2: C-Down
 *   bit  1: C-Left
 *   bit  0: C-Right
 *
 * Analog stick: bytes 2 (X, -128..127) and 3 (Y, -128..127)
 */

#include "global.h"
#include "n64/asm_defs.h"

/* Forward declaration — defined later in this file */
void N64_InputStartRead(void);

/* -----------------------------------------------------------------------
 * N64 SI / PIF register access — byte-swap wrappers for big-endian MMIO
 * --------------------------------------------------------------------- */
#define SI_REG_WR(off, val) N64_HW_WR(N64_SI_BASE_REG, (off), (val))
#define SI_REG_RD(off)      N64_HW_RD(N64_SI_BASE_REG, (off))
#define PIF_RAM_PHYS_ADDR   0x1FC007C0u   /* PIF-RAM physical address (64 bytes) */

/* -----------------------------------------------------------------------
 * PIF command / response buffers — 64 bytes each, DMA'd between RDRAM and
 * PIF-RAM by the SI.  Real N64 hardware/libultra protocol requires the
 * command block to reach PIF-RAM via an SI *write* DMA (RDRAM -> PIF-RAM)
 * before an SI *read* DMA (PIF-RAM -> RDRAM) can retrieve a valid
 * response: the write DMA is what triggers the PIF to parse the command
 * block (joyInit/joyParse in HLE terms) and populate per-channel state;
 * the read DMA runs the actual controller command (joyRun) using that
 * state.  A CPU store straight into PIF-RAM (bypassing the write DMA)
 * skips that parse step entirely, so the read DMA operates on stale/
 * uninitialized channel state and never reflects real button presses.
 *
 * Both buffers are accessed via the uncached KSEG1 alias so that CPU
 * writes/reads are immediately visible to/from the DMA engine without
 * needing an explicit cache writeback (same convention used for the VI
 * framebuffer in platform.c).                                          */
static u8 sPifCmd[64] __attribute__((aligned(64)));
static u8 sPifRsp[64] __attribute__((aligned(64)));
/* KSEG1 (uncached) alias: physical = virt & 0x1FFFFFFF, KSEG1 = | 0xA0000000 */
#define UNCACHED(p) ((volatile u8 *)((uintptr_t)(p) | 0x20000000u))

/* Controller data from the last completed read */
static volatile u16 sN64Buttons = 0;
static volatile s8  sAnalogX    = 0;
static volatile s8  sAnalogY    = 0;

/* SI transaction state machine — one controller poll spans two SI
 * interrupts (write DMA completion, then read DMA completion).        */
enum { SI_IDLE = 0, SI_WRITE_PENDING, SI_READ_PENDING };
static volatile int sSiState = SI_IDLE;

/* Analog stick dead zone (out of 127) */
#define ANALOG_THRESHOLD 32

/* -----------------------------------------------------------------------
 * N64_InitInput — set up the PIF and trigger the first controller poll
 * --------------------------------------------------------------------- */
void N64_InitInput(void)
{
    sN64Buttons = 0;
    sAnalogX    = 0;
    sAnalogY    = 0;
    sSiState    = SI_IDLE;

    /* Build the PIF command for a standard controller read on port 0:
     *   Byte 0: 0x01 — send 1 byte
     *   Byte 1: 0x03 — receive 4 bytes (status + 3 controller bytes? No,
     *                  for GetKeys command: send=1, recv=4)
     *   Actually the PIF GetKeys format is:
     *     [ff 01 03 00] = channel 0, send 1 byte (cmd 0x01 = GetKeysAsync)
     *                           recv 4 bytes (buttons hi, buttons lo, x, y)
     * Full PIF command block (64 bytes):
     *   0x00: ff (skip channel — not used here; some docs use fe)
     *   Actually the standard minimal PIF block for one controller:
     *     01 03 01 = controller info (recv 3 bytes device ID)
     *   For button read:
     *     01 04 01 = send 1 byte (cmd=0x01), recv 4 bytes, to port 0
     *
     * We use the libultra-compatible format:
     *   [01] [04] [01] [xx xx xx xx] [FE] [00 ... 00] [02]
     *    ↑    ↑    ↑    ↑↑↑↑          ↑
     *    ch0  rcv4 cmd  response buf  end of cmds
     */
    /* Written through the uncached alias so the bytes land in RDRAM
     * immediately -- the SI write DMA reads directly from physical
     * memory and would otherwise see whatever stale garbage was there
     * before this (cached) buffer's writes get evicted.               */
    volatile u8 *cmd = UNCACHED(sPifCmd);
    for (int i = 0; i < 64; i++)
        cmd[i] = 0;
    cmd[0]  = 0x01;  /* send count: 1 byte    */
    cmd[1]  = 0x04;  /* recv count: 4 bytes   */
    cmd[2]  = 0x01;  /* command: GetKeysAsync */
    /* bytes 3-6 will be filled by PIF with response */
    cmd[7]  = 0xFE;  /* end-of-commands marker */
    /* PIF control byte (offset 0x3F): bit 0 set tells the PIF to parse
     * this command block (joyInit+joyParse) when it arrives via an SI
     * write DMA -- this is the step our old direct-CPU-write approach
     * skipped entirely. */
    cmd[63] = 0x01;

    N64_InputStartRead();
}

/* -----------------------------------------------------------------------
 * N64_InputStartRead — initiate a controller poll
 *
 * Kicks off the *write* half of the SI transaction: DMA sPifCmd from
 * RDRAM into PIF-RAM.  This is what makes the PIF parse the command
 * block; the read half (started from N64_ControllerReadDone() once the
 * write DMA's SI interrupt arrives) then executes it.
 * --------------------------------------------------------------------- */
void N64_InputStartRead(void)
{
    if (sSiState != SI_IDLE)
        return;

    /* Physical RDRAM address = KSEG0 virtual & 0x1FFFFFFF. */
    SI_REG_WR(SI_DRAM_ADDR_REG,  (u32)((uintptr_t)sPifCmd & 0x1FFFFFFF));
    SI_REG_WR(SI_PIF_ADDR_WR64B, PIF_RAM_PHYS_ADDR);

    sSiState = SI_WRITE_PENDING;
}

/* -----------------------------------------------------------------------
 * N64_ControllerReadDone — called from interrupt.c on every SI interrupt
 *
 * One controller poll spans two SI interrupts:
 *   1. Write DMA (sPifCmd -> PIF-RAM) completes -> PIF has now parsed the
 *      command block.  Immediately follow up with the read DMA
 *      (PIF-RAM -> sPifRsp) that actually executes it.
 *   2. Read DMA completes -> sPifRsp holds the real response; parse it
 *      and update REG_KEYINPUT.
 * --------------------------------------------------------------------- */
void N64_ControllerReadDone(void)
{
    if (sSiState == SI_WRITE_PENDING) {
        SI_REG_WR(SI_DRAM_ADDR_REG,  (u32)((uintptr_t)sPifRsp & 0x1FFFFFFF));
        SI_REG_WR(SI_PIF_ADDR_RD64B, PIF_RAM_PHYS_ADDR);
        sSiState = SI_READ_PENDING;
        return;
    }

    if (sSiState != SI_READ_PENDING)
        return;
    sSiState = SI_IDLE;

    /* Response layout (from PIF GetKeys command):
     *   sPifRsp[3] = buttons high byte
     *   sPifRsp[4] = buttons low byte
     *   sPifRsp[5] = analog X (signed)
     *   sPifRsp[6] = analog Y (signed)
     * Read via uncached KSEG1 alias to bypass stale cache lines.       */
    volatile u8 *rsp = UNCACHED(sPifRsp);
    u16 buttons = ((u16)rsp[3] << 8) | (u16)rsp[4];
    sN64Buttons = buttons;
    sAnalogX    = (s8)rsp[5];
    sAnalogY    = (s8)rsp[6];

    /* ------------------------------------------------------------------
     * Build GBA-style KEYINPUT (active-LOW: pressed = bit clear)
     * ------------------------------------------------------------------ */
    u16 gbaKeys = 0x03FF;  /* start with all bits set (nothing pressed) */

    /* A button */
    if (buttons & (1 << 15)) gbaKeys &= ~(1 << 0);
    /* B button */
    if (buttons & (1 << 14)) gbaKeys &= ~(1 << 1);
    /* Select = Z button */
    if (buttons & (1 << 13)) gbaKeys &= ~(1 << 2);
    /* Start */
    if (buttons & (1 << 12)) gbaKeys &= ~(1 << 3);
    /* D-pad Right */
    if (buttons & (1 <<  8)) gbaKeys &= ~(1 << 4);
    /* D-pad Left */
    if (buttons & (1 <<  9)) gbaKeys &= ~(1 << 5);
    /* D-pad Up */
    if (buttons & (1 << 11)) gbaKeys &= ~(1 << 6);
    /* D-pad Down */
    if (buttons & (1 << 10)) gbaKeys &= ~(1 << 7);
    /* R trigger */
    if (buttons & (1 <<  6)) gbaKeys &= ~(1 << 8);
    /* L trigger */
    if (buttons & (1 <<  7)) gbaKeys &= ~(1 << 9);

    /* Analog stick fallback: apply digital D-pad if stick exceeds threshold */
    if (sAnalogX >  ANALOG_THRESHOLD) gbaKeys &= ~(1 << 4);  /* Right */
    if (sAnalogX < -ANALOG_THRESHOLD) gbaKeys &= ~(1 << 5);  /* Left  */
    if (sAnalogY >  ANALOG_THRESHOLD) gbaKeys &= ~(1 << 6);  /* Up    */
    if (sAnalogY < -ANALOG_THRESHOLD) gbaKeys &= ~(1 << 7);  /* Down  */

    /* Write to software REG_KEYINPUT */
    _REG16(REG_OFFSET_KEYINPUT) = gbaKeys;

    /* Do NOT start the next read here — that would immediately trigger
     * another SI interrupt, causing an infinite tight interrupt loop that
     * starves N64Main.  The VI handler calls N64_InputStartRead() once
     * per VBlank so the controller read rate is locked to 60 Hz.        */
}
