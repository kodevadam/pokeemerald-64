#ifndef GUARD_N64_DEFINES_H
#define GUARD_N64_DEFINES_H

/*
 * include/n64/defines.h
 *
 * N64 port — replaces include/gba/defines.h
 *
 * Key strategy: GBA memory-mapped regions (PLTT, VRAM, OAM) become RDRAM
 * buffers allocated by the linker.  The software compositor reads them
 * directly; no hardware register write is needed.
 *
 * All numeric constants that the game uses to index into those regions are
 * preserved identically so that the game source code needs zero changes.
 */

#include <stddef.h>

#define TRUE  1
#define FALSE 0

/* -----------------------------------------------------------------------
 * Section placement — on N64 everything lives in RDRAM.
 * We keep the attribute names identical so game code recompiles without
 * changes; we just strip the GBA-specific section placement.
 * --------------------------------------------------------------------- */
#define IWRAM_DATA  /* nothing — maps to RDRAM on N64 */
#define EWRAM_DATA  /* nothing — maps to RDRAM on N64 */
#define COMMON_DATA /* nothing — maps to RDRAM on N64 */
#define UNUSED      __attribute__((unused))
#define NOINLINE    __attribute__((noinline))
#define ALIGNED(n)  __attribute__((aligned(n)))

/* -----------------------------------------------------------------------
 * Software GBA hardware buffers — allocated by n64.ld
 * The linker exports __sw_palette_start, __sw_vram_start, __sw_oam_start,
 * __sw_ioregs_start.  platform.c assigns the extern pointers below.
 * The macros use the same numeric values as GBA so that all BG_CHAR_ADDR()
 * etc. arithmetic still produces correct offsets into the VRAM buffer.
 * --------------------------------------------------------------------- */
extern void *__n64_pltt_buf;   /* points to __sw_palette_start in RDRAM */
extern void *__n64_vram_buf;   /* points to __sw_vram_start  in RDRAM   */
extern void *__n64_oam_buf;    /* points to __sw_oam_start   in RDRAM   */

/* These macros match the GBA address-space constants but resolve to
 * actual RDRAM pointers at runtime via the above extern variables.     */
#define PLTT          ((u32)(uintptr_t)__n64_pltt_buf)
#define BG_PLTT       PLTT
#define BG_PLTT_SIZE  0x200
#define OBJ_PLTT      (PLTT + BG_PLTT_SIZE)
#define OBJ_PLTT_SIZE 0x200
#define PLTT_SIZE     (BG_PLTT_SIZE + OBJ_PLTT_SIZE)

#define VRAM           ((u32)(uintptr_t)__n64_vram_buf)
#define VRAM_SIZE      0x18000

#define BG_VRAM           VRAM
#define BG_VRAM_SIZE      0x10000
#define BG_CHAR_SIZE      0x4000
#define BG_SCREEN_SIZE    0x800
#define BG_CHAR_ADDR(n)   (BG_VRAM + (BG_CHAR_SIZE * (n)))
#define BG_SCREEN_ADDR(n) (BG_VRAM + (BG_SCREEN_SIZE * (n)))

#define BG_TILE_H_FLIP(n) (0x400 + (n))
#define BG_TILE_V_FLIP(n) (0x800 + (n))

#define NUM_BACKGROUNDS 4

/* text-mode OBJ tile area */
#define OBJ_VRAM0      (VRAM + 0x10000)
#define OBJ_VRAM0_SIZE 0x8000

/* bitmap-mode OBJ tile area */
#define OBJ_VRAM1      (VRAM + 0x14000)
#define OBJ_VRAM1_SIZE 0x4000

#define OAM      ((u32)(uintptr_t)__n64_oam_buf)
#define OAM_SIZE 0x400

/* -----------------------------------------------------------------------
 * M4A / sound info pointer — stored in a regular global on N64
 * (GBA stored it at a fixed IWRAM address 0x3007FF0)
 * --------------------------------------------------------------------- */
extern struct SoundInfo *__n64_sound_info_ptr;
extern u16               __n64_intr_check;
extern void             *__n64_intr_vector;

#define SOUND_INFO_PTR (__n64_sound_info_ptr)
#define INTR_CHECK     (__n64_intr_check)
#define INTR_VECTOR    (__n64_intr_vector)

/* -----------------------------------------------------------------------
 * Display constants — keep GBA values; the compositor renders at 240×160
 * and the VI scaler centres the image on the 320×240 display.
 * --------------------------------------------------------------------- */
#define TILE_WIDTH  8
#define TILE_HEIGHT 8

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 160

#define DISPLAY_TILE_WIDTH  (DISPLAY_WIDTH  / TILE_WIDTH)
#define DISPLAY_TILE_HEIGHT (DISPLAY_HEIGHT / TILE_HEIGHT)

#define TILE_SIZE(bpp) ((bpp) * TILE_WIDTH * TILE_HEIGHT / 8)
#define TILE_SIZE_1BPP TILE_SIZE(1)   /*  8 */
#define TILE_SIZE_4BPP TILE_SIZE(4)   /* 32 */
#define TILE_SIZE_8BPP TILE_SIZE(8)   /* 64 */

#define TILE_OFFSET_4BPP(n) ((n) * TILE_SIZE_4BPP)
#define TILE_OFFSET_8BPP(n) ((n) * TILE_SIZE_8BPP)

#define TOTAL_OBJ_TILE_COUNT 1024

#define PLTT_SIZEOF(n)    ((n) * sizeof(u16))
#define PLTT_SIZE_4BPP    PLTT_SIZEOF(16)
#define PLTT_SIZE_8BPP    PLTT_SIZEOF(256)
#define PLTT_OFFSET_4BPP(n) ((n) * PLTT_SIZE_4BPP)

/* ROM header size — unused on N64 but referenced in some game code */
#define ROM_HEADER_SIZE 0xC0

/* -----------------------------------------------------------------------
 * N64 VI (Video Interface) output resolution
 * The compositor renders at DISPLAY_WIDTH × DISPLAY_HEIGHT (240×160),
 * then the VI upscales / centres to N64_VI_WIDTH × N64_VI_HEIGHT.
 * --------------------------------------------------------------------- */
#define N64_VI_WIDTH   320
#define N64_VI_HEIGHT  240

/* Letterbox offsets to centre 240×160 inside 320×240 */
#define N64_FB_X_OFFSET ((N64_VI_WIDTH  - DISPLAY_WIDTH)  / 2)   /* 40 */
#define N64_FB_Y_OFFSET ((N64_VI_HEIGHT - DISPLAY_HEIGHT) / 2)   /* 40 */

/* -----------------------------------------------------------------------
 * N64 CPU clock — VR4300 at 93.75 MHz
 * The Count register increments every other cycle → 46.875 MHz effective.
 * --------------------------------------------------------------------- */
#define N64_CPU_FREQ       93750000UL
#define N64_COUNT_FREQ     (N64_CPU_FREQ / 2)
#define N64_VI_FREQ        60          /* frames per second (NTSC) */
#define N64_COUNTS_PER_FRAME (N64_COUNT_FREQ / N64_VI_FREQ)

/* -----------------------------------------------------------------------
 * FlashRAM save — 128 KB (largest supported by N64 hardware)
 * Matches the GBA 1Mbit (128KB) flash used by Pokémon Emerald exactly.
 * --------------------------------------------------------------------- */
#define N64_FLASHRAM_SIZE      0x20000   /* 128 KB */
#define N64_FLASHRAM_PI_BASE   0x08000000UL  /* PI bus address of FlashRAM */

#endif /* GUARD_N64_DEFINES_H */
