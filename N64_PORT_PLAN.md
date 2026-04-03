# Plan: Porting pokeemerald to Nintendo 64

## Hardware Comparison

| Feature | GBA | N64 |
|---|---|---|
| CPU | ARM7TDMI 16.78MHz (32-bit) | VR4300 93.75MHz (64-bit MIPS) |
| RAM | 32KB IWRAM + 256KB EWRAM | 4MB RDRAM (8MB w/ expansion) |
| VRAM | 96KB (hardware tile engine) | Shared RDRAM framebuffer |
| Graphics | 2D tile engine, 4 BG layers, 128 sprites | RCP (RSP + RDP), 3D-capable |
| Display | 240x160 | 320x240 (low-res) or 640x480 |
| Audio | DMA to FIFO, M4A engine | RSP audio microcode |
| Input | D-pad + A/B/L/R/Start/Select | Analog stick + D-pad + A/B/Z/L/R/C-buttons/Start |
| Save | Cartridge Flash (128KB) | EEPROM/SRAM/Flash/Controller Pak |
| ROM Max | 32MB | 64MB |

---

## Phase 0: Toolchain & Build System

**Estimated scope: ~500 lines of Makefile/linker scripts**

1. **Replace ARM toolchain with MIPS N64 toolchain** - Use libdragon (open-source N64 SDK) or the proprietary N64 SDK with GCC MIPS cross-compiler (`mips64-elf-gcc`)
2. **Rewrite the Makefile** - Replace `arm-none-eabi-*` toolchain references with `mips64-elf-*`, update compiler flags (`-march=vr4300 -mtune=vr4300`), change output from `.gba` to `.z64`
3. **Replace linker script** (`ld_script.txt`) - GBA memory map (EWRAM `0x02000000`, IWRAM `0x03000000`, ROM `0x08000000`) must become N64 memory map (RDRAM `0x80000000`, ROM `0xB0000000`)
4. **Replace ROM header** (`src/rom_header.s`) - N64 ROMs require a different 64-byte header with PI BSD domain settings, clock rate, entry point, CRC checksums
5. **Build asset pipeline** - Convert `.4bpp`/`.8bpp` tile data and `.gbapal` palettes to N64-compatible formats (RGBA16/RGBA32 textures or a custom indexed format the RDP can consume)

---

## Phase 1: Hardware Abstraction Layer (HAL)

**This is the critical foundation. Every other phase depends on it.**

### 1a. Memory & CPU Primitives

Files to replace/rewrite:

- `include/gba/defines.h` (96 lines) - All memory addresses, display constants
- `include/gba/types.h` - Volatile register types become regular types or MMIO macros for N64
- `include/gba/io_reg.h` (777 lines) - Complete replacement. All REG_* defines are GBA MMIO. Create an abstraction that maps these to N64 equivalents or software state
- `include/gba/macro.h` (150 lines) - CpuSet, CpuFastSet, DmaFill, DmaCopy become memcpy/memset or RSP DMA ops
- `src/crt0.s` (127 lines) - Complete rewrite in MIPS. N64 boot sequence: set up TLB, initialize RSP/RDP, configure VI, jump to main()
- `libagbsyscall/libagbsyscall.s` (432 lines) - Complete rewrite. Every GBA BIOS call needs a C or MIPS implementation:
  - VBlankIntrWait -> poll VI interrupt or use N64 OS message queue
  - CpuSet/CpuFastSet -> memcpy/memset
  - LZ77UnCompWram/Vram -> software LZ77 decompressor (already exists in some decomp tools)
  - Div, Sqrt, ArcTan, ArcTan2 -> C math functions (N64 CPU has hardware divide)
  - BgAffineSet/ObjAffineSet -> C implementations of the affine matrix math
  - SoftReset -> N64 warm reboot sequence

### 1b. Interrupt & Timer System

Files to replace:

