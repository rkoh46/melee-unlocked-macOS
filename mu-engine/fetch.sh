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
# Dear ImGui's Metal back end at the version melee-unlocked vendors (v1.92.8): melee-unlocked ships only its Windows and
# Linux back ends. Fetched, never committed (MIT).
imgui_ver="$(sed -n 's/^Dear ImGui v//p' "$tree/port/third_party/imgui/VERSION.txt")"
mkdir -p "$work/imgui-metal"
for f in imgui_impl_metal.h imgui_impl_metal.mm; do
  [ -s "$work/imgui-metal/$f" ] || curl -fsSL "https://raw.githubusercontent.com/ocornut/imgui/v$imgui_ver/backends/$f" -o "$work/imgui-metal/$f"
done
# Small permissive audio decoders for Use custom music (app/fe/media_decode.cpp), pinned by commit and SHA-256 as
# Melee-NX-NetPlay's fetch.sh has them: minimp3 (CC0), stb_vorbis (public domain or MIT), dr_wav and dr_flac (public domain
# or MIT No Attribution). Never committed.
dec="$work/audio-decoders"; mkdir -p "$dec"
pinned() {   # $1 URL, $2 SHA-256
  local name; name="$(basename "$1")"
  if [ -f "$dec/$name" ] && [ "$(shasum -a 256 "$dec/$name" | cut -d' ' -f1)" = "$2" ]; then echo "have  $name"; return; fi
  curl -fsSL --max-time 120 -o "$dec/$name.part" "$1"
  [ "$(shasum -a 256 "$dec/$name.part" | cut -d' ' -f1)" = "$2" ] || { rm -f "$dec/$name.part"; echo "fetch: $name is not the pinned file" >&2; exit 1; }
  mv "$dec/$name.part" "$dec/$name"; echo "fetch $name"
}
pinned https://raw.githubusercontent.com/lieff/minimp3/48e3a085f0e1a99446b34614e1e7f17f09dc2e82/minimp3.h eb4b44812cfe77a358aa1820462cc877473a8d97206009ebecd18b60a123875a
pinned https://raw.githubusercontent.com/lieff/minimp3/48e3a085f0e1a99446b34614e1e7f17f09dc2e82/minimp3_ex.h ec1240ad4f29961ed83a9da160fcb190ce2253b618aaae36dfbd7bab4d5eedcf
pinned https://raw.githubusercontent.com/nothings/stb/6b5799bae8a1fe3792691ef4b72ae26434c3e799/stb_vorbis.c 97a8febc738e6a29fafc8d8bc557c7f17ad3db7ba6544485f59395cc6008b764
pinned https://raw.githubusercontent.com/mackron/dr_libs/e381981afa52800b9a3864df548353ff67252fbc/dr_wav.h b36c32661cfbbfb1cc0beb19f7a9d2567065e5a297f64172dcebfff59885f37b
pinned https://raw.githubusercontent.com/mackron/dr_libs/e381981afa52800b9a3864df548353ff67252fbc/dr_flac.h bd569c910a7c465e481fd50af84e2871e81e2154e30ac510d98f953cc0d5a546
echo "tree: $tree"
