#!/bin/zsh
# Packages the macOS build as melee-unlocked-macOS.app (ad-hoc signed): the launcher (the bundle's main program), the
# melee-unlocked engine next to it (MeleeUnlockedEngine), the Slippi GameFiles the engine's Slippi menus read, and
# melee-unlocked's launcher artwork and icon converted from the fetched tree. Nothing from the game enters the bundle.
# Usage: tools/package_macos_app.sh <build-dir> <output-dir>
# APP_NAME=<name> packages a side-by-side copy under its own name and bundle identifier (it shares the settings, sign-in and
# replays in ~/Library/Application Support/Dashdance).
set -euo pipefail
ROOT="${0:A:h:h}"
BUILD="${1:?build dir}"
OUT="${2:?output dir}"
VERSION="$(head -n1 "$ROOT/VERSION")"
APP_NAME="${APP_NAME:-melee-unlocked-macOS}"
APP="$OUT/$APP_NAME.app"
EXE="$BUILD/port/melee_launcher_mac"
MU_EXE="$BUILD/mu-engine/app/MeleeUnlockedEngine"
MU_TREE="$ROOT/build/mu-engine/mu"
[[ -x "$EXE" ]] || { echo "missing $EXE; build target melee_launcher_mac first" >&2; exit 1; }
[[ -x "$MU_EXE" ]] || { echo "missing $MU_EXE; build target melee_unlocked_mac first" >&2; exit 1; }
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
sed "s/@VERSION@/$VERSION/g" "$ROOT/port/app/macos/Info.plist" > "$APP/Contents/Info.plist"
if [[ "$APP_NAME" != "melee-unlocked-macOS" ]]; then
  /usr/libexec/PlistBuddy -c "Set :CFBundleName $APP_NAME" -c "Set :CFBundleDisplayName $APP_NAME" \
    -c "Set :CFBundleIdentifier app.dashdance.macos.${APP_NAME:l}" "$APP/Contents/Info.plist"
fi
# The commit this app was built from, for the launcher's update check (it compares it with this repository's branch on GitHub).
if COMMIT="$(git -C "$ROOT" rev-parse HEAD 2>/dev/null)"; then
  /usr/libexec/PlistBuddy -c "Add :MUBuildCommit string $COMMIT" "$APP/Contents/Info.plist"
fi
cp "$EXE" "$APP/Contents/MacOS/melee-unlocked-macOS"
cp "$MU_EXE" "$APP/Contents/MacOS/MeleeUnlockedEngine"
# librashader with its Metal runtime (mu-engine/librashader.sh, MPL-2.0 or GPL-3.0-or-later), for the engine's shader presets:
# bundled with its licence when it was built; the engine finds it in Contents/Frameworks.
if [[ -f "$ROOT/build/mu-engine/out/librashader.dylib" ]]; then
  mkdir -p "$APP/Contents/Frameworks"
  cp "$ROOT/build/mu-engine/out/librashader.dylib" "$APP/Contents/Frameworks/librashader.dylib"
  for f in LICENSE.md LICENSE-GPL.md; do
    [[ -f "$ROOT/build/mu-engine/librashader-src/$f" ]] && cp "$ROOT/build/mu-engine/librashader-src/$f" "$APP/Contents/Frameworks/librashader-$f"
  done
fi
cp -R "$ROOT/port/slippi_sys" "$APP/Contents/Resources/slippi_sys"
# melee-unlocked's launcher artwork (the MU mark, Bailey) and icon, converted from the fetched tree; never committed.
python3 -I "$ROOT/tools/mu_launcher_art.py" "$MU_TREE/port/app" "$APP/Contents/Resources/mu-art" >/dev/null
# The engine embeds Source Sans 3 (the settings panel's fonts): its SIL Open Font License travels with it.
cp "$MU_TREE/port/runtime/gx/ui_sources/gd_melee/assets/kit/SourceSans3-OFL.md" "$APP/Contents/Resources/mu-art/SourceSans3-OFL.md"
# The licences of what else the engine compiles in from the fetched tree: Dear ImGui (MIT) and zstd (BSD-3-Clause).
cp "$MU_TREE/port/third_party/imgui/LICENSE.txt" "$APP/Contents/Resources/mu-art/imgui-LICENSE.txt"
# melee-unlocked's launcher translations (lang/*.txt, GPL-2.0-or-later) for the launcher's Language button; never committed.
mkdir -p "$APP/Contents/Resources/mu-lang" && cp "$MU_TREE/lang/"*.txt "$APP/Contents/Resources/mu-lang/"
cp "$ROOT/mu-engine/app/whats-new.txt" "$APP/Contents/Resources/whats-new.txt"   # the launcher's What's new
cp "$MU_TREE/port/third_party/zstd/LICENSE" "$APP/Contents/Resources/mu-art/zstd-LICENSE.txt"
# The app icon: melee-unlocked's own (melee_unlocked.ico from the fetched tree, converted above).
ICONSET="$(mktemp -d)/AppIcon.iconset"; mkdir -p "$ICONSET"
for size in 16 32 128 256 512; do
  sips -z $size $size "$APP/Contents/Resources/mu-art/melee_unlocked.png" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
  sips -z $((size*2)) $((size*2)) "$APP/Contents/Resources/mu-art/melee_unlocked.png" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$ICONSET" -o "$APP/Contents/Resources/AppIcon.icns"
rm -rf "${ICONSET:h}"
printf 'APPL????' > "$APP/Contents/PkgInfo"
codesign --force --sign - --deep "$APP"
codesign --verify --deep --strict "$APP"
echo "packaged $APP ($VERSION)"
