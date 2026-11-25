#!/bin/zsh
# Rebuild melee-unlocked-macOS after a source edit and install it to /Applications as $APP_NAME (default
# melee-unlocked-macOS): melee-unlocked's game image (mu-engine/build.sh game, in Docker), the engine and the launcher.
#   tools/mac/rebuild.sh                 build, package to dist/ and install
#   tools/mac/rebuild.sh --install-only  install the last dist/ build
# SKIP_MU_GAME=1 leaves the game image as it is (only app-side changes).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
B="$ROOT/build/mac"
cd "$ROOT"
unset DEVELOPER_DIR   # the Command Line Tools
export APP_NAME="${APP_NAME:-melee-unlocked-macOS}"
APP="/Applications/$APP_NAME.app"
running() { pgrep -f "/Applications/$APP_NAME.app/Contents/MacOS/" > /dev/null; }
if [[ "${1:-}" == "--install-only" ]]; then
  running && { echo "Quit $APP_NAME first."; exit 1; }
  rm -rf "$APP" && ditto "dist/$APP_NAME.app" "$APP" && echo "Installed $APP"; exit 0
fi
[[ -d "$ROOT/build/mu-engine/mu" ]] || bash mu-engine/fetch.sh
if [[ -z "${SKIP_MU_GAME:-}" ]]; then
  bash mu-engine/build.sh game > "$ROOT/build/mu-engine-game.log" 2>&1 || { tail -20 "$ROOT/build/mu-engine-game.log"; exit 1; }
fi
cmake -S . -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
cmake --build "$B" --target melee_launcher_mac melee_unlocked_mac
tools/package_macos_app.sh "$B" dist
if running; then
  echo "Built dist/$APP_NAME.app. $APP_NAME is running, so /Applications was not updated; quit it and run: $0 --install-only"
  exit 0
fi
rm -rf "$APP" && ditto "dist/$APP_NAME.app" "$APP"
echo "Installed $APP ($(git rev-parse --short HEAD)$(git diff --quiet || echo '+dirty'))"
