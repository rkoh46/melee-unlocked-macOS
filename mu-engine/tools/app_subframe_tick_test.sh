#!/bin/bash
# Sub-frames skipped at one present per game frame (D53, fe/features/present, as Melee-NX-NetPlay 0.4.5 and upstream's
# threaded_backend.cpp): with Predict (settings.ini subframe 1) and a 60 fps cap, re-posing is skipped and logged once; with a
# 120 cap it re-poses; MU_SUBFRAME_AT_60=1 keeps re-posing at 60. Hidden engine, no sound, 600 frames each.
#   app_subframe_tick_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
root="$repo/build/mu-engine/runs/subframe-tick"; rm -rf "$root"
for case in cap60 cap120 cap60-keep; do
  dir="$root/$case"; mkdir -p "$dir/melee-unlocked"
  fps=${case#cap}; fps=${fps%-keep}
  printf '%s\n' "subframe 1" "fps $fps" > "$dir/melee-unlocked/settings.ini"
  env $([ $case = cap60-keep ] && echo MU_SUBFRAME_AT_60=1) MELEE_NO_ALERTS=1 MELEE_UPDATE_CHECK=0 timeout 300 "$engine" --iso "$iso" --profile-dir "$dir" \
    --offline --hidden --volume 0 --no-audio-device --frames 600 --no-record > "$dir/log" 2>&1
  echo "== $case: engine exit $?"
  grep -hE "present: " "$dir/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200 | head -5
done
