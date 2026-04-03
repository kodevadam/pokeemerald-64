#ifndef GUARD_N64_IO_REG_H
#define GUARD_N64_IO_REG_H

/*
 * include/n64/io_reg.h
 *
 * N64 port — replaces include/gba/io_reg.h
 *
 * Strategy: every GBA I/O register (REG_DISPCNT, REG_BG0CNT, …) becomes a
 * field in the global software register file `gN64IoRegs`, which lives in
 * the .sw_ioregs RDRAM section.  The macros below redirect REG_* accesses
 * to that struct so that the game source code compiles unchanged.
 *
 * The software tile compositor (src/n64/tile_renderer.c) and the software
 * sprite compositor (src/n64/sprite_renderer.c) read the struct once per
 * frame to produce the final 240×160 image.
 *
 * Real N64 hardware registers (VI, AI, PI, SI, MI, SP, DP) are accessed
 * directly in the N64 platform files; they are NOT exposed through this
 * header to avoid name collisions.
 */

#include "n64/types.h"

/* -----------------------------------------------------------------------
 * I/O register offsets — kept identical to GBA so that any code that uses
 * REG_OFFSET_* constants (e.g. gpu_regs.c) recompiles without changes.
 * --------------------------------------------------------------------- */
#define REG_OFFSET_DISPCNT     0x000
#define REG_OFFSET_DISPSTAT    0x004
#define REG_OFFSET_VCOUNT      0x006
#define REG_OFFSET_BG0CNT      0x008
#define REG_OFFSET_BG1CNT      0x00A
#define REG_OFFSET_BG2CNT      0x00C
#define REG_OFFSET_BG3CNT      0x00E
#define REG_OFFSET_BG0HOFS     0x010
#define REG_OFFSET_BG0VOFS     0x012
#define REG_OFFSET_BG1HOFS     0x014
#define REG_OFFSET_BG1VOFS     0x016
#define REG_OFFSET_BG2HOFS     0x018
#define REG_OFFSET_BG2VOFS     0x01A
#define REG_OFFSET_BG3HOFS     0x01C
#define REG_OFFSET_BG3VOFS     0x01E
#define REG_OFFSET_BG2PA       0x020
#define REG_OFFSET_BG2PB       0x022
#define REG_OFFSET_BG2PC       0x024
#define REG_OFFSET_BG2PD       0x026
#define REG_OFFSET_BG2X        0x028
#define REG_OFFSET_BG2X_L      0x028
#define REG_OFFSET_BG2X_H      0x02A
#define REG_OFFSET_BG2Y        0x02C
#define REG_OFFSET_BG2Y_L      0x02C
#define REG_OFFSET_BG2Y_H      0x02E
#define REG_OFFSET_BG3PA       0x030
#define REG_OFFSET_BG3PB       0x032
#define REG_OFFSET_BG3PC       0x034
#define REG_OFFSET_BG3PD       0x036
#define REG_OFFSET_BG3X        0x038
#define REG_OFFSET_BG3X_L      0x038
#define REG_OFFSET_BG3X_H      0x03A
#define REG_OFFSET_BG3Y        0x03C
#define REG_OFFSET_BG3Y_L      0x03C
#define REG_OFFSET_BG3Y_H      0x03E
#define REG_OFFSET_WIN0H       0x040
#define REG_OFFSET_WIN1H       0x042
#define REG_OFFSET_WIN0V       0x044
#define REG_OFFSET_WIN1V       0x046
#define REG_OFFSET_WININ       0x048
#define REG_OFFSET_WINOUT      0x04A
#define REG_OFFSET_MOSAIC      0x04C
#define REG_OFFSET_BLDCNT      0x050
#define REG_OFFSET_BLDALPHA    0x052
#define REG_OFFSET_BLDY        0x054

/* Sound offsets (retained for source compatibility; actual audio is in n64/audio.c) */
#define REG_OFFSET_SOUND1CNT_L 0x060
#define REG_OFFSET_SOUND1CNT_H 0x062
#define REG_OFFSET_SOUND1CNT_X 0x064
#define REG_OFFSET_SOUND2CNT_L 0x068
#define REG_OFFSET_SOUND2CNT_H 0x06C
#define REG_OFFSET_SOUND3CNT_L 0x070
#define REG_OFFSET_SOUND3CNT_H 0x072
#define REG_OFFSET_SOUND3CNT_X 0x074
#define REG_OFFSET_SOUND4CNT_L 0x078
#define REG_OFFSET_SOUND4CNT_H 0x07C
#define REG_OFFSET_SOUNDCNT_L  0x080
#define REG_OFFSET_SOUNDCNT_H  0x082
#define REG_OFFSET_SOUNDCNT_X  0x084
#define REG_OFFSET_SOUNDBIAS   0x088
#define REG_OFFSET_FIFO_A      0x0A0
#define REG_OFFSET_FIFO_B      0x0A4

/* Sound wave RAM and bias */
#define REG_OFFSET_SOUNDBIAS_H 0x089
#define REG_OFFSET_WAVE_RAM0   0x090
#define REG_OFFSET_WAVE_RAM1   0x094
#define REG_OFFSET_WAVE_RAM2   0x098
#define REG_OFFSET_WAVE_RAM3   0x09C

