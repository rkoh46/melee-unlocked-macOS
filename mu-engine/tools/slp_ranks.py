#!/usr/bin/env python3
"""Each port's Slippi 3.7.0 ranked info and bot flag from a replay's game start (replay 3.20.0: 0x2F9, 16 bytes a port: rating
f32, update count u32, global and regional placement u16 each, mmr f32; then four bot flags), as melee-unlocked's shim
mu_record_start.inc writes them. usage: slp_ranks.py REPLAY.slp"""
# SPDX-License-Identifier: GPL-3.0-or-later
import struct, sys
data = open(sys.argv[1], "rb").read()
raw = data.find(b"raw[$U#l")
at = raw + 12   # after the array's 4-byte length
assert data[at] == 0x35, "no event sizes"
n = data[at + 1]
sizes = {data[at + 2 + i * 3]: struct.unpack(">H", data[at + 3 + i * 3: at + 5 + i * 3])[0] for i in range((n - 1) // 3)}
gs = at + 1 + n
assert data[gs] == 0x36, "no game start"
payload = data[gs: gs + 1 + sizes[0x36]]
if len(payload) < 0x33D: print("game start of %d bytes: older than replay 3.20.0" % len(payload)); sys.exit(1)
for port in range(4):
    rating, updates, glob, reg, mmr = struct.unpack(">fIHHf", payload[0x2F9 + port * 16: 0x2F9 + port * 16 + 16])
    print("port %d: rating %.2f, %d updates, global %d, regional %d, mmr %.1f, bot %d" % (port + 1, rating, updates, glob, reg, mmr, payload[0x339 + port]))
