#!/bin/bash
# The Replay library in the app's engine (fe/features/replay_library.cpp, from Melee-NX-NetPlay): a replay folder made here from
# recordings of earlier test runs under build/mu-engine/runs (never committed; dated an hour back so Compress all takes them),
# then a hidden offline start with --feature-test replay-library: the panel's Replays page (each replay's line and stats), Delete
# (to the folder's .trash), Undo the last delete, the search (MU_TEST_KEYBOARD answers the prompt), the watch queue, Compress all
# (.slpz, each read back before its .slp goes).
#   app_replay_library_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-replay-library"; rm -rf "$out"; mkdir -p "$out/Replays" "$out/melee-unlocked"
n=0
while IFS= read -r f; do
  n=$((n + 1)); cp "$f" "$out/Replays/$n-$(basename "$f")"; [ $n -ge 4 ] && break
done < <(find "$repo/build/mu-engine/runs" -path "$out" -prune -o -name "*.slp" -size +100k -print 2>/dev/null | sort)
[ $n -gt 0 ] || { echo "no recordings under build/mu-engine/runs: run a replay or online test first" >&2; exit 1; }
touch -t "$(date -v-1H +%Y%m%d%H%M)" "$out/Replays"/*.slp
echo "replay folder: $n recordings: $(ls "$out/Replays" | tr '\n' ' ')"
MU_TEST_KEYBOARD="${MU_SEARCH:-fox}" MELEE_UPDATE_CHECK=0 timeout 300 "$engine" --iso "$iso" --profile-dir "$out" --replay-dir "$out/Replays" --offline \
  --hidden --volume 0 --no-audio-device --frames 3000 --no-record --feature-test replay-library > "$out/log" 2>&1
echo "engine exit $?"
grep -h "library:" "$out/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200
echo "folder now: $(ls "$out/Replays" | tr '\n' ' ')"
