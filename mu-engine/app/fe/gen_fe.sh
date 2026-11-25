#!/bin/bash
# The build-time copies the frontend layer (mu-engine/app/fe, from Melee-NX-NetPlay's frontend) takes from the fetched
# melee-unlocked tree, as that frontend's build.sh sync makes them: phillip_ai and practice_menu with patches 0112 and
# 0113 (Fight a pro, the Practice menu), mu_ghost_ai.h, the special-move names and the action names the overlays show.
# Nothing of it is committed. usage: gen_fe.sh MU_TREE OUT_DIR
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
mu="$1"; gen="$2"
mkdir -p "$gen/proai" "$gen/abi"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
mkdir -p "$work/port/runtime/hle" "$work/port/runtime/gx"
cp "$mu/port/runtime/hle/phillip_ai.cpp" "$mu/port/runtime/hle/phillip_ai.h" "$work/port/runtime/hle/"
cp "$mu/port/runtime/gx/practice_menu.cpp" "$mu/port/runtime/gx/practice_menu.h" "$work/port/runtime/gx/"
patch -s -p1 -d "$work" < "$here/patches/0112-phillip-ai-own-engine.patch"
patch -s -p1 -d "$work" < "$here/patches/0113-practice-menu-download-question.patch"
put() { cmp -s "$1" "$2" || cp "$1" "$2"; }
for f in hle/phillip_ai.cpp hle/phillip_ai.h gx/practice_menu.cpp gx/practice_menu.h; do put "$work/port/runtime/$f" "$gen/proai/$(basename "$f")"; done
put "$mu/port/runtime/abi/mu_ghost_ai.h" "$gen/abi/mu_ghost_ai.h"
sed -n '/static const char\* const names\[27\]\[4\] = {/,/^  };/p' "$mu/port/runtime/hle/practice_cpu.cpp" > "$work/special_names.inc"
[ -s "$work/special_names.inc" ] || echo 'static const char* const names[27][4] = {};' > "$work/special_names.inc"
put "$work/special_names.inc" "$gen/proai/special_names.inc"
sed -n '/^const char\* const kActionNames\[\] = {/,/^};/p' "$mu/port/runtime/gx/training_overlay.cpp" > "$work/action_names.inc"
[ -s "$work/action_names.inc" ] || echo 'const char* const kActionNames[] = {""};' > "$work/action_names.inc"
put "$work/action_names.inc" "$gen/action_names.inc"
# Skins (fe/features/cosmetics.cpp): melee-unlocked's skeleton and drawing checks and its costume families, taken from its
# Windows-only cosmetic_mods.cpp (only these portable parts), and its pack_skin_rule, as Melee-NX-NetPlay's build.sh sync does.
cm="$mu/port/runtime/host/cosmetic_mods.cpp"
awk '/^\/\/ The player reads this beside "Online: standard costume"/{on=0} /^namespace skeleton \{/{on=1} on' "$cm" > "$work/cosmetics_skeleton.inc"
awk '/^constexpr FighterFamily families\[\] = \{/,/^\};/' "$cm" > "$work/cosmetics_families.inc"
grep -q "costume_skeleton_matches" "$work/cosmetics_skeleton.inc" && grep -q '"Fx", "Fox"' "$work/cosmetics_families.inc" || { echo "gen_fe: cosmetic_mods.cpp no longer has the parts cosmetics.cpp takes" >&2; exit 1; }
put "$work/cosmetics_skeleton.inc" "$gen/cosmetics_skeleton.inc"
put "$work/cosmetics_families.inc" "$gen/cosmetics_families.inc"
for f in pack_skin_rule.h pack_skin_rule.cpp; do put "$mu/port/runtime/host/$f" "$gen/$f"; done
# Use custom music (fe/features/audio.cpp): melee-unlocked's jukebox, LF line ends and no byte-order mark, with patches/0118
# (the app's MusicPacks folder), as Melee-NX-NetPlay's build.sh sync makes it.
mkdir -p "$gen/jukebox" "$work/jb/port/runtime/hle"
perl -pe 's/\r$//; s/^\xEF\xBB\xBF// if $. == 1' "$mu/port/runtime/hle/jukebox.cpp" > "$work/jb/port/runtime/hle/jukebox.cpp"
patch -s -p1 -d "$work/jb" < "$here/patches/0118-jukebox-on-the-mac.patch"
put "$work/jb/port/runtime/hle/jukebox.cpp" "$gen/jukebox/jukebox.cpp"
# The player's Gecko codes (fe/features/gecko.cpp): melee-unlocked's user_gecko.cpp, LF, with patches/0122 (the shim host header and
# the include paths of a copy), as Melee-NX-NetPlay's build.sh sync makes it.
mkdir -p "$gen/gecko" "$work/gk/port/runtime/host"
perl -pe 's/\r$//; s/^\xEF\xBB\xBF// if $. == 1' "$mu/port/runtime/host/user_gecko.cpp" > "$work/gk/port/runtime/host/user_gecko.cpp"
patch -s -p1 -d "$work/gk" < "$here/patches/0122-user-gecko-on-the-mac.patch"
put "$work/gk/port/runtime/host/user_gecko.cpp" "$gen/gecko/user_gecko.cpp"
