#!/bin/bash
# melee-unlocked's Multiplayer Lobby on the Mac launcher (port/runtime/host/mu_lobby.cpp, mu_launcher_lobby.inl) without the public
# DHT or any real server: two launchers (each under a scratch HOME, its window off every screen, driven by a lobby script:
# MELEE_LAUNCHER_LOBBY_SCRIPT) and a stand-in for a Melee Unlocked PC (mu-engine/tools/lobby_peer.cpp: melee-unlocked's own lobby
# code as fetched, both match transports in its hello; its game the app's engine) find each other through a local one-node BitTorrent DHT
# (tools/mac/dht_bootstrap.py, given to every lobby in lobby-profile.json as "url": "dht://ADDRESS:PORT", upstream's own developer
# key). jech/dht refuses 127.0.0.0/8 as martian, so the node and the lobbies use this Mac's own LAN address, which the kernel delivers
# locally. Everything runs under a sandbox that refuses every outgoing connection except to localhost and to the test's own UDP
# ports (the DHT node, the three lobbies, whose games take over their lobby's port): nothing can reach the public DHT or Slippi.
# The Slippi Direct match uses the loopback stand-in for the matchmaking queue (tools/mac/mock_matchmaking_ws.py) and made-up users.
# The games are the app's engine, hidden, silent (--volume 0, no audio device), alerts to the log; the request chime is written to
# a WAV file (MELEE_LOBBY_CHIME_FILE), never played.
#
#   1  a, b and the PC stand-in go online; every lobby lists the other two through the DHT
#   2  public chat: a writes, b receives
#   3  private chat: a asks b, b accepts, a writes in the room, b receives
#   4  auto reject: a switches "Auto reject invites" on; the PC stand-in sends a Slippi Direct request to a: declined at once
#   5  peer-to-peer match: the PC stand-in asks b for melee-unlocked's own match; b's request chime (660 then 880 Hz); b accepts;
#      b's lobby closes, its game plays the mu_net session against the stand-in's game (the app's engine, as a Slippi-layer PC); b back on the lobby page with
#      the lobby open again; the result in b's history
#   6  Slippi Direct match: a asks b, b accepts; both games go to Training and meet on Slippi Direct (the mock queue); the match
#      runs to its end (generated inputs); both back on the lobby page; the result in both histories
#   7  the Match history tab on both (screenshots)
#   lobby_loopback_test.sh ISO
# Output: build/mu-engine/runs/lobby/ (a/, b/ homes and launcher logs, pc/ the stand-in, pc-game/ its game, shots/).
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"
run="$repo/build/mu-engine/runs/lobby"; out="$repo/build/mu-engine/out"
launcher="${MU_LAUNCHER:-$repo/build/mac/port/melee_launcher_mac}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
res="${MU_RESOURCES:-$repo/dist/melee-unlocked-macOS.app/Contents/Resources}"   # the launcher's artwork and languages (optional)
ip=""; for ifc in en0 en1 en2 en3 en4 en5; do ip=$(ipconfig getifaddr $ifc 2>/dev/null) && [ -n "$ip" ] && break; done
[ -n "$ip" ] || { echo "lobby test: this Mac has no LAN address to give the local DHT node (jech/dht refuses 127.0.0.1)" >&2; exit 1; }
base=$((47100 + (RANDOM % 200) * 8)); dht=$base; pa=$((base + 1)); pb=$((base + 2)); pc=$((base + 3)); mm=$((base + 4)); hp=$((base + 5))
rm -rf "$run"; mkdir -p "$run/shots" "$run/pc" "$run/pc-game"
# The PC stand-in: melee-unlocked's lobby code as fetched, headless, built here (the fetched files as the launcher's build copies them).
gen="$repo/build/mac/mu-engine/app/gen/lobby"; T="$repo/build/mu-engine/mu"
[ -f "$gen/launcher_lobby_p2p.cpp" ] || { echo "lobby test: build the launcher first (its gen/lobby copy)" >&2; exit 1; }
ver=$(head -1 "$T/VERSION" | tr -d '\r')
( cd "$repo"; o=build/mu-engine/obj-lobby-peer; mkdir -p $o
  for f in dht/dht.c monocypher/monocypher.c monocypher/monocypher-ed25519.c; do clang -arch arm64 -O2 -w -c "$T/port/third_party/$f" -o $o/$(basename $f .c).o; done
  clang++ -arch arm64 -std=gnu++17 -O2 -w -DMU_PEER_BUILD="\"$ver:source\"" -Imu-engine/lobby_shim -I"$gen" -I"$T/port/third_party/dht" -I"$T/port/third_party/monocypher" \
    -Iport/third_party mu-engine/tools/lobby_peer.cpp "$gen/launcher_lobby_p2p.cpp" mu-engine/lobby_shim/lobby_shim.cpp $o/*.o -o "$out/lobby-peer" ) || exit 1
profile="(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip \"localhost:*\"))"
for p in $dht $pa $pb $pc; do profile="$profile(allow network-outbound (remote ip \"*:$p\"))"; done
sandboxed() { sandbox-exec -p "$profile" "$@"; }
# The two launchers' homes: the disc, made-up Slippi users (MOCK#001, MOCK#002: the engine's --fake-user names), the lobby profile.
for side in a b; do
  d="$run/$side/home/Library/Application Support/Dashdance"; mkdir -p "$d/Slippi" "$d/melee-unlocked/Lobby"
  n=1; name="Lobby A"; mains="[20, 2, 9]"; where="Test bench A"; port=$pa
  [ $side = b ] && { n=2; name="Lobby B"; mains="[2, 20, 9]"; where="Test bench B"; port=$pb; }
  printf 'iso=%s\nfullscreen=0\nonline=1\nonline_delay=2\nvolume=0\n' "$iso" > "$d/launcher.ini"
  printf 'slippi_menus 1\n' > "$d/melee-unlocked/settings.ini"
  printf '{"uid":"mock-test-lobby-%s","playKey":"mock-test-only","connectCode":"MOCK#%03d","displayName":"%s"}\n' $side $n "$name" > "$d/Slippi/user.json"
  printf '{"mode": "peer", "url": "dht://%s:%d", "peer_port": %d, "name": "", "location": "%s", "mains": %s}\n' "$ip" $dht $port "$where" "$mains" > "$d/melee-unlocked/Lobby/lobby-profile.json"
  # The match's controller script: in Slippi's online match (scene 8/2) the stick held left with a jump every 20 frames (a) or
  # 60 (b), so the fighters leave the stage and the match ends with one side out of stocks.
  { echo "# lobby_loopback_test.sh: off the stage in the online match"; echo "@scene 08 02"
    every=$([ $side = a ] && echo 20 || echo 60); for t in $(seq 60 $every 30000); do echo "$t X sx=-127"; echo "$((t + 6)) sx=-127"; done; } > "$run/$side/pads.txt"
  # The engine as the launcher starts it, plus the test's options: hidden, silent, a made-up user against the mock queue, the
  # script above; a peer-to-peer match plays the game's generated test inputs.
  cat > "$run/$side/engine.sh" <<SH
#!/bin/bash
for a in "\$@"; do case "\$a" in --p2p-port) exec "$engine" "\$@" --hidden --volume 0 --no-audio-device --frames 20000 --net-inputs $((n + 2)) --sys-dir "$repo/port/slippi_sys" ;; esac; done
exec "$engine" "\$@" --hidden --volume 0 --no-audio-device --frames 36000 --sys-dir "$repo/port/slippi_sys" --fake-user --fake-user-index $n --user-dir "$run/$side/net-user" --play-script "$run/$side/pads.txt"
SH
  chmod +x "$run/$side/engine.sh"
done
: > "$run/pc/commands.txt"
# Each launcher's lobby script (mu_launcher_lobby.inl lobbyScriptStep): the harness below hands steps over through files.
S="$run/shots"
cat > "$run/a/script.txt" <<EOF
online
wait-players 2
sleep 2500
shot $S/a-roster.png
chat Hello from the Mac launcher
pm Lobby B
pm-text A private hello
sleep 1500
shot $S/a-pm-open.png
auto-reject on
tab 3
sleep 800
shot $S/a-profile-auto-reject.png
tab 0
log ready for the auto reject
wait-notice automatically declined
sleep 500
shot $S/a-auto-rejected.png
auto-reject off
wait-file $run/go-a-direct
request Lobby B
wait-history 1
sleep 2500
shot $S/a-back-after-direct.png
tab 2
sleep 800
shot $S/a-history.png
wait-file $run/go-quit
quit
EOF
cat > "$run/b/script.txt" <<EOF
online
wait-players 2
wait-chat Hello from the Mac launcher
sleep 800
shot $S/b-chat.png
pm-accept
wait-pm A private hello
sleep 1000
shot $S/b-pm-open.png
tab 0
wait-request
sleep 1200
shot $S/b-p2p-incoming.png
accept
wait-history 1
sleep 2500
shot $S/b-back-after-p2p.png
wait-request
sleep 800
shot $S/b-direct-incoming.png
accept
wait-history 2
sleep 2500
shot $S/b-back-after-direct.png
tab 2
sleep 800
shot $S/b-history.png
wait-file $run/go-quit
quit
EOF
# Whatever happens (an interrupted run too), nothing this test started is left running.
cleanup() {
  kill ${mockpid:-} ${dhtpid:-} ${httppid:-} ${peerpid:-} 2>/dev/null
  pkill -f "$launcher" 2>/dev/null; pkill -f "$out/lobby-peer" 2>/dev/null; pkill -f "$run/pc-game" 2>/dev/null; pkill -f "$engine" 2>/dev/null
  pkill -f "$repo/tools/mac/dht_bootstrap.py" 2>/dev/null; pkill -f "$repo/tools/mac/mock_" 2>/dev/null
}
trap cleanup EXIT; trap "exit 1" INT TERM
sandboxed python3 -I "$repo/tools/mac/dht_bootstrap.py" "$ip" $dht > "$run/dht.txt" 2>&1 & dhtpid=$!
( while :; do sandboxed python3 -I "$repo/tools/mac/mock_matchmaking_ws.py" $mm; sleep 0.2; done ) > "$run/mock.txt" 2>&1 & mockpid=$!
sandboxed python3 -I "$repo/tools/mac/mock_slippi_http.py" $hp --save "$run/uploads" > "$run/http.txt" 2>&1 & httppid=$!
sleep 1
start_launcher() {   # side [extra environment]
  local side=$1; shift
  ( env HOME="$run/$side/home" MELEE_UPDATE_CHECK=0 MELEE_NO_ALERTS=1 MELEE_LAUNCHER_OFFSCREEN=1 MELEE_LAUNCHER_PAGE=4 MELEE_LAUNCHER_NOTES=0 \
      MELEE_LAUNCHER_LOBBY_SCRIPT="$run/$side/script.txt" MELEE_MU_ENGINE="$run/$side/engine.sh" MELEE_MU_ART="$res/mu-art" MELEE_MU_LANG="$res/mu-lang" \
      MELEE_MM_LOOPBACK_PORT=$mm MELEE_SERVICES_LOOPBACK_PORT=$hp "$@" sandbox-exec -p "$profile" timeout 1500 "$launcher" > "$run/$side/launcher.out" 2>&1
    echo "exit code $?" >> "$run/$side/launcher.out" ) &
}
start_launcher a
start_launcher b MELEE_LOBBY_CHIME_FILE="$run/b/chime.wav"
# The stand-in's game: the app's engine (hidden, silent) with Slippi's files, as a melee-unlocked PC with the Slippi layer runs its
# game for a peer-to-peer match (upstream's g_slippi_menus: the option word's MU_GAME_OPT_SLIPPI_MENUS, which both sides must share).
mkdir -p "$run/pc-game/melee-unlocked"; printf 'slippi_menus 1\n' > "$run/pc-game/melee-unlocked/settings.ini"
cat > "$run/pc/game.sh" <<SH
#!/bin/bash
args=(); while [ \$# -gt 0 ]; do case "\$1" in --out) shift; prof="\$1" ;; *) args+=("\$1") ;; esac; shift; done
exec "$engine" "\${args[@]}" --profile-dir "\$prof" --hidden --volume 0 --no-audio-device --sys-dir "$repo/port/slippi_sys" --lobby-status-file "\$prof/status.json"
SH
chmod +x "$run/pc/game.sh"
( cd "$run/pc" && PEER_GAME="$run/pc/game.sh" PEER_ISO="$iso" PEER_OUT="$run/pc-game" sandboxed timeout 1500 "$out/lobby-peer" "$run/pc" "dht://$ip:$dht" $pc 1400 "Lobby PC" "LOBC#303" "$run/pc/commands.txt" > "$run/peer.txt" 2>&1 ) & peerpid=$!
llog() { echo "$run/$1/home/Library/Application Support/Dashdance/melee-unlocked/launcher.log"; }
net() { echo "$run/$1/home/Library/Application Support/Dashdance/melee-unlocked/net/digest-net-confirmed.csv"; }
wait_log() { for i in $(seq $(( $3 * 10 ))); do grep -aq "$2" "$1" 2>/dev/null && return 0; sleep 0.1; done; echo "TIMEOUT waiting for: $2 in $1" >> "$run/harness.log"; return 1; }
# 4 auto reject: once a has switched it on, the stand-in asks a for a Slippi Direct match.
wait_log "$(llog a)" "lobby script: ready for the auto reject" 300 && { sleep 1; echo "request Lobby A" >> "$run/pc/commands.txt"; }
wait_log "$(llog a)" "automatically declined" 60
wait_log "$run/peer.txt" "Lobby A declined" 60
# 5 the peer-to-peer match: the stand-in asks b (once its declined request is gone).
sleep 5; echo "p2p Lobby B" >> "$run/pc/commands.txt"
wait_log "$(llog b)" "the lobby match's game ended" 600 && cp "$(net b)" "$run/b/p2p-digest.keep.csv" 2>/dev/null
wait_log "$(llog b)" "lobby: open again" 60
# 6 the Slippi Direct match: a asks b.
sleep 4; touch "$run/go-a-direct"
wait_log "$(llog a)" "lobby script: wait-history 1" 900
wait_log "$(llog b)" "lobby script: wait-history 2" 300
sleep 8; touch "$run/go-quit"
for i in $(seq 300); do grep -q "exit code" "$run/a/launcher.out" 2>/dev/null && grep -q "exit code" "$run/b/launcher.out" 2>/dev/null && break; sleep 0.2; done
kill $peerpid 2>/dev/null; pkill -f "$out/lobby-peer" 2>/dev/null; pkill -f "$run/pc-game" 2>/dev/null
kill $dhtpid $httppid 2>/dev/null; pkill -P $mockpid 2>/dev/null; kill $mockpid 2>/dev/null; wait 2>/dev/null
[ -f "$run/harness.log" ] && cat "$run/harness.log"
echo "== DHT node ($ip:$dht)"; grep -E "node on|announce on the public|get_peers on the public" "$run/dht.txt" | sed 's/:[0-9]*: [0-9]* peer(s)//' | sort | uniq -c | sort -rn | head -6
echo "== the melee-unlocked PC stand-in (upstream lobby code as fetched)"; grep -vE "^peer: [0-9]+ player" "$run/peer.txt" | cut -c1-200 | head -20
grep -E "^peer: [0-9]+ player" "$run/peer.txt" | cut -c1-300 | tail -3
echo "== the stand-in's game (the app's engine)"; grep -aE "p2p: (connected|both sides|session|game end|leaving)|net check:|stopped" "$run/pc-game/melee-unlocked/engine.log" 2>/dev/null | sed 's/^\[ *[0-9.]*\] //' | cut -c1-200 | head -8
echo "== matchmaking stand-in"; grep -h "match" "$run/mock.txt" | tail -2
for side in a b; do
  echo "== $side"; grep -aE "lobby: |lobby script: (online|chat|pm|accept|request|auto|quit)|lobby match" "$(llog $side)" | grep -v "lobby: chat$\|lobby: pm$" | sed 's/^\[[^]]*\] //' | cut -c1-200 | head -60
done
# The request chime in side b's WAV: 660 and 880 Hz (Goertzel) against a 440 Hz control.
python3 -I - "$run/b/chime.wav" <<'PY' || true
import math, struct, sys
try: data = open(sys.argv[1], "rb").read()
except OSError: print("request sound: not written"); sys.exit(0)
rate = struct.unpack("<I", data[24:28])[0]; pcm = data[44:]
x = [v / 32768 for v in struct.unpack("<%dh" % (len(pcm) // 2), pcm)]
def power(freq, xs):
    k = 2 * math.cos(2 * math.pi * freq / rate); s1 = s2 = 0.0
    for v in xs: s = v + k * s1 - s2; s2 = s1; s1 = s
    return (s1 * s1 + s2 * s2 - k * s1 * s2) / ((sum(v * v for v in xs) or 1) * len(xs) / 2)
h = len(x) // 2
print("request sound: %.2f s at %d Hz; first half at 660 Hz %.2f (440 Hz control %.3f), second half at 880 Hz %.2f (control %.3f)" % (len(x) / rate, rate, power(660, x[:h]), power(440, x[:h]), power(880, x[h:]), power(440, x[h:])))
PY
if [ -s "$run/b/p2p-digest.keep.csv" ] && [ -s "$run/pc-game/melee-unlocked/net/digest-net-confirmed.csv" ]; then
  python3 -I "$here/compare_net_digests.py" "$run/b/p2p-digest.keep.csv" "$run/pc-game/melee-unlocked/net/digest-net-confirmed.csv" | sed 's/^/peer-to-peer match (b vs the stand-in): /'
else echo "peer-to-peer match: digests MISSING"; fi
if [ -s "$(net a)" ] && [ -s "$(net b)" ]; then
  for side in a b; do awk -F, '$2==8 && $3==2' "$(net $side)" > "$run/$side/direct-match.csv"; done
  python3 -I "$here/compare_net_digests.py" "$run/a/direct-match.csv" "$run/b/direct-match.csv" | sed 's/^/Slippi Direct match (a vs b): /'
else echo "Slippi Direct match: digests MISSING"; fi
for side in a b; do
  h="$run/$side/home/Library/Application Support/Dashdance/melee-unlocked/Lobby/lobby-history.json"
  echo "history $side: $(python3 -I -c 'import json,sys; h=json.load(open(sys.argv[1])); print("; ".join("%s vs %s (%s)" % (g["result"], g["opponent"], g.get("transport","")) for g in h))' "$h" 2>/dev/null || echo none)"
  e="$run/$side/home/Library/Application Support/Dashdance/melee-unlocked"
  echo "desync, $side: $(cat "$e"/engine*.log 2>/dev/null | grep -aoE '[0-9]+ desync messages from the game' | tr '\n' ' '); p2p: $(cat "$e"/engine*.log 2>/dev/null | grep -aoE '[0-9]+ checksums compared, [0-9]+ mismatched' | tail -1)"
done
echo "stand-in game: $(grep -aoE '[0-9]+ checksums compared, [0-9]+ mismatched' "$run/pc-game/melee-unlocked/engine.log" 2>/dev/null | tail -1); its result: $(python3 -I -c 'import json,sys; b=json.load(open(sys.argv[1]))["body"]; print("slot %s winner %s desync %s" % (b.get("local_slot"), b.get("winner"), b.get("desync")))' "$run/pc-game/p2p-result.json" 2>/dev/null)"
echo "shots: $(ls "$run/shots" | wc -l | tr -d ' ') in $run/shots"
