/*
 * src/n64/dma3_stub.c
 *
 * N64 port — stubs for DMA3 manager functions
 *
 * The game's src/dma3_manager.c is excluded from the N64 build (it uses
 * REG_DMA3* hardware registers directly).  The functions it exports are
 * re-implemented here:
 *
 *   - ClearDma3Requests() — clear the pending DMA3 queue
 *   - RequestDma3Copy()   — queue a DMA3 copy request
 *   - RequestDma3Fill()   — queue a DMA3 fill request
 *   - ProcessDma3Requests() — process all pending requests (called VBlank)
 *
 * The actual queue and ProcessDma3Requests() implementation lives in
 * platform.c (where N64_DmaSet queues requests).  We forward here.
 */

#include <string.h>
#include "global.h"
#include "dma3.h"

/* Forward to platform.c */
extern void ProcessDma3Requests(void);

void ClearDma3Requests(void)
{
    /* platform.c's DMA3 queue uses a circular buffer.  Clearing it is
     * just advancing the tail to match the head. */
    extern volatile int sDma3Head;
    extern volatile int sDma3Tail;
    sDma3Tail = sDma3Head;
}

/* RequestDma3Copy — queue an asynchronous memcpy for the next VBlank */
void RequestDma3Copy(const void *src, void *dst, u16 size, u32 mode)
{
    /* On N64 there's no timing benefit to deferring; execute immediately */
    (void)mode;
    memcpy(dst, src, size);
}

void RequestDma3Fill(s32 value, void *dst, u16 size, u32 mode)
{
    (void)mode;
    memset(dst, value & 0xFF, size);
}

/* These are aliases in the original GBA code; provide them for link compat */
void RequestDma3CopyWithMode(const void *src, void *dst, u16 size, u32 mode)
{
    RequestDma3Copy(src, dst, size, mode);
}