/* DMA (stubs; real transfers done via memcpy on N64) */
#define REG_OFFSET_DMA0        0x0B0
#define REG_OFFSET_DMA0CNT     0x0B8
#define REG_OFFSET_DMA0CNT_L   0x0B8
#define REG_OFFSET_DMA0CNT_H   0x0BA
#define REG_OFFSET_DMA1        0x0BC
#define REG_OFFSET_DMA1SAD     0x0BC
#define REG_OFFSET_DMA1DAD     0x0C0
#define REG_OFFSET_DMA1CNT     0x0C4
#define REG_OFFSET_DMA1CNT_L   0x0C4
#define REG_OFFSET_DMA1CNT_H   0x0C6
#define REG_OFFSET_DMA2        0x0C8
#define REG_OFFSET_DMA2SAD     0x0C8
#define REG_OFFSET_DMA2DAD     0x0CC
#define REG_OFFSET_DMA2CNT     0x0D0
#define REG_OFFSET_DMA2CNT_L   0x0D0
#define REG_OFFSET_DMA2CNT_H   0x0D2
#define REG_OFFSET_DMA3        0x0D4
#define REG_OFFSET_DMA3CNT     0x0DC
#define REG_OFFSET_DMA3CNT_L   0x0DC
#define REG_OFFSET_DMA3CNT_H   0x0DE

/* Timers */
#define REG_OFFSET_TM0CNT_L    0x100
#define REG_OFFSET_TM0CNT_H    0x102
#define REG_OFFSET_TM1CNT_L    0x104
#define REG_OFFSET_TM1CNT_H    0x106
#define REG_OFFSET_TM2CNT_L    0x108
#define REG_OFFSET_TM2CNT_H    0x10A
#define REG_OFFSET_TM3CNT_L    0x10C
#define REG_OFFSET_TM3CNT_H    0x10E

/* Serial / SIO */
#define REG_OFFSET_SIOCNT      0x128
#define REG_OFFSET_SIODATA8    0x12A
#define REG_OFFSET_SIODATA32   0x120
#define REG_OFFSET_SIOMLT_SEND 0x12A
#define REG_OFFSET_SIOMLT_RECV 0x120

/* Keypad */
#define REG_OFFSET_KEYINPUT    0x130
#define REG_OFFSET_KEYCNT      0x132

/* Serial 2 */
#define REG_OFFSET_RCNT        0x134
#define REG_OFFSET_JOYCNT      0x140
#define REG_OFFSET_JOY_RECV    0x150
#define REG_OFFSET_JOY_TRANS   0x154
#define REG_OFFSET_JOYSTAT     0x158

/* Interrupt / system */
#define REG_OFFSET_IE          0x200
#define REG_OFFSET_IF          0x202
#define REG_OFFSET_WAITCNT     0x204
#define REG_OFFSET_IME         0x208

/* -----------------------------------------------------------------------
 * Software I/O register file
 *
 * The entire GBA I/O register space is modelled as a flat byte array in
 * RDRAM.  Accesses through the REG_* macros below are plain memory reads
 * and writes — no MMIO, no side effects at write time.  The compositor
 * snapshots the relevant fields once per VBlank.
 * --------------------------------------------------------------------- */
#define N64_IOREGS_SIZE 0x400   /* 1 KB covers all used GBA I/O offsets */

extern u8 gN64IoRegs[N64_IOREGS_SIZE];

/* Helper: typed pointer into the register file at a given byte offset */
#define _REG8(off)  (*(volatile u8  *)(gN64IoRegs + (off)))
#define _REG16(off) (*(volatile u16 *)(gN64IoRegs + (off)))
#define _REG32(off) (*(volatile u32 *)(gN64IoRegs + (off)))

/* -----------------------------------------------------------------------
 * Display registers
 * --------------------------------------------------------------------- */
#define REG_DISPCNT  _REG16(REG_OFFSET_DISPCNT)
#define REG_DISPSTAT _REG16(REG_OFFSET_DISPSTAT)
#define REG_VCOUNT   _REG16(REG_OFFSET_VCOUNT)

/* DISPCNT bit fields */
#define DISPCNT_MODE_MASK      0x0007
#define DISPCNT_HBLANK_INTERVAL 0x0020
#define DISPCNT_MODE_0        0x0000
#define DISPCNT_MODE_1        0x0001
#define DISPCNT_MODE_2        0x0002
#define DISPCNT_MODE_3        0x0003
#define DISPCNT_MODE_4        0x0004
#define DISPCNT_MODE_5        0x0005
#define DISPCNT_OBJ_1D_MAP    0x0040
#define DISPCNT_FORCED_BLANK  0x0080
#define DISPCNT_BG0_ON        0x0100
#define DISPCNT_BG1_ON        0x0200
#define DISPCNT_BG2_ON        0x0400
#define DISPCNT_BG3_ON        0x0800
#define DISPCNT_OBJ_ON        0x1000
#define DISPCNT_WIN0_ON       0x2000
#define DISPCNT_WIN1_ON       0x4000
#define DISPCNT_OBJWIN_ON     0x8000
#define DISPCNT_BG_ALL_ON     0x0F00

