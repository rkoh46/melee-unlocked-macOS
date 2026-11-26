#!/bin/bash
# CISO and GCZ discs (mu-engine/app/disc_ciso_gcz.cpp, from Melee-NX-NetPlay's tools/ciso_gcz_check.sh): a 48 MiB made-up image (a
# GALE01 header, random, repeated and empty blocks) written as CISO and as GCZ by ciso_gcz_make.py (the formats as Dolphin writes
# them), converted back by the app's code: both must equal the image byte for byte; a GCZ with one byte changed must be refused
# by its block check. With the disc as the second argument, the engine is also started (hidden, no sound) on a CISO copy of it
# (written to a scratch folder, deleted after): it converts it once into GALE01.iso beside it and boots the game from it.
#   ciso_gcz_check.sh [ISO]
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
python3 -I "$here/ciso_gcz_make.py" "$t"
cat > "$t/main.cpp" <<'CPP'
#include "disc_ciso_gcz.h"
#include <cstdio>
int main(int argc, char** argv) {
  std::string error;
  const bool ok = fe::disc_ciso_gcz::convert(argv[1], argv[2], nullptr, &error);
  std::printf("%s -> %s: %s%s\n", argv[1], argv[2], ok ? "converted" : "refused: ", error.c_str());
  return ok ? 0 : 1;
}
CPP
clang++ -std=c++17 -O2 -I"$repo/mu-engine/app" "$t/main.cpp" "$repo/mu-engine/app/disc_ciso_gcz.cpp" -lz -o "$t/conv"
for f in ciso gcz; do "$t/conv" "$t/disc.$f" "$t/out-$f.iso" > /dev/null; cmp "$t/disc.iso" "$t/out-$f.iso" && echo "$f: identical to the image ($(stat -f %z "$t/out-$f.iso") bytes)"; done
if "$t/conv" "$t/bad.gcz" "$t/out-bad.iso"; then echo "bad.gcz: ACCEPTED (wrong)"; else echo "bad.gcz: refused, as it must be"; fi
if [ -n "${1:-}" ]; then
  mkdir -p "$t/disc" "$t/profile/melee-unlocked"
  python3 -I - "$1" "$t/disc/melee.ciso" <<'PY'
import struct, sys
block = 2 << 20
src, dst = sys.argv[1], sys.argv[2]
with open(src, "rb") as f: data = f.read()
blocks = [data[i:i + block] for i in range(0, len(data), block)]
blocks[-1] = blocks[-1].ljust(block, b"\0")
header = bytearray(0x8000); header[0:4] = b"CISO"; header[4:8] = struct.pack("<I", block)
with open(dst, "wb") as out:
    kept = 0
    for i, b in enumerate(blocks):
        if any(b) or i == len(blocks) - 1: header[8 + i] = 1; kept += 1
    out.write(header)
    for i, b in enumerate(blocks):
        if header[8 + i]: out.write(b)
print("melee.ciso: %d of %d blocks stored" % (kept, len(blocks)))
PY
  MELEE_NO_ALERTS=1 MELEE_UPDATE_CHECK=0 timeout 300 "$engine" --iso "$t/disc/melee.ciso" --profile-dir "$t/profile" --offline --hidden --volume 0 \
    --no-audio-device --frames 300 --no-record > "$t/log" 2>&1 || true
  grep -hE "disc: |play: scene 40|play: scene 24" "$t/log" | sed 's/^\[ *[0-9.]*\] //' | grep -v "converting: [1-9]" | head -8
  head -c 6 "$t/disc/GALE01.iso" 2>/dev/null | xargs -0 printf "GALE01.iso starts with %s; " || true
  echo "$(stat -f %z "$t/disc/GALE01.iso") bytes written, the disc has $(stat -f %z "$1")"; python3 -I -c "
import sys
a, b = open(sys.argv[1], 'rb'), open(sys.argv[2], 'rb')
n = 0
while True:
    x = a.read(1 << 24)
    if not x: break
    if b.read(len(x)) != x: print('DIFFERENT from the disc near byte', n); sys.exit(1)
    n += len(x)
print('every byte of the disc identical; the rest zeros' if not b.read().strip(b'\\0') else 'extra bytes are not zeros')" "$1" "$t/disc/GALE01.iso" || true
fi
