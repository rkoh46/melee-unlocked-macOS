#!/bin/bash
# The settings panel read from a couch (ui/mu_panel.cpp UiSpace): the panel opens on a category (MELEE_MENU_OPEN,
# MELEE_MENU_PAGE), the test pad file walks down its rows (D-pad Down every half second), and a picture of the panel is taken
# every second (MELEE_CAPTURE_UI, --capture-every), in a 1280x720 and a 1920x1080 window (settings.ini window). Hidden window,
# no sound, no alerts. Pictures in build/shots-ui/tv-<size>-<page>-NN.png.
#   app_panel_tv_test.sh ISO [PAGE...]   (pages: VIDEO AUDIO GAME CONTROLS OVERLAYS CUSTOMIZE "GECKO CODES" MODS; default GAME VIDEO CONTROLS)
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; shift; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
pages=("$@"); [ ${#pages[@]} -gt 0 ] || pages=(GAME VIDEO CONTROLS)
shots="$repo/build/shots-ui"; mkdir -p "$shots"
for size in ${MU_TV_SIZES:-1280x720 1920x1080}; do
  for page in "${pages[@]}"; do
    tag="$(echo "$page" | tr 'A-Z ' 'a-z-')"
    dir="$repo/build/mu-engine/runs/panel-tv-$size-$tag"; rm -rf "$dir"; mkdir -p "$dir/melee-unlocked"
    printf 'window %s\n' "$size" > "$dir/melee-unlocked/settings.ini"
    cap=$(mktemp -d)   # a capture path without spaces (the engine's option)
    pad="$cap/pad.txt"; : > "$pad"
    # D-pad Down tapped every half second once the panel is up (retrace 300 is about 5 s in).
    ( sleep 7; for i in $(seq ${MU_TV_ROWS:-140}); do echo "DD" > "$pad"; sleep 0.1; : > "$pad"; sleep 0.4; done ) &
    walker=$!
    MELEE_PAD_FILE="$pad" MELEE_NO_ALERTS=1 MELEE_UPDATE_CHECK=0 MELEE_MENU_OPEN=300 MELEE_MENU_PAGE="$page" MELEE_CAPTURE_UI=1 \
      timeout 300 "$engine" --iso "$iso" --profile-dir "$dir" --offline --hidden --volume 0 --no-audio-device \
      --frames "${MU_TV_FRAMES:-4800}" --no-record --capture "$cap/shot.ppm" 420 --capture-every 60 > "$dir/engine.log" 2>&1
    echo "$size $page: engine exit $?"
    kill $walker 2>/dev/null; wait $walker 2>/dev/null
    n=0
    for f in $(ls "$cap"/shot*.ppm 2>/dev/null | sort -V); do
      out="$shots/tv-$size-$tag-$(printf %02d $n).png"; n=$((n + 1))
      python3 -I -c "from PIL import Image; import sys; Image.open(sys.argv[1]).save(sys.argv[2])" "$f" "$out" 2>/dev/null ||
        sips -s format png "$f" --out "$out" > /dev/null
    done
    echo "  $n pictures, focus walked: $(grep -c 'settings: focus' "$dir/engine.log")"
    rm -rf "$cap"
  done
done
