#!/bin/bash
# The engine's picture effects (fe/features/graphics.cpp, mu-engine/app/gfx): hidden offline VS matches on Onett from
# play_vs_script.py with each effect on, pictures from the capture burst (MELEE_CAPTURE_BURST=FRAME,COUNT: the picture as
# presented) or --capture with MELEE_CAPTURE_UI (the ImGui pass drawn over it, for the Lab view). Fixtures are made here:
# the texture pack from the game's own dumped textures (colours inverted), the menu video by ffmpeg's test pattern (when ffmpeg
# is installed). Nothing is written outside build/mu-engine/runs/app-graphics; no Finder window opens.
#   app_graphics_test.sh ISO [CASE...]   cases: ssao msaa capture glitch texpack video lab shader (default: all)
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; shift; cases="${*:-ssao msaa capture glitch texpack video lab shader}"
engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
root="$repo/build/mu-engine/runs/app-graphics"; mkdir -p "$root"
python3 -I "$here/play_vs_script.py" "$repo/build/mu-engine/mu/port/scripts/parity_vs_onett.txt" "$root/vs.txt"
fresh() { rm -rf "$root/$1"; mkdir -p "$root/$1/melee-unlocked"; printf '%b' "$2" > "$root/$1/melee-unlocked/settings.ini"; }
run() {   # $1 case folder, $2 frames, then extra engine arguments
  local name="$1" dir="$root/$1" frames="$2"; shift 2
  MELEE_UPDATE_CHECK=0 MELEE_NO_FINDER=1 timeout 600 "$engine" --iso "$iso" --profile-dir "$dir" --offline --hidden --volume 0 \
    --no-audio-device --no-record --frames "$frames" --play-script "$root/vs.txt" "$@" >> "$dir/log" 2>&1
  echo "$name: engine exit $?"
}
mean_diff() {   # two PPMs: the mean absolute difference per channel, and the share of pixels that differ
  python3 -I - "$1" "$2" <<'PY'
import sys
def read(p):
    d = open(p, "rb").read(); h = d.split(b"\n", 3); return d[len(b"\n".join(h[:3])) + 1:]
a, b = read(sys.argv[1]), read(sys.argv[2])
n = min(len(a), len(b)); diff = sum(abs(a[i] - b[i]) for i in range(0, n, 7)); px = sum(1 for i in range(0, n - 2, 21) if a[i:i+3] != b[i:i+3])
print("mean difference %.2f, %.1f%% of pixels differ" % (diff / (n / 7), 100.0 * px / (n / 21)))
PY
}
for c in $cases; do case "$c" in
ssao)
  fresh ssao-off 'ssao 0\n'; MELEE_CAPTURE_BURST=1700,1 run ssao-off 1720
  fresh ssao-on 'ssao 1\n'; MELEE_CAPTURE_BURST=1700,1 run ssao-on 1720
  echo "ssao: $(mean_diff "$root/ssao-off/melee-unlocked/capture/blink_01700.ppm" "$root/ssao-on/melee-unlocked/capture/blink_01700.ppm")";;
msaa)   # launcher.ini msaa (melee-unlocked's key) at a fixed internal resolution, against the same frame without it
  fresh msaa-off ''; printf 'msaa=1\nscale=2\n' > "$root/msaa-off/launcher.ini"; MELEE_CAPTURE_BURST=1700,1 run msaa-off 1720 --scale 2
  fresh msaa-4 ''; printf 'msaa=4\nscale=2\n' > "$root/msaa-4/launcher.ini"; MELEE_CAPTURE_BURST=1700,1 run msaa-4 1720 --scale 2
  grep -h "MSAA" "$root/msaa-4/log" | sed 's/^\[ *[0-9.]*\] //' | head -2
  echo "msaa: $(mean_diff "$root/msaa-off/melee-unlocked/capture/blink_01700.ppm" "$root/msaa-4/melee-unlocked/capture/blink_01700.ppm")";;
capture)
  fresh capture ''; MELEE_CAPTURE_BURST=1600,90 run capture 1720
  echo "capture: $(ls "$root/capture/melee-unlocked/capture" | wc -l | tr -d ' ') pictures; $(grep -c 'captured ' "$root/capture/log") logged";;
glitch)
  fresh glitch 'flickerscan 1\n'; MELEE_TEST_GLITCH_AT=1650 run glitch 1720
  echo "glitch: $(grep -c 'flicker:' "$root/glitch/log") reports; the injected frame: $(grep 'presented frame 1650 ' "$root/glitch/log" | sed 's/^\[ *[0-9.]*\] //' | head -1)";;
texpack)
  fresh texdump 'dumptextures 1\n'; run texdump 1720
  dump="$root/texdump/melee-unlocked/Dump/Textures/GALE01"
  echo "texpack: $(ls "$dump" 2>/dev/null | wc -l | tr -d ' ') textures dumped"
  fresh texpack 'customtextures 1\nprefetchtextures 1\n'; pack="$root/texpack/melee-unlocked/TexturePacks/Inverted"; mkdir -p "$pack"
  # Every dumped texture bigger than 64 x 64, colours inverted (sips cannot invert: a small PNG round trip through Core Image)
  python3 -I - "$dump" "$pack" <<'PY'
