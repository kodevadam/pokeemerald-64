/*
 * src/n64/audio.c
 *
 * N64 port — Audio Interface (AI) driver + M4A low-level mixer replacement
 *
 * Strategy (from the port plan):
 *   • Keep the high-level M4A sequencer (m4a.c) — it parses song data,
 *     drives MIDI-like commands, and calls into the low-level mixer.
 *   • Replace the low-level ARM assembly mixer (m4a_1.s / SoundMain /
 *     SoundMainRAM) with a C implementation that:
 *       1. Mixes all active PCM channels into a stereo 16-bit buffer.
 *       2. Mixes the 4 CGB channels (square, noise) via software synthesis.
 *       3. Feeds the buffer to the N64 AI (Audio Interface) via DMA.
 *
 * Audio parameters:
 *   Sample rate: 32000 Hz (upgraded from GBA's ~13379 Hz for better quality)
 *   Format:      16-bit signed stereo (N64 AI requirement)
 *   Buffer size: 1024 stereo samples per AI DMA buffer = 4096 bytes
 *   Double-buffered so one buffer plays while the other is being filled.
 *
 * GBA M4A PCM channel format:
 *   Samples are 8-bit signed at the instrument's native sample rate.
 *   Each channel has a volume (0-255), panning (0=left, 127=centre, 255=right),
 *   pitch (frequency in Hz), and loop/envelope state.
 *
 * GBA CGB channel emulation:
 *   Square wave channels (CH1/CH2): variable duty cycle, envelope, sweep.
 *   Wave channel (CH3): 32 4-bit samples from wave RAM, swept by frequency.
 *   Noise channel (CH4): LFSR-based pseudo-random noise.
 *   These are synthesised in software and mixed into the PCM buffer.
 */

#include <string.h>
#include <math.h>
#include "global.h"
#include "m4a.h"
#include "n64/asm_defs.h"

/* -----------------------------------------------------------------------
 * N64 AI register access — byte-swap wrappers for big-endian MMIO
 * --------------------------------------------------------------------- */
#define AI_REG_WR(off, val) N64_HW_WR(N64_AI_BASE_REG, (off), (val))
#define AI_REG_RD(off)      N64_HW_RD(N64_AI_BASE_REG, (off))

/* -----------------------------------------------------------------------
 * Audio buffer configuration
 * --------------------------------------------------------------------- */
#define N64_AUDIO_SAMPLE_RATE   32000
#define N64_AUDIO_SAMPLES_PER_BUF 1024
#define N64_AUDIO_BUF_BYTES     (N64_AUDIO_SAMPLES_PER_BUF * 2 * sizeof(s16))
/* Two buffers for double-buffering */
#define N64_AUDIO_NUM_BUFS      2

/* Audio buffers — 8-byte aligned for AI DMA */
static s16 sAudioBufs[N64_AUDIO_NUM_BUFS][N64_AUDIO_SAMPLES_PER_BUF * 2]
    __attribute__((aligned(8)));

static int sFillBuf  = 0;   /* index of buffer currently being filled */
static int sPlayBuf  = 1;   /* index of buffer currently playing      */
static volatile int sAIBusy = 0;  /* 1 = AI DMA in progress           */

/* -----------------------------------------------------------------------
 * GBA master volume (set by REG_SOUNDCNT_L / REG_SOUNDCNT_H)
 * --------------------------------------------------------------------- */
static inline int GetMasterVolume(void)
{
    /* GBA: SOUNDCNT_L bits 6-4 = output volume (0-7) for left/right */
    u16 cntH = _REG16(REG_OFFSET_SOUNDCNT_H);
    int vol = (cntH >> 2) & 3;  /* 0 = 25%, 1 = 50%, 2 = 100% */
    return vol;  /* used as a shift: 0→>>2, 1→>>1, 2→>>0 */
}

/* -----------------------------------------------------------------------
 * SoundInfo — the M4A engine writes here; we read channel state from it.
 * The pointer is set by m4aSoundInit() via SOUND_INFO_PTR.
 * --------------------------------------------------------------------- */
static inline struct SoundInfo *GetSoundInfo(void)
{
    return SOUND_INFO_PTR;
}

