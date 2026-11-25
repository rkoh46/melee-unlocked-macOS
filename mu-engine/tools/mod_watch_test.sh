#!/usr/bin/env bash
# Builds and runs mod_watch_test.cpp: the mod installer's link-only path (a download found in ~/Downloads or chosen with
# Choose File, checked by SHA-256 before it is installed), offline, with no disc. Needs mu-engine/fetch.sh first (the LZMA
# SDK decoders come from the fetched melee-unlocked tree).
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
repo="$(cd "$here/.." && pwd -P)"
mu="$repo/build/mu-engine/mu"
src="$here/modinstall"
out="$repo/build/mod-watch-test"; mkdir -p "$out/obj"
CXX="${CXX:-c++}"; CC="${CC:-cc}"
for c in LzmaDec Lzma2Dec; do "$CC" -O2 -c "$mu/port/third_party/lzma/$c.c" -I"$mu/port/third_party/lzma" -o "$out/obj/$c.o"; done
"$CXX" -std=c++17 -O2 -Wall -Wextra -o "$out/mod_watch_test" "$here/tools/mod_watch_test.cpp" \
  "$src"/mi_hash.cpp "$src"/mi_io.cpp "$src"/mi_archive.cpp "$src"/mi_lzma.cpp "$src"/mi_vcdiff.cpp "$src"/mi_catalog.cpp \
  "$src"/mod_install.cpp "$out/obj/LzmaDec.o" "$out/obj/Lzma2Dec.o" \
  -I"$src" -I"$mu/port/third_party/lzma" -I"$repo/port/third_party" -lz -lpthread
"$out/mod_watch_test" "$out/run"
