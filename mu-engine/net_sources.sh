#!/usr/bin/env bash
# The netplay object's sources: this repository's port/runtime/hle copied to OUT_DIR/port/runtime/hle with
# mu-engine/patches/net-*.patch applied (the way Melee-NX-NetPlay's experiments/native-engine/net_sources.sh does it).
# port/ itself is never changed, so the legacy engine and its provenance manifest are untouched.
#   net_sources.sh OUT_DIR      prints the stamp of what it made (a hash of the copy)
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo="$(cd "$here/.." && pwd -P)"
out="${1:?output folder}"
stage="$out.stage"; rm -rf "$stage"; mkdir -p "$stage/port/runtime"
cp -R "$repo/port/runtime/hle" "$stage/port/runtime/hle"
for p in "$here"/patches/net-*.patch; do [ -f "$p" ] && patch -s -p1 -d "$stage" < "$p"; done
mkdir -p "$out"
# By content, so a build only sees files that changed.
rsync -rlpc --delete "$stage/" "$out/"
rm -rf "$stage"
(cd "$out" && find port -type f | LC_ALL=C sort | xargs cat) | shasum -a 256 | cut -c1-32