/* BGCNT flags */
#define BGCNT_PRIORITY(n)          (n)
#define BGCNT_CHARBASE(n)   ((n) << 2)
#define BGCNT_MOSAIC            0x0040
#define BGCNT_16COLOR           0x0000
#define BGCNT_256COLOR          0x0080
#define BGCNT_SCREENBASE(n) ((n) << 8)
#define BGCNT_WRAP              0x2000
#define BGCNT_TXT256x256        0x0000
#define BGCNT_TXT512x256        0x4000
#define BGCNT_TXT256x512        0x8000
#define BGCNT_TXT512x512        0xC000
#define BGCNT_AFF128x128        0x0000
#define BGCNT_AFF256x256        0x4000
#define BGCNT_AFF512x512        0x8000
#define BGCNT_AFF1024x1024      0xC000

/* DISPSTAT bit fields */
#define DISPSTAT_VBLANK       0x0001
#define DISPSTAT_HBLANK       0x0002
#define DISPSTAT_VCOUNT_MATCH 0x0004
#define DISPSTAT_VBLANK_INTR  0x0008
#define DISPSTAT_HBLANK_INTR  0x0010
#define DISPSTAT_VCOUNT_INTR  0x0020

/* -----------------------------------------------------------------------
 * Background control registers
 * --------------------------------------------------------------------- */
#define REG_BG0CNT  _REG16(REG_OFFSET_BG0CNT)
#define REG_BG1CNT  _REG16(REG_OFFSET_BG1CNT)
#define REG_BG2CNT  _REG16(REG_OFFSET_BG2CNT)
#define REG_BG3CNT  _REG16(REG_OFFSET_BG3CNT)

/* BG scroll */
#define REG_BG0HOFS _REG16(REG_OFFSET_BG0HOFS)
#define REG_BG0VOFS _REG16(REG_OFFSET_BG0VOFS)
#define REG_BG1HOFS _REG16(REG_OFFSET_BG1HOFS)
#define REG_BG1VOFS _REG16(REG_OFFSET_BG1VOFS)
#define REG_BG2HOFS _REG16(REG_OFFSET_BG2HOFS)
#define REG_BG2VOFS _REG16(REG_OFFSET_BG2VOFS)
#define REG_BG3HOFS _REG16(REG_OFFSET_BG3HOFS)
#define REG_BG3VOFS _REG16(REG_OFFSET_BG3VOFS)

/* BG2 / BG3 affine parameters */
#define REG_BG2PA _REG16(REG_OFFSET_BG2PA)
#define REG_BG2PB _REG16(REG_OFFSET_BG2PB)
#define REG_BG2PC _REG16(REG_OFFSET_BG2PC)
#define REG_BG2PD _REG16(REG_OFFSET_BG2PD)
#define REG_BG2X  _REG32(REG_OFFSET_BG2X)
#define REG_BG2Y  _REG32(REG_OFFSET_BG2Y)
#define REG_BG3PA _REG16(REG_OFFSET_BG3PA)
#define REG_BG3PB _REG16(REG_OFFSET_BG3PB)
#define REG_BG3PC _REG16(REG_OFFSET_BG3PC)
#define REG_BG3PD _REG16(REG_OFFSET_BG3PD)
#define REG_BG3X  _REG32(REG_OFFSET_BG3X)
#define REG_BG3Y  _REG32(REG_OFFSET_BG3Y)

/* -----------------------------------------------------------------------
 * Window registers
 * --------------------------------------------------------------------- */
#define REG_WIN0H  _REG16(REG_OFFSET_WIN0H)
#define REG_WIN1H  _REG16(REG_OFFSET_WIN1H)
#define REG_WIN0V  _REG16(REG_OFFSET_WIN0V)
#define REG_WIN1V  _REG16(REG_OFFSET_WIN1V)
#define REG_WININ  _REG16(REG_OFFSET_WININ)
#define REG_WINOUT _REG16(REG_OFFSET_WINOUT)

/* -----------------------------------------------------------------------
 * Blend / alpha registers
 * --------------------------------------------------------------------- */
#define REG_BLDCNT   _REG16(REG_OFFSET_BLDCNT)
#define REG_BLDALPHA _REG16(REG_OFFSET_BLDALPHA)
#define REG_BLDY     _REG16(REG_OFFSET_BLDY)

