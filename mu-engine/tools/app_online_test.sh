#!/bin/bash
# The app's melee-unlocked engine (MeleeUnlockedEngine, with picture and sound, windows hidden) plays Unranked against a
# second copy on this Mac: the app's matchmaking client against the loopback stand-in (tools/mac/mock_matchmaking),
# generated inputs on both sides, everything under a sandbox that refuses outgoing connections except to 127.0.0.1.
#   app_online_test.sh ISO [FRAMES] [ENGINE]
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; frames="${2:-3000}"; engine="${3:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-online"; port=$((43800 + RANDOM % 500))
rm -rf "$out"; mkdir -p "$out"
profile='(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip "localhost:*"))'
sandbox-exec -p "$profile" "$repo/build/mu-engine/out/mock_matchmaking" --port "$port" --seconds 900 > "$out/mock.log" 2>&1 &
mock=$!
sleep 1
for side in a b; do
  seed=$([ $side = a ] && echo 303 || echo 404)
  MELEE_MM_LOOPBACK_PORT=$port sandbox-exec -p "$profile" timeout 1200 "$engine" --iso "$iso" --profile-dir "$out/$side" --hidden --volume 0 \
    --frames "$frames" --online unranked --fake-user --user-dir "$out/$side/user" --net-inputs $seed $([ $side = b ] && echo "${MU_B_ARGS:-}") > "$out/$side.log" 2>&1 &
  eval "pid_$side=$!"
  sleep 2
done
wait $pid_a; ra=$?; wait $pid_b; rb=$?
kill $mock 2>/dev/null; wait $mock 2>/dev/null
echo "side a exit $ra, side b exit $rb"
grep -h "mock mm: matched" "$out/mock.log"
for side in a b; do echo "== $side"; grep -E "net check|rollbacks \(loads\)|online: on|frames presented|audio: [0-9]" "$out/$side.log" | sed 's/^\[ *[0-9.]*\] //' | head -8; done
ca=$(grep -h "net check (" "$out/a.log" | sed 's/^\[ *[0-9.]*\] //'); cb=$(grep -h "net check (" "$out/b.log" | sed 's/^\[ *[0-9.]*\] //')
if [ -n "$ca" ] && [ "$ca" = "$cb" ]; then echo "net check: EQUAL on both sides"; else echo "net check: DIFFERENT or missing"; fi
