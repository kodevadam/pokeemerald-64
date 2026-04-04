/*
 * src/n64/platform.c
 *
 * N64 port — platform initialisation and main entry point
 *
 * N64Main() is called from crt0.s after BSS is cleared.
 * It initialises all N64 hardware subsystems, then calls AgbMain()
 * which is the original Pokémon Emerald entry point in src/main.c.
 * AgbMain() never returns.
 */

#include <string.h>
#include "global.h"
#include "n64/asm_defs.h"
#include "n64/defines.h"
#include "main.h"
#include "malloc.h"

/* -----------------------------------------------------------------------
 * IS-Viewer64 debug output — readable live via "sc64deployer debug"
 * Physical 0x13FF0014 = write-length trigger
 * Physical 0x13FF0020 = string buffer
 * --------------------------------------------------------------------- */
void N64_DebugPrint(const char *str)
{
    volatile u32 *isv_len = (volatile u32 *)0xB3FF0014;
    volatile u8  *isv_buf = (volatile u8  *)0xB3FF0020;
    u32 len = 0;
    while (str[len]) { isv_buf[len] = (u8)str[len]; len++; }
    isv_buf[len++] = '\n';
    *isv_len = len;
}

/* -----------------------------------------------------------------------
 * Linker-exported symbols (from n64.ld)
 * --------------------------------------------------------------------- */
extern u8  __sw_palette_start[];
extern u8  __sw_vram_start[];
extern u8  __sw_oam_start[];
extern u8  __sw_ioregs_start[];
extern u8  __heap_start[];
extern u8  __stack_bottom[];
extern u8  __fb0_start[];
extern u8  __fb1_start[];
extern u8  __bss_start[];
extern u8  __bss_end[];

/* -----------------------------------------------------------------------
 * Software hardware buffer pointers (referenced by defines.h macros)
 * --------------------------------------------------------------------- */
void *__n64_pltt_buf = NULL;
void *__n64_vram_buf = NULL;
void *__n64_oam_buf  = NULL;

/* Software I/O register file */
u8 gN64IoRegs[N64_IOREGS_SIZE];

/* GBA defines.h global variable stubs */
struct SoundInfo *__n64_sound_info_ptr = NULL;
u16               __n64_intr_check     = 0;
void             *__n64_intr_vector    = NULL;

/* -----------------------------------------------------------------------
 * N64 hardware register helpers — byte-swap wrappers for big-endian MMIO
 * --------------------------------------------------------------------- */
/* N64_HW_RD / N64_HW_WR are defined in n64/asm_defs.h */

/* -----------------------------------------------------------------------
 * N64_InitMI — Memory Interface
 * Enable all MI interrupts that we care about.
 * --------------------------------------------------------------------- */
static void N64_InitMI(void)
{
    /* Set MI mode: clear DP interrupt, set upper mode */
    N64_HW_WR(N64_MI_BASE_REG, MI_MODE_REG, 0x0500);

    /* Enable VI, AI, SI, PI, DP interrupts in MI mask.
     * MI mask write format: bits 1,3,5,7,9,11 = set mask for SP,SI,AI,VI,PI,DP */
    N64_HW_WR(N64_MI_BASE_REG, MI_INTR_MASK_REG,
          (1 << 3)   /* SI set */
        | (1 << 5)   /* AI set */
        | (1 << 7)   /* VI set */
        | (1 << 9)); /* PI set */
}

/* N64_InitRI removed — do NOT reinitialize the RDRAM Interface after IPL3.
 * The HLE IPL3 (SC64 firmware / emulator) sets RI registers correctly.
 * Writing RI_CONFIG/RI_REFRESH with wrong values instantly corrupts all
 * RDRAM access, crashing the CPU before a single frame is rendered. */

/* -----------------------------------------------------------------------
 * N64_InitSP — Signal Processor
 * Halt the RSP; we do not use it for custom microcode in this port.
 * --------------------------------------------------------------------- */
static void N64_InitSP(void)
{
    /* Set SP_STATUS: halt RSP, clear broke, clear interrupt */
    N64_HW_WR(N64_SP_BASE_REG, 0x10, 0x0E);   /* SET_HALT(b1)|CLR_BROKE(b2)|CLR_INTR(b3) */
    /* Wait for RSP to halt */
    while (!(N64_HW_RD(N64_SP_BASE_REG, 0x10) & 1))
        ;
}

