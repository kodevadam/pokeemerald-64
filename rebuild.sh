#!/usr/bin/env bash
# rebuild.sh — fast incremental build for pokeemerald-64
#
# Usage:
#   ./rebuild.sh           — git pull + incremental build (fast, ~5-10s for code changes)
#   ./rebuild.sh --clean   — full clean rebuild (slow; use after graphics/Makefile changes)
#   ./rebuild.sh --no-pull — skip git pull (useful if you're editing locally too)
#
# One-time setup: set IPL3_SRC below to your local SC64/libdragon ROM path.
# The extracted binary is cached; subsequent runs skip the extraction step.

set -e

IPL3_SRC="${IPL3_SRC:-/home/adam/newkind-64/newkind-64/elite_newkind.z64}"
IPL3_BIN="tools/ipl3_libdragon.bin"
MAKEFILE="Makefile.n64"
JOBS="-j$(nproc)"

# Parse flags
CLEAN=0
PULL=1
for arg in "$@"; do
    case "$arg" in
        --clean)   CLEAN=1 ;;
        --no-pull) PULL=0 ;;
        *) echo "Unknown flag: $arg"; exit 1 ;;
    esac
done

# ── 1. Sync ─────────────────────────────────────────────────────────────────
if [ "$PULL" -eq 1 ]; then
    echo "[pull] git pull..."
    git pull
fi

# ── 2. Extract IPL3 (once only) ─────────────────────────────────────────────
if [ ! -f "$IPL3_BIN" ]; then
    echo "[ipl3] Extracting IPL3 from $IPL3_SRC..."
    python3 tools/extract_ipl3.py "$IPL3_SRC" "$IPL3_BIN"
else
    echo "[ipl3] $IPL3_BIN already present, skipping extraction."
fi

# ── 3. Build ─────────────────────────────────────────────────────────────────
if [ "$CLEAN" -eq 1 ]; then
    echo "[build] Full clean rebuild..."
    make -f "$MAKEFILE" clean
    rm -f build/n64/.gfx_done
fi

echo "[build] Building (incremental)..."
make -f "$MAKEFILE" $JOBS 2>&1 | tail -5
