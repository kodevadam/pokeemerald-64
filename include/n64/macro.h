#ifndef GUARD_N64_MACRO_H
#define GUARD_N64_MACRO_H

/*
 * include/n64/macro.h
 *
 * N64 port — replaces include/gba/macro.h
 *
 * All GBA DMA and CpuSet/CpuFastSet operations become plain C
 * memcpy / memset calls.  The macro signatures are kept identical so that
 * no game source code needs modification.
 *
 * CpuSet / CpuFastSet are declared in include/n64/bios.h and implemented
 * in src/n64/bios.c.  They behave identically to the GBA BIOS routines.
 *
 * DmaSet / DmaFill / DmaCopy expand directly to memcpy / memset —
 * the dmaNum argument is accepted but ignored.
 */

#include <string.h>
#include "n64/types.h"

/* -----------------------------------------------------------------------
 * CpuSet / CpuFastSet — forward declarations
 * (implemented in src/n64/bios.c)
 * --------------------------------------------------------------------- */
void CpuSet(const void *src, void *dst, u32 ctrl);
void CpuFastSet(const void *src, void *dst, u32 ctrl);

#define CPU_SET_16BIT         0x00000000
#define CPU_SET_32BIT         0x04000000
#define CPU_SET_SRC_FIXED     0x01000000
#define CPU_FAST_SET_SRC_FIXED 0x01000000

/* -----------------------------------------------------------------------
 * CpuFill / CpuCopy — same interface as GBA macros
 * --------------------------------------------------------------------- */
#define CPU_FILL_UNCHECKED(value, dest, size, bit)                  \
do {                                                                \
    vu##bit _tmp = (vu##bit)(value);                                \
    CpuSet((const void *)&_tmp, (dest),                             \
           CPU_SET_##bit##BIT | CPU_SET_SRC_FIXED |                 \
           (((size) / ((bit) / 8)) & 0x1FFFFF));                    \
} while (0)

#define CPU_FILL(value, dest, size, bit)        \
    CPU_FILL_UNCHECKED(value, dest, size, bit)

#define CpuFill16(value, dest, size) CPU_FILL(value, dest, size, 16)
#define CpuFill32(value, dest, size) CPU_FILL(value, dest, size, 32)

