#!/usr/bin/env python3
"""A copy of a replay with another stage id in its game start (the game info block's stage, u16 at 0x13 counting the command
byte): tests of the mod-stage refusal (a stage above 85 is not in the standard game). usage: slp_set_stage.py IN OUT STAGE"""
# SPDX-License-Identifier: GPL-3.0-or-later
import struct, sys
data = bytearray(open(sys.argv[1], "rb").read())
raw = data.find(b"raw[$U#l"); start = raw + 12
n = data[start + 1]; gs = start + 1 + n
assert data[gs] == 0x36
print("stage %d -> %s" % (struct.unpack(">H", data[gs + 0x13: gs + 0x15])[0], sys.argv[3]))
data[gs + 0x13: gs + 0x15] = struct.pack(">H", int(sys.argv[3]))
open(sys.argv[2], "wb").write(data)
