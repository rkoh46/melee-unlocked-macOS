#!/bin/bash
# The Lab dummy's own reactions (fe/features/training.cpp lab_di / lab_react / lab_tech, the game image's mu_lab.c), as Melee-NX-NetPlay's
# test-overlays-linux.sh lab-dummy: offline Training (--feature-test training) with jabs and forward smashes on the dummy, the
# dummy DI-ing out, shielding after hitstun and teching at random; the game logs each hit's DI and reaction ("[lab] dummy ...").
# With nothing set (the default) the same run logs none.
#   app_lab_dummy_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
root="$repo/build/mu-engine/runs/lab-dummy"; rm -rf "$root"
for case in set default; do
  dir="$root/$case"; mkdir -p "$dir/melee-unlocked"
  python3 -I - "$dir/script.txt" <<'PY'
import sys
lines = ["@scene 7f 7f", "@match", "60 sx=127", "100"]   # (an unreached @scene line selects the scene format, where @match counts from the match)
t = 160
for i in range(16): lines += [f"{t} A", f"{t+3}"]; t += 24
for i in range(6): lines += [f"{t} sx=127 A", f"{t+3}", f"{t+60}"]; t += 90      # forward smashes: hitstun, tumble
open(sys.argv[1], "w").write("\n".join(lines) + "\n")
PY
  [ $case = set ] && printf '%s\n' "lab_di 2" "lab_react 2" "lab_tech 4" > "$dir/melee-unlocked/settings.ini"
  MELEE_NO_ALERTS=1 MELEE_UPDATE_CHECK=0 timeout 300 "$engine" --iso "$iso" --profile-dir "$dir" --offline --hidden --volume 0 --no-audio-device \
    --frames 2400 --no-record --play-script "$dir/script.txt" --feature-test training > "$dir/log" 2>&1
  echo "== $case: engine exit $?, $(grep -c '\[lab\] dummy' "$dir/log") dummy lines"
  grep -h "\[lab\] dummy" "$dir/log" | sed 's/^\[ *[0-9.]*\] //' | head -6
done
