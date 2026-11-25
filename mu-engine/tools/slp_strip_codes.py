#!/usr/bin/env python3
"""A legacy-style copy of a replay: the Gecko code list event (0x3D) removed from the raw stream, as replays recorded before
Slippi stored one (tests of the legacy code list path). usage: slp_strip_codes.py IN.slp OUT.slp"""
# SPDX-License-Identifier: GPL-3.0-or-later
import struct, sys
data = bytearray(open(sys.argv[1], "rb").read())
raw = data.find(b"raw[$U#l"); start = raw + 12
length = struct.unpack(">I", data[raw + 8: raw + 12])[0]
n = data[start + 1]
sizes = {data[start + 2 + i * 3]: struct.unpack(">H", data[start + 3 + i * 3: start + 5 + i * 3])[0] for i in range((n - 1) // 3)}
at, out, removed = start, bytearray(), 0
while at < start + length:
    cmd = data[at]
    size = n + 1 if cmd == 0x35 else sizes[cmd] + 1
    # the list comes whole (0x3D) or in message splitter events (0x10: 512 data bytes, size, internal command 0x3D, last)
    if cmd == 0x3D or (cmd == 0x10 and data[at + 1 + 514] == 0x3D): removed += size
    else: out += data[at: at + size]
    at += size
new = data[:raw + 8] + struct.pack(">I", length - removed) + out + data[start + length:]
open(sys.argv[2], "wb").write(new)
print("removed %d bytes of Gecko list events" % removed)
