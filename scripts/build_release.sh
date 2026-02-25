#!/usr/bin/env bash
# Build release binaries for CYD_DataDisplay
#
# Produces in bin/:
#   CYD_DataDisplay_v<version>_FULL.bin   — full flash image (Web Serial / esptool)
#   CYD_DataDisplay_v<version>_OTA.bin    — app-only image  (HTTP OTA updater)
#
# Run from the project root (where platformio.ini lives):
#   bash scripts/build_release.sh
#
# Requirements: PlatformIO CLI (`pio`) on PATH

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

echo "=================================================="
echo " CYD_DataDisplay Release Binary Builder"
echo "=================================================="
echo " Project: $PROJECT_DIR"
echo ""

# Recreate bin/ so every run starts clean
BIN_DIR="$PROJECT_DIR/bin"
echo "Preparing output directory: $BIN_DIR"
rm -rf "$BIN_DIR"
mkdir -p "$BIN_DIR"
echo ""

# ------------------------------------------------------------------
# Release build
# ------------------------------------------------------------------
echo "1/2  Building [env:release] …"
echo "--------------------------------------------------"

echo "  → Clean"
pio run -e release -t clean

echo "  → Build + export FULL (merged) binary"
pio run -e release -t merged

echo "  → Export OTA binary"
pio run -e release -t ota

echo ""
echo "=================================================="
echo " Done!  Binaries in $BIN_DIR:"
ls -lh "$BIN_DIR"
echo "=================================================="
