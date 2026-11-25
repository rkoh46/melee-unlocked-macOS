#!/bin/bash
# The launcher's "Save crash report" (mu-engine/app/crash_report.cpp, melee-unlocked's launcher_crash_*.h from the fetched tree):
# a made-up data folder (an engine.log and engine.prev.log with a connect code, an IP address, an e-mail address, this Mac's
# user name and a full file location in them, and a Slippi user.json), then crash_report.cpp built with a small main and run;
# prints what the zip holds and whether any of those words is still in it. Nothing is sent anywhere.
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
out="$repo/build/mu-engine/runs/crash-report"; rm -rf "$out"; mkdir -p "$out/support/melee-unlocked" "$out/support/Slippi"
me="$(id -un)"
printf '{"uid":"abcUID123","displayName":"Testy Name","connectCode":"TEST#123","playKey":"secret-key"}\n' > "$out/support/Slippi/user.json"
for f in engine.log engine.prev.log; do
  printf '%s\n' "[ 1.0] play: settings: 3 lines read from /Users/$me/Library/Application Support/Dashdance/melee-unlocked/settings.ini" \
    "[ 2.0] online: matched with TEST#123 (Testy Name) at 203.0.113.7:51820" "[ 3.0] mail me at someone@example.com" \
    "[ 4.0] scene: major 02 minor 00 (frame 635)" > "$out/support/melee-unlocked/$f"
done
printf '%s\n' "[ 2.5] host: disc /Users/$me/Games/GALE01.iso" > "$out/support/melee-unlocked/engine.log.host"
cat > "$out/main.cpp" <<'CPP'
#include <cstdio>
#include <string>
namespace mu_crash { bool save(const std::string&, const std::string&, std::string*, std::string*); }
int main(int, char** argv) {
  std::string where, why;
  const bool ok = mu_crash::save(argv[1], "", &where, &why);
  std::printf("%s %s\n", ok ? "saved" : "FAILED", ok ? where.c_str() : why.c_str());
  return ok ? 0 : 1;
}
CPP
( unset DEVELOPER_DIR; clang++ -std=c++17 -O1 -w -DMELEE_PORT_VERSION='"test"' -I"$repo/build/mu-engine/mu/port/app" \
    "$repo/mu-engine/app/crash_report.cpp" "$out/main.cpp" -o "$out/crash_report_test" )
"$out/crash_report_test" "$out/support"
zip="$out/support/melee-unlocked/Logs/melee_crash_report.zip"
mkdir -p "$out/unzipped"; (cd "$out/unzipped" && unzip -oq "$zip")
echo "zip holds: $(cd "$out/unzipped" && ls | tr '\n' ' ')"
n=0
for w in "TEST#123" "Testy Name" "203.0.113.7" "someone@example.com" "/Users/$me" "$me" "abcUID123"; do
  if grep -rqF "$w" "$out/unzipped" "$out/support/melee-unlocked/Logs/melee_crash_report.md"; then echo "STILL THERE: a test word"; else echo "removed: test word $((++n))"; fi
done
echo "kept: $(grep -h "scene: major" "$out/unzipped"/* | head -2 | tr '\n' '|')"
