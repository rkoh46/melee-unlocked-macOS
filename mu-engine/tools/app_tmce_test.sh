#!/bin/bash
# Training Mode CE in the app's engine: the TM-CE disc built by the app's installer from the official TM-CE.zip (MU_TMCE_ISO, the
# player's own build, never committed) as the tmce mod profile; a hidden offline start driven by a controller script (Melee-NX-
# NetPlay's test-mods-linux.sh tmce-lab path): 1-P Mode > Event Match, page 2 > Training Lab, Fox on a stage, then in the match
# the Lab menu (Start) > CPU Options, CPU Percent up. TM-CE's on-screen displays are its own (the Lab menu's settings, on the
# memory card), drawn by the game as upstream draws them. A screenshot at CAPTURE (default 2500) with the menu open;
# MU_TMCE_CASE=osd does four wavedashes instead (TM-CE's Wavedash display reports each).
#   app_tmce_test.sh ISO
# Output: build/mu-engine/runs/app-tmce/ (log, tmce.ppm).
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
tmce="${MU_TMCE_ISO:?the TM-CE disc the installer built (tmce.iso)}"
out="$repo/build/mu-engine/runs/app-tmce"; rm -rf "$out"; mkdir -p "$out/melee-unlocked/Mods/tmce"
ln -s "$tmce" "$out/melee-unlocked/Mods/tmce/tmce.iso"
printf '%s\n' "mod_profile tmce" > "$out/melee-unlocked/settings.ini"
python3 -I - "$out/script.txt" "${MU_TMCE_CASE:-menu}" <<'PY'
import sys
L = ["130 A", "140", "400 START", "410", "520 A", "524", "600 sy=-127", "604", "660 A", "664"]
t = 900
def p(w, n=1, gap=20, hold=4):
    global t
    for _ in range(n): L.extend([f"{t} {w}", f"{t+hold}"]); t += gap
p("sx=127", gap=80); p("A", gap=60)                                # page 2 (General Tech) > Training Lab
L += ["@scene 2b 00", "60 sy=127", "70", "72 sx=127", "78", "100 A", "104", "180 START", "184",   # Fox, then Start
      "@scene 2b 02", "60 sy=127", "66", "90 A", "94", "@scene 2b 01"]                          # a stage, the match
if len(sys.argv) > 2 and sys.argv[2] == "osd":   # wavedashes (jump, then air dodge down and right): TM-CE's Wavedash display
    for t in (200, 260, 320, 380):
        L += [f"{t} X", f"{t+3}", f"{t+5} R sx=110 sy=-90", f"{t+12}", f"{t+40} sx=-127", f"{t+50}"]
else:
    t = 200
    p("START", gap=60); p("sy=-127", gap=30); p("A", gap=60)      # the Lab menu > CPU Options
    p("sx=127", 6, gap=12)                                        # CPU Percent up
open(sys.argv[1], "w").write("\n".join(L) + "\n")
PY
MELEE_UPDATE_CHECK=0 MELEE_CAPTURE_UI=1 timeout 400 "$engine" --iso "$iso" --profile-dir "$out" --offline --hidden --volume 0 --no-audio-device \
  --frames $(( ${CAPTURE:-2500} + 20 )) --no-record --window 1280x720 --play-script "$out/script.txt" --capture "$out/tmce.ppm" "${CAPTURE:-2500}" > "$out/log" 2>&1
echo "engine exit $?"
grep -hE "mods:|play: scene|captured" "$out/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200 | head -20
