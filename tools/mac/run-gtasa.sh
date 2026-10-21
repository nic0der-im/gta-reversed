#!/usr/bin/env bash
# Run GTA SA + gta_reversed.asi on macOS through Wine (x86 via Rosetta 2).
# Mirrors the install part of scripts/vm/Build-GtaReversed.ps1.
#
# Usage: tools/mac/run-gtasa.sh <game-dir> [<asi-path>]
# Env:   WINE        wine binary (default: Gcenx "Wine Stable.app" in ~/Applications)
#        WINEPREFIX  prefix dir (default: ~/.wine-gtasa)
set -euo pipefail

COMPACT_EXE_BYTES=5190656
GAME_EXE="${GAME_EXE:-gta_sa_5190656.exe}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
WINE="${WINE:-$HOME/Applications/Wine Stable.app/Contents/Resources/wine/bin/wine}"
export WINEPREFIX="${WINEPREFIX:-$HOME/.wine-gtasa}"

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: $0 <game-dir> [<asi-path>]" >&2
    exit 2
fi
GAME_DIR="$1"
ASI="${2:-}"

if [ ! -x "$WINE" ]; then
    echo "Wine not found at $WINE (see plan A1)" >&2
    exit 1
fi

if [ ! -f "$GAME_DIR/$GAME_EXE" ] || [ "$(stat -f %z "$GAME_DIR/$GAME_EXE")" != "$COMPACT_EXE_BYTES" ]; then
    echo "Expected the compact exe ($COMPACT_EXE_BYTES bytes)" >&2
    exit 1
fi

if [ ! -d "$WINEPREFIX" ]; then
    "$WINE" wineboot --init
    "$(dirname "$WINE")/wineserver" -w
fi

if [ -f "$GAME_DIR/vorbisFile.dll" ] && [ ! -f "$GAME_DIR/vorbisFile.dll.stock" ]; then
    cp "$GAME_DIR/vorbisFile.dll" "$GAME_DIR/vorbisFile.dll.stock"
    echo "Backed up the original vorbisFile.dll to vorbisFile.dll.stock"
fi

unzip -o -q "$REPO/contrib/plugins.zip" -d "$GAME_DIR"

if [ -n "$ASI" ]; then
    mkdir -p "$GAME_DIR/scripts"
    cp "$ASI" "$GAME_DIR/scripts/gta_reversed.asi"
    PDB="${ASI%.asi}.pdb"
    if [ -f "$PDB" ]; then
        cp "$PDB" "$GAME_DIR/scripts/gta_reversed.pdb"
    fi
fi

cd "$GAME_DIR"
WINEDLLOVERRIDES="vorbisFile=n,b;dinput8=n,b" exec "$WINE" "$GAME_EXE"