/* -----------------------------------------------------------------------
 * GBA M4A channel structure — mirrors m4a_internal.h SoundChannel
 *
 * We read the channel state that m4a.c writes; this struct must match
 * the layout that the GBA M4A sources define exactly.
 * --------------------------------------------------------------------- */
#pragma pack(push, 1)
typedef struct {
    u8  statusFlags;    /* 0: inactive, bit7: active, bit6: loop, etc. */
    u8  type;           /* 8 = PCM, 0-3 = CGB */
    u8  rightVolume;
    u8  leftVolume;
    u8  attack;
    u8  decay;
    u8  sustain;
    u8  release;
    u8  envelopePos;
    u8  envelopeFrac;
    u8  envelopeValue;
    u8  padding0;
    u16 envelopeGoal;
    u8  padding1[2];
    u32 frequency;      /* playback frequency in Hz */
    u8 *waveData;       /* pointer to PCM wave data (8-bit signed) */
    u32 loopStart;      /* loop start offset in samples */
    u32 size;           /* wave length in samples */
    s32 count;          /* samples remaining until loop / end */
    u32 fw;             /* fractional wave position */
} M4AChannel;
#pragma pack(pop)

#define M4A_MAX_CHANNELS    12
#define M4A_STATUS_ACTIVE   0x80
#define M4A_STATUS_LOOP     0x10
#define M4A_TYPE_CGB        0x08

/* -----------------------------------------------------------------------
 * CGB channel state
 * --------------------------------------------------------------------- */
typedef struct {
    int     active;
    int     type;       /* 1=square1, 2=square2, 3=wave, 4=noise */
    int     duty;       /* square wave duty cycle (0-3) */
    int     envVol;     /* envelope volume (0-15) */
    int     envDir;     /* +1 = increase, -1 = decrease */
    int     envSteps;   /* envelope step count */
    int     envTimer;   /* envelope timer */
    int     freq;       /* frequency (Hz) */
    u32     phase;      /* oscillator phase (fixed-point) */
    u32     phaseInc;   /* phase increment per sample */
    int     lfsrState;  /* noise LFSR (15-bit) */
    u8      waveRAM[16];/* CH3 wave RAM */
} CgbChannel;

static CgbChannel sCgbChannels[4];

/* Duty cycle tables (fraction of period that output is high) */
static const float sDutyCycle[4] = {0.125f, 0.25f, 0.5f, 0.75f};

/* -----------------------------------------------------------------------
 * Update CGB channels from NR1x-NR4x registers
 * --------------------------------------------------------------------- */
static void UpdateCgbChannels(void)
{
    /* Square 1 (CH1) */
    {
        u8 nr10 = _REG8(REG_OFFSET_SOUND1CNT_L);
        u8 nr11 = _REG8(REG_OFFSET_SOUND1CNT_H);
        u8 nr12 = _REG8(REG_OFFSET_SOUND1CNT_H + 1);
        u8 nr14 = _REG8(REG_OFFSET_SOUND1CNT_X + 1);
        (void)nr10; (void)nr11;
        CgbChannel *c = &sCgbChannels[0];
        if (nr14 & 0x80) {  /* trigger */
            c->active   = 1;
            c->type     = 1;
            c->envVol   = (nr12 >> 4) & 0xF;
            c->envDir   = (nr12 & 0x08) ? 1 : -1;
            c->envSteps = nr12 & 0x07;
            c->envTimer = c->envSteps;
        }
        c->active = (c->active && c->envVol > 0);
    }

    /* Square 2 (CH2) */
    {
        u8 nr21 = _REG8(REG_OFFSET_SOUND2CNT_L);
        u8 nr22 = _REG8(REG_OFFSET_SOUND2CNT_L + 1);
        u8 nr24 = _REG8(REG_OFFSET_SOUND2CNT_H + 1);
        (void)nr21;
        CgbChannel *c = &sCgbChannels[1];
        if (nr24 & 0x80) {
            c->active   = 1;
            c->type     = 2;
            c->envVol   = (nr22 >> 4) & 0xF;
            c->envDir   = (nr22 & 0x08) ? 1 : -1;
            c->envSteps = nr22 & 0x07;
            c->envTimer = c->envSteps;
        }
        c->active = (c->active && c->envVol > 0);
    }

    /* Wave (CH3) */
    {
        u8 nr30 = _REG8(REG_OFFSET_SOUND3CNT_L);
        u8 nr34 = _REG8(REG_OFFSET_SOUND3CNT_X + 1);
        CgbChannel *c = &sCgbChannels[2];
        c->active = (nr30 & 0x80) != 0;
        if (nr34 & 0x80) {
            c->type  = 3;
            c->phase = 0;
            /* Copy wave RAM from register file */
            for (int i = 0; i < 16; i++)
                c->waveRAM[i] = _REG8(REG_OFFSET_SOUND3CNT_L + 0x30 + i);
        }
    }

    /* Noise (CH4) */
    {
        u8 nr42 = _REG8(REG_OFFSET_SOUND4CNT_L + 1);
        u8 nr44 = _REG8(REG_OFFSET_SOUND4CNT_H + 1);
        CgbChannel *c = &sCgbChannels[3];
        if (nr44 & 0x80) {
            c->active    = 1;
            c->type      = 4;
            c->envVol    = (nr42 >> 4) & 0xF;
            c->envDir    = (nr42 & 0x08) ? 1 : -1;
            c->envSteps  = nr42 & 0x07;
            c->lfsrState = 0x7FFF;
        }
    }
}