/* BLDCNT flags */
#define BLDCNT_TGT1_BG0        0x0001
#define BLDCNT_TGT1_BG1        0x0002
#define BLDCNT_TGT1_BG2        0x0004
#define BLDCNT_TGT1_BG3        0x0008
#define BLDCNT_TGT1_OBJ        0x0010
#define BLDCNT_TGT1_BD         0x0020
#define BLDCNT_EFFECT_MASK     0x00C0
#define BLDCNT_EFFECT_NONE     0x0000
#define BLDCNT_EFFECT_BLEND    0x0040
#define BLDCNT_EFFECT_LIGHTEN  0x0080
#define BLDCNT_EFFECT_DARKEN   0x00C0
#define BLDCNT_TGT2_BG0        0x0100
#define BLDCNT_TGT2_BG1        0x0200
#define BLDCNT_TGT2_BG2        0x0400
#define BLDCNT_TGT2_BG3        0x0800
#define BLDCNT_TGT2_OBJ        0x1000
#define BLDCNT_TGT2_BD         0x2000
#define BLDALPHA_BLEND(target1, target2) (((target2) << 8) | (target1))
#define BLDCNT_TGT1_BG_ALL   (BLDCNT_TGT1_BG0 | BLDCNT_TGT1_BG1 | BLDCNT_TGT1_BG2 | BLDCNT_TGT1_BG3)
#define BLDCNT_TGT1_ALL      (BLDCNT_TGT1_BG_ALL | BLDCNT_TGT1_OBJ | BLDCNT_TGT1_BD)
#define BLDCNT_TGT2_BG_ALL   (BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3)
#define BLDCNT_TGT2_ALL      (BLDCNT_TGT2_BG_ALL | BLDCNT_TGT2_OBJ | BLDCNT_TGT2_BD)

/* WININ flags */
#define WININ_WIN0_BG0      (1 << 0)
#define WININ_WIN0_BG1      (1 << 1)
#define WININ_WIN0_BG2      (1 << 2)
#define WININ_WIN0_BG3      (1 << 3)
#define WININ_WIN0_BG_ALL   (WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_BG3)
#define WININ_WIN0_OBJ      (1 << 4)
#define WININ_WIN0_CLR      (1 << 5)
#define WININ_WIN0_ALL      (WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR)
#define WININ_WIN1_BG0      (1 << 8)
#define WININ_WIN1_BG1      (1 << 9)
#define WININ_WIN1_BG2      (1 << 10)
#define WININ_WIN1_BG3      (1 << 11)
#define WININ_WIN1_BG_ALL   (WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_BG3)
#define WININ_WIN1_OBJ      (1 << 12)
#define WININ_WIN1_CLR      (1 << 13)
#define WININ_WIN1_ALL      (WININ_WIN1_BG_ALL | WININ_WIN1_OBJ | WININ_WIN1_CLR)

/* WINOUT flags */
#define WINOUT_WIN01_BG0    (1 << 0)
#define WINOUT_WIN01_BG1    (1 << 1)
#define WINOUT_WIN01_BG2    (1 << 2)
#define WINOUT_WIN01_BG3    (1 << 3)
#define WINOUT_WIN01_BG_ALL (WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3)
#define WINOUT_WIN01_OBJ    (1 << 4)
#define WINOUT_WIN01_CLR    (1 << 5)
#define WINOUT_WIN01_ALL    (WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR)
#define WINOUT_WINOBJ_BG0   (1 << 8)
#define WINOUT_WINOBJ_BG1   (1 << 9)
#define WINOUT_WINOBJ_BG2   (1 << 10)
#define WINOUT_WINOBJ_BG3   (1 << 11)
#define WINOUT_WINOBJ_BG_ALL (WINOUT_WINOBJ_BG0 | WINOUT_WINOBJ_BG1 | WINOUT_WINOBJ_BG2 | WINOUT_WINOBJ_BG3)
#define WINOUT_WINOBJ_OBJ   (1 << 12)
#define WINOUT_WINOBJ_CLR   (1 << 13)
#define WINOUT_WINOBJ_ALL   (WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR)

/* Button constants */
#define A_BUTTON        0x0001
#define B_BUTTON        0x0002
#define SELECT_BUTTON   0x0004
#define START_BUTTON    0x0008
#define DPAD_RIGHT      0x0010
#define DPAD_LEFT       0x0020
#define DPAD_UP         0x0040
#define DPAD_DOWN       0x0080
#define R_BUTTON        0x0100
#define L_BUTTON        0x0200
#define DPAD_ANY        ((DPAD_RIGHT | DPAD_LEFT | DPAD_UP | DPAD_DOWN))

/* -----------------------------------------------------------------------
 * Sound registers (values stored for M4A sequencer compatibility;
 * actual audio output goes through n64/audio.c)
 * --------------------------------------------------------------------- */
