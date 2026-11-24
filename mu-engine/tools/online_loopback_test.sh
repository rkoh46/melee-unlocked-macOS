#!/bin/bash
# Two melee-unlocked engines play an Unranked match against each other on this Mac, through the app's own matchmaking
# client and netplay, with the loopback stand-in for the matchmaking service (tools/mac/mock_matchmaking). Everything runs
# under a sandbox profile that refuses every outgoing connection except to 127.0.0.1, so nothing can reach Slippi's
# servers. Generated inputs drive both sides (--net-inputs). Afterwards it compares the two sides' "net check" lines.
#   online_loopback_test.sh ISO [FRAMES] [OUT_DIR]
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; frames="${2:-3600}"; out="${3:-$repo/build/mu-engine/runs/online}"
bin="$repo/build/mu-engine/out"; port=$((43200 + RANDOM % 500))
rm -rf "$out"; mkdir -p "$out/a" "$out/b"
profile='(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip "localhost:*"))'
sandboxed() { sandbox-exec -p "$profile" "$@"; }
sandboxed "$bin/mock_matchmaking" --port "$port" --seconds 900 > "$out/mock.log" 2>&1 &
mock=$!
sleep 1
for side in a b; do
  seed=$([ $side = a ] && echo 101 || echo 202)
  MELEE_MM_LOOPBACK_PORT=$port sandboxed timeout 1200 "$bin/mu-runner" --iso "$iso" --out "$out/$side" --frames "$frames" \
    --online unranked --fake-user --user-dir "$out/$side/user" --net-inputs $seed > "$out/$side.log" 2>&1 &
  eval "pid_$side=$!"
  sleep 2
done
wait $pid_a; ra=$?; wait $pid_b; rb=$?
kill $mock 2>/dev/null; wait $mock 2>/dev/null
echo "side a exit $ra, side b exit $rb"
grep -h "mock mm: matched" "$out/mock.log"
for side in a b; do echo "== $side"; grep -E "net check|desync|rollbacks|online:|connected|stopped" "$out/$side.log" | head -12; done
ca=$(grep -h "net check (" "$out/a.log"); cb=$(grep -h "net check (" "$out/b.log")   # the per-frame hashes; the frame counts differ with where each side stopped
if [ -n "$ca" ] && [ "$ca" = "$cb" ]; then echo "net check: EQUAL on both sides"; else echo "net check: DIFFERENT or missing"; fi
