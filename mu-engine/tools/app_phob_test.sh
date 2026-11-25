#!/bin/bash
# melee-unlocked's PhobGCC menu in the engine (mu-engine/app/phob, fe/features/phob.cpp): hidden starts that open it at retrace
# 300 (MELEE_OPEN_PHOB) on one of its screens (MELEE_TEST_PHOB, upstream's test switch) and take a picture with the ImGui pass
# over the game (MELEE_CAPTURE_UI). Without a GameCube adapter the
# screens show no controller; with one on port 1 they show it live. Nothing is written outside build/mu-engine/runs/app-phob.
#   app_phob_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
root="$repo/build/mu-engine/runs/app-phob"; rm -rf "$root"; mkdir -p "$root"
# screens: 0 main, 3 stick calibration, 18 remap, 25 input viewer, 26 stick map (XY scope), 27 time scope, 29 reaction test,
# 31 gate, 33 Phob Check, 34 setups, 35 coordinates
for screen in 0 3 18 25 26 27 29 31 33 34 35; do
  dir="$root/s$screen"; mkdir -p "$dir/melee-unlocked"
  MELEE_UPDATE_CHECK=0 MELEE_NO_FINDER=1 MELEE_OPEN_PHOB=300 MELEE_TEST_PHOB=$screen MELEE_CAPTURE_UI=1 timeout 300 "$engine" --iso "$iso" \
    --profile-dir "$dir" --offline --hidden --volume 0 --no-audio-device --no-record --frames 380 --capture "$dir/phob.ppm" 360 > "$dir/log" 2>&1
  echo "screen $screen: exit $?, $(grep -c 'phob' "$dir/log") phob lines, picture $( [ -s "$dir/phob.ppm" ] && echo yes || echo no)"
done
grep -h "phob" "$root/s0/log" | sed 's/^\[ *[0-9.]*\] //' | head -5
