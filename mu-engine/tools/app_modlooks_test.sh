#!/bin/bash
# Get mod looks (fe/features/modlooks.cpp) end to end without the network: MU_LOOKS_TEST_DIR answers every download from files
# made here and gives the pins (pins.txt). The fixtures stand in for the authors' files: a MediaFire-style page naming a
# download host, an archive (.zip for Animelee, .tar.gz for Diet Melee) holding xdelta3 patches made here from the player's own
# disc to small made-up outputs (xdelta3 from Homebrew only writes the fixtures; the app applies them with its own decoder).
# Cases: animelee (one patch), diet (the Classic patch, then Crystal and Diet 64 as differences of it), person (a page without a
# download address: the install stops and says so). Hidden offline starts; output build/mu-engine/runs/app-modlooks.
#   app_modlooks_test.sh ISO
# SPDX-License-Identifier: GPL-3.0-or-later
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"; repo="$(cd "$here/../.." && pwd -P)"
iso="${1:?disc image}"; engine="${MU_ENGINE:-$repo/build/mac/mu-engine/app/MeleeUnlockedEngine}"
command -v xdelta3 > /dev/null || { echo "app_modlooks_test: needs xdelta3 to make the fixtures (brew install xdelta)"; exit 1; }
root="$repo/build/mu-engine/runs/app-modlooks"; rm -rf "$root"; mkdir -p "$root/fx"
fx="$root/fx"
san() { python3 -I -c 'import sys; print("".join(c if c.isalnum() else "_" for c in sys.argv[1]))' "$1"; }
pinline() { printf '%s\t%s\t%s\n' "$1" "$(stat -f %z "$2")" "$(shasum -a 256 "$2" | cut -d' ' -f1)" >> "$fx/pins.txt"; }
pinline retail "$iso"
# Made-up outputs: the disc's first 3 MB with a few changes each.
mk() { head -c 3145728 "$iso" > "$2"; printf "$1" | dd of="$2" bs=1 seek=1048576 conv=notrunc 2> /dev/null; }
mk "ANIMELEE-FIXTURE" "$fx/animelee.out"; mk "CLASSIC-FIXTURE" "$fx/classic.out"; mk "CRYSTAL-FIXTURE!" "$fx/crystal.out"; mk "DIET64-FIXTURE!!" "$fx/d64.out"
xdelta3 -e -f -s "$iso" "$fx/animelee.out" "$fx/ANIMELEE - COMPLETE EDITION.xdelta"
xdelta3 -e -f -s "$iso" "$fx/classic.out" "$fx/DietMeleeClassic.xdelta"
xdelta3 -e -f -s "$iso" "$fx/crystal.out" "$fx/crystal.xdelta"; xdelta3 -e -f -s "$iso" "$fx/d64.out" "$fx/64.xdelta"
xdelta3 -e -f -s "$fx/DietMeleeClassic.xdelta" "$fx/crystal.xdelta" "$fx/crystaldiff.xdelta"
xdelta3 -e -f -s "$fx/DietMeleeClassic.xdelta" "$fx/64.xdelta" "$fx/64diff.xdelta"
(cd "$fx" && zip -q -0 "Animelee CE 1.0.zip" "ANIMELEE - COMPLETE EDITION.xdelta")
(cd "$fx" && tar -czf "DietMeleeLinuxPatcher.tar.gz" DietMeleeClassic.xdelta crystaldiff.xdelta 64diff.xdelta)
for f in "Animelee CE 1.0.zip" "ANIMELEE - COMPLETE EDITION.xdelta" DietMeleeLinuxPatcher.tar.gz DietMeleeClassic.xdelta crystaldiff.xdelta 64diff.xdelta; do pinline "$f" "$fx/$f"; done
pinline "Animelee CE 1.0.iso" "$fx/animelee.out"; pinline "Diet Classic.iso" "$fx/classic.out"; pinline "Crystal.iso" "$fx/crystal.out"; pinline "Diet 64.iso" "$fx/d64.out"
# The authors' pages as MediaFire serves them (the download button's address), and the files at those addresses.
page() { printf '<html><body><a class="input popsok" aria-label="Download file" href="%s" id="downloadButton">Download</a></body></html>\n' "$2" > "$fx/$(san "$1")"; }
ap="https://www.mediafire.com/file/2ztqob93nrug8yg/ANIMELEE_-_COMPLETE_EDITION_-_PATCH_1.0.zip/file"; al="https://download1500.mediafire.com/fixture/a/ANIMELEE.zip"
dp="https://www.mediafire.com/file/v6v5yuqd9le7jav/DietMeleeLinuxPatcher.tar.gz/file"; dl="https://download2201.mediafire.com/fixture/d/DietMeleeLinuxPatcher.tar.gz"
page "$ap" "$al"; cp "$fx/Animelee CE 1.0.zip" "$fx/$(san "$al")"
page "$dp" "$dl"; cp "$fx/DietMeleeLinuxPatcher.tar.gz" "$fx/$(san "$dl")"
run() {   # $1 case, $2 which
  local dir="$root/$1"; mkdir -p "$dir/melee-unlocked"
  MELEE_UPDATE_CHECK=0 MELEE_NO_FINDER=1 MU_LOOKS_TEST_DIR="$fx" MU_LOOKS_TEST_INSTALL=$2 timeout 600 "$engine" --iso "$iso" --profile-dir "$dir" \
    --offline --hidden --volume 0 --no-audio-device --no-record --frames 1500 > "$dir/log" 2>&1
  echo "$1: engine exit $?"; grep -h "looks: " "$dir/log" | sed 's/^\[ *[0-9.]*\] //' | grep -v "looks: Off\|match frames"
  find "$dir/melee-unlocked/Mods" -name "*.iso" 2> /dev/null | sed "s|$dir/||"
}
run animelee 1
run diet 0
# A page that asks for a person (no download address): the install stops and says so.
printf '<html><body>Please complete the security check to continue.</body></html>\n' > "$fx/$(san "$ap")"
run person 1
