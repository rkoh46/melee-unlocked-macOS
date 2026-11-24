#!/usr/bin/env bash
# Builds and runs mod_install_test.cpp natively with the system's clang, libcurl and zlib: the mod installer library
# (mu-engine/modinstall, from Melee-NX-NetPlay) outside the app. Needs mu-engine/fetch.sh first (the catalog and the LZMA
# SDK decoders come from the fetched melee-unlocked tree).
#   mu-engine/tools/mod_install_test.sh /path/to/disc-folder [mod ids...]
# The disc folder holds GALE01.iso (vanilla NTSC 1.02). Downloads come only from the catalog's own URLs and are kept in
# build/mod-downloads/; built discs go to build/mod-install-test/ and are deleted after each comparison.
# Adapted from Melee-NX-NetPlay's experiments/native-engine/tools/mod_install_test.sh.
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
repo="$(cd "$here/.." && pwd -P)"
disc="${1:?folder with GALE01.iso}"; shift
mu="$repo/build/mu-engine/mu"
src="$here/modinstall"
out="$repo/build/mod-install-test"; mkdir -p "$out/obj"
CXX="${CXX:-c++}"; CC="${CC:-cc}"
for c in LzmaDec Lzma2Dec; do "$CC" -O2 -c "$mu/port/third_party/lzma/$c.c" -I"$mu/port/third_party/lzma" -o "$out/obj/$c.o"; done
"$CXX" -std=c++17 -O2 -Wall -Wextra -o "$out/mod_install_test" "$here/tools/mod_install_test.cpp" \
  "$src"/mi_hash.cpp "$src"/mi_io.cpp "$src"/mi_archive.cpp "$src"/mi_lzma.cpp "$src"/mi_vcdiff.cpp "$src"/mi_catalog.cpp \
  "$src"/mod_install.cpp "$src"/mi_fetch_curl.cpp "$out/obj/LzmaDec.o" "$out/obj/Lzma2Dec.o" \
  -I"$src" -I"$mu/port/third_party/lzma" -I"$repo/port/third_party" -lcurl -lz -lpthread
"$out/mod_install_test" "$mu/mods-catalog-2.json" "$disc" "$repo/build/mod-downloads" "$out" "$@"
