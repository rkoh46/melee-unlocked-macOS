#!/bin/bash
# The Lab view's silhouette packs downloaded when the silhouette look finds none (gfx/lab_mac.cpp, from Melee-NX-NetPlay 0.4.8):
# never from the internet here. A stand-in for melee-unlocked's content release on 127.0.0.1 (python's http.server under a
# sandbox that refuses every other outgoing connection) serves a made-up MeleeUnlocked-Lab.zip of 27 small <id>.lab files
# (made here; not real packs, so the view then reports them unreadable or draws markers); MU_LAB_URL / MU_LAB_SHA256 name it
# (honoured only on the loopback with MELEE_PRO_AI_TEST_MIRROR). Case good: the zip's own hash, the 27 packs installed and the
# zip deleted. Case bad: a wrong hash, refused, nothing installed.
#   app_lab_packs_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
root="$repo/build/mu-engine/runs/lab-packs"; rm -rf "$root"; mkdir -p "$root/www"
python3 -I - "$root/www/MeleeUnlocked-Lab.zip" <<'PY'
import sys, zipfile
with zipfile.ZipFile(sys.argv[1], "w", zipfile.ZIP_DEFLATED) as z:
    for i in range(27): z.writestr("MeleeUnlocked-Lab/%d.lab" % i, b"test pack %d, not a real silhouette\n" % i * 40)
PY
sha=$(shasum -a 256 "$root/www/MeleeUnlocked-Lab.zip" | cut -d' ' -f1)
port=$((45500 + RANDOM % 400))
profile='(version 1)(allow default)(deny network-outbound)(allow network-outbound (remote ip "localhost:*"))'
sandbox-exec -p "$profile" python3 -I -m http.server "$port" --bind 127.0.0.1 --directory "$root/www" > "$root/http.log" 2>&1 &
server=$!
sleep 1
python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$root/vs.txt"
for case in good bad; do
  dir="$root/$case"; mkdir -p "$dir/melee-unlocked"
  printf '%s\n' "labview 1" "look_lab_fighters 0" > "$dir/melee-unlocked/settings.ini"
  want=$sha; [ $case = bad ] && want=0000000000000000000000000000000000000000000000000000000000000000
  MU_LAB_URL="http://127.0.0.1:$port/MeleeUnlocked-Lab.zip" MU_LAB_SHA256=$want MELEE_PRO_AI_TEST_MIRROR=1 MELEE_NO_ALERTS=1 MELEE_UPDATE_CHECK=0 \
    sandbox-exec -p "$profile" timeout 300 "$engine" --iso "$iso" --profile-dir "$dir" --offline --hidden --volume 0 --no-audio-device \
    --frames 1800 --no-record --play-script "$root/vs.txt" > "$dir/log" 2>&1
  echo "== $case: engine exit $?, $(ls "$dir/melee-unlocked/Lab" 2>/dev/null | grep -c '\.lab$') packs in Lab/, zip left: $(ls "$dir/melee-unlocked/Lab"/*.zip 2>/dev/null | wc -l | tr -d ' ')"
  grep -hE "lab view: (downloading|download failed|.*installed|unpacking)" "$dir/log" | sed "s/^\[ *[0-9.]*\] //" | cut -c1-200
done
kill $server 2>/dev/null; wait $server 2>/dev/null
echo "requests to the stand-in: $(grep -c 'GET /MeleeUnlocked-Lab.zip' "$root/http.log")"