#define REG_SOUNDCNT_L  _REG16(REG_OFFSET_SOUNDCNT_L)
#define REG_SOUNDCNT_H  _REG16(REG_OFFSET_SOUNDCNT_H)
#define REG_SOUNDCNT_X  _REG16(REG_OFFSET_SOUNDCNT_X)
#define REG_SOUNDBIAS   _REG16(REG_OFFSET_SOUNDBIAS)
#define REG_NR10        _REG8(REG_OFFSET_SOUND1CNT_L)
#define REG_NR11        _REG8(REG_OFFSET_SOUND1CNT_H)
#define REG_NR12        _REG8(REG_OFFSET_SOUND1CNT_H + 1)
#define REG_NR13        _REG8(REG_OFFSET_SOUND1CNT_X)
#define REG_NR14        _REG8(REG_OFFSET_SOUND1CNT_X + 1)
#define REG_NR21        _REG8(REG_OFFSET_SOUND2CNT_L)
#define REG_NR22        _REG8(REG_OFFSET_SOUND2CNT_L + 1)
#define REG_NR23        _REG8(REG_OFFSET_SOUND2CNT_H)
#define REG_NR24        _REG8(REG_OFFSET_SOUND2CNT_H + 1)
#define REG_NR30        _REG8(REG_OFFSET_SOUND3CNT_L)
#define REG_NR31        _REG8(REG_OFFSET_SOUND3CNT_H)
#define REG_NR32        _REG8(REG_OFFSET_SOUND3CNT_H + 1)
#define REG_NR33        _REG8(REG_OFFSET_SOUND3CNT_X)
#define REG_NR34        _REG8(REG_OFFSET_SOUND3CNT_X + 1)
#define REG_NR41        _REG8(REG_OFFSET_SOUND4CNT_L)
#define REG_NR42        _REG8(REG_OFFSET_SOUND4CNT_L + 1)
#define REG_NR43        _REG8(REG_OFFSET_SOUND4CNT_H)
#define REG_NR44        _REG8(REG_OFFSET_SOUND4CNT_H + 1)
#define REG_NR50        _REG8(REG_OFFSET_SOUNDCNT_L)
#define REG_NR51        _REG8(REG_OFFSET_SOUNDCNT_L + 1)
#define REG_NR52        _REG8(REG_OFFSET_SOUNDCNT_X)
#define REG_FIFO_A      _REG32(REG_OFFSET_FIFO_A)
#define REG_FIFO_B      _REG32(REG_OFFSET_FIFO_B)

/* -----------------------------------------------------------------------
 * DMA registers — stubs; all DMA becomes memcpy/memset on N64.
 * The register fields are still stored so that m4a.c can write to them
 * without crashing; dma3_stub.c reads them and executes the transfer.
 * --------------------------------------------------------------------- */
#define REG_ADDR_DMA0 (gN64IoRegs + REG_OFFSET_DMA0)
#define REG_ADDR_DMA1     (gN64IoRegs + REG_OFFSET_DMA1)
#define REG_ADDR_DMA1SAD  (gN64IoRegs + REG_OFFSET_DMA1SAD)
#define REG_ADDR_DMA1DAD  (gN64IoRegs + REG_OFFSET_DMA1DAD)
#define REG_ADDR_DMA2     (gN64IoRegs + REG_OFFSET_DMA2)
#define REG_ADDR_DMA2SAD  (gN64IoRegs + REG_OFFSET_DMA2SAD)
#define REG_ADDR_DMA2DAD  (gN64IoRegs + REG_OFFSET_DMA2DAD)
#define REG_ADDR_DMA3     (gN64IoRegs + REG_OFFSET_DMA3)
#define REG_DMA1SAD _REG32(REG_OFFSET_DMA1SAD)
#define REG_DMA1DAD _REG32(REG_OFFSET_DMA1DAD)
#define REG_DMA2SAD _REG32(REG_OFFSET_DMA2SAD)
#define REG_DMA2DAD _REG32(REG_OFFSET_DMA2DAD)

/* Sound wave RAM registers */
#define REG_ADDR_WAVE_RAM0  (gN64IoRegs + REG_OFFSET_WAVE_RAM0)
#define REG_ADDR_WAVE_RAM1  (gN64IoRegs + REG_OFFSET_WAVE_RAM1)
#define REG_ADDR_WAVE_RAM2  (gN64IoRegs + REG_OFFSET_WAVE_RAM2)
#define REG_ADDR_WAVE_RAM3  (gN64IoRegs + REG_OFFSET_WAVE_RAM3)
#define REG_ADDR_SOUNDBIAS_H (gN64IoRegs + REG_OFFSET_SOUNDBIAS_H)
#define REG_WAVE_RAM0  _REG32(REG_OFFSET_WAVE_RAM0)
#define REG_WAVE_RAM1  _REG32(REG_OFFSET_WAVE_RAM1)
#define REG_WAVE_RAM2  _REG32(REG_OFFSET_WAVE_RAM2)
#define REG_WAVE_RAM3  _REG32(REG_OFFSET_WAVE_RAM3)
#define REG_SOUNDBIAS_H _REG8(REG_OFFSET_SOUNDBIAS_H)

/* CGB sound channel NR addresses */
#define REG_ADDR_NR10  (gN64IoRegs + REG_OFFSET_SOUND1CNT_L)
#define REG_ADDR_NR11  (gN64IoRegs + REG_OFFSET_SOUND1CNT_H)
#define REG_ADDR_NR12  (gN64IoRegs + REG_OFFSET_SOUND1CNT_H + 1)
#define REG_ADDR_NR13  (gN64IoRegs + REG_OFFSET_SOUND1CNT_X)
#define REG_ADDR_NR14  (gN64IoRegs + REG_OFFSET_SOUND1CNT_X + 1)
#define REG_ADDR_NR21  (gN64IoRegs + REG_OFFSET_SOUND2CNT_L)
#define REG_ADDR_NR22  (gN64IoRegs + REG_OFFSET_SOUND2CNT_L + 1)
#define REG_ADDR_NR23  (gN64IoRegs + REG_OFFSET_SOUND2CNT_H)
#define REG_ADDR_NR24  (gN64IoRegs + REG_OFFSET_SOUND2CNT_H + 1)
#define REG_ADDR_NR30  (gN64IoRegs + REG_OFFSET_SOUND3CNT_L)
#define REG_ADDR_NR31  (gN64IoRegs + REG_OFFSET_SOUND3CNT_H)
#define REG_ADDR_NR32  (gN64IoRegs + REG_OFFSET_SOUND3CNT_H + 1)
#define REG_ADDR_NR33  (gN64IoRegs + REG_OFFSET_SOUND3CNT_X)
#define REG_ADDR_NR34  (gN64IoRegs + REG_OFFSET_SOUND3CNT_X + 1)
#define REG_ADDR_NR41  (gN64IoRegs + REG_OFFSET_SOUND4CNT_L)
#define REG_ADDR_NR42  (gN64IoRegs + REG_OFFSET_SOUND4CNT_L + 1)
#define REG_ADDR_NR43  (gN64IoRegs + REG_OFFSET_SOUND4CNT_H)
#define REG_ADDR_NR44  (gN64IoRegs + REG_OFFSET_SOUND4CNT_H + 1)

