#!/bin/bash
# Alternate stages by held buttons (M12; fe/features/cosmetics.cpp, command 0xFD; melee-unlocked's host/cosmetic_mods.cpp), as
# Melee-NX-NetPlay's test-mods-linux.sh alt-stage: a mod profile whose disc is a copy of the player's own (cp -c: only the changes
# take space; deleted after; nothing committed) with Battlefield's file added as GrNBa.lat, its textures inverted (make_alt_stage_disc.py).
# The play-mode VS script (play_vs_script.py, its stage select stick held 3 frames instead of 40: Battlefield, the stage above
# the cursor) runs twice: with L held after the stage pick into the match (the
# alternate) and without (the standard stage); a picture of the match each time (build/shots-ui/alt-stage-*.png).
#   app_alt_stage_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
root="$repo/build/mu-engine/runs/alt-stage"; shots="$repo/build/shots-ui"; rm -rf "$root"; mkdir -p "$shots"
cap=$(mktemp -d)
for hold in L none; do
  dir="$root/$hold"; mkdir -p "$dir/melee-unlocked/Mods/altstages"
  disc="$dir/melee-unlocked/Mods/altstages/altstages.iso"
  cp -c "$iso" "$disc" 2>/dev/null || cp "$iso" "$disc"
  python3 -I "$here/make_alt_stage_disc.py" "$disc" GrNBa | sed 's#^.*/##'
  printf '%s\n' "mod_profile altstages" > "$dir/melee-unlocked/settings.ini"
  python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$dir/vs.txt"
  # Battlefield; with L held from just after the stage pick (960 A on the stage select) until the match runs.
  python3 -I - "$dir/vs.txt" $hold <<'PY'
import sys
p = sys.argv[1]; s = open(p).read()
s = s.replace("900 sy=100\n940\n", "900 sy=100\n903\n", 1)
if sys.argv[2] == "L": s = s.replace("960 A\n970\n", "960 A\n970\n976 L\n1300\n", 1)
open(p, "w").write(s)
PY
  MELEE_NO_ALERTS=1 MELEE_UPDATE_CHECK=0 timeout 300 "$engine" --iso "$iso" --profile-dir "$dir" --offline --hidden --volume 0 --no-audio-device \
    --frames 1700 --no-record --play-script "$dir/vs.txt" --capture "$cap/$hold.ppm" 1650 > "$dir/log" 2>&1
  echo "== L held: $hold, engine exit $?"
  grep -hE "stage alternate|held button picks|mods: .*(added|differ)" "$dir/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200 | head -6
  sips -s format png "$cap/$hold.ppm" --out "$shots/alt-stage-$hold.png" > /dev/null 2>&1
  rm -f "$disc"
done
rm -rf "$cap"
echo "pictures: $shots/alt-stage-L.png $shots/alt-stage-none.png"