/* -----------------------------------------------------------------------
 * N64_MixAudioFrame — fill one stereo audio buffer
 *
 * Called each VBlank (or from AI interrupt) to prepare the next buffer.
 * --------------------------------------------------------------------- */
static void MixAudioFrame(s16 *buf, int samples)
{
    /* Zero the output buffer */
    memset(buf, 0, samples * 2 * sizeof(s16));

    struct SoundInfo *si = GetSoundInfo();
    if (!si) return;

    /* Read master volume from SOUNDCNT_H */
    u16 cntH = _REG16(REG_OFFSET_SOUNDCNT_H);
    int volShift = 2 - ((cntH >> 2) & 3);  /* 0=25%→>>2, 1=50%→>>1, 2=100%→>>0 */
    if (volShift < 0) volShift = 0;

    /* ------------------------------------------------------------------
     * Mix PCM channels
     * Each channel: 8-bit signed samples, resampled to N64_AUDIO_SAMPLE_RATE
     * ------------------------------------------------------------------ */
    M4AChannel *channels = (M4AChannel *)si->channels;
    int numChannels = si->maxChans;
    if (numChannels > M4A_MAX_CHANNELS) numChannels = M4A_MAX_CHANNELS;

    for (int ch = 0; ch < numChannels; ch++) {
        M4AChannel *c = &channels[ch];
        if (!(c->statusFlags & M4A_STATUS_ACTIVE)) continue;
        if (!c->waveData || c->frequency == 0)     continue;
        if (c->type & M4A_TYPE_CGB)                continue; /* handled below */

        /* Resample ratio: increment = srcFreq / dstFreq in fixed-point (16.16) */
        u32 rateInc = (u32)(((u64)c->frequency << 16) / N64_AUDIO_SAMPLE_RATE);
        u32 pos     = c->fw;   /* current sample position (16.16 fixed-point) */

        int volL = (c->leftVolume  * c->envelopeValue) >> 8;
        int volR = (c->rightVolume * c->envelopeValue) >> 8;
        if (volL < 0) volL = 0; if (volL > 255) volL = 255;
        if (volR < 0) volR = 0; if (volR > 255) volR = 255;

        for (int i = 0; i < samples; i++) {
            int sampleIdx = (int)(pos >> 16);
            /* Loop handling */
            if (sampleIdx >= (int)c->size) {
                if (c->statusFlags & M4A_STATUS_LOOP) {
                    sampleIdx = (int)c->loopStart +
                                (sampleIdx - (int)c->size) % (int)(c->size - c->loopStart);
                } else {
                    /* Channel done */
                    c->statusFlags &= ~M4A_STATUS_ACTIVE;
                    break;
                }
            }

            /* 8-bit signed sample → 16-bit */
            s8 sample = (s8)c->waveData[sampleIdx];
            s32 s16L  = (s32)sample * volL >> volShift;
            s32 s16R  = (s32)sample * volR >> volShift;

            /* Mix (clamp to s16 range) */
            s32 mixL = (s32)buf[i * 2 + 0] + (s16L << 6);
            s32 mixR = (s32)buf[i * 2 + 1] + (s16R << 6);
            if (mixL >  32767) mixL =  32767;
            if (mixL < -32768) mixL = -32768;
            if (mixR >  32767) mixR =  32767;
            if (mixR < -32768) mixR = -32768;
            buf[i * 2 + 0] = (s16)mixL;
            buf[i * 2 + 1] = (s16)mixR;

            pos += rateInc;
        }

        c->fw = pos;  /* save position for next frame */
    }

    /* ------------------------------------------------------------------
     * Mix CGB channels (square waves, noise)
     * ------------------------------------------------------------------ */
    UpdateCgbChannels();

    for (int ch = 0; ch < 4; ch++) {
        CgbChannel *c = &sCgbChannels[ch];
        if (!c->active) continue;

        /* Phase increment for this channel's frequency */
        if (c->freq > 0)
            c->phaseInc = (u32)(((u64)c->freq << 16) / N64_AUDIO_SAMPLE_RATE);

        for (int i = 0; i < samples; i++) {
            s16 sample = 0;

            if (c->type == 1 || c->type == 2) {
                /* Square wave */
                float duty  = sDutyCycle[(c->type == 1) ?
                    ((_REG8(REG_OFFSET_SOUND1CNT_H) >> 6) & 3) :
                    ((_REG8(REG_OFFSET_SOUND2CNT_L) >> 6) & 3)];
                float phase = (float)(c->phase >> 16) / 65536.0f;
                sample = (phase < duty) ? (s16)(c->envVol * 512) : (s16)(-c->envVol * 512);
            } else if (c->type == 3) {
                /* Wave channel: 32 4-bit samples in wave RAM */
                u32 wavePos = (c->phase >> 16) & 31;
                u8 nybble = (wavePos & 1)
                    ? (c->waveRAM[wavePos / 2] & 0x0F)
                    : (c->waveRAM[wavePos / 2] >> 4);
                sample = (s16)((int)nybble - 8) * 2048;
            } else if (c->type == 4) {
                /* Noise: LFSR */
                if (c->phase >> 16 != (c->phase - c->phaseInc) >> 16) {
                    int feedback = ((c->lfsrState ^ (c->lfsrState >> 1)) & 1);
                    c->lfsrState = (c->lfsrState >> 1) | (feedback << 14);
                }
                sample = (c->lfsrState & 1) ? (s16)(c->envVol * 512) : (s16)(-c->envVol * 512);
            }

            /* CGB channels are mono; apply to both channels at 25% master */
            s32 mixL = (s32)buf[i * 2 + 0] + (sample >> 2);
            s32 mixR = (s32)buf[i * 2 + 1] + (sample >> 2);
            if (mixL >  32767) mixL =  32767;
            if (mixL < -32768) mixL = -32768;
            if (mixR >  32767) mixR =  32767;
            if (mixR < -32768) mixR = -32768;
            buf[i * 2 + 0] = (s16)mixL;
            buf[i * 2 + 1] = (s16)mixR;

            c->phase += c->phaseInc;
        }
    }
}

