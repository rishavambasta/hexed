#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== Building HexEd for Linux x86_64 ==="
make

APPDIR="$PROJECT_ROOT/AppDir_x86_64"
DISTDIR="$PROJECT_ROOT/dist"

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/hexed"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$DISTDIR"

# Copy binary
cp "$PROJECT_ROOT/bin/hexed" "$APPDIR/usr/bin/hexed"

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

echo "=== Packaging x86_64 AppImage ==="
VERSION=$(grep -E 'HEXED_VERSION\s+' "$PROJECT_ROOT/src/version.h" | awk -F'"' '{print $2}')
OUTPUT_APPIMAGE="$DISTDIR/HexEd-${VERSION}-x86_64.AppImage"
rm -f "$OUTPUT_APPIMAGE"

ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$OUTPUT_APPIMAGE"


chmod +x "$OUTPUT_APPIMAGE"
rm -rf "$APPDIR"

echo "Successfully built: $OUTPUT_APPIMAGE"
ls -lh "$OUTPUT_APPIMAGE"
