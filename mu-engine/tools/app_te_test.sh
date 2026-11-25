#!/bin/bash
# 20XX TE's host features in the app's engine, with the TE save as a mod profile (Mods/te/<the .gci>; MU_TE_GCI names the save,
# the player's own copy, never committed): hidden offline starts driven by controller scripts through the app's input path.
# As Melee-NX-NetPlay's frontend/test-overlays-linux.sh te and test-audio-linux.sh te-music.
#   app_te_test.sh ISO CASE...
# Cases:
#   inputdisplay  TE's input display bit (settings.ini.state te_options2 2000, as TE's own menu sets it): a VS match on Onett
#                 with the overlays' TE pads (one per player) and no inputoverlay; a screenshot at retrace 1700 (te.ppm)
#   menumusic     TE's "menu music from Sound Test": Data > Sound Test, the fourth song played with A (command 0xF7 keeps it as
#                 te_menu_music in settings.ini.state; musicpacks 1 so the jukebox names each song); a second start's main menu
#                 plays it
#   lock          TE's "lock settings" (te_options2 40000): --feature-test te-lock sends command 0xF8 as TE's menu does: a change
#                 is refused while locked, turning the lock off is taken, the same change then goes through
#   pair          TE's Disable screen rumble and this app's No screen shake linked (--feature-test te-pair: ours switched on,
#                 then TE's switched off, each followed by the other)
# Output: build/mu-engine/runs/app-te/<case>/.
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; shift
engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
gci="${MU_TE_GCI:?the 20XX TE save (.gci) to load as the te profile}"
root="$repo/build/mu-engine/runs/app-te"; mkdir -p "$root"
python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$root/vs.txt"
fresh() { rm -rf "$1"; mkdir -p "$1/melee-unlocked/Mods/te"; cp "$gci" "$1/melee-unlocked/Mods/te/"; }
run() {   # $1 folder, $2 frames, $3 script, more engine arguments
  local dir=$1 frames=$2 script=$3; shift 3
  MELEE_UPDATE_CHECK=0 MELEE_CAPTURE_UI=1 timeout 300 "$engine" --iso "$iso" --profile-dir "$dir" --offline --hidden --volume 0 --no-audio-device \
    --frames "$frames" --no-record --window 1280x720 --play-script "$script" "$@" >> "$dir/log" 2>&1
  echo "engine exit $?" >> "$dir/log"
}
for case in "$@"; do
  dir="$root/$case"; fresh "$dir"
  case "$case" in
  inputdisplay)
    printf '%s\n' "mod_profile te" > "$dir/melee-unlocked/settings.ini"
    printf '%s\n' "te_options2 2000" > "$dir/melee-unlocked/settings.ini.state"
    run "$dir" 1720 "$root/vs.txt" --capture "$dir/te.ppm" 1700 ;;
  menumusic)
    printf '%s\n' "mod_profile te" "musicpacks 1" > "$dir/melee-unlocked/settings.ini"
    python3 -I - "$dir/script.txt" <<'PY'
import sys
L = ["130 A", "140", "400 START", "410"]
t = 560
def p(w, n=1, gap=20, hold=4):
    global t
    for _ in range(n): L.extend([f"{t} {w}", f"{t+hold}"]); t += gap
p("sy=-127", 4); t += 30; p("A", gap=90); p("sy=-127", 2); t += 30; p("A", gap=150); p("sx=127", 3); t += 30; p("A", gap=200)
open(sys.argv[1], "w").write("\n".join(L) + "\n")
PY
    run "$dir" 1600 "$dir/script.txt"
    echo "settings.ini.state: $(grep te_menu_music "$dir/melee-unlocked/settings.ini.state" 2>/dev/null)"
    printf '%s\n' "130 A" "140" "400 START" "410" > "$dir/script2.txt"; echo "--- second start" >> "$dir/log"
    run "$dir" 900 "$dir/script2.txt" ;;
  lock)
    printf '%s\n' "mod_profile te" > "$dir/melee-unlocked/settings.ini"
    printf '%s\n' "te_options2 40000" > "$dir/melee-unlocked/settings.ini.state"
    printf '%s\n' "130 A" "140" > "$dir/script.txt"
    run "$dir" 200 "$dir/script.txt" --feature-test te-lock ;;
  pair)
    printf '%s\n' "mod_profile te" > "$dir/melee-unlocked/settings.ini"
    printf '%s\n' "130 A" "140" > "$dir/script.txt"
    run "$dir" 240 "$dir/script.txt" --feature-test te-pair ;;
  *) echo "unknown case $case" >&2; exit 2 ;;
  esac
  echo "== $case"
  grep -hE "mods:|jukebox:|te_menu|0xF7|TE |te-lock|te-pair|screen rumble|second start|engine exit|captured" "$dir/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200 | tail -20
done
