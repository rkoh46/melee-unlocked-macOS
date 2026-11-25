#!/bin/bash
# The build-time copies the engine's picture effects (mu-engine/app/gfx) take from the fetched melee-unlocked tree: its
# texture packs (texture_pack.cpp), menu video backgrounds and border art (video_background.cpp), one-frame glitch
# detector (flicker_scan.cpp) and Lab view (lab_view.cpp), LF line ends and no byte-order mark, with patches/013x; and the headers they need beside them.
# Nothing of it is committed. usage: gen_gfx.sh MU_TREE OUT_DIR
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
mu="$1"; gen="$2"
mkdir -p "$gen"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
mkdir -p "$work/port/runtime/gx"
for f in texture_pack.cpp video_background.cpp flicker_scan.cpp lab_view.cpp; do
  perl -pe 's/\r$//; s/^\xEF\xBB\xBF// if $. == 1' "$mu/port/runtime/gx/$f" > "$work/port/runtime/gx/$f"
done
for p in 0130-texture-packs-on-the-mac 0131-video-backgrounds-on-the-mac 0132-glitch-detector-on-the-mac 0133-lab-view-on-the-mac; do
  patch -s -p1 -d "$work" < "$here/patches/$p.patch"
done
put() { cmp -s "$1" "$2" || cp "$1" "$2"; }
for f in texture_pack.cpp video_background.cpp flicker_scan.cpp lab_view.cpp; do put "$work/port/runtime/gx/$f" "$gen/$f"; done
for f in texture_pack.h video_background.h video_background_learning.h video_background_style.h flicker_scan.h \
         companion_texture_match.h companion_texture_map.h lab_view.h; do
  put "$mu/port/runtime/gx/$f" "$gen/$f"
done