/* -----------------------------------------------------------------------
 * N64_InitPI — Peripheral Interface (cartridge bus)
 * Standard bus timing values for commercial N64 carts.
 * --------------------------------------------------------------------- */
static void N64_InitPI(void)
{
    N64_HW_WR(N64_PI_BASE_REG, PI_STATUS_REG,       3);     /* clear DMA busy/error */
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM1_LAT_REG, 0x40);
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM1_PWD_REG, 0x12);
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM1_PGS_REG, 0x07);
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM1_RLS_REG, 0x03);
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM2_LAT_REG, 0x05);  /* FlashRAM domain */
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM2_PWD_REG, 0x0C);
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM2_PGS_REG, 0x02);
    N64_HW_WR(N64_PI_BASE_REG, PI_BSD_DOM2_RLS_REG, 0x02);
}

/* -----------------------------------------------------------------------
 * N64_EnableCPUInterrupts — turn on MIPS interrupt handling
 * --------------------------------------------------------------------- */
static void N64_EnableCPUInterrupts(void)
{
    /* CP0 Status: set IE (global enable) and IM2 (RCP interrupt mask) */
    u32 sr;
    asm volatile (
        "mfc0  %0, $12\n\t"
        "ori   %0, %0, 0x0401\n\t"   /* IE=1, IM2=1 */
        "and   %0, %0, ~0x6\n\t"     /* clear EXL, ERL */
        "mtc0  %0, $12\n\t"
        : "=r"(sr)
    );
}

/* -----------------------------------------------------------------------
 * Forward declarations for subsystems
 * --------------------------------------------------------------------- */
extern void N64_InitVI(void);   /* vi.c      */
extern void N64_InitAI(void);   /* audio.c   */
extern void N64_InitInput(void);/* input.c   */
extern void N64_InitFlashRAM(void); /* flashram.c */

/* -----------------------------------------------------------------------
 * N64Main — platform entry point (called from crt0.s)
 * --------------------------------------------------------------------- */
void N64Main(void)
{
    /* ------------------------------------------------------------------
     * Point software hardware buffers at linker-allocated RDRAM regions
     * ------------------------------------------------------------------ */
    __n64_pltt_buf = __sw_palette_start;
    __n64_vram_buf = __sw_vram_start;
    __n64_oam_buf  = __sw_oam_start;

    /* Clear all software buffers */
    memset(__n64_pltt_buf, 0, 0x400);
    memset(__n64_vram_buf, 0, 0x18000);
    memset(__n64_oam_buf,  0, 0x400);
    memset(gN64IoRegs,     0, sizeof(gN64IoRegs));

    /* REG_KEYINPUT (0x130) is active-LOW: all bits set = no keys pressed.
     * The zero-init above sets it to 0x0000 (all keys "pressed"), which
     * would cause ReadKeys() to see spurious button presses at startup. */
    *(u16 *)(gN64IoRegs + 0x130) = 0x03FF;

    /* ------------------------------------------------------------------
     * Hardware initialisation — order matters:
     *   1. MI (interrupt controller) first so sub-systems can register
     *   2. RI (RDRAM interface)
     *   3. PI (cartridge bus) — needed for flash save init
     *   4. SP (halt RSP)
     *   5. VI (video) — sets up framebuffers and VI registers
     *   6. AI (audio) — sets up AI DMA and sample rate
     *   7. Input (SI) — initiates first controller poll
     *   8. FlashRAM — detects save media
     *   9. CPU interrupts — enable last
     * ------------------------------------------------------------------ */
    N64_DebugPrint("[N64] N64Main: start");
    N64_InitMI();   N64_DebugPrint("[N64] InitMI done");
    N64_InitPI();   N64_DebugPrint("[N64] InitPI done");
    N64_InitSP();   N64_DebugPrint("[N64] InitSP done");
    N64_InitVI();   N64_DebugPrint("[N64] InitVI done");
    N64_InitAI();   N64_DebugPrint("[N64] InitAI done");
    N64_InitInput();N64_DebugPrint("[N64] InitInput done");
    N64_InitFlashRAM(); N64_DebugPrint("[N64] InitFlashRAM done");
    N64_EnableCPUInterrupts(); N64_DebugPrint("[N64] CPU interrupts enabled");

    /* ------------------------------------------------------------------
     * Seed the software TM1CNT_L register with the N64 CP0 Count register
     * so that SeedRngAndSetTrainerId() in main.c gets a non-zero seed.
     * CP0 Count increments at 46.875 MHz from CPU reset; by this point
     * a few milliseconds have elapsed giving a non-deterministic value.
     * ------------------------------------------------------------------ */
    {
        u32 count;
        asm volatile ("mfc0 %0, $9" : "=r"(count));
        extern u8 gN64IoRegs[];
        *(u16 *)(gN64IoRegs + 0x104) = (u16)(count ^ (count >> 16));
    }

    /* ------------------------------------------------------------------
     * Hand off to the game's main function.
     * AgbMain() contains the game loop and never returns.
     * ------------------------------------------------------------------ */
    N64_DebugPrint("[N64] calling AgbMain");
    AgbMain();

    /* Should never reach here */
    for (;;)
        ;
}

