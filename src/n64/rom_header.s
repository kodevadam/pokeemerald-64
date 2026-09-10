/*
 * src/n64/rom_header.s
 *
 * N64 ROM header — 64 bytes, big-endian
 *
 * Layout (from N64 hardware documentation):
 *   0x00-0x03: PI BSD Domain 1 register (initial PI cart timing)
 *   0x04-0x07: Clock rate override (0 = use default)
 *   0x08-0x0B: Entry point address (RDRAM virtual address)
 *   0x0C-0x0F: Release offset / OS version
 *   0x10-0x13: CRC1 (filled by chksum64 tool after build)
 *   0x14-0x17: CRC2 (filled by chksum64 tool after build)
 *   0x18-0x1F: Reserved (zero)
 *   0x20-0x33: Game title (ASCII, 20 bytes, padded with spaces)
 *   0x34-0x37: Reserved
 *   0x38-0x3A: Game ID (4-char code, e.g. "BPEE")
 *   0x3B:      Destination code ('E' = NTSC North America)
 *   0x3C:      Cart ID
 *   0x3D:      Reserved
 *   0x3E:      ROM version (0)
 *   0x3F:      IPL3 complement (set by chksum64)
 *
 * The 4032-byte IPL3 boot code (bytes 0x40-0xFFF) must come from a
 * licensed IPL3 binary (e.g. from CIC-NUS-6102).  Since we cannot
 * distribute the IPL3 binary, we reserve the space with zeros here.
 * The build system should inject the IPL3 binary before CRC calculation.
 *
 * Entry point: 0x80000400 = start of __n64_boot in RDRAM (KSEG0)
 */

    .section .n64header, "a"
    .set noreorder

    /* PI BSD Domain 1 register values */
    .byte   0x80        /* PI_BSD_DOM1_LAT_REG */
    .byte   0x37        /* PI_BSD_DOM1_PWD_REG */
    .byte   0x12        /* PI_BSD_DOM1_PGS_REG */
    .byte   0x40        /* PI_BSD_DOM1_RLS_REG */

    /* Clock rate (0 = default) — must be 0x00000000 for libdragon IPL3 */
    .byte   0x00, 0x00, 0x00, 0x00

    /* Entry point / load address in RDRAM (KSEG0 0x80400000) — big-endian.
     * libdragon's compat IPL3 reads this single field for BOTH the DMA
     * destination and the jump target, so .boot's VMA in n64.ld must match. */
    .byte   0x80, 0x40, 0x00, 0x00

    /* Release / OS version */
    .byte   0x00, 0x00, 0x00, 0x00

    /* CRC1 — placeholder (filled by chksum64) */
    .byte   0x00, 0x00, 0x00, 0x00

    /* CRC2 — placeholder (filled by chksum64) */
    .byte   0x00, 0x00, 0x00, 0x00

    /* Reserved */
    .byte   0x00, 0x00, 0x00, 0x00
    .byte   0x00, 0x00, 0x00, 0x00

    /* Game title: "POKEMON EMERALD64" padded to 20 bytes */
    .ascii  "POKEMON EMERALD64   "

    /* Reserved (4 bytes) */
    .byte   0x00, 0x00, 0x00, 0x00

    /* Game ID: "BPEN" (B=GBA era, P=Pokemon, E=Emerald, N=N64 port) */
    .ascii  "BPEN"

    /* Destination code: 'E' = North America */
    .byte   0x45

    /* Cart ID */
    .byte   0x00

    /* Reserved */
    .byte   0x00

    /* ROM version */
    .byte   0x00

    /* IPL3 boot code region — 4032 bytes, filled by tools/patch_ipl3.py.
     * Leave as zeros here; the build injects libdragon's IPL3 after linking.
     *
     * This .space ends the header at exactly 0x1000 bytes, so the .boot
     * section's LMA is ROM[0x1000] — which is where IPL3 starts reading.
     *
     * The actual libdragon compat IPL3 protocol (boot/loader_compat.c):
     *   entrypoint = ROM[0x08]          (header entry point field, above)
     *   size       = ROM[0x10]          (the CRC1 field)
     *   if size == 0 or size is too large to fit below the reserved area
     *       at the top of RDRAM, size defaults to 1 MiB
     *   DMA ROM[0x1000 .. 0x1000+size] -> entrypoint, then jump to entrypoint
     *
     * Because ROM[0x10] holds CRC1 (patched by tools/n64crc), the size check
     * fails and IPL3 falls back to DMA'ing 1 MiB.  So the 1 MiB window at
     * the entry point must be free: 0x80400000..0x80500000 sits above
     * __bss_end (0x80332030) and below __sw_palette_start (0x80790000), and
     * crt0's stack grows down from 0x803A0000 (away from it).
     *
     * .boot must NOT be placed in the top 32 KiB of RDRAM
     * (0x807F8000..0x80800000): libdragon reserves that for its own loader
     * and first-stage stack (LOADER_SIZE 28K + STACK1_SIZE 4K) and clears it
     * before jumping to the entry point.
     */
    .space  0x1000 - 0x40
    /* .boot section (crt0.s) follows immediately at ROM[0x1000]              */
