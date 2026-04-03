/*
 * n64.ld — Nintendo 64 linker script for pokeemerald-64
 *
 * N64 memory map:
 *   0x80000000 - 0x80400000  RDRAM (4MB standard)
 *   0x80400000 - 0x80800000  RDRAM expansion pak (4MB, optional)
 *   0xA0000000 - ...         Uncached mirrors of RDRAM
 *   0xB0000000 - 0xBFFFFFFF  ROM via PI bus (cart)
 *
 * ROM is accessed at 0xB0000000 (uncached) or 0x90000000 (cached).
 * The N64 boot process copies the first 1MB of ROM to RDRAM via PI DMA,
 * then jumps to the entry point in RDRAM.
 *
 * We use the libdragon convention:
 *   - Code/data live in RDRAM at 0x80000400 (after the 1KB exception vectors)
 *   - ROM image is built as a flat binary starting at offset 0x1000 (after
 *     the 4KB ROM header / IPL3 boot code region)
 */

OUTPUT_ARCH(mips)
ENTRY(__n64_boot)

MEMORY
{
    /* RDRAM — 4MB base. We reserve the low 1KB for N64 exception vectors.     */
    /* The "seg" address is what the CPU sees at runtime (cached KSEG0).       */
    RDRAM  (rwx) : ORIGIN = 0x80000400, LENGTH = 4M - 0x400

    /* ROM image as seen through the PI bus (used for LMA / load addresses).  */
    /* Offset 0x1000 = after the 4KB header+IPL3 region.                      */
    ROM    (rx)  : ORIGIN = 0xB0001000, LENGTH = 64M - 0x1000
}

/*
 * Software-emulated GBA VRAM, OAM and Palette RAM.
 * These live in normal RDRAM — the software compositor reads them directly.
 * We keep them at fixed well-known addresses so that GBA-style pointer casts
 * (e.g. (u16*)PLTT) still compile; we simply redefine the constants to point
 * here instead of to GBA MMIO space.
 *
 * Layout (within the RDRAM segment, after BSS):
 *   PLTT_BUF  : 1024 bytes  (512 BG + 512 OBJ palette, RGB555)
 *   VRAM_BUF  : 98304 bytes (96KB tile + screen data)
 *   OAM_BUF   : 1024 bytes  (128 × 8-byte OAM entries)
 *   SW_IOREGS : 4096 bytes  (software-state I/O register file)
 */

SECTIONS
{
    /* ------------------------------------------------------------------ */
    /*  Boot stub — must be first in ROM so IPL3 jumps to it              */
    /* ------------------------------------------------------------------ */
    .boot :
    {
        KEEP(*(.boot))
        KEEP(*(.boot.*))
    } > RDRAM AT > ROM

    /* ------------------------------------------------------------------ */
    /*  Exception vector area (0x80000000 – 0x80000400)                   */
    /*  These are placed by libdragon's crt0; we just reserve space.       */
    /* ------------------------------------------------------------------ */

    /* ------------------------------------------------------------------ */
    /*  Read-only data / code                                              */
    /* ------------------------------------------------------------------ */
    .text :
    {
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        KEEP(*(.text.*))
        . = ALIGN(8);
    } > RDRAM AT > ROM

    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(8);
    } > RDRAM AT > ROM

    /* GBA ROM data (tile graphics, maps, scripts etc.) are linked here   */
    .rodata.gba :
    {
        *(.rodata.gba .rodata.gba.*)
        . = ALIGN(8);
    } > RDRAM AT > ROM

    /* ------------------------------------------------------------------ */
    /*  Initialised data                                                   */
    /* ------------------------------------------------------------------ */
    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN(8);
    } > RDRAM AT > ROM

    /* GBA "EWRAM data" → maps to normal RDRAM                            */
    .ewram_data :
    {
        *(.ewram_data)
        . = ALIGN(8);
    } > RDRAM AT > ROM

    /* GBA "IWRAM data" → also maps to RDRAM (no speed penalty on N64)    */
    .iwram_data :
    {
        *(.iwram_data)
        . = ALIGN(8);
    } > RDRAM AT > ROM

    /* GBA "common data" section                                           */
    .common_data :
    {
        *(.common_data)
        . = ALIGN(8);
    } > RDRAM AT > ROM

    /* ------------------------------------------------------------------ */
    /*  BSS (zero-initialised)                                             */
    /* ------------------------------------------------------------------ */
    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    } > RDRAM

    /* ------------------------------------------------------------------ */
    /*  Software GBA hardware buffers (all NOLOAD — zeroed at boot)        */
    /* ------------------------------------------------------------------ */

    .sw_palette (NOLOAD) :
    {
        __sw_palette_start = .;
        . += 0x400;   /* 1024 bytes = 512 BG + 512 OBJ palette entries    */
        __sw_palette_end = .;
    } > RDRAM

    .sw_vram (NOLOAD) :
    {
        __sw_vram_start = .;
        . += 0x18000; /* 96KB  = BG tile + screen data + OBJ tile area    */
        __sw_vram_end = .;
    } > RDRAM

    .sw_oam (NOLOAD) :
    {
        __sw_oam_start = .;
        . += 0x400;   /* 1024 bytes = 128 sprites × 8 bytes               */
        __sw_oam_end = .;
    } > RDRAM

    .sw_ioregs (NOLOAD) :
    {
        __sw_ioregs_start = .;
        . += 0x1000;  /* 4096 bytes of software I/O register state         */
        __sw_ioregs_end = .;
    } > RDRAM

    /* ------------------------------------------------------------------ */
    /*  Heap (grows upward from here; top is set in platform.c)           */
    /* ------------------------------------------------------------------ */
    .heap (NOLOAD) :
    {
        __heap_start = .;
        . = ALIGN(8);
    } > RDRAM

    /* ------------------------------------------------------------------ */
    /*  Double-buffered framebuffers at the TOP of RDRAM                   */
    /*  320×240×2 bytes = 153600 bytes ≈ 150 KB each                       */
    /* ------------------------------------------------------------------ */
    .framebuffers (NOLOAD) :
    {
        . = ALIGN(64);
        __fb0_start = .;
        . += 320 * 240 * 2;   /* framebuffer 0                            */
        . = ALIGN(64);
        __fb1_start = .;
        . += 320 * 240 * 2;   /* framebuffer 1                            */
        . = ALIGN(64);
    } > RDRAM

    /* ------------------------------------------------------------------ */
    /*  Symbols for the runtime                                            */
    /* ------------------------------------------------------------------ */
    __rdram_start    = ORIGIN(RDRAM);
    __rdram_size     = LENGTH(RDRAM);
    __rom_start      = ORIGIN(ROM);

    /* Stack — 64KB, placed just below 4MB RDRAM limit                    */
    __stack_top      = ORIGIN(RDRAM) + LENGTH(RDRAM);
    __stack_size     = 0x10000;   /* 64KB                                 */
    __stack_bottom   = __stack_top - __stack_size;

    /DISCARD/ :
    {
        *(.MIPS.abiflags)
        *(.reginfo)
        *(.pdr)
        *(.mdebug*)
        *(.comment)
        *(.note.*)
    }
}
