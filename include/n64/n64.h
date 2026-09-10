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

/* -----------------------------------------------------------------------
 * Block the real GBA headers that the five includes above replace.
 *
 * Every file in the game includes global.h, which includes THIS file
 * (n64/n64.h) FIRST and then unconditionally includes gba/gba.h afterward
 * (mirroring the original GBA build, so game source needs no changes).
 * gba/gba.h in turn includes gba/defines.h, gba/io_reg.h, gba/types.h,
 * gba/macro.h and gba/syscall.h.
 *
 * None of those five files are guarded out above (unlike multiboot.h and
 * isagbprint.h below), so without this block they get fully processed a
 * second time and their plain #defines -- PLTT, VRAM, OAM, INTR_CHECK,
 * SOUND_INFO_PTR, every REG_OFFSET_ and REG_ADDR_ constant, IWRAM_DATA, etc.
 * -- silently redefine (last one wins) the N64 versions this file just set
 * up, with real GBA hardware addresses (example: PLTT = 0x05000000).
 * Nearly every source file in the game references at least one of these,
 * so this corrupted essentially all rendering/sound/interrupt state: any
 * write through the clobbered macros lands on an unmapped KUSEG address
 * with no TLB entry, which raises a TLB Refill exception; the handler for
 * that in src/n64/crt0.s just spins forever, stalling the CPU with the
 * framebuffer permanently frozen and no crash or diagnostic.  -Wall does
 * warn PLTT redefined etc for every clobbered macro, but the warnings are
 * silent by default and easy to miss among the rest of the build output. */
#ifndef GUARD_GBA_DEFINES_H
#define GUARD_GBA_DEFINES_H
#endif
#ifndef GUARD_GBA_IO_REG_H
#define GUARD_GBA_IO_REG_H
#endif
#ifndef GUARD_GBA_TYPES_H
#define GUARD_GBA_TYPES_H
#endif
#ifndef GUARD_GBA_MACRO_H
#define GUARD_GBA_MACRO_H
#endif
#ifndef GUARD_GBA_SYSCALL_H
#define GUARD_GBA_SYSCALL_H
#endif

/* Compatibility stubs for GBA-specific headers that are not needed on N64 */

/* gba/multiboot.h — full struct with all fields for source compatibility */
#ifndef GUARD_GBA_MULTIBOOT_H
#define GUARD_GBA_MULTIBOOT_H
#define MULTIBOOT_NCHILD        3
#define MULTIBOOT_HEADER_SIZE   0xc0
#define MULTIBOOT_SEND_SIZE_MIN 0x100
#define MULTIBOOT_SEND_SIZE_MAX 0x40000
struct MultiBootParam {
    u32 system_work[5];
    u8 handshake_data;
    u16 handshake_timeout;
    u8 probe_count;
    u8 client_data[MULTIBOOT_NCHILD];
    u8 palette_data;
    u8 response_bit;
    u8 client_bit;
    u8 reserved1;
    const u8 *boot_srcp;
    const u8 *boot_endp;
    const u8 *masterp;
    u8 *reserved2[MULTIBOOT_NCHILD];
    u32 system_work2[4];
    u8 sendflag;
    u8 probe_target_bit;
    u8 check_wait;
    u8 server_type;
};
#define MULTIBOOT_ERROR_04                0x04
#define MULTIBOOT_ERROR_08                0x08
#define MULTIBOOT_ERROR_0c                0x0c
#define MULTIBOOT_ERROR_40                0x40
#define MULTIBOOT_ERROR_44                0x44
#define MULTIBOOT_ERROR_48                0x48
#define MULTIBOOT_ERROR_4c                0x4c
#define MULTIBOOT_ERROR_80                0x80
#define MULTIBOOT_ERROR_84                0x84
#define MULTIBOOT_ERROR_88                0x88
#define MULTIBOOT_ERROR_8c                0x8c
#define MULTIBOOT_ERROR_NO_PROBE_TARGET   0x50
#define MULTIBOOT_ERROR_NO_DLREADY        0x60
#define MULTIBOOT_ERROR_BOOT_FAILURE      0x70
#define MULTIBOOT_ERROR_HANDSHAKE_FAILURE 0x71
#define MULTIBOOT_CONNECTION_CHECK_WAIT 15
#define MULTIBOOT_SERVER_TYPE_NORMAL 0
#define MULTIBOOT_SERVER_TYPE_QUICK  1
#define MULTIBOOT_HANDSHAKE_TIMEOUT 400
#endif

/* gba/isagbprint.h — GBA IS-AGB debug printing via JTAG; stub for N64 */
#ifndef GUARD_GBA_ISAGBPRINT_H
#define GUARD_GBA_ISAGBPRINT_H
static inline int MgbaOpen(void)    { return 0; }
static inline void MgbaClose(void)  {}
static inline void AGBPrintInit(void) {}
/* Assert/warning macros — no-ops for N64 release build */
#define AGB_ASSERT(exp)                ((void)0)
#define AGB_WARNING(exp)               ((void)0)
#define AGB_ASSERT_EX(exp, file, line) ((void)0)
#define AGB_WARNING_EX(exp, file, line)((void)0)
#endif

#endif /* GUARD_N64_H */