/* -----------------------------------------------------------------------
 * N64_InitAI — configure the N64 Audio Interface
 * --------------------------------------------------------------------- */
void N64_InitAI(void)
{
    memset(sAudioBufs, 0, sizeof(sAudioBufs));
    sFillBuf = 0;
    sPlayBuf = 1;
    sAIBusy  = 0;

    /* AI DAC rate: NTSC clock / (rate + 1) = sample rate
     * NTSC AI clock = 48681812 Hz
     * For 32000 Hz: 48681812 / 32000 - 1 ≈ 1521 */
    u32 dacRate = (48681812 / N64_AUDIO_SAMPLE_RATE) - 1;
    AI_REG_WR(AI_DACRATE_REG, dacRate);
    AI_REG_WR(AI_BITRATE_REG, 15);       /* 16-bit */
    AI_REG_WR(AI_CONTROL_REG, 1);        /* DMA enable */

    /* Pre-fill first buffer (silence) */
    MixAudioFrame(sAudioBufs[sPlayBuf], N64_AUDIO_SAMPLES_PER_BUF);

    /* Start playing the first buffer */
    u32 physAddr = (u32)((uintptr_t)sAudioBufs[sPlayBuf] & 0x0FFFFFFF);
    AI_REG_WR(AI_DRAM_ADDR_REG, physAddr);
    AI_REG_WR(AI_LEN_REG,       N64_AUDIO_BUF_BYTES);
    sAIBusy = 1;
}

