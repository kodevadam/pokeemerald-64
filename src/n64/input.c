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

#include <string.h>
#include "global.h"
#include "n64/asm_defs.h"

/* -----------------------------------------------------------------------
 * N64 SI / PIF register access — byte-swap wrappers for big-endian MMIO
 * --------------------------------------------------------------------- */
#define SI_REG_WR(off, val) N64_HW_WR(N64_SI_BASE_REG, (off), (val))
#define SI_REG_RD(off)      N64_HW_RD(N64_SI_BASE_REG, (off))
#define N64_PIF_RAM  ((volatile u8 *)0xBFC007C0)   /* PIF-RAM (64 bytes) */

/* -----------------------------------------------------------------------
 * PIF command buffer — 64 bytes
 * Layout: controller command (4 bytes) + padding, terminated by 0xFE.
 * The SI copies this to PIF-RAM and executes the commands.
 *
 * sPifRsp is accessed via an uncached KSEG1 alias to avoid stale cache
 * entries after the SI DMA writes new button data to RDRAM.            */
static u8 sPifCmd[64] __attribute__((aligned(64)));
static u8 sPifRsp[64] __attribute__((aligned(64)));
/* KSEG1 (uncached) alias: physical = virt & 0x1FFFFFFF, KSEG1 = | 0xA0000000 */
#define UNCACHED(p) ((volatile u8 *)((uintptr_t)(p) | 0x20000000u))

/* Controller data from the last completed read */
static volatile u16 sN64Buttons = 0;
static volatile s8  sAnalogX    = 0;
static volatile s8  sAnalogY    = 0;
static volatile int sReadPending = 0;

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
    sReadPending = 0;

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
    memset(sPifCmd, 0, sizeof(sPifCmd));
    sPifCmd[0]  = 0x01;  /* send count: 1 byte    */
    sPifCmd[1]  = 0x04;  /* recv count: 4 bytes   */
    sPifCmd[2]  = 0x01;  /* command: GetKeysAsync */
    /* bytes 3-6 will be filled by PIF with response */
    sPifCmd[7]  = 0xFE;  /* end-of-commands marker */
    sPifCmd[63] = 0x01;  /* PIF control: read mode */

    N64_InputStartRead();
}

/* -----------------------------------------------------------------------
 * N64_InputStartRead — initiate an SI DMA read from PIF-RAM
 * --------------------------------------------------------------------- */

/* Byte-safe copy for PIF-RAM (big-endian peripheral).
 * The CPU is little-endian (-EL); unaligned or word-level writes to
 * 0xBFC007C0 would byte-swap within words.  Use byte-by-byte volatile
 * writes to ensure the bytes land in the correct order.               */
static inline void pif_write(const u8 *src, int len)
{
    volatile u8 *pif = N64_PIF_RAM;
    for (int i = 0; i < len; i++)
        pif[i] = src[i];
}

static inline void pif_read(u8 *dst, int len)
{
    volatile const u8 *pif = N64_PIF_RAM;
    for (int i = 0; i < len; i++)
        dst[i] = pif[i];
}

void N64_InputStartRead(void)
{
    if (sReadPending)
        return;

    /* Write command block to PIF-RAM byte-by-byte */
    pif_write(sPifCmd, 64);

    /* Start SI DMA: PIF-RAM → sPifRsp (in RDRAM).
     * Physical RDRAM address = KSEG0 virtual & 0x1FFFFFFF.             */
    SI_REG_WR(SI_DRAM_ADDR_REG,  (u32)((uintptr_t)sPifRsp & 0x1FFFFFFF));
    SI_REG_WR(SI_PIF_ADDR_RD64B, 0x1FC007C0); /* PIF-RAM physical address */

    sReadPending = 1;
}

/* -----------------------------------------------------------------------
 * N64_ControllerReadDone — called from interrupt.c on SI interrupt
 *
 * Parses the 4-byte response from PIF-RAM and updates REG_KEYINPUT.
 * --------------------------------------------------------------------- */
void N64_ControllerReadDone(void)
{
    if (!sReadPending)
        return;
    sReadPending = 0;

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

    /* Kick off the next controller read for the next frame */
    N64_InputStartRead();
}
