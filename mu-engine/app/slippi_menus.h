// Slippi's online menus in the melee-unlocked engine (slippi_menus.cpp).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace slippi_menus {
void set_sys_dir(const std::string& dir);   // the folder with GameFiles/GALE01 (the bundle's slippi_sys)
void disc_ready();                           // the host's disc table is loaded: build and serve the files
bool active();                               // the files are served: the game may show its online rows
int read(uint64_t offset, void* out, size_t bytes);   // -1 not ours, 0 failed, 1 done
}