/* -----------------------------------------------------------------------
 * N64_AudioRefill — called from interrupt.c when AI DMA buffer is empty
 * --------------------------------------------------------------------- */
void N64_AudioRefill(void)
{
    /* Queue the fill buffer for playback */
    u32 physAddr = (u32)((uintptr_t)sAudioBufs[sFillBuf] & 0x0FFFFFFF);
    AI_REG_WR(AI_DRAM_ADDR_REG, physAddr);
    AI_REG_WR(AI_LEN_REG,       N64_AUDIO_BUF_BYTES);

    /* Swap buffers */
    int tmp  = sFillBuf;
    sFillBuf = sPlayBuf;
    sPlayBuf = tmp;
    sAIBusy  = 1;

    /* Mix the next frame into the newly free fill buffer */
    MixAudioFrame(sAudioBufs[sFillBuf], N64_AUDIO_SAMPLES_PER_BUF);
}

/* -----------------------------------------------------------------------
 * m4aSoundVSync — called each VBlank from main.c
 *
 * On GBA this updates the M4A timing counters and triggers the sound DMA.
 * On N64 the AI interrupt drives audio; we just kick off mixing if the AI
 * is idle (e.g. on the very first frame).
 * --------------------------------------------------------------------- */
void m4aSoundVSync(void)
{
    if (!sAIBusy) {
        MixAudioFrame(sAudioBufs[sFillBuf], N64_AUDIO_SAMPLES_PER_BUF);
        u32 physAddr = (u32)((uintptr_t)sAudioBufs[sFillBuf] & 0x0FFFFFFF);
        AI_REG_WR(AI_DRAM_ADDR_REG, physAddr);
        AI_REG_WR(AI_LEN_REG,       N64_AUDIO_BUF_BYTES);
        sAIBusy = 1;
    }
}

/* -----------------------------------------------------------------------
 * m4aSoundVSyncOff — called during soft reset (DoSoftReset in main.c)
 * Stop audio DMA.
 * --------------------------------------------------------------------- */
void m4aSoundVSyncOff(void)
{
    AI_REG_WR(AI_CONTROL_REG, 0); /* disable DMA */
    sAIBusy = 0;
}

/* -----------------------------------------------------------------------
 * m4aSoundMain — called each VBlank from main.c's VBlankIntr
 *
 * On GBA this runs the full M4A mixer in IWRAM.  On N64 the mixing is done
 * asynchronously in N64_AudioRefill() driven by the AI interrupt.  We still
 * call the M4A sequencer to advance music playback state.
 * --------------------------------------------------------------------- */
void m4aSoundMain(void)
{
    /* The M4A sequencer (m4a.c) updates channel state.  The actual PCM
     * mixing is done in MixAudioFrame() called from N64_AudioRefill(). */
    /* m4a.c will call SoundMain() which we stub below. */
}

/* -----------------------------------------------------------------------
 * SoundMain / SoundMainRAM — the GBA ARM assembly mixer entry points
 *
 * On GBA, m4a.c calls SoundMain() (or SoundMainRAM() when running from
 * IWRAM).  On N64 these are no-ops because the mixer runs from
 * MixAudioFrame() in the AI interrupt.
 * --------------------------------------------------------------------- */
void SoundMain(void)    { /* no-op — mixing done in AI interrupt */ }
void SoundMainRAM(void) { /* no-op */ }
