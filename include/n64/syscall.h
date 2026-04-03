#ifndef GUARD_N64_SYSCALL_H
#define GUARD_N64_SYSCALL_H

/*
 * include/n64/syscall.h
 *
 * N64 port — replaces include/gba/syscall.h
 *
 * All GBA BIOS calls are replaced with C implementations in src/n64/bios.c.
 * The function signatures are identical to the GBA BIOS so game code
 * recompiles without changes.
 */

#include "n64/types.h"

/* -----------------------------------------------------------------------
 * GBA BIOS call equivalents (implemented in src/n64/bios.c)
 * --------------------------------------------------------------------- */

/* SoftReset flags */
#define RESET_EWRAM    0x01
#define RESET_IWRAM    0x02
#define RESET_PALETTE  0x04
#define RESET_VRAM     0x08
#define RESET_OAM      0x10
#define RESET_SIO      0x20
#define RESET_SOUND    0x40
#define RESET_ALL      0xFF

void SoftReset(u8 resetFlags);
void RegisterRamReset(u8 resetFlags);
void VBlankIntrWait(void);
void Halt(void);
void Stop(void);
void IntrWait(u32 clearBeforeWait, u16 intrFlags);

/* Memory operations */
void CpuSet(const void *src, void *dst, u32 ctrl);
void CpuFastSet(const void *src, void *dst, u32 ctrl);

/* Decompression — software implementations replacing GBA BIOS */
void LZ77UnCompWram(const void *src, void *dst);
void LZ77UnCompVram(const void *src, void *dst);
void RLUnCompWram(const void *src, void *dst);
void RLUnCompVram(const void *src, void *dst);
void HuffUnComp(const void *src, void *dst);
void Diff8bitUnFilterWram(const void *src, void *dst);
void Diff8bitUnFilterVram(const void *src, void *dst);
void Diff16bitUnFilter(const void *src, void *dst);
void BitUnPack(const void *src, void *dst, const void *unpackInfo);

/* Math */
s32 Div(s32 numerator, s32 denominator);
s32 DivMod(s32 numerator, s32 denominator);
u32 DivAbs(s32 numerator, s32 denominator);
u32 Sqrt(u32 n);
s16 ArcTan(s16 tan);
s16 ArcTan2(s16 x, s16 y);

/* Affine matrix computation */
struct BgAffineSrcData;
struct BgAffineDstData;
struct ObjAffineSrcData;
void BgAffineSet(const struct BgAffineSrcData *src, struct BgAffineDstData *dst, u32 count);
void ObjAffineSet(const struct ObjAffineSrcData *src, void *dst, u32 count, u32 offset);

/* Multi-boot */
struct MultiBootParam;
s32 MultiBoot(struct MultiBootParam *param, u32 mode);

/* Sound */
void SoundBias(u32 bias);

/* -----------------------------------------------------------------------
 * Timer interrupt — referenced by src/main.c
 * --------------------------------------------------------------------- */
void Timer3Intr(void);

/* -----------------------------------------------------------------------
 * Flash timer shim — referenced by src/main.c
 * --------------------------------------------------------------------- */
void SetFlashTimerIntr(u8 timerNum, void (**intrFunc)(void));

#endif /* GUARD_N64_SYSCALL_H */
