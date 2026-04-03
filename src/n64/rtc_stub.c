/*
 * src/n64/rtc_stub.c
 *
 * N64 port — Real-Time Clock stub (Phase 6)
 *
 * Strategy: Option B from the port plan — fake a clock using frame counting
 * from boot.  The game uses the RTC primarily for:
 *   1. Berry growth timer (time of day)
 *   2. In-game clock display
 *   3. Seeding the RNG on startup (SeedRngWithRtc)
 *
 * We maintain a software clock:
 *   - Starts at 00:00:00 on boot (or reads from save data if available)
 *   - Increments by one second every 60 VBlanks
 *   - Provides RtcGetInfo() with the current time
 *
 * For RNG seeding: we use the N64 CP0 Count register (hardware cycle
 * counter) instead of the RTC minute count — provides good entropy.
 */

#include <string.h>
#include "global.h"
#include "rtc.h"
#include "siirtc.h"

/* -----------------------------------------------------------------------
 * Software clock state
 * --------------------------------------------------------------------- */
static struct SiiRtcInfo sSoftClock = {
    .year    = 5,    /* 2005 — matching the BuildDateTime in main.c */
    .month   = 2,
    .day     = 21,
    .weekDay = 1,    /* Monday */
    .hour    = 11,
    .minute  = 10,
    .second  = 0,
    .status  = 0x40, /* STAT1_24HOUR */
};

static u32 sVBlankCount    = 0;
static u32 sTotalSeconds   = 0;
static u32 sMinuteCount    = 0;

/* Days per month (non-leap year) */
static const u8 sDaysInMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

/* -----------------------------------------------------------------------
 * Advance the software clock by one second
 * --------------------------------------------------------------------- */
static void SoftClockTick(void)
{
    sTotalSeconds++;
    sMinuteCount = sTotalSeconds / 60;

    sSoftClock.second++;
    if (sSoftClock.second >= 60) {
        sSoftClock.second = 0;
        sSoftClock.minute++;
        if (sSoftClock.minute >= 60) {
            sSoftClock.minute = 0;
            sSoftClock.hour++;
            if (sSoftClock.hour >= 24) {
                sSoftClock.hour = 0;
                sSoftClock.weekDay = (sSoftClock.weekDay + 1) % 7;
                sSoftClock.day++;
                u8 maxDays = sDaysInMonth[sSoftClock.month - 1];
                /* Leap year: year is BCD-encoded (last 2 digits of year) */
                if (sSoftClock.month == 2) {
                    u16 yr = 2000 + ((sSoftClock.year >> 4) * 10) + (sSoftClock.year & 0xF);
                    if ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0)
                        maxDays = 29;
                }
                if (sSoftClock.day > maxDays) {
                    sSoftClock.day = 1;
                    sSoftClock.month++;
                    if (sSoftClock.month > 12) {
                        sSoftClock.month = 1;
                        sSoftClock.year = (u8)(((sSoftClock.year + 1) & 0xFF));
                    }
                }
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * RtcInit — called from AgbMain()
 * --------------------------------------------------------------------- */
void RtcInit(void)
{
    sVBlankCount = 0;
    sTotalSeconds = 0;
    sMinuteCount  = 0;
    /* sSoftClock already initialised above */
}

/* -----------------------------------------------------------------------
 * RtcGetInfo — fill a SiiRtcInfo struct with current time
 * --------------------------------------------------------------------- */
void RtcGetInfo(struct SiiRtcInfo *rtc)
{
    memcpy(rtc, &sSoftClock, sizeof(struct SiiRtcInfo));
}

/* -----------------------------------------------------------------------
 * RtcGetDateTime / RtcGetTime — convenience wrappers
 * --------------------------------------------------------------------- */
void RtcGetDateTime(struct SiiRtcInfo *rtc)
{
    RtcGetInfo(rtc);
}

void RtcGetTime(struct SiiRtcInfo *rtc)
{
    RtcGetInfo(rtc);
}

/* -----------------------------------------------------------------------
 * RtcGetMinuteCount — used by SeedRngWithRtc()
 *
 * On N64 we return the CP0 Count register value (hardware cycle counter)
 * for better entropy than minute counting from a zeroed clock.
 * --------------------------------------------------------------------- */
u32 RtcGetMinuteCount(void)
{
    u32 count;
    asm volatile ("mfc0 %0, $9" : "=r"(count));  /* CP0 Count register */
    return count ^ sTotalSeconds;
}

/* -----------------------------------------------------------------------
 * RtcIsValid — always return TRUE on N64 (we always have a soft clock)
 * --------------------------------------------------------------------- */
bool8 RtcIsValid(void)
{
    return TRUE;
}

/* -----------------------------------------------------------------------
 * RtcCalcLocalTime — convert raw RTC to local time (no-op adjustment)
 * --------------------------------------------------------------------- */
void RtcCalcLocalTime(void)
{
    /* No time zone adjustment needed */
}

/* -----------------------------------------------------------------------
 * RtcReset — reset the soft clock to midnight
 * --------------------------------------------------------------------- */
void RtcReset(void)
{
    sSoftClock.hour   = 0;
    sSoftClock.minute = 0;
    sSoftClock.second = 0;
}

/* -----------------------------------------------------------------------
 * N64_RtcVBlankTick — called each VBlank from interrupt.c to advance clock
 * (Not a GBA API function — called internally by interrupt.c)
 * --------------------------------------------------------------------- */
void N64_RtcVBlankTick(void)
{
    sVBlankCount++;
    if (sVBlankCount >= 60) {   /* 60 VBlanks = ~1 second at 60 fps */
        sVBlankCount = 0;
        SoftClockTick();
    }
}

/* -----------------------------------------------------------------------
 * SiiRtcProtect — called during DoSoftReset() in main.c
 * On GBA this write-protects the SII RTC chip; no-op on N64.
 * --------------------------------------------------------------------- */
void SiiRtcProtect(void)
{
    /* no-op */
}

/* -----------------------------------------------------------------------
 * Stub for siirtc.c functions that are excluded from the N64 build
 * --------------------------------------------------------------------- */
void SiiRtcUnprotect(void) {}
void SiiRtcGetRawInfo(struct SiiRtcInfo *rtc) { RtcGetInfo(rtc); }
bool8 SiiRtcProbe(void) { return TRUE; }
