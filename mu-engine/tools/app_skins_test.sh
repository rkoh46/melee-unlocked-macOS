#!/bin/bash
# Skins in the app's engine (fe/features/cosmetics.cpp, from Melee-NX-NetPlay): two costumes made here from the disc's own
# PlFcNr.dat in <profile>/melee-unlocked/Cosmetics/Costumes (one with a joint moved by 1e-4, inside melee-unlocked's
# tolerance, one moved by 5 units; never committed), then a hidden offline start with --feature-test cosmetics-verdicts,
# which logs melee-unlocked's online verdict (its pack skin rule and skeleton check) for every costume skin.
#   app_skins_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
out="$repo/build/mu-engine/runs/app-skins"; rm -rf "$out"; mkdir -p "$out/melee-unlocked/Cosmetics/Costumes"
python3 -I - "$iso" "$out/melee-unlocked/Cosmetics/Costumes" <<'PY'
import struct, sys
iso, out = sys.argv[1], sys.argv[2]
f = open(iso, "rb"); f.seek(0x424); fst_off, fst_size = struct.unpack(">II", f.read(8)); f.seek(fst_off); fst = f.read(fst_size)
count = struct.unpack(">I", fst[8:12])[0]; names = count * 12
for i in range(1, count):
    e = fst[i*12:i*12+12]
    if e[0]: continue
    n = fst[names + (struct.unpack(">I", e[0:4])[0] & 0xFFFFFF):].split(b"\0")[0].decode()
    if n.lower() == "plfcnr.dat":
        off, size = struct.unpack(">II", e[4:12]); f.seek(off); dat = bytearray(f.read(size)); break
dsize, nrel, nroot, nref = struct.unpack(">IIII", dat[4:20])
table = 0x20 + dsize + nrel * 4; strings = table + (nroot + nref) * 8
for r in range(nroot):
    j, name_at = struct.unpack(">II", dat[table + r*8: table + r*8 + 8])
    if dat[strings + name_at:].split(b"\0")[0].endswith(b"_Share_joint"): root = j; break
child = struct.unpack(">I", dat[0x20 + root + 8: 0x20 + root + 12])[0]
at = 0x20 + child + 0x2C
for label, delta in (("Nudged", 1e-4), ("Moved bone", 5.0)):
    b = bytearray(dat); x = struct.unpack(">f", b[at:at+4])[0]; b[at:at+4] = struct.pack(">f", x + delta)
    open(f"{out}/PlFcNr - {label}.dat", "wb").write(bytes(b))
PY
timeout 300 "$engine" --iso "$iso" --profile-dir "$out" --offline --hidden --volume 0 --no-audio-device --frames 400 --feature-test cosmetics-verdicts > "$out/log" 2>&1
echo "engine exit $?"
grep -h "cosmetics" "$out/log" | sed 's/^\[ *[0-9.]*\] //' | cut -c1-220
