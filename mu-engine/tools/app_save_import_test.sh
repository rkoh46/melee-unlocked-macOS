#!/bin/bash
# Save import for mods (fe/mods.cpp, as Melee-NX-NetPlay's): a profile whose folder holds a .gci that is not the 20XX TE save
# (here a Melee save the game wrote in an earlier test run under build/mu-engine/runs, never committed; MU_SAVE names another)
# goes onto that profile's memory card once, the game reads it, and a second start leaves the card's copy alone.
#   app_save_import_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
save="${MU_SAVE:-$(find "$repo/build/mu-engine/runs" -path '*/card/*.gci' -size +1k 2>/dev/null | grep -v 20XXTE | head -1)}"
[ -f "$save" ] || { echo "no save to import: run a reference script first, or set MU_SAVE" >&2; exit 1; }
out="$repo/build/mu-engine/runs/app-save-import"; rm -rf "$out"; mkdir -p "$out/melee-unlocked/Mods/imported"
cp "$save" "$out/melee-unlocked/Mods/imported/"
printf '%s\n' "mod_profile imported" > "$out/melee-unlocked/settings.ini"
for n in 1 2; do
  echo "--- start $n" >> "$out/log"
  MELEE_UPDATE_CHECK=0 timeout 200 "$engine" --iso "$iso" --profile-dir "$out" --offline --hidden --volume 0 --no-audio-device --frames 600 --no-record >> "$out/log" 2>&1
done
grep -hE "^--- start|mods:|card: native slot A" "$out/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200 | awk '!seen[$0]++ || /start/'
echo "the profile's card: $(ls "$out/melee-unlocked/card-imported" 2>/dev/null | tr '\n' ' ')"
