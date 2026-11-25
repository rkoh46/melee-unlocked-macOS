#!/bin/bash
# The L-cancel flash on the fighter's model (fe/features/overlays.cpp, mu-engine/app/gx_looks_mac.cpp): a hidden offline VS
# match on Onett from play_vs_script.py where P1 (Ness) short-hops into fast-fallen neutral airs over and over, every other landing L-cancelled
# (R pulses before the landing), with settings.ini lcancel_flash_mode 6 (MU: both). The session log names the made and missed
# L-cancels and how many fighter model draws the flash tinted; pictures every 4 frames from frame 2300 (PPM) show P1's model
# red (missed) or green (made). Nothing is written outside build/mu-engine/runs.
#   app_lcancel_flash_test.sh ISO [MODE]
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; mode="${2:-6}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-lcancel-flash-$mode"; rm -rf "$out"; mkdir -p "$out/melee-unlocked" "$out/shots"
python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$out/vs.txt"
python3 -I - "$out/vs.txt" <<'PY'
import sys
path = sys.argv[1]
lines = open(path).read().split("\n")
head = lines[:lines.index("@match")]
match = ["@match"]
for k in range(20):                      # one short-hop neutral air every 60 frames from match frame 900 (as the parity scripts)
    t = 900 + 60 * k
    match += [f"{t} X", f"{t + 2}", f"{t + 6} A", f"{t + 8}"]   # neutral air, then down taps after the peak: fast-fallen, it lands attacking
    for d in (14, 18, 22): match += [f"{t + d} sy=-127", f"{t + d + 2}"]
    if k % 2:                            # every other one L-cancelled: R pulses through the fall
        for p in (t + 24, t + 27): match += [f"{p} R", f"{p + 1}"]
open(path, "w").write("\n".join(head + match) + "\n")
PY
printf 'lcancel_flash_mode %s\nlcancel_success_flash 2\n' "$mode" > "$out/melee-unlocked/settings.ini"
MELEE_UPDATE_CHECK=0 MELEE_NO_FINDER=1 MU_TEST_FLASH_FRAMES=12 timeout 600 "$engine" --iso "$iso" --profile-dir "$out" --offline --hidden --volume 0 \
  --no-audio-device --no-record --frames 3500 --play-script "$out/vs.txt" --capture "$out/shots/f.ppm" 2300 --capture-every 4 > "$out/log" 2>&1
echo "engine exit $?"
grep -h "overlays: L-cancel\|looks: the L-cancel\|TE L-cancel" "$out/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200
ls "$out/shots" | wc -l | sed 's/^ */pictures: /'
