#!/bin/bash
# The player's Gecko codes in the app's engine (fe/features/gecko.cpp, from Melee-NX-NetPlay): a GeckoCodes.ini made here with a
# code that writes a game variable (P1's model scale in the VS start data, 1.5), a PowerPC injection (C2) and a function rewrite
# (waxo's No Character Shadows), all three switched on in settings.ini, then a hidden offline start driven into a VS match on
# Onett (play_vs_script.py), with MU_GECKO_PROBE=1 logging the variable the first code writes as the game image hands it back.
# The first code runs offline; the other two are refused with their reasons. As Melee-NX-NetPlay's test-mods-linux.sh gecko.
#   app_gecko_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-gecko"; rm -rf "$out"; mkdir -p "$out/melee-unlocked"
python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$out/vs.txt"
printf '%s\n' '[Gecko]' '$Giant P1 (VS)' '*P1 at 1.5 times its size in VS matches' '044805B0 3FC00000' \
  '$C2 injection test' 'C2000000 00000001' '60000000 00000000' '$No Character Shadows' '040872F8 38600000' > "$out/melee-unlocked/GeckoCodes.ini"
printf '%s\n' "geckocode Giant P1 (VS)" "geckocode C2 injection test" "geckocode No Character Shadows" "geckochosen 1" > "$out/melee-unlocked/settings.ini"
MU_GECKO_PROBE=1 MELEE_UPDATE_CHECK=0 timeout 300 "$engine" --iso "$iso" --profile-dir "$out" --offline --hidden --volume 0 --no-audio-device \
  --frames 1800 --no-record --play-script "$out/vs.txt" > "$out/log" 2>&1
echo "engine exit $?"
grep -hE "gecko|play: scene 2/2" "$out/log" | grep -v "card:" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-220 | awk '!seen[$0]++' | head -20