/* -----------------------------------------------------------------------
 * N64_DmaSet — called by the DmaSet macro in macro.h
 *
 * For DMA channels 0-2 (used by M4A and the graphics engine for small
 * immediate transfers), execute synchronously via memcpy/memset.
 * For DMA channel 3 (the game's async VRAM DMA manager), store the
 * request in a pending-request queue and process it during VBlank.
 * --------------------------------------------------------------------- */

#define DMA3_QUEUE_SIZE 64

typedef struct {
    const void *src;
    void       *dst;
    u32         control;
} Dma3Request;

static Dma3Request sDma3Queue[DMA3_QUEUE_SIZE];
volatile int sDma3Head = 0;
volatile int sDma3Tail = 0;

void N64_DmaSet(int dmaNum, const void *src, void *dst, u32 control)
{
    u32 ctrl   = control >> 16;
    u32 count  = control & 0xFFFF;
    int is32   = (ctrl & DMA_32BIT) != 0;
    int fixed  = (ctrl & DMA_SRC_FIXED) != 0;
    u32 bytes  = count * (is32 ? 4 : 2);

    if (dmaNum == 3) {
        /* Queue for VBlank processing */
        int next = (sDma3Head + 1) % DMA3_QUEUE_SIZE;
        if (next != sDma3Tail) {
            sDma3Queue[sDma3Head].src     = src;
            sDma3Queue[sDma3Head].dst     = dst;
            sDma3Queue[sDma3Head].control = control;
            sDma3Head = next;
        }
        return;
    }

    /* Immediate execution for DMA0-2 */
    if (!(ctrl & DMA_ENABLE))
        return;

    if (fixed) {
        if (is32) {
            u32 val = *(const u32 *)src;
            u32 *d = (u32 *)dst;
            for (u32 i = 0; i < count; i++) *d++ = val;
        } else {
            u16 val = *(const u16 *)src;
            u16 *d = (u16 *)dst;
            for (u32 i = 0; i < count; i++) *d++ = val;
        }
    } else {
        memcpy(dst, src, bytes);
    }
}

/* ProcessDma3Requests — called from VBlank handler (replaces ProcessDma3Requests
 * in src/dma3_manager.c; the original is excluded from the N64 build) */
void ProcessDma3Requests(void)
{
    while (sDma3Tail != sDma3Head) {
        Dma3Request *req = &sDma3Queue[sDma3Tail];
        u32 ctrl  = req->control >> 16;
        u32 count = req->control & 0xFFFF;
        int is32  = (ctrl & DMA_32BIT) != 0;
        int fixed = (ctrl & DMA_SRC_FIXED) != 0;
        u32 bytes = count * (is32 ? 4 : 2);

        if (ctrl & DMA_ENABLE) {
            if (fixed) {
                if (is32) {
                    u32 val = *(const u32 *)req->src;
                    u32 *d = (u32 *)req->dst;
                    for (u32 i = 0; i < count; i++) *d++ = val;
                } else {
                    u16 val = *(const u16 *)req->src;
                    u16 *d = (u16 *)req->dst;
                    for (u32 i = 0; i < count; i++) *d++ = val;
                }
            } else {
                memcpy(req->dst, req->src, bytes);
            }
        }

        sDma3Tail = (sDma3Tail + 1) % DMA3_QUEUE_SIZE;
    }
}

/* -----------------------------------------------------------------------
 * Stubs for PI / AI done callbacks referenced in interrupt.c
 * (full implementations are in flashram.c and audio.c)
 * --------------------------------------------------------------------- */
__attribute__((weak)) void N64_PiDmaDone(void) {}
