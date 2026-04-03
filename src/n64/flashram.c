/*
 * src/n64/flashram.c
 *
 * N64 port — 128 KB FlashRAM save driver
 *
 * Hardware: Macronix MX29L010 / compatible N64 FlashRAM chip
 *   Size:       1 Mbit = 128 KB (matches GBA Pokémon Emerald save exactly)
 *   Interface:  PI bus domain 2 (0x08000000)
 *   Sector:     128 bytes (64 sectors × 128 bytes = 8 KB erase block)
 *               Actually N64 FlashRAM erases in 128-byte sectors
 *   Write:      128-byte page program
 *
 * FlashRAM PI bus address: 0x08000000 (physical: 0x08000000)
 *
 * N64 FlashRAM command protocol:
 *   The FlashRAM chip is at PI cart address 0x08000000.
 *   Commands are sent by writing 32-bit values to specific offsets.
 *   The chip has a status register accessible via read.
 *
 * This driver implements:
 *   ReadFlash()         — PI DMA from FlashRAM to RDRAM
 *   ProgramFlashSector()— write a 128-byte sector
 *   EraseFlashSector()  — erase a 128-byte sector
 *   CheckForFlashMemory()— detect FlashRAM presence
 *
 * These functions replace the GBA flash driver stubs in src/agb_flash_*.c.
 * The game's save logic in src/save.c calls through these interfaces.
 *
 * FlashRAM command set (from N64 hardware documentation):
 *   0xD2000000  execute erase
 *   0xE1000000  set erase offset
 *   0xB4000000  set write offset
 *   0xA5000000  execute write
 *   0xF0000000  set read mode
 *   0x78000000  status read (read from 0x08000000)
 *
 * Reference: http://n64dev.org/n64cart.html and libdragon flashram.c
 */

#include <string.h>
#include "global.h"
#include "n64/asm_defs.h"
#include "n64/defines.h"
#include "gba/flash_internal.h"

/* -----------------------------------------------------------------------
 * PI bus register access
 * --------------------------------------------------------------------- */
#define PI_REG(off)  (*(volatile u32 *)(N64_PI_BASE_REG + (off)))

/* -----------------------------------------------------------------------
 * FlashRAM constants
 * --------------------------------------------------------------------- */
#define FLASHRAM_PI_ADDR        0x08000000UL  /* Cart domain 2 address    */
#define FLASHRAM_SIZE           0x20000       /* 128 KB                   */
#define FLASHRAM_SECTOR_SIZE    128           /* erase / write granularity */
#define FLASHRAM_NUM_SECTORS    (FLASHRAM_SIZE / FLASHRAM_SECTOR_SIZE)

/* FlashRAM command register (write-only, at PI cart address 0x08000000) */
#define FLASHRAM_CMD_ADDR       0xA8000000UL  /* uncached KSEG1 view      */

/* FlashRAM commands */
#define FLASHRAM_CMD_ERASE_EXEC  0xD2000000
#define FLASHRAM_CMD_ERASE_SET   0xE1000000
#define FLASHRAM_CMD_WRITE_SET   0xB4000000
#define FLASHRAM_CMD_WRITE_EXEC  0xA5000000
#define FLASHRAM_CMD_READ_MODE   0xF0000000
#define FLASHRAM_CMD_STATUS      0x78000000

/* FlashRAM status bits */
#define FLASHRAM_STATUS_ERASE_OK  (1 << 2)
#define FLASHRAM_STATUS_WRITE_OK  (1 << 4)

/* -----------------------------------------------------------------------
 * PI DMA helpers
 * --------------------------------------------------------------------- */
static void PiWaitDone(void)
{
    /* Wait for PI DMA busy and IO busy bits to clear */
    while (PI_REG(PI_STATUS_REG) & 3)
        ;
}

/* Read from FlashRAM (cart address) into RDRAM buffer via PI DMA */
static void PiReadToRdram(u32 cartAddr, void *rdramDst, u32 length)
{
    PiWaitDone();
    PI_REG(PI_DRAM_ADDR_REG) = (u32)((uintptr_t)rdramDst & 0x0FFFFFFF);
    PI_REG(PI_CART_ADDR_REG) = cartAddr;
    PI_REG(PI_RD_LEN_REG)    = length - 1;
    PiWaitDone();
}

/* Write from RDRAM buffer to FlashRAM write buffer (128-byte pages) */
static void PiWriteFromRdram(u32 cartAddr, const void *rdramSrc, u32 length)
{
    PiWaitDone();
    PI_REG(PI_DRAM_ADDR_REG) = (u32)((uintptr_t)rdramSrc & 0x0FFFFFFF);
    PI_REG(PI_CART_ADDR_REG) = cartAddr;
    PI_REG(PI_WR_LEN_REG)    = length - 1;
    PiWaitDone();
}

/* -----------------------------------------------------------------------
 * FlashRAM command write — write a 32-bit command word
 * --------------------------------------------------------------------- */
static void FlashRAMCmd(u32 cmd)
{
    *(volatile u32 *)FLASHRAM_CMD_ADDR = cmd;
}

