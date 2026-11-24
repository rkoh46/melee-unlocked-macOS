#!/usr/bin/env python3
"""Converts melee-unlocked's launcher artwork from the fetched tree (build/mu-engine/mu/port/app: melee_mark.bmp and
bailey_cut.bmp, 32-bit premultiplied cutouts) into PNG files with straight alpha for the Mac launcher
(port/runtime/host/mu_launcher.mm). The images are melee-unlocked's and are never committed: package_macos_app.sh
writes them into the app bundle.

Also writes melee_unlocked.png: the largest image of melee-unlocked's own icon (melee_unlocked.ico), for the app icon.

usage: mu_launcher_art.py MU_APP_DIR OUT_DIR
SPDX-License-Identifier: GPL-3.0-or-later
"""
import struct
import sys
import zlib
from pathlib import Path


def read_bmp32(path):
    data = path.read_bytes()
    if data[:2] != b'BM':
        raise ValueError(f'{path}: not a BMP')
    offset = struct.unpack_from('<I', data, 10)[0]
    width, height, _, bits, compression = struct.unpack_from('<iiHHI', data, 18)
    if bits not in (24, 32) or compression not in (0, 3):
        raise ValueError(f'{path}: {bits}-bit BMP compression {compression} is not handled')
    top_down = height < 0
    height = abs(height)
    stride = (width * bits // 8 + 3) & ~3
    rows = []
    for y in range(height):
        src = y if top_down else height - 1 - y
        row = data[offset + src * stride: offset + src * stride + width * bits // 8]
        out = bytearray()
        for x in range(width):
            if bits == 32:
                b, g, r, a = row[x * 4: x * 4 + 4]
                if a:   # premultiplied in the file: back to straight alpha for PNG
                    r, g, b = (min(255, c * 255 // a) for c in (r, g, b))
            else:
                b, g, r = row[x * 3: x * 3 + 3]
                a = 255
            out += bytes((r, g, b, a))
        rows.append(bytes(out))
    return width, height, rows


def write_png(path, width, height, rows):
    def chunk(kind, payload):
        return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind + payload) & 0xFFFFFFFF)
    raw = b''.join(b'\x00' + r for r in rows)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    path.write_bytes(png)


def largest_ico_png(path):
    data = path.read_bytes()
    _, kind, count = struct.unpack_from('<HHH', data, 0)
    if kind != 1:
        raise ValueError(f'{path}: not an icon file')
    best = None
    for i in range(count):
        w, h, _, _, _, _, size, offset = struct.unpack_from('<BBBBHHII', data, 6 + 16 * i)
        side = w or 256
        image = data[offset: offset + size]
        if image[:8] == b'\x89PNG\r\n\x1a\n' and (best is None or side > best[0]):
            best = (side, image)
    if best is None:
        raise ValueError(f'{path}: no PNG image inside')
    return best[1]


def main():
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    for name, target in (('melee_mark.bmp', 'melee_mark.png'), ('bailey_cut.bmp', 'bailey.png')):
        w, h, rows = read_bmp32(src / name)
        write_png(out / target, w, h, rows)
        print(f'{target}: {w}x{h}')
    (out / 'melee_unlocked.png').write_bytes(largest_ico_png(src / 'melee_unlocked.ico'))
    print('melee_unlocked.png: from melee_unlocked.ico')


if __name__ == '__main__':
    main()