- `src/crt0.s` IntrMain handler - N64 uses a different interrupt model (MIPS Cause/Status registers, MI interrupts). Rewrite to dispatch VI (vblank), SI (controller), AI (audio), PI (cartridge) interrupts
- Timer usage (REG_TM0-3) - N64 has the CPU Count register (increments every other cycle at ~46.875MHz). Implement timer abstraction using Count register or VI-based timing
- REG_IME/IE/IF interrupt enable/flag registers - Map to MIPS CP0 Status register IE bit and N64 MI_INTR_MASK

### 1c. DMA Replacement

Files to replace:

- `src/dma3_manager.c` (manages async VRAM transfers) - N64 has PI DMA (cart->RDRAM) and SP DMA (RDRAM<->RSP DMEM). Replace GBA DMA with either direct memcpy or PI/SP DMA as appropriate
- All DmaCopy16/32, DmaFill16/32 macros in `include/gba/macro.h` - Simple memcpy/memset wrappers

---

## Phase 2: Graphics Engine (Largest phase)

**~421K lines of C reference GBA graphics. This is the hardest part.**

The GBA has a hardware tile engine; the N64 has a programmable rasterizer (RDP). The core strategy is: **implement a software 2D tile compositor that renders to an RDRAM framebuffer, then display it via the N64 Video Interface (VI).**

### 2a. Display & Framebuffer Setup

- Configure N64 VI for 320x240 @ 16-bit color (RGBA5551) - closest to GBA's RGB555
- Allocate double-buffered framebuffers in RDRAM (~150KB each at 320x240x16bpp)
- The GBA's 240x160 output will be centered in the 320x240 frame with letterboxing (40px top/bottom bars), OR scaled up
- For 100% faithful: render at 240x160 internally, then scale/center on display

### 2b. Background Tile Renderer (Software)

Files affected: `src/bg.c` (1,247 lines), `src/gpu_regs.c`, all code using REG_BGxCNT/HOFS/VOFS

Implement a software tile renderer that:

1. Reads the same tilemap/charblock data structures the game already builds
2. For each scanline (or per-frame), composites 4 BG layers by reading tile indices from screenblocks, looking up 8x8 tile pixel data from charblocks, applying palette lookup
3. Handles: scroll offsets (HOFS/VOFS), priority ordering, 4bpp/8bpp modes, screen sizes (256x256 to 512x512), tile flipping
4. For affine BGs (Mode 1/2): apply 2x2 matrix transform per pixel (PA/PB/PC/PD registers + reference point X/Y)

Performance target: The N64 CPU at 93.75MHz is ~5.6x faster than the GBA CPU. Software tile compositing of 240x160 pixels with 4 layers is feasible. The RDP can also help via textured rectangle commands for tiles.

### 2c. Sprite/OAM Renderer (Software)

Files affected: `src/sprite.c` (1,759 lines), all OAM users

Implement a software sprite renderer that:

1. Reads the 64-entry OAM buffer the game already builds via BuildOamBuffer()
2. For each sprite: decode shape/size, fetch tile data from OBJ VRAM region, apply palette, handle h/v flip, apply affine matrix if affine mode enabled
3. Composite sprites onto the framebuffer respecting priority vs BG layers
4. Handle sprite windowing and semi-transparency

### 2d. Palette System

Files affected: `src/palette.c` (1,042 lines)

- GBA uses 512 bytes of palette RAM (256 BG colors + 256 OBJ colors, RGB555)
- On N64: keep palette buffers in RDRAM at the same logical layout
- TransferPlttBuffer() currently DMAs to palette RAM hardware - change to just update the RDRAM palette array
- The software renderer reads from this palette array during compositing
- All fade/blend/tint operations in palette.c already work on the RAM buffer (gPlttBufferFaded) - these need zero changes

### 2e. Window System

Files affected: `src/window.c` (400 lines)

