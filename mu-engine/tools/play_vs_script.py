#!/usr/bin/env python3
"""A controller script for play mode tests: melee-unlocked's parity_vs_onett.txt (from the fetched tree, never committed) with
its character select presses moved for play mode, where melee-unlocked's codes start the hands elsewhere than the retail game
(the script's own picks leave both hands on the title bar): after them both hands go down to the first row and pick, then
START, and the stage select picks Onett as the script does. Two human players on a VS match on Onett.
usage: play_vs_script.py PARITY_VS_ONETT.txt OUT.txt"""
# SPDX-License-Identifier: GPL-3.0-or-later
import sys

src, out = sys.argv[1], sys.argv[2]
lines = open(src).read().split("\n")
result, in_menu = [], False
for line in lines:
    words = line.split()
    if line.startswith("@scene 1:0"):
        in_menu = True
    elif line.startswith("@"):
        in_menu = False
    # the script's own START on the character select and its stage select presses are replaced below
    if in_menu and words and words[0] in ("500", "510", "700", "740", "760", "770") and not line.startswith("#"):
        continue
    result.append(line)
    if in_menu and line.strip() == "450 p=2":
        result += ["600 sy=-127", "600 p=2 sy=-127", "612", "612 p=2", "620 A", "620 p=2 A", "630", "630 p=2",
                   "700 START", "710", "900 sy=100", "940", "960 A", "970"]
open(out, "w").write("\n".join(result))
