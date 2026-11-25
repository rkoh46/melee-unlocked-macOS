#!/bin/bash
# melee-unlocked's character select easter egg in the app's engine (fe/egg.cpp): a hidden offline start driven by a controller
# script through the app's input path to the VS character select (play_vs_script.py's path, up to the picks), then Up Up Down
# Down Left Right Left Right B A (Bailey) and the same with A B (Walter) on port 1; the log names each face, and a screenshot
# with the panel layer (MELEE_CAPTURE_UI) shows both bouncing.
#   app_egg_test.sh ISO
# Output: build/mu-engine/runs/app-egg/ (log, egg.ppm).
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-egg"; rm -rf "$out"; mkdir -p "$out"
python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$out/vs.txt"
python3 -I - "$out/vs.txt" "$out/egg.txt" <<'PY'
import sys
out = []
for line in open(sys.argv[1]).read().split("\n"):
    out.append(line)
    if line.strip() == "450 p=2": break   # both players on the character select, nothing picked
t = 470
for code in (["DU", "DU", "DD", "DD", "DL", "DR", "DL", "DR", "B", "A"], ["DU", "DU", "DD", "DD", "DL", "DR", "DL", "DR", "A", "B"]):
    for b in code:
        out += ["%d %s" % (t, b), "%d" % (t + 5)]; t += 12
    t += 60
open(sys.argv[2], "w").write("\n".join(out) + "\n")
PY
MELEE_UPDATE_CHECK=0 MELEE_CAPTURE_UI=1 timeout 300 "$engine" --iso "$iso" --profile-dir "$out" --offline --hidden --volume 0 --no-audio-device \
  --frames 1250 --no-record --window 1280x720 --play-script "$out/egg.txt" --capture "$out/egg.ppm" 1235 > "$out/log" 2>&1
echo "engine exit $?"
grep -h "walter egg\|captured" "$out/log" | sed 's/^\[ *[0-9.]*\] //'
