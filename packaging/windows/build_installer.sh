#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

echo "=== Building HexEd for Windows 64-bit ==="

# Ensure GTK3 Windows packages are present
if [ ! -d "$PROJECT_ROOT/tools/gtk3-win64/mingw64/include/gtk-3.0" ]; then
    echo "Fetching GTK3 Windows packages..."
    python3 "$PROJECT_ROOT/packaging/windows/fetch_gtk3_win64.py"
fi

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
    WIN_CFLAGS=$(PKG_CONFIG_PATH="$PROJECT_ROOT/tools/gtk3-win64/mingw64/lib/pkgconfig" pkg-config --define-prefix --cflags gtk+-3.0)
    WIN_LIBS=$(PKG_CONFIG_PATH="$PROJECT_ROOT/tools/gtk3-win64/mingw64/lib/pkgconfig" pkg-config --define-prefix --libs gtk+-3.0)

    mkdir -p "$PROJECT_ROOT/build/win64/core" "$PROJECT_ROOT/build/win64/ui" "$PROJECT_ROOT/bin"
    echo "Compiling core modules for Windows..."
    for f in "$PROJECT_ROOT"/src/core/*.c; do
        "$CC_WIN64" -std=c11 -Wall -Wextra -O3 -g -D_GNU_SOURCE $WIN_CFLAGS -c "$f" -o "$PROJECT_ROOT/build/win64/core/$(basename "$f" .c).o"
    done
    echo "Compiling UI modules for Windows..."
    for f in "$PROJECT_ROOT"/src/ui/*.c; do
        "$CC_WIN64" -std=c11 -Wall -Wextra -O3 -g -D_GNU_SOURCE $WIN_CFLAGS -c "$f" -o "$PROJECT_ROOT/build/win64/ui/$(basename "$f" .c).o"
    done
    "$CC_WIN64" -std=c11 -Wall -Wextra -O3 -g -D_GNU_SOURCE $WIN_CFLAGS -c "$PROJECT_ROOT/src/main.c" -o "$PROJECT_ROOT/build/win64/main.o"

    echo "Linking hexed.exe GUI binary..."
    "$CC_WIN64" "$PROJECT_ROOT"/build/win64/core/*.o "$PROJECT_ROOT"/build/win64/ui/*.o "$PROJECT_ROOT"/build/win64/main.o $WIN_LIBS -mwindows -o "$PROJECT_ROOT/bin/hexed.exe"
fi

if [ ! -f "$PROJECT_ROOT/bin/hexed.exe" ]; then
    echo "Error: bin/hexed.exe not found!"
    exit 1
fi

echo "=== Assembling Windows Staging Bundle ==="
STAGING="$PROJECT_ROOT/build/win64_staging"
rm -rf "$STAGING"
mkdir -p "$STAGING"
cp "$PROJECT_ROOT/bin/hexed.exe" "$STAGING/"
cp "$PROJECT_ROOT"/tools/gtk3-win64/mingw64/bin/*.dll "$STAGING/"
mkdir -p "$STAGING/etc"
cp -r "$PROJECT_ROOT"/tools/gtk3-win64/mingw64/etc/* "$STAGING/etc/"
mkdir -p "$STAGING/lib/gdk-pixbuf-2.0/2.10.0/loaders"
cp "$PROJECT_ROOT"/tools/gtk3-win64/mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll "$STAGING/lib/gdk-pixbuf-2.0/2.10.0/loaders/"
cp "$PROJECT_ROOT"/tools/gtk3-win64/mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache "$STAGING/lib/gdk-pixbuf-2.0/2.10.0/"
mkdir -p "$STAGING/share/glib-2.0/schemas"
cp "$PROJECT_ROOT"/tools/gtk3-win64/mingw64/share/glib-2.0/schemas/gschemas.compiled "$STAGING/share/glib-2.0/schemas/"
mkdir -p "$STAGING/share/icons"
cp -r "$PROJECT_ROOT"/tools/gtk3-win64/mingw64/share/icons/* "$STAGING/share/icons/" 2>/dev/null || true
mkdir -p "$STAGING/assets"
cp -r "$PROJECT_ROOT"/assets/* "$STAGING/assets/"
cp "$PROJECT_ROOT/README.md" "$STAGING/"

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

