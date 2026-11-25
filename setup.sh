#!/bin/zsh
# melee-unlocked-macOS one-command setup. Installs the build tools, fetches melee-unlocked at its pinned release, builds
# its game from source in a container (Docker: OrbStack or Docker Desktop), builds the engine and the launcher, packages
# dist/melee-unlocked-macOS.app and opens it. Nothing from the game is downloaded or committed: the game runs from your
# own disc image, which stays where it is.
#
#   ./setup.sh [/path/to/melee.iso]     the disc is only remembered and opened in the launcher; the build does not read it
#
# Environment: DASHDANCE_ISO (disc path), DASHDANCE_JOBS (parallel jobs), DASHDANCE_NO_OPEN=1 (do not open the app),
# DASHDANCE_APP_NAME (package under another name). Re-running is safe: every step skips work that is already done.
set -euo pipefail
ROOT="${0:A:h}"
step() { printf '\n\033[1;33m==> %s\033[0m\n' "$1"; }
fail() { printf '\033[1;31merror:\033[0m %s\n' "$1" >&2; exit 1; }
ISO="${DASHDANCE_ISO:-}"
for arg in "$@"; do
  case "$arg" in
    --*) fail "unknown option $arg";; *) ISO="$arg";;
  esac
done
[[ "$(uname)" == "Darwin" ]] || fail "melee-unlocked-macOS builds on macOS (Apple silicon)."
[[ "$(uname -m)" == "arm64" ]] || fail "an Apple silicon Mac is required."
[[ -z "$ISO" || -f "$ISO" ]] || fail "disc image not found: $ISO"

step "Command Line Tools"
xcode-select -p >/dev/null 2>&1 || { echo "installing the Command Line Tools (a dialog will open)…"; xcode-select --install || true; fail "re-run ./setup.sh once the Command Line Tools have finished installing."; }
unset DEVELOPER_DIR

step "Homebrew packages (cmake ninja python rust)"
if ! command -v brew >/dev/null; then
  echo "Homebrew is not installed; installing it (https://brew.sh)…"
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  eval "$(/opt/homebrew/bin/brew shellenv)"
fi
for pkg in cmake ninja python rust; do brew list --versions "$pkg" >/dev/null 2>&1 || brew install "$pkg"; done

step "Docker (melee-unlocked's game is compiled with GCC 15 in a Linux container)"
command -v docker >/dev/null 2>&1 || fail "Docker is not installed. Install OrbStack (https://orbstack.dev) or Docker Desktop, start it, and run this again."
docker info >/dev/null 2>&1 || fail "Docker is not running. Start OrbStack or Docker Desktop and run this again."

step "melee-unlocked: fetching v1.0.5 and building its game (a few minutes the first time)"
bash "$ROOT/mu-engine/fetch.sh"
bash "$ROOT/mu-engine/build.sh" game

JOBS="${DASHDANCE_JOBS:-$(sysctl -n hw.ncpu)}"
BUILD="$ROOT/build/mac"
# A checkout that built the earlier static-recompilation engine: its decomp checkout, the DOL taken from the disc, the
# translated game and the playback build are not used any more (several GB).
if [[ -f "$BUILD/port-inputs.json" || -d "$ROOT/deps/melee" || -d "$ROOT/build/mac-playback" ]]; then
  step "Removing the earlier engine's build files"
  rm -rf "$BUILD" "$ROOT/build/mac-playback" "$ROOT/build/playback-codes" "$ROOT/build/deps" "$ROOT/deps"
fi
step "Building the engine and the launcher"
cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null
cmake --build "$BUILD" --target melee_launcher_mac melee_unlocked_mac --parallel "$JOBS"

export APP_NAME="${DASHDANCE_APP_NAME:-melee-unlocked-macOS}"
step "Packaging $APP_NAME.app"
"$ROOT/tools/package_macos_app.sh" "$BUILD" "$ROOT/dist"
echo
echo "Done: dist/$APP_NAME.app. Choose your disc in the launcher the first time (it is remembered)."
if [[ -z "${DASHDANCE_NO_OPEN:-}" ]]; then   # install.sh opens the Applications copy instead
  if [[ -n "$ISO" ]]; then open "$ROOT/dist/$APP_NAME.app" --args --iso "$ISO" --choose-disc; else open "$ROOT/dist/$APP_NAME.app"; fi
fi
