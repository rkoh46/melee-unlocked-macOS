#!/usr/bin/env bash
# No dead rows: every key the launcher's Settings window writes (port/runtime/host/mu_settings_window.mm) must be read by the
# engine or the launcher. settings.ini keys (store E) must appear as a quoted key in the engine's settings parsers
# (mu-engine/app: fe/settings.cpp, the feature modules, the panel); launcher.ini keys (store L) in the engine, the panel or
# the launcher's main program. Prints each key with where it is read; exits 1 if any key is read nowhere.
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd -P)"
win="$repo/port/runtime/host/mu_settings_window.mm"
fail=0; n=0
while IFS=$'\t' read -r key store; do
  [[ -z "$key" || "$key" == "GeckoCodes.ini" ]] && continue
  n=$((n + 1))
  case "$store" in
    E|B) where=$(/usr/bin/grep -rlE "\"$key\"|\"$key \"" "$repo/mu-engine/app" --include=*.cpp --include=*.h --include=*.mm 2>/dev/null | head -1 || true)
         # a numbered family parsed by its prefix (shield_colorp1..4: "shield_colorp" and the digit)
         [[ -z "$where" && "$key" =~ [0-9]$ ]] && where=$(/usr/bin/grep -rlE "\"${key%?}\"" "$repo/mu-engine/app" --include=*.cpp 2>/dev/null | head -1 || true) ;;
    L)   where=$(/usr/bin/grep -rlE "\"$key\"|\"$key=\"|$key=" "$repo/mu-engine/app" "$repo/port/app/main_mac.cpp" "$repo/port/runtime/host/mu_launcher.mm" --include=*.cpp --include=*.h --include=*.mm 2>/dev/null | head -1 || true) ;;
    *)   where="launcher choice" ;;
  esac
  if [[ -z "$where" ]]; then echo "DEAD  $key ($store)"; fail=1; else echo "ok    $key ($store): ${where#$repo/}"; fi
done < <(python3 -I - "$win" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
for m in re.finditer(r'\{"[^"]+", "[^"]+", "[^"]*", "([^"]*)", ([ELSB]), (TOGGLE|CHOICE|SLIDER|TEXT|PATH|NOTE)', text):
    if m.group(3) != "NOTE": print(f"{m.group(1)}\t{m.group(2)}")
PY
)
echo "$n keys checked"
exit $fail