/* VCOUNT address */
#define REG_ADDR_VCOUNT (gN64IoRegs + REG_OFFSET_VCOUNT)
#define REG_ADDR_DMA0CNT   (gN64IoRegs + REG_OFFSET_DMA0CNT)
#define REG_ADDR_DMA0CNT_L (gN64IoRegs + REG_OFFSET_DMA0CNT_L)
#define REG_ADDR_DMA0CNT_H (gN64IoRegs + REG_OFFSET_DMA0CNT_H)
#define REG_ADDR_DMA1CNT   (gN64IoRegs + REG_OFFSET_DMA1CNT)
#define REG_ADDR_DMA1CNT_L (gN64IoRegs + REG_OFFSET_DMA1CNT_L)
#define REG_ADDR_DMA1CNT_H (gN64IoRegs + REG_OFFSET_DMA1CNT_H)
#define REG_ADDR_DMA2CNT   (gN64IoRegs + REG_OFFSET_DMA2CNT)
#define REG_ADDR_DMA2CNT_L (gN64IoRegs + REG_OFFSET_DMA2CNT_L)
#define REG_ADDR_DMA2CNT_H (gN64IoRegs + REG_OFFSET_DMA2CNT_H)
#define REG_ADDR_DMA3CNT   (gN64IoRegs + REG_OFFSET_DMA3CNT)
#define REG_ADDR_DMA3CNT_L (gN64IoRegs + REG_OFFSET_DMA3CNT_L)
#define REG_ADDR_DMA3CNT_H (gN64IoRegs + REG_OFFSET_DMA3CNT_H)
#define REG_DMA0CNT   _REG32(REG_OFFSET_DMA0CNT)
#define REG_DMA0CNT_L _REG16(REG_OFFSET_DMA0CNT_L)
#define REG_DMA0CNT_H _REG16(REG_OFFSET_DMA0CNT_H)
#define REG_DMA1CNT   _REG32(REG_OFFSET_DMA1CNT)
#define REG_DMA1CNT_L _REG16(REG_OFFSET_DMA1CNT_L)
#define REG_DMA1CNT_H _REG16(REG_OFFSET_DMA1CNT_H)
#define REG_DMA2CNT   _REG32(REG_OFFSET_DMA2CNT)
#define REG_DMA2CNT_L _REG16(REG_OFFSET_DMA2CNT_L)
#define REG_DMA2CNT_H _REG16(REG_OFFSET_DMA2CNT_H)
#define REG_DMA3CNT   _REG32(REG_OFFSET_DMA3CNT)
#define REG_DMA3CNT_L _REG16(REG_OFFSET_DMA3CNT_L)
#define REG_DMA3CNT_H _REG16(REG_OFFSET_DMA3CNT_H)

/* Sound control register flags (writes go to software regs; N64 audio handled separately) */
#define SOUND_A_RIGHT_OUTPUT  0x0100
#define SOUND_A_LEFT_OUTPUT   0x0200
#define SOUND_A_TIMER_0       0x0000
#define SOUND_A_TIMER_1       0x0400
#define SOUND_A_FIFO_RESET    0x0800
#define SOUND_B_RIGHT_OUTPUT  0x1000
#define SOUND_B_LEFT_OUTPUT   0x2000
#define SOUND_B_TIMER_0       0x0000
#define SOUND_B_TIMER_1       0x4000
#define SOUND_B_FIFO_RESET    0x8000
#define SOUND_1_ON          0x0001
#define SOUND_2_ON          0x0002
#define SOUND_3_ON          0x0004
#define SOUND_4_ON          0x0008
#define SOUND_MASTER_ENABLE 0x0080

