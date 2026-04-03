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
    /* RDRAM — 4MB base + 4MB expansion pak = 8MB total.
     * We reserve the low 1KB for N64 exception vectors.
     * Code and writable data live here; large rodata stays in ROM.           */
    RDRAM  (rwx) : ORIGIN = 0x80000400, LENGTH = 8M - 0x400

    /* ROM image: the full cartridge space via KSEG1 uncached (0xB0000000+).
     * Sections with `AT > ROM` store their bytes here but execute/load from
     * their VMA (RDRAM for code/data, or ROM itself for read-only data).
     * The 4KB at 0xB0000000 is the N64 header; game sections start at +0x1000 */
    ROM    (rx)  : ORIGIN = 0xB0000000, LENGTH = 64M
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
    /*  N64 ROM header — lives at ROM byte 0 (LMA = 0xB0000000).         */
    /*  VMA is also ROM since the header is never copied to RDRAM.        */
    /* ------------------------------------------------------------------ */
    .n64header 0xB0000000 :
    {
        KEEP(*(.n64header))
        KEEP(*(.n64header.*))
    } > ROM

    /* ------------------------------------------------------------------ */
    /*  Boot / entry stub — runs in RDRAM at 0x80000400.                  */
    /*  IPL3 copies ROM[0x1000..] → RDRAM[0x80000000..].                  */
    /*  The .n64header section includes a 0x400-byte zero pad after the   */
    /*  4KB IPL3 region so crt0.s code lands at RDRAM 0x80000400.         */
    /* ------------------------------------------------------------------ */
    .boot 0x80000400 :
    {
        KEEP(*(.boot))
        KEEP(*(.boot.*))
    } > RDRAM AT > ROM

    /* ------------------------------------------------------------------ */
    /*  Code and writable data: VMA in RDRAM, LMA in ROM (copied at boot) */
    /* ------------------------------------------------------------------ */
    .text :
    {
        __text_start = .;
        *(.text.unlikely .text.*_unlikely .text.unlikely.*)
        *(.text.exit .text.exit.*)
        *(.text.startup .text.startup.*)
        *(.text.hot .text.hot.*)
        *(.text .stub .text.* .gnu.linkonce.t.*)
        KEEP(*(.text.*))
        . = ALIGN(8);
        __text_end = .;
    } > RDRAM AT > ROM

    /* LMA of the text section (ROM address to copy from at boot) */
    __text_lma = LOADADDR(.text);

    /* ------------------------------------------------------------------ */
    /*  Read-only game data: stays in ROM, accessed via KSEG1 PI bus.     */
    /*  VMA = LMA = ROM address (0xB0001000+), no RDRAM copy needed.      */
    /*  The N64 CPU reads from 0xB0000000+ (KSEG1 uncached) or           */
    /*  0x90000000+ (KSEG0 cached) — both map the same cartridge ROM.     */
    /* ------------------------------------------------------------------ */
    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(8);
    } > ROM

    /* GBA ROM data (tile graphics, maps, scripts etc.) */
    .rodata.gba :
    {
        *(.rodata.gba .rodata.gba.*)
        . = ALIGN(8);
    } > ROM

    /* GBA script/event/text data from data/*.s
     * NOTE: input sections are named "script_data" (no leading dot),
     * so we capture both forms: with and without the leading dot.      */
    .script_data :
    {
        *(script_data script_data.*)
        *(.script_data .script_data.*)
        . = ALIGN(8);
    } > ROM

    /* ------------------------------------------------------------------ */
    /*  Initialised data                                                   */
    /* ------------------------------------------------------------------ */
    .data :
    {
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN(8);
    } > RDRAM AT > ROM

    /* LMA of the first data section (ROM address to copy from at boot)   */
    __data_lma = LOADADDR(.data);

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
        __data_end = .;
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

    /* Software buffers at fixed addresses so BG_VRAM/PLTT are compile-time constants */
    /* ------------------------------------------------------------------ */
    /*  Heap (grows upward from BSS end; bounded by stack bottom)         */
    /* ------------------------------------------------------------------ */
    .heap (NOLOAD) :
    {
        __heap_start = .;
        . = ALIGN(8);
    } > RDRAM

    /* ------------------------------------------------------------------ */
    /*  Upper RDRAM fixed layout (expansion pak required for 8MB).        */
    /*                                                                     */
    /*  0x807B0000 - 0x807FB000: Double-buffered N64 framebuffers (2×150KB)*/
    /*  0x807B0000 - 0x807D5800: FB0 (320×240 RGBA5551)                  */
    /*  0x807D6000 - 0x807FB800: FB1 (320×240 RGBA5551)                  */
    /*                                                                     */
    /*  0x80790000 - 0x807AC000: Software GBA hardware buffers           */
    /*  0x80790000: PLTT  (1024 bytes)                                    */
    /*  0x80791000: VRAM  (96KB = 0x18000)                                */
    /*  0x807A9000: OAM   (1024 bytes)                                    */
    /*  0x807A9400: IOREGS(4096 bytes)                                    */
    /* ------------------------------------------------------------------ */

    .sw_palette 0x80790000 (NOLOAD) :
    {
        __sw_palette_start = .;
        . += 0x400;   /* 1024 bytes = 512 BG + 512 OBJ palette entries    */
        __sw_palette_end = .;
    } > RDRAM

    .sw_vram 0x80791000 (NOLOAD) :
    {
        __sw_vram_start = .;
        . += 0x18000; /* 96KB  = BG tile + screen data + OBJ tile area    */
        __sw_vram_end = .;
    } > RDRAM

    .sw_oam 0x807A9000 (NOLOAD) :
    {
        __sw_oam_start = .;
        . += 0x400;   /* 1024 bytes = 128 sprites × 8 bytes               */
        __sw_oam_end = .;
    } > RDRAM

    .sw_ioregs 0x807A9400 (NOLOAD) :
    {
        __sw_ioregs_start = .;
        . += 0x1000;  /* 4096 bytes of software I/O register state         */
        __sw_ioregs_end = .;
    } > RDRAM

    /* N64 scaled-up framebuffers (320×240, 2 bytes/pixel = 153600 bytes each) */
    .framebuffers 0x807B0000 (NOLOAD) :
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

    /* _gp: MIPS global pointer — unused (G0 means no GP-relative addressing)
     * but crt0 loads it; define it as 0 to satisfy the reference. */
    PROVIDE(_gp = 0);

    /* Stack — 64KB, placed just below SW buffers at 0x80790000           */
    __stack_top      = 0x80790000;
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
