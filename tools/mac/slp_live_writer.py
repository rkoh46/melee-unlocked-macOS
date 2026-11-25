#!/usr/bin/env python3
# Writes a finished .slp again as a game in progress, the way the Slippi Launcher's spectate folder holds a broadcast: the
# header with a raw length of 0, then the event stream a little at a time (SPEED x real time, 60 frames a second), and at
# the end the rest (Game End) without the metadata. For testing the engine's live playback (Watch live).
#   slp_live_writer.py SOURCE.slp OUT.slp [SPEED]
# SPDX-License-Identifier: GPL-3.0-or-later
import struct, sys, time

src, out = sys.argv[1], sys.argv[2]
speed = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
data = open(src, "rb").read()
start = data.index(b"raw[$U#l") + 8
length = struct.unpack(">I", data[start:start + 4])[0]
raw = data[start + 4:start + 4 + length]
sizes = {}
if raw[0] == 0x35:
    n = raw[1]
    for i in range(0, n - 1, 3):
        sizes[raw[2 + i]] = struct.unpack(">H", raw[3 + i:5 + i])[0]
# Event boundaries: each Frame Bookend (0x3C) ends a frame; Game End (0x39) is held back to the end.
pos, frames, end_at = 0, [], len(raw)
while pos < len(raw):
    cmd = raw[pos]
    size = (raw[1] if cmd == 0x35 else sizes.get(cmd, 0)) + 1
    if cmd == 0x39:
        end_at = pos
        break
    pos += size
    if cmd == 0x3C:
        frames.append(pos)
if not frames:
    sys.exit("live writer: no frame bookends in %s (Slippi 3.0 or newer needed)" % src)
with open(out, "wb") as f:
    f.write(data[:start] + b"\x00\x00\x00\x00")
    f.flush()
    written, t0 = 0, time.monotonic()
    for i, at in enumerate(frames):
        delay = t0 + i / (60.0 * speed) - time.monotonic()
        if delay > 0:
            time.sleep(delay)
        f.write(raw[written:at]); f.flush(); written = at
    f.write(raw[written:])   # Game End (and anything after it in the stream); no metadata, as a live file has none
    f.flush()
print("live writer: %d frames written to %s at %.2fx, then Game End" % (len(frames), out, speed))
