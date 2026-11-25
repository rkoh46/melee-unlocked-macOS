#!/bin/zsh
# Builds the release DMG and, with --publish, a draft GitHub release for the current VERSION.
#
#   tools/release.sh            build dist/melee-unlocked-macOS-<v>.dmg, checksums, notes
#   tools/release.sh --publish  ...and create a DRAFT GitHub release with them (gh CLI)
#
# READ THIS FIRST. The app contains melee-unlocked's game compiled from the Melee decompilation. A release on a public
# repository distributes that to everyone, which is illegal and against the Slippi team's wishes. Publish only from a private
# repository (or keep the assets local). The script therefore refuses --publish unless the repository is private, and always
# creates a draft.
set -euo pipefail
ROOT="${0:A:h:h}"
PUBLISH=0
for arg in "$@"; do case "$arg" in --publish) PUBLISH=1;; *) echo "unknown option $arg" >&2; exit 2;; esac; done
VERSION="$(head -n1 "$ROOT/VERSION")"
TAG="v$VERSION"
NAME=melee-unlocked-macOS
step() { printf '\n\033[1;33m==> %s\033[0m\n' "$1"; }
fail() { printf '\033[1;31merror:\033[0m %s\n' "$1" >&2; exit 1; }

step "macOS app ($TAG)"
DASHDANCE_NO_OPEN=1 "$ROOT/setup.sh" >/dev/null
step "DMG"
DMG="$ROOT/dist/$NAME-$VERSION.dmg"; rm -f "$DMG"
STAGE="$(mktemp -d)"; cp -R "$ROOT/dist/$NAME.app" "$STAGE/"; ln -s /Applications "$STAGE/Applications"
hdiutil create -volname "$NAME $VERSION" -srcfolder "$STAGE" -ov -format UDZO -quiet "$DMG"
rm -rf "$STAGE"
step "Checksums and notes"
(cd "$ROOT/dist" && shasum -a 256 "$NAME-$VERSION.dmg" > "SHA256SUMS-$VERSION.txt" && cat "SHA256SUMS-$VERSION.txt")
python3 "$ROOT/tools/release_notes.py" > "$ROOT/dist/RELEASE_NOTES-$VERSION.md"
echo; cat "$ROOT/dist/RELEASE_NOTES-$VERSION.md"
if (( PUBLISH )); then
  step "Publishing a draft release"
  command -v gh >/dev/null || fail "install the GitHub CLI (brew install gh) and run gh auth login"
  VIS="$(gh repo view --json visibility -q .visibility 2>/dev/null || echo unknown)"
  [[ "$VIS" == "PRIVATE" ]] || fail "this repository is $VIS. Releases with the built app must stay private (they contain the compiled game). Fork privately, or skip --publish."
  git -C "$ROOT" tag -f "$TAG" >/dev/null && git -C "$ROOT" push -q --force origin "$TAG"
  gh release create "$TAG" --draft --title "$NAME $VERSION" --notes-file "$ROOT/dist/RELEASE_NOTES-$VERSION.md" \
    "$DMG" "$ROOT/dist/SHA256SUMS-$VERSION.txt"
  echo "Draft release $TAG created. Review it on GitHub, then publish it there."
fi
