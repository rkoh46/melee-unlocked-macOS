#!/usr/bin/env bash
# Fetches melee-unlocked (MU) at the pinned commit into build/mu-engine/mu (gitignored) and applies the
# build-time patches in mu-engine/patches. Nothing fetched here is ever committed to this repository.
# Adapted from Melee-NX-NetPlay's experiments/native-engine/fetch.sh (same author, GPL-3.0-or-later).
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo="$(cd "$here/.." && pwd -P)"
work="$repo/build/mu-engine"; tree="$work/mu"
PIN=67868aa                      # melee-unlocked v1.0.5
SRC="${MU_SOURCE:-https://github.com/Hero88go/melee-unlocked.git}"   # MU_SOURCE=file:///path/to/clone works offline
IMG="${MU_LINUX_IMAGE:-mu-arm-gcc15}"   # aarch64 Linux image with GCC 15.2 (see linux-gcc15.Dockerfile)
mkdir -p "$work"
if ! docker image inspect "$IMG" >/dev/null 2>&1; then
  echo "fetch: building the $IMG image (aarch64 Linux, GCC 15.2)"
  timeout 1500 docker build -t "$IMG" -f "$here/linux-gcc15.Dockerfile" "$here"
fi
if [ ! -d "$tree/.git" ]; then
  git clone --depth 1 --branch v1.0.5 "$SRC" "$tree"
  [ "$(git -C "$tree" rev-parse --short=7 HEAD)" = "$PIN" ] || { echo "fetch: not the pinned commit $PIN" >&2; exit 1; }
  git -C "$tree" submodule update --init --depth 1 sourceport/extern/melee
  # MU's own source preparation (applies its melee-native.patch to the decomp). Third-party script: container only,
  # no network, time limit.
  timeout 600 docker run --rm --name "mumac-prepare-$$" --network none \
    -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory -e 'GIT_CONFIG_VALUE_0=*' \
    -v "$tree:/mu" -w /mu "$IMG" python3 tools/prepare_native_sources.py
  git -C "$tree/sourceport/extern/melee" add -A      # index = prepared baseline, so our decomp patches show as a diff
else
  echo "fetch: $tree exists (delete it to start over); applying the patches it does not have yet"
fi
# Every patch once. Each tree keeps the names of the patches applied to it (.mumac-applied).
apply() {   # $1 the git tree, $2 the patch
  local name rec="$1/.mumac-applied"; name="$(basename "$2")"
  if [ -f "$rec" ] && grep -qxF "$name" "$rec"; then echo "have  $name"
  elif git -C "$1" apply --reverse --check "$2" 2>/dev/null; then echo "$name" >> "$rec"; echo "have  $name"
  elif git -C "$1" apply --check "$2" 2>/dev/null; then git -C "$1" apply "$2"; echo "$name" >> "$rec"; echo "apply $name"
  else echo "fetch: $name does not apply to $1 (if it is already in, add its name to $rec)" >&2; exit 1; fi
}
for p in "$here"/patches/0*.patch; do apply "$tree" "$p"; done
for p in "$here"/patches/decomp-*.patch; do apply "$tree/sourceport/extern/melee" "$p"; done
echo "tree: $tree"
