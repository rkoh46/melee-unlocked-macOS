#!/usr/bin/env bash
# RVZ and WIA discs (mu-engine/app/disc_convert.cpp): melee-unlocked's own disc_rvz_test.cpp and fixtures (made by its
# tests/rvz_fixtures.py from Dolphin's writer: every compression, RVZ packing, WIA purge, damaged and Wii images), built from the
# fetched tree with the decoders the app links (its bzip2, the LZMA SDK and its vendored Zstandard) with the Command Line Tools
# and run on this Mac. As Melee-NX-NetPlay's tools/rvz_check.sh (same author).
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
mu="$repo/build/mu-engine/mu/port"; tp="$mu/third_party"; o="$repo/build/mu-engine/rvz-check"; rm -rf "$o"; mkdir -p "$o"
unset DEVELOPER_DIR
for c in "$tp"/bzip2/{blocksort,huffman,crctable,randtable,compress,decompress,bzlib}.c "$tp"/lzma/LzmaDec.c "$tp"/lzma/Lzma2Dec.c \
         "$tp"/zstd/common/*.c "$tp"/zstd/decompress/*.c; do
  clang -O2 -w -DBZ_EXPORT -DZSTD_DISABLE_ASM=1 -I"$tp" -c "$c" -o "$o/$(basename "$c" .c).o"
done
clang++ -std=c++17 -O2 -w -DBZ_EXPORT -DMU_RVZ_DATA="\"$mu/tests/data/rvz\"" -I"$tp" -I"$mu/app" "$mu/tests/disc_rvz_test.cpp" "$mu/app/disc_rvz.cpp" \
  "$o"/*.o -o "$o/disc_rvz_test"
"$o/disc_rvz_test" "$@"
