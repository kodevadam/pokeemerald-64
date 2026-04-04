# test/Makefile — Build the standalone N64 miniboot diagnostic ROM
#
# Produces test/miniboot.z64 — a ~6 KB ROM that just fills the screen RED.
# Purpose: verify SC64 boot path works independently of the Emerald codebase.
#
# Usage: make -C test   (from repo root: make -f test/Makefile)

AS      := mipsel-linux-gnu-as
LD      := mipsel-linux-gnu-ld
OBJCOPY := mipsel-linux-gnu-objcopy

ASFLAGS := -EB -mips3 -mabi=32 -G0
LDFLAGS := -T miniboot.ld -m elf32btsmip -nostdlib -static

TOOLS   := ../tools
CRC     := ../build/n64/tools/n64crc

.PHONY: all clean

all: miniboot.z64

miniboot.o: miniboot.s
	$(AS) $(ASFLAGS) $< -o $@

miniboot.elf: miniboot.o miniboot.ld
	$(LD) $(LDFLAGS) $< -o $@

miniboot.z64: miniboot.elf
	$(OBJCOPY) -O binary $< $@
	@# Pad to at least 0x101000 bytes so n64crc can compute CRC
	@SIZE=$$(wc -c < $@); \
	 PAD=$$((1052672 - $$SIZE)); \
	 if [ $$PAD -gt 0 ]; then dd if=/dev/zero bs=1 count=$$PAD >> $@ 2>/dev/null; fi
	@# Patch IPL3 into bytes 0x040–0x0FFF
	python3 $(TOOLS)/patch_ipl3.py $@
	@# Fix CRC
	$(CRC) $@
	@echo "==> Miniboot ROM: $@ ($$(wc -c < $@) bytes)"
	@echo "==> Flash this ROM and verify RED appears on screen"
	@echo "==> RED = boot works; BLACK = SC64/IPL3/hardware issue"

clean:
	rm -f miniboot.o miniboot.elf miniboot.z64