/* -----------------------------------------------------------------------
 * FlashRAMReadStatus — put FlashRAM in status mode and read the status
 * --------------------------------------------------------------------- */
static u32 FlashRAMReadStatus(void)
{
    u32 status[2];  /* aligned 8-byte buffer for PI DMA */
    FlashRAMCmd(FLASHRAM_CMD_STATUS);
    PiReadToRdram(FLASHRAM_PI_ADDR, status, 8);
    FlashRAMCmd(FLASHRAM_CMD_READ_MODE);
    return status[0];
}

/* -----------------------------------------------------------------------
 * N64_InitFlashRAM — detect and initialise the FlashRAM chip
 * Sets gFlashMemoryPresent in save.c via CheckForFlashMemory() hook.
 * --------------------------------------------------------------------- */
static int sFlashRAMPresent = 0;

void N64_InitFlashRAM(void)
{
    /* Attempt to read the FlashRAM status register.
     * If status bits are within expected range, the chip is present. */
    u32 status = FlashRAMReadStatus();

    /* Valid FlashRAM status: bits 25-24 indicate the chip type.
     * All valid states have bits 7-0 = 0 and bits 31-26 are type IDs.
     * If the read returns 0xFFFFFFFF (all ones), no chip present. */
    if (status != 0xFFFFFFFF && status != 0x00000000) {
        sFlashRAMPresent = 1;
    } else {
        /* Try legacy detection: read a known byte and see if it's not 0xFF */
        u8 probe[8] __attribute__((aligned(8)));
        PiReadToRdram(FLASHRAM_PI_ADDR, probe, 8);
        if (probe[0] != 0xFF || probe[1] != 0xFF)
            sFlashRAMPresent = 1;
    }
}

/* -----------------------------------------------------------------------
 * CheckForFlashMemory — called from AgbMain() (main.c)
 *
 * Sets gFlashMemoryPresent so that the game knows save is available.
 * --------------------------------------------------------------------- */
extern bool8 gFlashMemoryPresent;

void CheckForFlashMemory(void)
{
    gFlashMemoryPresent = sFlashRAMPresent ? TRUE : FALSE;
    if (!sFlashRAMPresent) {
        /* No FlashRAM: disable save functions gracefully */
        gFlashMemoryPresent = FALSE;
    }
}

/* -----------------------------------------------------------------------
 * ReadFlash — read 'size' bytes from flash offset 'src' into 'dst'
 *
 * Replaces the GBA ReadFlash() / ReadFlash16() functions.
 * The game calls this to load save data.
 * --------------------------------------------------------------------- */
