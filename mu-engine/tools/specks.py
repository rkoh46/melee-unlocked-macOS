#!/usr/bin/env python3
# Counts dark specks in a picture (PPM, P6): near-black pixels whose two neighbours across them, left and right or above and
# below, are both bright: the one-pixel-thin black marks MSAA left where a white vertex colour wrapped to black (gap 5 in
# docs/MAC_FIXES.md). A real dark line in the game's art is thicker than one pixel, or not flanked by bright pixels on both sides.
#   specks.py PICTURE.ppm [--list N]
# SPDX-License-Identifier: GPL-3.0-or-later
import sys
path = sys.argv[1]; listn = int(sys.argv[sys.argv.index("--list") + 1]) if "--list" in sys.argv else 0
d = open(path, "rb").read(); parts = d.split(maxsplit=4); w, h = int(parts[1]), int(parts[2]); a = d[len(d) - w * h * 3:]
lum = [a[i] + a[i + 1] + a[i + 2] for i in range(0, w * h * 3, 3)]
found = []
for y in range(1, h - 1):
    row = y * w
    for x in range(1, w - 1):
        v = lum[row + x]
        if v >= 60: continue
        if (lum[row + x - 1] > 300 and lum[row + x + 1] > 300) or (lum[row - w + x] > 300 and lum[row + w + x] > 300): found.append((x, y))
print("specks: %d" % len(found))
for f in found[:listn]: print("  at %d,%d" % f)