import os, subprocess, sys, struct, zlib
src, out = sys.argv[1], sys.argv[2]
def png_read(p):
    d = open(p, "rb").read(); i = 8; chunks = []; w = h = 0; idat = b""
    while i < len(d):
        n, = struct.unpack(">I", d[i:i+4]); t = d[i+4:i+8]; body = d[i+8:i+8+n]; i += 12 + n
        if t == b"IHDR": w, h, bd, ct = struct.unpack(">IIBB", body[:10])
        elif t == b"IDAT": idat += body
    if ct != 6 or bd != 8: return None
    raw = zlib.decompress(idat); stride = w * 4; rows = []; prev = bytearray(stride); p = 0
    for y in range(h):
        f = raw[p]; line = bytearray(raw[p+1:p+1+stride]); p += 1 + stride
        for x in range(stride):
            a = line[x-4] if x >= 4 else 0; b = prev[x]; c = prev[x-4] if x >= 4 else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + ((a + b) >> 1)) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(line); prev = line
    return w, h, rows
def png_write(p, w, h, rows):
    raw = b"".join(b"\0" + bytes(r) for r in rows)
    def chunk(t, b): return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b))
    open(p, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
n = 0
for f in sorted(os.listdir(src)):
    if not f.startswith("tex1_") or "_mip" in f: continue
    r = png_read(os.path.join(src, f))
    if not r or r[0] * r[1] < 64 * 64: continue
    w, h, rows = r
    for row in rows:
        for x in range(0, len(row), 4): row[x], row[x+1], row[x+2] = 255 - row[x], 255 - row[x+1], 255 - row[x+2]
    png_write(os.path.join(out, f), w, h, rows); n += 1
print("texpack: %d textures in the test pack" % n)
PY
  MELEE_CAPTURE_BURST=1700,1 run texpack 1720
  fresh texoff ''; MELEE_CAPTURE_BURST=1700,1 run texoff 1720
  grep -h "textures:\|graphics: .*replaced" "$root/texpack/log" | sed 's/^\[ *[0-9.]*\] //' | head -5
  echo "texpack: $(mean_diff "$root/texoff/melee-unlocked/capture/blink_01700.ppm" "$root/texpack/melee-unlocked/capture/blink_01700.ppm")";;
video)
  if ! command -v ffmpeg > /dev/null; then echo "video: skipped (no ffmpeg to make the test video)"; continue; fi
  fresh video 'videobackgrounds 1\n'; mkdir -p "$root/video/melee-unlocked/VideoBackgrounds"
  ffmpeg -loglevel error -f lavfi -i testsrc2=size=640x480:rate=30 -t 4 -pix_fmt yuv420p -c:v h264_videotoolbox -b:v 2M "$root/video/melee-unlocked/VideoBackgrounds/css.mp4" -y \
    || ffmpeg -loglevel error -f lavfi -i testsrc2=size=640x480:rate=30 -t 4 -pix_fmt yuv420p "$root/video/melee-unlocked/VideoBackgrounds/css.mp4" -y
  MELEE_CAPTURE_BURST=800,1 run video 820
  fresh video-off ''; MELEE_CAPTURE_BURST=800,1 run video-off 820
  grep -h "video backgrounds" "$root/video/log" | sed 's/^\[ *[0-9.]*\] //' | sort | uniq -c | sort -rn | head -6
  echo "video: $(mean_diff "$root/video-off/melee-unlocked/capture/blink_00800.ppm" "$root/video/melee-unlocked/capture/blink_00800.ppm")";;
lab)
  for look in 1 2; do
    fresh lab-$look "labview 1\nlook_lab_fighters $look\n"; MELEE_CAPTURE_UI=1 run lab-$look 1720 --capture "$root/lab-$look/lab.ppm" 1700
    grep -h "lab view" "$root/lab-$look/log" | sed 's/^\[ *[0-9.]*\] //' | sort -u | head -3
  done;;
shader)   # mu-engine/tests/shaders/scanlines.slangp (written for this test) through librashader (MELEE_LIBRASHADER: the built dylib)
  lib="${MELEE_LIBRASHADER:-$repo/build/mu-engine/out/librashader.dylib}"
  fresh shader 'shaderpreset test/scanlines.slangp\n'; mkdir -p "$root/shader/melee-unlocked/Shaders/test"
  cp "$repo/mu-engine/tests/shaders/scanlines.slang" "$repo/mu-engine/tests/shaders/scanlines.slangp" "$root/shader/melee-unlocked/Shaders/test/"
  MELEE_LIBRASHADER="$lib" MELEE_CAPTURE_BURST=1600,1 run shader 1720
  fresh shader-off ''; MELEE_CAPTURE_BURST=1600,1 run shader-off 1720
  grep -h "shaders:" "$root/shader/log" | sed 's/^\[ *[0-9.]*\] //' | head -3
  echo "shader: $(mean_diff "$root/shader-off/melee-unlocked/capture/blink_01600.ppm" "$root/shader/melee-unlocked/capture/blink_01600.ppm")";;
esac; done
