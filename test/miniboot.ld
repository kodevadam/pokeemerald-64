/*
 * test/miniboot.ld — Linker script for the standalone miniboot test ROM
 *
 * Produces a flat binary with this layout:
 *   offset 0x0000  .n64hdr    (64 bytes  — N64 ROM header)
 *   offset 0x0040  .ipl3slot  (4032 bytes — zeroed, patched post-link)
 *   offset 0x1000  .boothdr   (72 bytes — boot size word at 0x1040)
 *   offset 0x1048  .boot      (boot code  — DMA'd to RDRAM[RDRAM_SIZE-0x1000])
 */

OUTPUT_ARCH(mips)
ENTRY(_start)

MEMORY
{
    ROM (rx) : ORIGIN = 0xB0000000, LENGTH = 64M
}

SECTIONS
{
    .n64hdr  0xB0000000 : { *(.n64hdr)  } > ROM
    .ipl3slot           : { *(.ipl3slot)} > ROM
    .boothdr 0xB0001000 : { *(.boothdr) } > ROM
    .boot    0xB0001048 : { *(.boot)    } > ROM

    /DISCARD/ :
    {
        *(.MIPS.abiflags)
        *(.reginfo)
        *(.pdr)
        *(.comment)
    }
}