u32 ReadFlash(u16 sectorNum, u32 offset, u8 *dst, u32 size)
{
    if (!sFlashRAMPresent)
        return size;  /* simulate read (data already cleared to 0) */

    u32 cartAddr = FLASHRAM_PI_ADDR
                 + (u32)sectorNum * FLASHRAM_SECTOR_SIZE
                 + offset;

    /* PI DMA requires 8-byte aligned destination and even length */
    if (((uintptr_t)dst & 7) || (size & 1)) {
        /* Unaligned: read into aligned bounce buffer */
        u8 alignedBuf[FLASHRAM_SECTOR_SIZE + 8] __attribute__((aligned(8)));
        u32 alignedSize = (size + 1) & ~1;
        PiReadToRdram(cartAddr & ~7, alignedBuf, alignedSize + 8);
        memcpy(dst, alignedBuf + (cartAddr & 7), size);
    } else {
        PiReadToRdram(cartAddr, dst, size);
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * ProgramFlashSector — write a 128-byte sector to FlashRAM
 *
 * N64 FlashRAM write sequence:
 *   1. Set write offset: CMD = 0xB4000000 | (sector & 0xFF)
 *   2. DMA 128 bytes from RDRAM to FlashRAM write buffer at 0x08000000
 *   3. Execute write: CMD = 0xA5000000
 *   4. Poll status until WRITE_OK
 * --------------------------------------------------------------------- */
u32 ProgramFlashSector(u16 sectorNum, const void *src)
{
    if (!sFlashRAMPresent)
        return 0;

    /* Ensure aligned source for PI DMA */
    u8 alignedBuf[FLASHRAM_SECTOR_SIZE] __attribute__((aligned(8)));
    memcpy(alignedBuf, src, FLASHRAM_SECTOR_SIZE);

    /* Step 1: Set write sector offset */
    FlashRAMCmd(FLASHRAM_CMD_WRITE_SET | (sectorNum & 0xFF));

    /* Step 2: DMA data into FlashRAM write buffer */
    PiWriteFromRdram(FLASHRAM_PI_ADDR, alignedBuf, FLASHRAM_SECTOR_SIZE);

    /* Step 3: Execute write */
    FlashRAMCmd(FLASHRAM_CMD_WRITE_EXEC | (sectorNum & 0xFF));

    /* Step 4: Poll for completion */
    u32 timeout = 100000;
    u32 status;
    do {
        status = FlashRAMReadStatus();
        if (!--timeout) return 1;  /* timeout error */
    } while (!(status & FLASHRAM_STATUS_WRITE_OK));

    return 0;  /* success */
}

/* -----------------------------------------------------------------------
 * EraseFlashSector — erase a 128-byte sector
 *
 * N64 FlashRAM erase sequence:
 *   1. Set erase offset: CMD = 0xE1000000 | (sector & 0xFF)
 *   2. Execute erase:    CMD = 0xD2000000
 *   3. Poll status until ERASE_OK
 * --------------------------------------------------------------------- */
u32 EraseFlashSector(u16 sectorNum)
{
    if (!sFlashRAMPresent)
        return 0;

    /* Step 1: Set erase sector */
    FlashRAMCmd(FLASHRAM_CMD_ERASE_SET | (sectorNum & 0xFF));

    /* Step 2: Execute erase */
    FlashRAMCmd(FLASHRAM_CMD_ERASE_EXEC);

    /* Step 3: Poll for completion */
    u32 timeout = 1000000;
    u32 status;
    do {
        status = FlashRAMReadStatus();
        if (!--timeout) return 1;  /* timeout error */
    } while (!(status & FLASHRAM_STATUS_ERASE_OK));

    return 0;  /* success */
}

/* -----------------------------------------------------------------------
 * EraseFlashChip — erase the entire 128 KB FlashRAM
 * --------------------------------------------------------------------- */
u32 EraseFlashChip(void)
{
    for (u16 s = 0; s < FLASHRAM_NUM_SECTORS; s++) {
        u32 result = EraseFlashSector(s);
        if (result != 0) return result;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * WriteFlash — write 'size' bytes to flash at sector 'sectorNum'
 *
 * The game calls this through the flash chip driver interface.
 * We break the write into 128-byte sector pages.
 * --------------------------------------------------------------------- */
u32 WriteFlash(u16 sectorNum, u32 offset, const u8 *src, u32 size)
{
    if (!sFlashRAMPresent)
        return 0;

    /* Read-modify-write if not on a sector boundary */
    u8 sectorBuf[FLASHRAM_SECTOR_SIZE] __attribute__((aligned(8)));
    u32 written = 0;

    while (written < size) {
        u16 curSector = sectorNum + (u16)((offset + written) / FLASHRAM_SECTOR_SIZE);
        u32 secOffset = (offset + written) % FLASHRAM_SECTOR_SIZE;
        u32 toWrite   = FLASHRAM_SECTOR_SIZE - secOffset;
        if (toWrite > size - written) toWrite = size - written;

        /* Read existing sector data */
        ReadFlash(curSector, 0, sectorBuf, FLASHRAM_SECTOR_SIZE);

        /* Modify */
        memcpy(sectorBuf + secOffset, src + written, toWrite);

        /* Erase then program */
        EraseFlashSector(curSector);
        ProgramFlashSector(curSector, sectorBuf);

        written += toWrite;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * N64_PiDmaDone — PI interrupt callback (registered in interrupt.c)
 * Called when a PI DMA transfer completes.  For FlashRAM, we just
 * acknowledge; no special post-processing is needed.
 * --------------------------------------------------------------------- */
void N64_PiDmaDone(void)
{
    /* Acknowledge PI interrupt — done in interrupt.c */
}

/* -----------------------------------------------------------------------
 * GBA flash driver compatibility stubs
 *
 * The game code in save.c calls the GBA flash driver through a function
 * pointer table (gFlashFuncTable) defined in agb_flash.c / agb_flash_mx.c.
 * We provide a minimal compatibility shim here so the save system works.
 *
 * Note: agb_flash.c is excluded from the N64 build (see Makefile.n64).
 * We re-export the symbols it would have defined.
 * --------------------------------------------------------------------- */

/* Flash "type" descriptor — the game uses this to select flash routines.
 * We return a synthetic descriptor that causes the game to call our
 * functions above. */
const struct FlashType gFlashType = {
    .wait      = {0, 0},
    .maxTime   = {10000, 10000, 200000},
};

/* VerifyFlashSector — compare written data against source */
u32 VerifyFlashSector(u16 sectorNum, const u8 *src, u32 size)
{
    u8 verifyBuf[FLASHRAM_SECTOR_SIZE] __attribute__((aligned(8)));
    for (u32 off = 0; off < size; off += FLASHRAM_SECTOR_SIZE) {
        u32 chunkSize = size - off;
        if (chunkSize > FLASHRAM_SECTOR_SIZE) chunkSize = FLASHRAM_SECTOR_SIZE;
        ReadFlash(sectorNum, off, verifyBuf, chunkSize);
        if (memcmp(verifyBuf, src + off, chunkSize) != 0)
            return 1;  /* mismatch */
    }
    return 0;  /* match */
}

/* VerifyFlashSectorNBytes — verify N bytes starting at sector offset */
u32 VerifyFlashSectorNBytes(u16 sectorNum, const u8 *src, u32 offset, u32 size)
{
    u8 verifyBuf[FLASHRAM_SECTOR_SIZE] __attribute__((aligned(8)));
    ReadFlash(sectorNum, offset, verifyBuf, size);
    return memcmp(verifyBuf, src, size) != 0 ? 1 : 0;
}