#define CPU_COPY_UNCHECKED(src, dest, size, bit)                    \
    CpuSet((src), (dest),                                           \
           CPU_SET_##bit##BIT | (((size) / ((bit) / 8)) & 0x1FFFFF))

#define CPU_COPY(src, dest, size, bit) CPU_COPY_UNCHECKED(src, dest, size, bit)

#define CpuCopy16(src, dest, size) CPU_COPY(src, dest, size, 16)
#define CpuCopy32(src, dest, size) CPU_COPY(src, dest, size, 32)

/* -----------------------------------------------------------------------
 * CpuFastFill / CpuFastCopy
 * --------------------------------------------------------------------- */
#define CpuFastFill(value, dest, size)                              \
do {                                                                \
    vu32 _tmp = (vu32)(value);                                      \
    CpuFastSet((const void *)&_tmp, (dest),                         \
               CPU_FAST_SET_SRC_FIXED |                             \
               (((size) / 4) & 0x1FFFFF));                          \
} while (0)

#define CpuFastFill16(value, dest, size) \
    CpuFastFill(((u32)(value) << 16) | (u16)(value), (dest), (size))

#define CpuFastFill8(value, dest, size)  \
    CpuFastFill(((u32)(value) << 24) | ((u32)(value) << 16) | \
                ((u32)(value) << 8)  | (u8)(value),  (dest), (size))

#define CpuFastCopy(src, dest, size) \
    CpuFastSet((src), (dest), (((size) / 4) & 0x1FFFFF))

/* -----------------------------------------------------------------------
 * DMA — all expand to memcpy / memset on N64.
 * The DmaSet macro writes to the software register file so that M4A
 * can use it; dma3_stub.c processes pending DMA3 requests each VBlank.
 * DMA0/1/2 are immediate — just do the copy inline.
 * --------------------------------------------------------------------- */

/* Immediate copy / fill helpers */
#define _N64_DMA_DO_COPY(src, dest, size)  memcpy((dest), (src), (size))
#define _N64_DMA_DO_FILL(value, dest, size, bit) \
do {                                              \
    vu##bit _v = (vu##bit)(value);                \
    u32 _n = (size) / sizeof(vu##bit);            \
    vu##bit *_d = (vu##bit *)(dest);              \
    while (_n--) *_d++ = _v;                      \
} while (0)

/* DmaSet — for DMA3 (used by the game's async DMA manager) we store the
 * request in the software register file; for DMA0-2 we execute immediately. */
void N64_DmaSet(int dmaNum, const void *src, void *dest, u32 control);

#define DmaSetUnchecked(dmaNum, src, dest, control) \
    N64_DmaSet((dmaNum), (const void *)(src), (void *)(dest), (u32)(control))

#define DmaSet(dmaNum, src, dest, control) \
    DmaSetUnchecked(dmaNum, src, dest, control)

/* Fill macros */
#define DMA_FILL_UNCHECKED(dmaNum, value, dest, size, bit)          \
do {                                                                \
    vu##bit _v = (vu##bit)(value);                                  \
    N64_DmaSet((dmaNum), (const void *)&_v, (void *)(dest),         \
               ((DMA_ENABLE | DMA_START_NOW | DMA_##bit##BIT |      \
                 DMA_SRC_FIXED | DMA_DEST_INC) << 16) |             \
               ((size) / ((bit) / 8)));                             \
} while (0)

#define DMA_FILL(dmaNum, value, dest, size, bit) \
    DMA_FILL_UNCHECKED(dmaNum, value, dest, size, bit)

#define DmaFill16(dmaNum, value, dest, size) \
    DMA_FILL(dmaNum, value, dest, size, 16)
#define DmaFill32(dmaNum, value, dest, size) \
    DMA_FILL(dmaNum, value, dest, size, 32)

/* Clear macros */
#define DMA_CLEAR_UNCHECKED(dmaNum, dest, size, bit) \
    DmaFill##bit(dmaNum, 0, dest, size)

#define DMA_CLEAR(dmaNum, dest, size, bit) \
    DMA_CLEAR_UNCHECKED(dmaNum, dest, size, bit)

#define DmaClear16(dmaNum, dest, size) DMA_CLEAR(dmaNum, dest, size, 16)
#define DmaClear32(dmaNum, dest, size) DMA_CLEAR(dmaNum, dest, size, 32)

/* Copy macros */
#define DMA_COPY_UNCHECKED(dmaNum, src, dest, size, bit)            \
    N64_DmaSet((dmaNum), (const void *)(src), (void *)(dest),       \
               ((DMA_ENABLE | DMA_START_NOW | DMA_##bit##BIT |      \
                 DMA_SRC_INC | DMA_DEST_INC) << 16) |               \
               ((size) / ((bit) / 8)))

#define DMA_COPY(dmaNum, src, dest, size, bit) \
    DMA_COPY_UNCHECKED(dmaNum, src, dest, size, bit)

#define DmaCopy16(dmaNum, src, dest, size) DMA_COPY(dmaNum, src, dest, size, 16)
#define DmaCopy32(dmaNum, src, dest, size) DMA_COPY(dmaNum, src, dest, size, 32)

/* Large copy / fill (chunked) */
#define DmaCopyLarge(dmaNum, src, dest, size, block, bit)   \
do {                                                        \
    const void *_s = (src);                                 \
    void *_d = (dest);                                      \
    u32 _sz = (size);                                       \
    while (_sz > (block)) {                                 \
        DmaCopy##bit(dmaNum, _s, _d, (block));              \
        _s = (const u8 *)_s + (block);                      \
        _d = (u8 *)_d + (block);                            \
        _sz -= (block);                                     \
    }                                                       \
    DmaCopy##bit(dmaNum, _s, _d, _sz);                      \
} while (0)

#define DmaCopyLarge16(dmaNum, src, dest, size, block) \
    DmaCopyLarge(dmaNum, src, dest, size, block, 16)
#define DmaCopyLarge32(dmaNum, src, dest, size, block) \
    DmaCopyLarge(dmaNum, src, dest, size, block, 32)

#define DmaFillLarge(dmaNum, value, dest, size, block, bit) \
do {                                                        \
    void *_d = (dest);                                      \
    u32 _sz = (size);                                       \
    while (_sz > (block)) {                                 \
        DmaFill##bit(dmaNum, value, _d, (block));           \
        _d = (u8 *)_d + (block);                            \
        _sz -= (block);                                     \
    }                                                       \
    DmaFill##bit(dmaNum, value, _d, _sz);                   \
} while (0)

#define DmaFillLarge16(dmaNum, value, dest, size, block) \
    DmaFillLarge(dmaNum, value, dest, size, block, 16)
#define DmaFillLarge32(dmaNum, value, dest, size, block) \
    DmaFillLarge(dmaNum, value, dest, size, block, 32)

#define DmaClearLarge(dmaNum, dest, size, block, bit) \
    DmaFillLarge(dmaNum, 0, dest, size, block, bit)
#define DmaClearLarge16(dmaNum, dest, size, block) \
    DmaClearLarge(dmaNum, dest, size, block, 16)
#define DmaClearLarge32(dmaNum, dest, size, block) \
    DmaClearLarge(dmaNum, dest, size, block, 32)

/* Defvars variants */
#define DmaCopyDefvars(dmaNum, src, dest, size, bit)    \
do {                                                    \
    const void *_s = (src);                             \
    void *_d = (dest);                                  \
    u32 _sz = (size);                                   \
    DmaCopy##bit(dmaNum, _s, _d, _sz);                  \
} while (0)

#define DmaCopy16Defvars(dmaNum, src, dest, size) \
    DmaCopyDefvars(dmaNum, src, dest, size, 16)
#define DmaCopy32Defvars(dmaNum, src, dest, size) \
    DmaCopyDefvars(dmaNum, src, dest, size, 32)

#define DmaFillDefvars(dmaNum, value, dest, size, bit)  \
do {                                                    \
    void *_d = (dest);                                  \
    u32 _sz = (size);                                   \
    DmaFill##bit(dmaNum, value, _d, _sz);               \
} while (0)

#define DmaFill16Defvars(dmaNum, value, dest, size) \
    DmaFillDefvars(dmaNum, value, dest, size, 16)
#define DmaFill32Defvars(dmaNum, value, dest, size) \
    DmaFillDefvars(dmaNum, value, dest, size, 32)

#define DmaClearDefvars(dmaNum, dest, size, bit)        \
do {                                                    \
    void *_d = (dest);                                  \
    u32 _sz = (size);                                   \
    DmaClear##bit(dmaNum, _d, _sz);                     \
} while (0)

#define DmaClear16Defvars(dmaNum, dest, size) \
    DmaClearDefvars(dmaNum, dest, size, 16)
#define DmaClear32Defvars(dmaNum, dest, size) \
    DmaClearDefvars(dmaNum, dest, size, 32)

/* DmaStop — no-op on N64 (all DMAs complete synchronously) */
#define DmaStop(dmaNum) do { (void)(dmaNum); } while (0)

/* -----------------------------------------------------------------------
 * IntrEnable — update software IME/IE registers
 * (actual N64 interrupt masking is done in interrupt.c)
 * --------------------------------------------------------------------- */
#define IntrEnable(flags)                    \
do {                                         \
    u16 _ime = REG_IME;                      \
    REG_IME = 0;                             \
    REG_IE |= (flags);                       \
    REG_IME = _ime;                          \
    N64_IntrEnable(flags);                   \
} while (0)

void N64_IntrEnable(u16 flags);

/* EnableInterrupts / DisableInterrupts are implemented as real functions
 * in src/n64/gpu_regs_n64.c to avoid macro/declaration conflicts with
 * gpu_regs.h which declares them as regular functions. */

#endif /* GUARD_N64_MACRO_H */
