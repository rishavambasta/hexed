#!/usr/bin/env python3
import os, urllib.request, subprocess, concurrent.futures

project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
target_dir = os.path.join(project_root, "tools", "gtk3-win64")
os.makedirs(target_dir, exist_ok=True)

db_dir = "/tmp/msys2_db"
if not os.path.isdir(db_dir) or not os.listdir(db_dir):
    os.makedirs(db_dir, exist_ok=True)
    db_tar = "/tmp/mingw64.db.tar.zst"
    if not os.path.exists(db_tar):
        print("Downloading MSYS2 package database...")
        urllib.request.urlretrieve("https://repo.msys2.org/mingw/mingw64/mingw64.db.tar.zst", db_tar)
    subprocess.check_call(["tar", "--zstd", "-xf", db_tar, "-C", db_dir])

packages_to_fetch = [
    "mingw-w64-x86_64-gtk3",
    "mingw-w64-x86_64-glib2",
    "mingw-w64-x86_64-cairo",
    "mingw-w64-x86_64-pango",
    "mingw-w64-x86_64-gdk-pixbuf2",
    "mingw-w64-x86_64-atk",
    "mingw-w64-x86_64-harfbuzz",
    "mingw-w64-x86_64-libepoxy",
    "mingw-w64-x86_64-librsvg",
    "mingw-w64-x86_64-freetype",
    "mingw-w64-x86_64-fontconfig",
    "mingw-w64-x86_64-fribidi",
    "mingw-w64-x86_64-graphite2",
    "mingw-w64-x86_64-pixman",
    "mingw-w64-x86_64-libdatrie",
    "mingw-w64-x86_64-libthai",
    "mingw-w64-x86_64-libpng",
    "mingw-w64-x86_64-libjpeg-turbo",
    "mingw-w64-x86_64-libtiff",
    "mingw-w64-x86_64-libdeflate",
    "mingw-w64-x86_64-zlib",
    "mingw-w64-x86_64-zstd",
    "mingw-w64-x86_64-bzip2",
    "mingw-w64-x86_64-brotli",
    "mingw-w64-x86_64-expat",
    "mingw-w64-x86_64-libxml2",
    "mingw-w64-x86_64-libffi",
    "mingw-w64-x86_64-pcre2",
    "mingw-w64-x86_64-gettext-runtime",
    "mingw-w64-x86_64-libiconv",
    "mingw-w64-x86_64-libwinpthread",
    "mingw-w64-x86_64-libgcc",
    "mingw-w64-x86_64-libstdc++",
    "mingw-w64-x86_64-json-glib",
    "mingw-w64-x86_64-adwaita-icon-theme",
    "mingw-w64-x86_64-hicolor-icon-theme",
    "mingw-w64-x86_64-jbigkit",
    "mingw-w64-x86_64-lerc",
    "mingw-w64-x86_64-xz",
    "mingw-w64-x86_64-libwebp",
    "mingw-w64-x86_64-giflib",
    "mingw-w64-x86_64-libb2",
]

pkg_files = {}
for entry in os.listdir(db_dir):
    desc_path = os.path.join(db_dir, entry, "desc")
    if not os.path.isfile(desc_path): continue
    with open(desc_path) as f:
        lines = f.read().splitlines()
    fn = None
    name = None
    for i, line in enumerate(lines):
        if line == "%FILENAME%" and i + 1 < len(lines):
            fn = lines[i + 1]
        elif line == "%NAME%" and i + 1 < len(lines):
            name = lines[i + 1]
    if name and fn:
        pkg_files[name] = fn

dl_dir = "/tmp/msys2_pkgs"
os.makedirs(dl_dir, exist_ok=True)

def fetch_and_extract(pkg):
    fn = pkg_files.get(pkg)
    if not fn:
        print("Not found in db:", pkg)
        return
    url = f"https://repo.msys2.org/mingw/mingw64/{fn}"
    local_pkg = os.path.join(dl_dir, fn)
    if not os.path.exists(local_pkg):
        print(f"Downloading {fn}...")
        urllib.request.urlretrieve(url, local_pkg)
    print(f"Extracting {fn}...")
    subprocess.check_call(["tar", "--zstd", "-xf", local_pkg, "-C", target_dir])

print(f"Starting parallel download and extraction of {len(packages_to_fetch)} packages...")
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
    list(executor.map(fetch_and_extract, packages_to_fetch))

print("All packages downloaded and extracted successfully!")
