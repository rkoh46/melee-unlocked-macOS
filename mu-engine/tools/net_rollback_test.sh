#!/bin/bash
# Two melee-unlocked engines peer directly on loopback (--net, no matchmaking) through a relay that adds delay and
# jitter (udp_delay_relay.py, from Melee-NX-NetPlay), so the match rolls back; then the two sides' "net check" lines are
# compared. Sandboxed: no outgoing connection except to 127.0.0.1.
#   net_rollback_test.sh ISO [FRAMES] [DELAY_MS] [JITTER_MS]
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; frames="${2:-3600}"; delay="${3:-60}"; jitter="${4:-40}"
out="$repo/build/mu-engine/runs/net-rollback"; bin="$repo/build/mu-engine/out"
rm -rf "$out"; mkdir -p "$out/a" "$out/b"
profile='(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip "localhost:*"))'
sandbox-exec -p "$profile" python3 -I "$here/udp_delay_relay.py" 51001 52001 51002 52002 "$delay" "$jitter" > "$out/relay.log" 2>&1 &
relay=$!
sleep 1
sandbox-exec -p "$profile" timeout 1200 "$bin/mu-runner" --iso "$iso" --out "$out/a" --frames "$frames" --net 0:51001:127.0.0.1:52002 > "$out/a.log" 2>&1 &
a=$!
sandbox-exec -p "$profile" timeout 1200 "$bin/mu-runner" --iso "$iso" --out "$out/b" --frames "$frames" --net 1:51002:127.0.0.1:52001 > "$out/b.log" 2>&1 &
b=$!
wait $a; ra=$?; wait $b; rb=$?
kill $relay 2>/dev/null; wait $relay 2>/dev/null
echo "side a exit $ra, side b exit $rb"
for side in a b; do echo "== $side"; grep -E "net check|rollbacks|net: [0-9]+ of|desync" "$out/$side.log" | head -8; done
ca=$(grep -h "net check (" "$out/a.log"); cb=$(grep -h "net check (" "$out/b.log")
if [ -n "$ca" ] && [ "$ca" = "$cb" ]; then echo "net check: EQUAL on both sides"; else echo "net check: DIFFERENT or missing"; fi
