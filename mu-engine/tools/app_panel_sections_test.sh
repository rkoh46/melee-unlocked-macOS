#!/bin/bash
# The panel's sections stay closed between launches (A66, ui/mu_panel.cpp section_remembered: launcher.ini panel_closed).
# First start: the panel opens on VIDEO (MELEE_MENU_OPEN, MELEE_MENU_PAGE) and the "Display" section's header is clicked once
# (MELEE_TEST_SECTION_CLICK: a pointer click; the controller's navigation passes over headers, as upstream's), which closes it;
# the log names it and launcher.ini keeps it. Second start: nothing is
# pressed, and the same section is drawn closed. Pictures of both (MELEE_CAPTURE_UI) in build/shots-ui/sections-*.png.
#   app_panel_sections_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
dir="$repo/build/mu-engine/runs/panel-sections"; shots="$repo/build/shots-ui"; rm -rf "$dir"; mkdir -p "$dir/melee-unlocked" "$shots"
cap=$(mktemp -d)   # a capture path without spaces (the engine's option)
run() {   # run NAME [ENV=VALUE...]
  local name=$1; shift
  env "$@" MELEE_NO_ALERTS=1 MELEE_UPDATE_CHECK=0 MELEE_MENU_OPEN=300 MELEE_MENU_PAGE=VIDEO MELEE_CAPTURE_UI=1 timeout 300 "$engine" --iso "$iso" \
    --profile-dir "$dir" --offline --hidden --volume 0 --no-audio-device --frames 560 --no-record --capture "$cap/$name.ppm" 540 \
    > "$dir/$name.log" 2>&1
  echo "$name: engine exit $?"
  python3 -I -c "from PIL import Image; import sys; Image.open(sys.argv[1]).save(sys.argv[2])" "$cap/$name.ppm" "$shots/sections-$name.png" 2>/dev/null ||
    sips -s format png "$cap/$name.ppm" --out "$shots/sections-$name.png" > /dev/null
}
run first MELEE_TEST_SECTION_CLICK=Display
grep -h "settings: section" "$dir/first.log" | sed 's/^\[ *[0-9.]*\] //'
echo "launcher.ini: $(grep '^panel_closed' "$dir/launcher.ini" 2>/dev/null || echo 'no panel_closed line')"
run second
grep -h "settings: section" "$dir/second.log" | sed 's/^\[ *[0-9.]*\] //' || true
echo "pictures: $shots/sections-first.png $shots/sections-second.png"
rm -rf "$cap"