- GBA hardware windows (WIN0/WIN1/WINOUT) mask which layers are visible per-pixel
- Implement in the software compositor: for each pixel, check if it falls inside WIN0/WIN1 rectangles, apply the appropriate layer enable mask from WININ/WINOUT registers

### 2f. Blend/Alpha Effects

- REG_BLDCNT/BLDALPHA/BLDY control per-pixel blending between layers
- Implement in the compositor: after determining the top two visible layers at each pixel, apply alpha blend (EVA/EVB coefficients) or brightness adjustment

### 2g. Scanline Effects

Files affected: `src/scanline_effect.c`

- The GBA uses HBlank DMA to change BG scroll registers per-scanline (used for battle wave effects, etc.)
- Implement by running the compositor scanline-by-scanline and applying the per-scanline register array before each line

### 2h. Asset Conversion Pipeline

- Tile data (.4bpp, .8bpp): Keep the indexed format as-is in ROM; the software renderer reads it natively
- Palettes (.gbapal): Keep as RGB555 arrays; convert to N64 RGBA5551 during palette load (just set alpha bit to 1)
- Compressed assets: Re-implement LZ77/RLE decompressors in C (replacing BIOS calls)

---

## Phase 3: Audio Engine

Files to replace: `src/m4a.c` (1,781 lines), `src/m4a_1.s` (2,000+ lines ASM), `include/gba/m4a_internal.h`

### 3a. Strategy: Rewrite M4A to target N64 audio hardware

The M4A engine is a software synthesizer that mixes PCM audio and writes to GBA DMA FIFOs. For N64:

1. **Keep the high-level M4A sequencer** (m4a.c music player logic, track parsing, MIDI-like command interpretation) - this is ~1,000 lines of portable C
2. **Replace the low-level mixer** (m4a_1.s and SoundMain/SoundMainRAM) - currently ARM assembly that mixes channels into a PCM buffer. Rewrite in C targeting N64:
   - Mix channels to a PCM buffer in RDRAM
   - Use N64 AI (Audio Interface) DMA to play the buffer at the correct sample rate
   - N64 AI supports 16-bit stereo at configurable sample rates - upgrade from GBA's 8-bit mono
3. **CGB channel emulation** - The 4 Game Boy sound channels (square waves, noise) are synthesized in software by M4A. Keep this logic, just retarget output
4. **Sample rate**: Upgrade from 13,379Hz to 22,050Hz or 32,000Hz for better quality (N64 can handle it easily)

### 3b. Audio Asset Compatibility

- PCM samples (sound/direct_sound_samples/): 8-bit signed PCM - works as-is, optionally upconvert to 16-bit
- Song sequences (sound/songs/): MIDI-like bytecode read by the sequencer - no changes needed
- Voice groups (sound/voicegroups/): Instrument definitions - no changes needed (they reference wave data pointers)

---

## Phase 4: Input System

Files affected: All code reading REG_KEYINPUT

- GBA has 10 buttons mapped to REG_KEYINPUT (active-low bitmask)
- N64 controller: read via SI (Serial Interface) using osContGetReadData() or libdragon's joypad_get_buttons()
- Create a shim: poll N64 controller each frame, write result into a fake REG_KEYINPUT variable with the same bitmask layout

Button mapping:

| GBA | N64 |
|---|---|
| D-pad | D-pad (or analog stick with threshold) |
| A | A |
| B | B |
| L | L |
| R | R |
| Start | Start |
| Select | Z |

This is a ~50-line shim. All 421K lines of game logic read gMain.newKeys/heldKeys which derive from REG_KEYINPUT - one function change propagates everywhere.

---

## Phase 5: Save System

Files to replace: `src/agb_flash.c`, `src/agb_flash_1m.c`, `src/agb_flash_le.c`, `src/agb_flash_mx.c`

