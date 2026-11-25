// The one function melee-unlocked's phob_store.cpp takes from its controller_profiles.h: a setup name made safe as a file name
// (phob_host_mac.cpp).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
namespace host { std::string profile_clean_name(const std::string& name); }
