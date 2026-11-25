#!/usr/bin/env bash
# The netplay object's sources: Melee-NX-NetPlay's Slippi 3.7.0 netplay (mu-engine/net/hle, a copy of its port/runtime/hle
# files, and its ENet changes in mu-engine/net/enet) copied to OUT_DIR with its patches (mu-engine/net/patches/net-*.patch,
# its experiments/native-engine/patches) and the Mac's (mac-*.patch) applied, the way its net_sources.sh does it. The
# patches include headers from the fetched melee-unlocked tree (build/mu-engine/mu): copied in here, never committed.
#   net_sources.sh OUT_DIR      prints the stamp of what it made (a hash of the copy)
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo="$(cd "$here/.." && pwd -P)"
out="${1:?output folder}"; tree="$repo/build/mu-engine/mu"
stage="$out.stage"; rm -rf "$stage"; mkdir -p "$stage/port/runtime" "$stage/enet/include/enet"
cp -R "$here/net/hle" "$stage/port/runtime/hle"
[ -f "$tree/port/runtime/hle/slippi_input_suggestions.h" ] || { echo "net_sources: $tree is missing: run mu-engine/fetch.sh first" >&2; exit 1; }
cp "$tree/port/runtime/hle/slippi_input_suggestions.h" "$tree/port/runtime/abi/mu_pick_preview.h" "$stage/port/runtime/hle/"
for p in "$here"/net/patches/net-*.patch "$here"/net/patches/mac-*.patch; do [ -f "$p" ] && patch -s -p1 -d "$stage" < "$p"; done
find "$stage" -name '*.orig' -delete
cp "$here/net/enet/enet.h" "$stage/enet/include/enet/"; cp "$here/net/enet/peer.c" "$here/net/enet/protocol.c" "$stage/enet/"
mkdir -p "$out"
rsync -rlpc --delete "$stage/" "$out/"
rm -rf "$stage"
(cd "$out" && find . -type f | LC_ALL=C sort | xargs cat) | shasum -a 256 | cut -c1-32
