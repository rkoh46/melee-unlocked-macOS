#!/usr/bin/env bash
# Builds and runs discord_app_id_test.cpp against port/runtime/hle/discord_rpc.cpp: the default id, a chosen id, a refused
# one. TMPDIR is an empty folder, so the presence never finds a Discord app's socket (nothing is shown in Discord).
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)"
out="$repo/build/discord-test"; mkdir -p "$out/tmp"
c++ -std=gnu++20 -O1 -w -o "$out/discord_app_id_test" "$repo/mu-engine/tools/discord_app_id_test.cpp" "$repo/port/runtime/hle/discord_rpc.cpp" \
  "$repo/port/runtime/hle/slippi_login.cpp" "$repo/port/runtime/hle/slippi_http_apple.mm" -fobjc-arc \
  -I"$repo/port/runtime/hle" -I"$repo/port/runtime/host" -I"$repo/port/runtime/ppc" -I"$repo/port/runtime/gx" -I"$repo/port/runtime" \
  -I"$repo/port/third_party" -I"$repo/port/third_party/nlohmann/include" -framework Foundation
own=$(/usr/bin/grep -o 'kApplicationId = "[0-9]*"' "$repo/port/runtime/hle/discord_rpc.h" "$repo/port/runtime/hle/discord_rpc.cpp" | head -1 | /usr/bin/grep -o '[0-9]\{6,\}')
TMPDIR="$out/tmp" "$out/discord_app_id_test" "$own"
TMPDIR="$out/tmp" MELEE_DISCORD_APP_ID=4242 "$out/discord_app_id_test" 4242
TMPDIR="$out/tmp" MELEE_DISCORD_APP_ID="12ab" "$out/discord_app_id_test" "$own"
