// The mod installer (mu-engine/modinstall) set up with the Mac engine's folders and melee-unlocked's catalog, for the
// settings panel's Mods screens (mu_mods.cpp). The same Manager configuration as mu-engine/app/mods_facade.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "mod_install.h"
#include <string>
#include <vector>

namespace fe {
modinstall::Manager& mod_manager();
bool mod_downloads_available();
std::string mod_profile_next();
bool mod_profile_set(const std::string& profile);
std::vector<std::string> mod_profile_choices();
}
