#!/bin/bash
# melee-unlocked's mod disc fingerprint as fe/layer_fingerprint.cpp computes it, against an independent Python version of
# upstream's algorithm (source_mod_overlay.cpp add_iso + layer_fingerprint.h) on small synthetic GameCube discs made here: a
# changed file, an unchanged one, a deleted one, an added folder, a main.dol with two change runs further apart than 32 bytes
# and one pair closer than that. With a real disc pair given, also prints both fingerprints for it.
#   layer_fingerprint_test.sh [BASE.iso MOD.iso]
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
out="$repo/build/mu-engine/runs/layer-fingerprint"; rm -rf "$out"; mkdir -p "$out"
clang++ -std=c++17 -O2 -DMU_LAYER_FINGERPRINT_MAIN "$repo/mu-engine/app/fe/layer_fingerprint.cpp" -o "$out/layer_fingerprint" || exit 1
cat > "$out/ref.py" <<'PY'
import hashlib, struct, sys
def disc(path):
    d = open(path, "rb").read()
    fo, fs = struct.unpack(">II", d[0x424:0x42C]); fst = d[fo:fo + fs]
    count = struct.unpack(">I", fst[8:12])[0]; ns = count * 12
    files, names, dirs = {}, set(), [(count, "")]
    for i in range(1, count):
        while len(dirs) > 1 and i >= dirs[-1][0]: dirs.pop()
        e = fst[i * 12:i * 12 + 12]; at = struct.unpack(">I", e[0:4])[0] & 0xFFFFFF
        name = fst[ns + at:].split(b"\0")[0].decode().lower(); full = dirs[-1][1] + "/" + name; names.add(full)
        if e[0]: dirs.append((struct.unpack(">I", e[8:12])[0], full)); continue
        off, ln = struct.unpack(">II", e[4:12]); files[full] = (off, ln)
    do = struct.unpack(">I", d[0x420:0x424])[0]; h = d[do:do + 0x100]; ext = 0x100
    for i in range(18): ext = max(ext, struct.unpack(">I", h[i*4:i*4+4])[0] + struct.unpack(">I", h[0x90+i*4:0x94+i*4])[0])
    return d, files, names, do, ext
bd, bf, bn, bdo, bext = disc(sys.argv[1]); md, mf, mn, mdo, mext = disc(sys.argv[2])
h = hashlib.sha256()
for p in sorted(mf):
    o, l = mf[p]; data = md[o:o + l]
    if p in bf and bf[p][1] == l and bd[bf[p][0]:bf[p][0] + l] == data: continue
    h.update(f"{p}\t{l}\n".encode()); h.update(data)
common = min(mext, bext); runs = []; start = -1; last = 0
for at in range(common):
    if md[mdo + at] == bd[bdo + at]: continue
    if start >= 0 and at - last > 32: runs.append((start, last + 1 - start)); start = -1
    if start < 0: start = at
    last = at
if start >= 0: runs.append((start, last + 1 - start))
for s, l in runs: h.update(f"dol {s} {l}\n".encode())
for p in sorted(bf):
    if p not in mn: h.update(f"deleted {p}\n".encode())
print(h.hexdigest())
PY
python3 -I - "$out" <<'PY'
import struct, sys
out = sys.argv[1]
def make(path, files, dol):
    # header (0x440), main.dol at 0x2000, FST at 0x1000, file data from 0x8000
    entries = [("", True, None)]           # root
    names = b""; table = []; data = b""; offsets = {}
    tree = {}
    for p, b in files.items():
        parts = p.strip("/").split("/"); node = tree
        for d in parts[:-1]: node = node.setdefault(d, {})
        node[parts[-1]] = b
    flat = []
    def walk(node, parent):
        for k in sorted(node):
            if isinstance(node[k], dict):
                idx = len(flat) + 1; flat.append([k, True, parent, None]); walk(node[k], idx); flat[idx - 1][3] = len(flat) + 1
            else: flat.append([k, False, None, node[k]])
    walk(tree, 0)
    count = len(flat) + 1; pos = 0x8000; blobs = []
    table = struct.pack(">BBHII", 1, 0, 0, 0, count)
    for name, is_dir, parent, val in flat:
        no = len(names); names += name.encode() + b"\0"
        if is_dir: table += struct.pack(">I", (1 << 24) | no) + struct.pack(">II", parent, val)
        else: table += struct.pack(">III", no, pos, len(val)); blobs.append((pos, val)); pos += (len(val) + 31) & ~31
    fst = table + names
    img = bytearray(max(pos, 0x8000))
    img[0:6] = b"GALE01"; img[7] = 2
    img[0x420:0x42C] = struct.pack(">III", 0x2000, 0x1000, len(fst))
    img[0x1000:0x1000 + len(fst)] = fst
    img[0x2000:0x2000 + len(dol)] = dol
    for p, v in blobs: img[p:p + len(v)] = v
    open(path, "wb").write(bytes(img))
dol = bytearray(0x1800)
struct.pack_into(">I", dol, 0, 0x100); struct.pack_into(">I", dol, 0x90, 0x1700)   # one section: 0x100..0x1800
for i in range(0x100, 0x1800): dol[i] = i & 0xFF
mdol = bytearray(dol)
for i in (0x300, 0x310, 0x330, 0x900, 0x1000): mdol[i] ^= 0x5A   # 0x300..0x330 merge (gaps <= 32), 0x900 and 0x1000 apart
base = {"/audio/a.ssm": b"A" * 300, "/plco.dat": b"common" * 50, "/gone.dat": b"bye" * 40, "/menu/mnslchr.usd": b"m" * 100}
mod = {"/audio/a.ssm": b"A" * 300, "/plco.dat": b"COMMON" * 50, "/menu/mnslchr.usd": b"m" * 99 + b"x", "/added/new.dat": b"new" * 33}
make(f"{out}/base.iso", base, bytes(dol)); make(f"{out}/mod.iso", mod, bytes(mdol))
PY
mine="$("$out/layer_fingerprint" "$out/base.iso" "$out/mod.iso")"; ref="$(python3 -I "$out/ref.py" "$out/base.iso" "$out/mod.iso")"
echo "synthetic discs: engine $mine"
echo "synthetic discs: python $ref"
[ "$mine" = "$ref" ] && echo "layer fingerprint: EQUAL" || echo "layer fingerprint: DIFFERENT"
if [ $# -ge 2 ]; then
  echo "given discs: engine $("$out/layer_fingerprint" "$1" "$2")"
  echo "given discs: python $(python3 -I "$out/ref.py" "$1" "$2")"
fi
