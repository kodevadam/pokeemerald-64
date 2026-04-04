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

    /* Entry point in RDRAM (KSEG0 0x80000400) — big-endian for IPL3 */
    .byte   0x80, 0x00, 0x04, 0x00

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

    /* IPL3 boot code region — 4032 bytes, must be filled externally */
    /* Leave as zeros; inject IPL3 before building final ROM */
    .space  0x1000 - 0x40

    /* Exception vector area — 0x400 bytes of zero padding.
     * IPL3 copies ROM[0x1000..0x1400) → RDRAM[0x80000000..0x80000400).
     * crt0.s __n64_boot runs from RDRAM 0x80000400 and installs
     * the real exception vectors at RDRAM 0x80000000-0x800001FF. */
    .space  0x400
