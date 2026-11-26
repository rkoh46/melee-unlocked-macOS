#!/bin/bash
# melee-unlocked's own peer-to-peer match (mu_net, P2P Direct: mu-engine/runner/p2p_bridge.cpp, net_host's --p2p-* mode) between two
# instances on loopback, as a lobby match starts it: two headless runners, or with "app" two of the app's engines (hidden, silent).
# Generated inputs drive both sides until the match is over. Under a sandbox that refuses every outgoing connection except to
# localhost. Afterwards: mu_net's checksum counts, the game's net checks, the result files and the confirmed states compared.
#   p2p_loopback_test.sh ISO [runner|app]
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; kind="${2:-runner}"
out="$repo/build/mu-engine/runs/p2p-$kind"; pa=$((45000 + RANDOM % 400)); pb=$((pa + 1))
rm -rf "$out"; mkdir -p "$out/a" "$out/b"
profile='(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip "localhost:*"))'
for side in a b; do
  slot=0; lp=$pa; rp=$pb; seed=3; [ $side = b ] && { slot=1; lp=$pb; rp=$pa; seed=4; }
  p2p=(--p2p-port $lp --p2p-peer 127.0.0.1:$rp --p2p-slot $slot --p2p-chars 2/0:20/0 --p2p-stage 31 --p2p-seed 1234ABCD --p2p-delay 2
       --p2p-identity "$out/$side/id.json" --p2p-result "$out/$side/result.json" --p2p-names Alpha:Beta --p2p-connect-seconds 30)
  if [ "$kind" = app ]; then
    MELEE_NO_ALERTS=1 sandbox-exec -p "$profile" timeout 600 "$repo/build/mac/mu-engine/app/MeleeUnlockedEngine" --iso "$iso" --profile-dir "$out/$side" \
      --hidden --volume 0 --no-audio-device --frames 9000 --net-inputs $seed "${p2p[@]}" --lobby-status-file "$out/$side/status.json" > "$out/$side.log" 2>&1 &
  else
    sandbox-exec -p "$profile" timeout 600 "$repo/build/mu-engine/out/mu-runner" --iso "$iso" --out "$out/$side" --frames 9000 --net-inputs $seed "${p2p[@]}" > "$out/$side.log" 2>&1 &
  fi
  eval "pid_$side=$!"
done
wait $pid_a; ra=$?; wait $pid_b; rb=$?
echo "side a exit $ra, side b exit $rb"
for side in a b; do
  log="$out/$side.log"; [ "$kind" = app ] && log="$out/$side/melee-unlocked/engine.log"
  echo "== $side"; grep -aE "p2p: (connected|both sides|game end|session:|leaving)|net check:|desync" "$log" | sed 's/^\[ *[0-9.]*\] //' | head -8
done
digest() { [ "$kind" = app ] && echo "$out/$1/melee-unlocked/net/digest-net-confirmed.csv" || echo "$out/$1/digest-net-confirmed.csv"; }
python3 -I "$here/compare_net_digests.py" "$(digest a)" "$(digest b)" | sed 's/^/confirmed states: /'
python3 -I - "$out/a/result.json" "$out/b/result.json" <<'PY'
import json, sys
for side, path in zip("ab", sys.argv[1:]):
    try: b = json.load(open(path))["body"]
    except Exception as e: print("result %s: none (%s)" % (side, e)); continue
    print("result %s: slot %s, winner %s, outcome %s, desync %s, disconnected %s, stocks %s" % (side, b.get("local_slot"), b.get("winner"), b.get("has_outcome"),
          b.get("desync"), b.get("disconnected"), [p.get("stocks") for p in b.get("players", [])]))
PY
