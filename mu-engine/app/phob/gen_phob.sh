#!/bin/bash
# The build-time copies of melee-unlocked's PhobGCC screens (PhobVision's menus, PhobScope and Phob Check: its
# port/runtime/gx/phob_menu.cpp, phob_stickmaps.h, host/phobscope.cpp, phob_store.cpp, phob_adapter_input.h) for
# mu-engine/app/phob, LF line ends and no byte-order mark. Nothing of it is committed. usage: gen_phob.sh MU_TREE OUT_DIR
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
mu="$1"; gen="$2"
mkdir -p "$gen"
put() { cmp -s "$1" "$2" || cp "$1" "$2"; }
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
for f in gx/phob_menu.cpp gx/phob_menu.h gx/phob_stickmaps.h host/phobscope.cpp host/phobscope.h host/phob_store.cpp host/phob_adapter_input.h; do
  perl -pe 's/\r$//; s/^\xEF\xBB\xBF// if $. == 1' "$mu/port/runtime/$f" > "$work/$(basename "$f")"
  put "$work/$(basename "$f")" "$gen/$(basename "$f")"
done
