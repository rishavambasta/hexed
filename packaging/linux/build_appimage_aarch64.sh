#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== Building HexEd for Linux aarch64 ==="

# Build aarch64 binary if not already present or if sources updated
if [ -x "$PROJECT_ROOT/tools/bin/aarch64-linux-gnu-gcc" ]; then
    CC_AARCH64="$PROJECT_ROOT/tools/bin/aarch64-linux-gnu-gcc"
elif which aarch64-linux-gnu-gcc >/dev/null 2>&1; then
    CC_AARCH64="aarch64-linux-gnu-gcc"
else
    CC_AARCH64="gcc"
fi

mkdir -p "$PROJECT_ROOT/bin"
"$CC_AARCH64" -std=c11 -Wall -Wextra -O3 -D_GNU_SOURCE \
    "$PROJECT_ROOT"/src/core/*.c \
    "$PROJECT_ROOT"/src/cli_main.c \
    -lm -o "$PROJECT_ROOT/bin/hexed_aarch64"

APPDIR="$PROJECT_ROOT/AppDir_aarch64"
DISTDIR="$PROJECT_ROOT/dist"

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/hexed"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$DISTDIR"

# Copy binary
cp "$PROJECT_ROOT/bin/hexed_aarch64" "$APPDIR/usr/bin/hexed"

# Copy assets
cp -r "$PROJECT_ROOT/assets" "$APPDIR/usr/share/hexed/"
cp -r "$PROJECT_ROOT/assets" "$APPDIR/"

# Copy desktop and icon metadata
cp "$PROJECT_ROOT/packaging/linux/hexed.desktop" "$APPDIR/hexed.desktop"
cp "$PROJECT_ROOT/packaging/linux/hexed.desktop" "$APPDIR/usr/share/applications/hexed.desktop"
cp "$PROJECT_ROOT/assets/icons/hexed.png" "$APPDIR/hexed.png"
cp "$PROJECT_ROOT/assets/icons/hexed.png" "$APPDIR/.DirIcon"
cp "$PROJECT_ROOT/assets/icons/hexed.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/hexed.png"

# AppRun launcher
cp "$PROJECT_ROOT/packaging/linux/AppRun" "$APPDIR/AppRun"
chmod +x "$APPDIR/AppRun"

# Locate or download appimagetool
if [ -x "$PROJECT_ROOT/tools/bin/appimagetool" ]; then
    APPIMAGETOOL="$PROJECT_ROOT/tools/bin/appimagetool"
elif [ -x "$PROJECT_ROOT/tools/appimagetool-x86_64-extracted/AppRun" ]; then
    APPIMAGETOOL="$PROJECT_ROOT/tools/appimagetool-x86_64-extracted/AppRun"
elif which appimagetool >/dev/null 2>&1; then
    APPIMAGETOOL="appimagetool"
else
    echo "Downloading appimagetool..."
    mkdir -p "$PROJECT_ROOT/tools"
    wget -q -O "$PROJECT_ROOT/tools/appimagetool-x86_64.AppImage" "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "$PROJECT_ROOT/tools/appimagetool-x86_64.AppImage"
    (cd "$PROJECT_ROOT/tools" && ./appimagetool-x86_64.AppImage --appimage-extract >/dev/null 2>&1 && mv squashfs-root appimagetool-x86_64-extracted)
    APPIMAGETOOL="$PROJECT_ROOT/tools/appimagetool-x86_64-extracted/AppRun"
fi

# Locate or download aarch64 runtime
if [ ! -f "$PROJECT_ROOT/tools/runtime-aarch64" ]; then
    echo "Downloading aarch64 AppImage runtime..."
    mkdir -p "$PROJECT_ROOT/tools"
    wget -q -O "$PROJECT_ROOT/tools/runtime-aarch64" "https://github.com/AppImage/AppImageKit/releases/download/continuous/runtime-aarch64"
    chmod +x "$PROJECT_ROOT/tools/runtime-aarch64"
fi

RUNTIME_ARG="--runtime-file $PROJECT_ROOT/tools/runtime-aarch64"

echo "=== Packaging aarch64 AppImage ==="
OUTPUT_APPIMAGE="$DISTDIR/HexEd-1.1-aarch64.AppImage"
rm -f "$OUTPUT_APPIMAGE"

ARCH=arm_aarch64 "$APPIMAGETOOL" $RUNTIME_ARG "$APPDIR" "$OUTPUT_APPIMAGE"

chmod +x "$OUTPUT_APPIMAGE"
rm -rf "$APPDIR"

echo "Successfully built: $OUTPUT_APPIMAGE"
ls -lh "$OUTPUT_APPIMAGE"
