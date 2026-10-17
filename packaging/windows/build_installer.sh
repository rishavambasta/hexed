#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== Building HexEd for Windows 64-bit ==="

# Build Windows 64-bit binary if needed
if [ -x "$PROJECT_ROOT/tools/bin/x86_64-w64-mingw32-gcc" ]; then
    CC_WIN64="$PROJECT_ROOT/tools/bin/x86_64-w64-mingw32-gcc"
elif which x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    CC_WIN64="x86_64-w64-mingw32-gcc"
else
    echo "Warning: MinGW-w64 not found, checking existing bin/hexed.exe"
    CC_WIN64=""
fi

if [ -n "$CC_WIN64" ]; then
    mkdir -p "$PROJECT_ROOT/bin"
    "$CC_WIN64" -std=c11 -Wall -Wextra -O3 \
        "$PROJECT_ROOT"/src/core/*.c \
        "$PROJECT_ROOT"/src/cli_main.c \
        -lm -o "$PROJECT_ROOT/bin/hexed.exe"
fi

if [ ! -f "$PROJECT_ROOT/bin/hexed.exe" ]; then
    echo "Error: bin/hexed.exe not found!"
    exit 1
fi

mkdir -p "$PROJECT_ROOT/dist"

# Locate makensis
if [ -x "$PROJECT_ROOT/tools/bin/makensis" ]; then
    MAKENSIS="$PROJECT_ROOT/tools/bin/makensis"
elif which makensis >/dev/null 2>&1; then
    MAKENSIS="makensis"
else
    echo "Error: makensis not found!"
    exit 1
fi

echo "=== Compiling NSIS Windows Installer ==="
cd "$PROJECT_ROOT/packaging/windows"
"$MAKENSIS" installer.nsi
cd "$PROJECT_ROOT"

VERSION=$(grep -E 'HEXED_VERSION\s+' "$PROJECT_ROOT/src/version.h" | awk -F'"' '{print $2}')
echo "Successfully built: dist/HexEd-${VERSION}-Setup-win64.exe"
ls -lh "dist/HexEd-${VERSION}-Setup-win64.exe"