/* Background scroll register addresses (used by DMA/scanline effect code) */
#define REG_ADDR_BG0CNT  (gN64IoRegs + REG_OFFSET_BG0CNT)
#define REG_ADDR_BG1CNT  (gN64IoRegs + REG_OFFSET_BG1CNT)
#define REG_ADDR_BG2CNT  (gN64IoRegs + REG_OFFSET_BG2CNT)
#define REG_ADDR_BG3CNT  (gN64IoRegs + REG_OFFSET_BG3CNT)
#define REG_ADDR_BG0HOFS (gN64IoRegs + REG_OFFSET_BG0HOFS)
#define REG_ADDR_BG0VOFS (gN64IoRegs + REG_OFFSET_BG0VOFS)
#define REG_ADDR_BG1HOFS (gN64IoRegs + REG_OFFSET_BG1HOFS)
#define REG_ADDR_BG1VOFS (gN64IoRegs + REG_OFFSET_BG1VOFS)
#define REG_ADDR_BG2HOFS (gN64IoRegs + REG_OFFSET_BG2HOFS)
#define REG_ADDR_BG2VOFS (gN64IoRegs + REG_OFFSET_BG2VOFS)
#define REG_ADDR_BG3HOFS (gN64IoRegs + REG_OFFSET_BG3HOFS)
#define REG_ADDR_BG3VOFS (gN64IoRegs + REG_OFFSET_BG3VOFS)
#define REG_ADDR_BG2PA   (gN64IoRegs + REG_OFFSET_BG2PA)
#define REG_ADDR_BG2PB   (gN64IoRegs + REG_OFFSET_BG2PB)
#define REG_ADDR_BG2PC   (gN64IoRegs + REG_OFFSET_BG2PC)
#define REG_ADDR_BG2PD   (gN64IoRegs + REG_OFFSET_BG2PD)
#define REG_ADDR_BG2X_L  (gN64IoRegs + REG_OFFSET_BG2X_L)
#define REG_ADDR_BG2X_H  (gN64IoRegs + REG_OFFSET_BG2X_H)
#define REG_ADDR_BG2Y_L  (gN64IoRegs + REG_OFFSET_BG2Y_L)
#define REG_ADDR_BG2Y_H  (gN64IoRegs + REG_OFFSET_BG2Y_H)
#define REG_ADDR_WIN0H   (gN64IoRegs + REG_OFFSET_WIN0H)
#define REG_ADDR_WIN0V   (gN64IoRegs + REG_OFFSET_WIN0V)
#define REG_ADDR_WIN1H   (gN64IoRegs + REG_OFFSET_WIN1H)
#define REG_ADDR_WIN1V   (gN64IoRegs + REG_OFFSET_WIN1V)
#define REG_ADDR_WININ   (gN64IoRegs + REG_OFFSET_WININ)
#define REG_ADDR_WINOUT  (gN64IoRegs + REG_OFFSET_WINOUT)
#define REG_ADDR_BLDCNT  (gN64IoRegs + REG_OFFSET_BLDCNT)
#define REG_ADDR_BLDALPHA (gN64IoRegs + REG_OFFSET_BLDALPHA)
#define REG_ADDR_BLDY    (gN64IoRegs + REG_OFFSET_BLDY)
#define REG_ADDR_DISPCNT (gN64IoRegs + REG_OFFSET_DISPCNT)
#define REG_ADDR_DISPSTAT (gN64IoRegs + REG_OFFSET_DISPSTAT)

/* DMA control bit fields (same as GBA) */
#define DMA_DEST_INC    0x0000
#define DMA_DEST_DEC    0x0020
#define DMA_DEST_FIXED  0x0040
#define DMA_DEST_RELOAD 0x0060
#define DMA_SRC_INC     0x0000
#define DMA_SRC_DEC     0x0080
#define DMA_SRC_FIXED   0x0100
#define DMA_REPEAT      0x0200
#define DMA_16BIT       0x0000
#define DMA_32BIT       0x0400
#define DMA_DREQ_ON     0x0800
#define DMA_START_NOW     0x0000
#define DMA_START_VBLANK  0x1000
#define DMA_START_HBLANK  0x2000
#define DMA_START_SPECIAL 0x3000
#define DMA_START_MASK    0x3000
#define DMA_INTR_ENABLE   0x4000
#define DMA_ENABLE        0x8000

/* Sound mix constants */
#define SOUND_CGB_MIX_QUARTER 0x0000
#define SOUND_CGB_MIX_HALF    0x0001
#define SOUND_CGB_MIX_FULL    0x0002
#define SOUND_A_MIX_HALF      0x0000
#define SOUND_A_MIX_FULL      0x0004
#define SOUND_B_MIX_HALF      0x0000
#define SOUND_B_MIX_FULL      0x0008
#define SOUND_ALL_MIX_FULL    0x000E

/* -----------------------------------------------------------------------
 * Timer registers — backed by N64 CP0 Count register.
 * Reads of TM*CNT_L return a value derived from Count; writes are ignored.
 * --------------------------------------------------------------------- */
#define REG_TM0CNT_L _REG16(REG_OFFSET_TM0CNT_L)
#define REG_TM0CNT_H _REG16(REG_OFFSET_TM0CNT_H)
#define REG_TM1CNT_L _REG16(REG_OFFSET_TM1CNT_L)
#define REG_TM1CNT_H _REG16(REG_OFFSET_TM1CNT_H)
#define REG_TM2CNT_L _REG16(REG_OFFSET_TM2CNT_L)
#define REG_TM2CNT_H _REG16(REG_OFFSET_TM2CNT_H)
#define REG_TM3CNT_L _REG16(REG_OFFSET_TM3CNT_L)
#define REG_TM3CNT_H _REG16(REG_OFFSET_TM3CNT_H)

