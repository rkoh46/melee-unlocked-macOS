#!/bin/bash
# librashader (https://github.com/SnowflakePowered/librashader), the RetroArch shader runtime melee-unlocked loads for its
# shader presets, built from source with its Metal runtime only: the tag melee-unlocked's headers come from (v0.12.0), checked
# against its commit, its crates pinned by the tag's Cargo.lock (cargo --locked verifies every crate's checksum). Output:
# build/mu-engine/out/librashader.dylib, which tools/package_macos_app.sh puts in the app's Contents/Frameworks and the engine
# loads at run time (mu-engine/app/gfx/effects_mac.mm). Needs a Rust toolchain (brew install rust); without one it says so and
# the app is built without the library (the shader preset row then says librashader.dylib is missing).
#   mu-engine/librashader.sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd -P)"; repo="$(cd "$here/.." && pwd -P)"
tag="librashader-v0.12.0"; commit="87e8a97b50516d997defeaa168173dcd185d4022"
src="$repo/build/mu-engine/librashader-src"; out="$repo/build/mu-engine/out/librashader.dylib"
if ! command -v cargo > /dev/null; then echo "librashader: no Rust toolchain (brew install rust): shader presets stay off" >&2; exit 0; fi
if [ ! -d "$src/.git" ] || [ "$(git -C "$src" rev-parse HEAD 2>/dev/null)" != "$commit" ]; then
  rm -rf "$src"
  git clone -q --depth 1 --branch "$tag" --recurse-submodules --shallow-submodules https://github.com/SnowflakePowered/librashader "$src"
fi
got="$(git -C "$src" rev-parse HEAD)"
[ "$got" = "$commit" ] || { echo "librashader: $tag is $got, expected $commit; not built" >&2; exit 1; }
(cd "$src" && cargo build --locked -q -p librashader-capi --profile optimized --no-default-features --features runtime-metal -j "${MU_JOBS:-6}")
mkdir -p "$(dirname "$out")"
cp "$src/target/optimized/liblibrashader_capi.dylib" "$out.tmp"
install_name_tool -id "@rpath/librashader.dylib" "$out.tmp"
codesign --force --sign - "$out.tmp" 2> /dev/null || true
mv "$out.tmp" "$out"
echo "built $out ($tag, Metal runtime)"
