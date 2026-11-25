#!/bin/bash
# The app's melee-unlocked engine (MeleeUnlockedEngine, with picture and sound, windows hidden) plays Unranked against a
# second copy on this Mac: the app's matchmaking client against the loopback stand-in (tools/mac/mock_matchmaking_ws.py, Slippi 3.7.0's WebSocket queue),
# generated inputs on both sides, everything under a sandbox that refuses outgoing connections except to 127.0.0.1. Slippi's
# HTTP services are the loopback stand-in (tools/mac/mock_slippi_http.py): game reports, profile refresh, replay uploads.
#   app_online_test.sh ISO [FRAMES] [ENGINE]
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; frames="${2:-3000}"; engine="${3:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-online"; port=$((43800 + RANDOM % 500))
rm -rf "$out"; mkdir -p "$out"
# MU_INI: settings.ini lines for both sides ("session_trace 2" keeps every session trace), separated by ";".
# MU_COSMETICS: a Cosmetics folder copied to side b (skins and their picks, selection.txt).
if [ -n "${MU_COSMETICS:-}" ]; then mkdir -p "$out/b/melee-unlocked"; cp -R "$MU_COSMETICS" "$out/b/melee-unlocked/Cosmetics"; fi
if [ -n "${MU_INI:-}" ]; then for side in a b; do mkdir -p "$out/$side/melee-unlocked"; echo "$MU_INI" | tr ';' '\n' > "$out/$side/melee-unlocked/settings.ini"; done; fi
profile='(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip "localhost:*"))'
sandboxed() { sandbox-exec -p "$profile" "$@"; }
mock_loop() { while :; do sandboxed python3 -I "$repo/tools/mac/mock_matchmaking_ws.py" "$port" ${MU_MOCK_ARGS:-}; sleep 0.2; done; }
mock_loop > "$out/mock.log" 2>&1 &
mock=$!
hport=$((port + 600))
sandbox-exec -p "$profile" python3 -I "$repo/tools/mac/mock_slippi_http.py" $hport --save "$out/uploads" > "$out/http.log" 2>&1 &
mockhttp=$!
sleep 1
for side in a b; do
  seed=$([ $side = a ] && echo 303 || echo 404)
  MELEE_MM_LOOPBACK_PORT=$port MELEE_SERVICES_LOOPBACK_PORT=$hport sandbox-exec -p "$profile" timeout 1200 "$engine" --iso "$iso" --profile-dir "$out/$side" --hidden --volume 0 \
    --frames "$frames" --online unranked --fake-user --user-dir "$out/$side/user" --net-inputs $seed $([ $side = b ] && echo "${MU_B_ARGS:-}") > "$out/$side.log" 2>&1 &
  eval "pid_$side=$!"
  sleep 2
done
wait $pid_a; ra=$?; wait $pid_b; rb=$?
pkill -P $mock 2>/dev/null; pkill -P $mockhttp 2>/dev/null; kill $mock $mockhttp 2>/dev/null; wait $mock $mockhttp 2>/dev/null
echo "side a exit $ra, side b exit $rb"
grep -h "match" "$out/mock.log" | head -4
grep -h "mock http:" "$out/http.log" | head -12
for side in a b; do echo "== $side"; grep -E "net check|rollbacks \(loads\)|online: on|frames presented|audio: [0-9]" "$out/$side.log" | sed 's/^\[ *[0-9.]*\] //' | head -8; done
# The per-frame hashes both sides reached (each side stops at its own frame count): equal on every checkpoint they share.
python3 -I - "$out/a.log" "$out/b.log" <<'PY'
import re, sys
def points(path):
    out = {}
    for line in open(path, errors="replace"):
        if "net check (" in line:
            out.update(dict(re.findall(r"(\d+):([0-9a-f]{8})", line.split(":", 1)[1])))
    return out
a, b = points(sys.argv[1]), points(sys.argv[2])
common = sorted(set(a) & set(b), key=int)
same = common and all(a[k] == b[k] for k in common)
print("net check: %s on both sides (%d checkpoints compared, up to frame %s)" % ("EQUAL" if same else "DIFFERENT", len(common), common[-1] if common else "-") if common else "net check: missing")
PY