- GBA uses 128KB Flash ROM on the cartridge
- N64 options: SRAM (32KB, common on flashcarts), Flash (128KB on some carts), Controller Pak (32KB)
- For flashcart compatibility: use SRAM or FlashRAM via PI bus
- Create a platform_save.c that implements ReadFlash(), ProgramFlashSector(), EraseFlashSector() targeting N64 save media
- The game's save logic in src/save.c calls these through a clean interface - only the bottom layer changes

---

## Phase 6: RTC (Real-Time Clock)

Files to replace: `src/rtc.c`, `src/siirtc.c` (Seiko RTC via GPIO bit-banging)

- N64 doesn't have an RTC by default, but some flashcarts (EverDrive 64) expose one
- Option A: Read RTC from flashcart if available
- Option B: Fake a clock using frame counting from boot (loses time between sessions)
- Option C: Store a "base time" in save data, increment by frame count - crude but functional

---

## Phase 7: Link/Multiplayer

Files to replace: `src/link.c` (1,000+ lines), `src/link_rfu.c`, all `librfu_*.c` files

- GBA link cable -> N64 has 4 controller ports (SI interface)
- Option A (faithful): Implement multiplayer over N64 controller ports using a custom serial protocol - would require custom hardware or Transfer Pak tricks
- Option B (practical): Stub out multiplayer. The single-player game is fully playable without it. Trade/battle features would be disabled
- Option C (modern): If targeting emulators, implement over network sockets

This is the least critical phase for a "100% faithful" single-player experience.

---

## Phase 8: Miscellaneous Platform Code

| Component | Scope | Action |
|---|---|---|
| `src/libgcnmultiboot.s` | GameCube multiboot | Remove entirely |
| `src/agb_flash_*.c` | Flash chip variants | Replace with Phase 5 |
| `src/librfu_*.c` | Wireless adapter | Stub or remove |
| Berry glitch fix / clock reset | RTC-dependent | Adapt to Phase 6 |
| `src/random.c` | RNG | Works as-is (pure C) |
| `src/main.c` | Main loop | Minor changes (init calls) |
| `src/gpu_regs.c` | Register buffering | Replace with software state writes |

---

## Execution Order & Dependencies

```
Phase 0: Toolchain          --+
Phase 1: HAL                --+ (foundation - everything depends on this)
                              |
Phase 4: Input              --+ (tiny, unblocks testing)
Phase 2: Graphics           --+ (largest, ~60% of total effort)
Phase 3: Audio              --+ (independent of graphics)
Phase 5: Save               --+ (independent)
Phase 6: RTC                --+ (independent)
Phase 7: Link               --+ (lowest priority)
Phase 8: Misc cleanup
```

## Estimated Scope

| Phase | New/Modified Lines | Difficulty |
|---|---|---|
| 0 - Toolchain | ~500 | Medium |
| 1 - HAL | ~2,000 | High |
| 2 - Graphics | ~5,000-8,000 | **Very High** |
| 3 - Audio | ~3,000 | High |
| 4 - Input | ~100 | Low |
| 5 - Save | ~300 | Medium |
| 6 - RTC | ~200 | Low |
| 7 - Link | ~500 (stubs) | Medium |
| 8 - Misc | ~500 | Low |
| **Total** | **~12,000-15,000** | |

The game logic itself (~400K lines of C) is almost entirely portable - it operates on abstract game state and calls into the hardware layer through well-defined interfaces. The key insight is that pokeemerald's decompilation has already separated concerns reasonably well.

## Key Risk: Graphics Performance

The biggest risk is the software tile compositor (Phase 2). Rendering 240x160 pixels with 4 BG layers + 64 sprites + windowing + blending at 60fps requires careful optimization. Mitigations:

- N64 CPU is 5.6x faster than GBA and has hardware multiply/divide
- RDP textured rectangles can accelerate tile blitting (batch 8x8 tiles as textured rects)
- RSP can be programmed as a tile compositor coprocessor via custom microcode
- Worst case: render at 30fps (still playable for a turn-based RPG)
