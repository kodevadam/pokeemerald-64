#ifndef GUARD_N64_H
#define GUARD_N64_H

/*
 * include/n64/n64.h
 *
 * Master N64 platform header — included by the game's global.h in place
 * of include/gba/gba.h when building for N64.
 *
 * The include order here mirrors what gba/gba.h does so that any file
 * that includes global.h gets the full set of platform definitions.
 */

#include "n64/types.h"
#include "n64/defines.h"
#include "n64/io_reg.h"
#include "n64/macro.h"
#include "n64/syscall.h"

/* Compatibility stubs for GBA-specific headers that are not needed on N64 */

/* gba/multiboot.h — GBA multi-boot protocol (GameCube link); not used on N64 */
#ifndef GUARD_GBA_MULTIBOOT_H
#define GUARD_GBA_MULTIBOOT_H
struct MultiBootParam { u32 reserved[16]; };
#endif

/* gba/isagbprint.h — GBA IS-AGB debug printing via JTAG; stub for N64 */
#ifndef GUARD_GBA_ISAGBPRINT_H
#define GUARD_GBA_ISAGBPRINT_H
static inline int MgbaOpen(void)    { return 0; }
static inline void MgbaClose(void)  {}
static inline void AGBPrintInit(void) {}
#endif

#endif /* GUARD_N64_H */