/* TM*CNT_H flags */
#define TIMER_PRESCALER_1    0x00
#define TIMER_PRESCALER_64   0x01
#define TIMER_PRESCALER_256  0x02
#define TIMER_PRESCALER_1024 0x03
#define TIMER_COUNT_UP       0x04
#define TIMER_1CLK           0x00
#define TIMER_64CLK          0x01
#define TIMER_256CLK         0x02
#define TIMER_1024CLK        0x03
#define TIMER_INTR_ENABLE    0x40
#define TIMER_INTR_ENABLE    0x40
#define TIMER_ENABLE         0x80

/* -----------------------------------------------------------------------
 * Serial / keypad / interrupt registers
 * --------------------------------------------------------------------- */
#define REG_KEYINPUT _REG16(REG_OFFSET_KEYINPUT)
#define REG_KEYCNT   _REG16(REG_OFFSET_KEYCNT)
#define REG_SIOCNT       _REG16(REG_OFFSET_SIOCNT)
#define REG_SIODATA8     _REG16(REG_OFFSET_SIODATA8)
#define REG_SIODATA32    _REG32(REG_OFFSET_SIODATA32)
#define REG_SIOMLT_SEND  _REG16(REG_OFFSET_SIOMLT_SEND)
#define REG_SIOMLT_RECV  _REG32(REG_OFFSET_SIOMLT_RECV)
#define REG_RCNT         _REG16(REG_OFFSET_RCNT)
#define REG_ADDR_SIOCNT      (gN64IoRegs + REG_OFFSET_SIOCNT)
#define REG_ADDR_SIODATA8    (gN64IoRegs + REG_OFFSET_SIODATA8)
#define REG_ADDR_SIODATA32   (gN64IoRegs + REG_OFFSET_SIODATA32)
#define REG_ADDR_SIOMLT_SEND (gN64IoRegs + REG_OFFSET_SIOMLT_SEND)
#define REG_ADDR_SIOMLT_RECV (gN64IoRegs + REG_OFFSET_SIOMLT_RECV)

/* SIO mode/control flags */
#define SIO_32BIT_MODE     0x1000
#define SIO_MULTI_MODE     0x2000
#define SIO_38400_BPS      0x0001
#define SIO_115200_BPS     0x0003
#define SIO_MULTI_SI       0x0004
#define SIO_MULTI_SD       0x0008
#define SIO_MULTI_BUSY     0x0080
#define SIO_ENABLE         0x0080
#define SIO_INTR_ENABLE    0x4000
#define SIO_MULTI_SI_SHIFT 2
#define SIO_MULTI_SI_MASK  0x1
#define SIO_MULTI_DI_SHIFT 3
#define SIO_MULTI_DI_MASK  0x1

/* Keypad bitmask — active high on N64 (GBA is active low; the shim inverts) */
#define KEYS_MASK 0x03FF

/* -----------------------------------------------------------------------
 * Interrupt / system registers
 * These affect the N64 interrupt state; the platform layer watches them.
 * --------------------------------------------------------------------- */
#define REG_IE     _REG16(REG_OFFSET_IE)
#define REG_IF     _REG16(REG_OFFSET_IF)
#define REG_WAITCNT _REG16(REG_OFFSET_WAITCNT)   /* ignored on N64 */
#define REG_IME    _REG16(REG_OFFSET_IME)

/* Interrupt flag bits (same as GBA) */
#define INTR_FLAG_VBLANK  0x0001
#define INTR_FLAG_HBLANK  0x0002
#define INTR_FLAG_VCOUNT  0x0004
#define INTR_FLAG_TIMER0  0x0008
#define INTR_FLAG_TIMER1  0x0010
#define INTR_FLAG_TIMER2  0x0020
#define INTR_FLAG_TIMER3  0x0040
#define INTR_FLAG_SERIAL  0x0080
#define INTR_FLAG_DMA0    0x0100
#define INTR_FLAG_DMA1    0x0200
#define INTR_FLAG_DMA2    0x0400
#define INTR_FLAG_DMA3    0x0800
#define INTR_FLAG_KEYPAD  0x1000
#define INTR_FLAG_GAMEPAK 0x2000

/* -----------------------------------------------------------------------
 * WAITCNT — ignored on N64 (no wait states), kept for source compat
 * --------------------------------------------------------------------- */
#define WAITCNT_PREFETCH_ENABLE 0x4000
#define WAITCNT_WS0_N_4         0x0000
#define WAITCNT_WS0_N_3         0x0001
#define WAITCNT_WS0_N_2         0x0002
#define WAITCNT_WS0_N_8         0x0003
#define WAITCNT_WS0_S_2         0x0000
#define WAITCNT_WS0_S_1         0x0004

/* WIN_RANGE: encode window horizontal/vertical boundary registers */
#define WIN_RANGE(a, b)  (((a) << 8) | (b))
#define WIN_RANGE2(a, b) ((b) | ((a) << 8))

#endif /* GUARD_N64_IO_REG_H */
