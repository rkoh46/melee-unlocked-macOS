#!/bin/bash
# Two copies of the app's engine (MeleeUnlockedEngine, windows hidden, no sound) go online the way a player does: from the
# game's own menus (Slippi's Online Play page) by controller scripts (--play-script, melee-unlocked's format), against the
# loopback stand-ins for the matchmaking service (tools/mac/mock_matchmaking_ws.py, Slippi 3.7.0's WebSocket queue) and Slippi's HTTP services
# (tools/mac/mock_slippi_http.py), under a sandbox that refuses outgoing connections except to 127.0.0.1. After the scripted
# menus both sides press generated buttons (as Melee-NX-NetPlay's frontend/test-online-linux.sh does), then the two sides'
# "net check" lines are compared.
#   app_online_menus_test.sh ISO [FRAMES] [SCRIPT_A] [SCRIPT_B]     scripts from mu-engine/tests/online (default online_unranked)
#   e.g. play while you search: app_online_menus_test.sh ISO 4800 online_keep_search_a online_keep_search_b
#   MU_INI_A / MU_INI_B: extra settings.ini lines for a side ("slippi_menus 1" is always there); MU_USER_A / MU_USER_B: a folder
#   copied into that side's user folder (mu-engine/tests/online/users/direct-a: a Direct code history); MU_ARGS_A / MU_ARGS_B:
#   more engine options for a side (the warm-up room: MU_ARGS_A="--feature-test unranked-as-direct" with online_warmup_a/b).
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; frames="${2:-4200}"; sa="${3:-online_unranked}"; sb="${4:-online_unranked}"
engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-online-menus"; port=$((44400 + RANDOM % 500)); hport=$((port + 600))
rm -rf "$out"; mkdir -p "$out/a/melee-unlocked" "$out/b/melee-unlocked"
python3 -I - "$here/../tests/online/$sa.txt" "$here/../tests/online/$sb.txt" "$out" <<'PY'
import sys
for side, given, period, stick in (("a", sys.argv[1], 47, 80), ("b", sys.argv[2], 61, -80)):
    text = open(given).read()
    if side == "b" and given.endswith("online_unranked.txt"): text = text.replace("132\n150 A", "132 sx=127\n138\n150 A")   # side b picks Fox
    head, gates = text.split("@scene", 1)
    lines = []
    for t in range(600, 30000, period):
        lines += [f"{t} sx={stick}", f"{t + period // 3} A sx={stick}", f"{t + period // 3 + 4} sx={-stick}", f"{t + 2 * period // 3} X", f"{t + 2 * period // 3 + 4}"]
    open(f"{sys.argv[3]}/{side}/pads.txt", "w").write(head + "\n".join(lines) + "\n@scene" + gates)
PY
for side in a b; do v="MU_USER_$(echo $side | tr a-z A-Z)"; if [ -n "${!v:-}" ]; then mkdir -p "$out/$side/user"; cp -R "${!v}"/. "$out/$side/user/"; fi; done
printf 'slippi_menus 1\n%b\n' "${MU_INI_A:-}" > "$out/a/melee-unlocked/settings.ini"
printf 'slippi_menus 1\n%b\n' "${MU_INI_B:-}" > "$out/b/melee-unlocked/settings.ini"
profile='(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip "localhost:*"))'
sandboxed() { sandbox-exec -p "$profile" "$@"; }
mock_loop() { while :; do sandboxed python3 -I "$repo/tools/mac/mock_matchmaking_ws.py" "$port" ${MU_MOCK_ARGS:-}; sleep 0.2; done; }
mock_loop > "$out/mock.log" 2>&1 &
mock=$!
sandbox-exec -p "$profile" python3 -I "$repo/tools/mac/mock_slippi_http.py" $hport --save "$out/uploads" > "$out/http.log" 2>&1 &
mockhttp=$!
sleep 1
for side in a b; do
  MELEE_MM_LOOPBACK_PORT=$port MELEE_SERVICES_LOOPBACK_PORT=$hport sandbox-exec -p "$profile" timeout 1200 "$engine" --iso "$iso" --profile-dir "$out/$side" \
    --sys-dir "$repo/port/slippi_sys" --hidden --volume 0 --no-audio-device --frames "$frames" --fake-user --user-dir "$out/$side/user" --play-script "$out/$side/pads.txt" \
    --net-inputs $([ $side = a ] && echo 1 || echo 2) $([ $side = a ] && echo "${MU_ARGS_A:-}" || echo "${MU_ARGS_B:-}") > "$out/$side.log" 2>&1 &
  eval "pid_$side=$!"
done
wait $pid_a; ra=$?; wait $pid_b; rb=$?
pkill -P $mock 2>/dev/null; pkill -P $mockhttp 2>/dev/null; kill $mock $mockhttp 2>/dev/null; wait $mock $mockhttp 2>/dev/null
echo "side a exit $ra, side b exit $rb"
grep -h "match" "$out/mock.log" | head -4
grep -h "mock http:" "$out/http.log" | head -12
for side in a b; do echo "== $side"; grep -E "kept search|search:|net check|online: on|Opponent|play while" "$out/$side.log" | sed 's/^\[ *[0-9.]*\] //' | head -14; done
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
