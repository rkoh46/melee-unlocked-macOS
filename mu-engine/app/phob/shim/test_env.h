// melee-unlocked's test_env.h for its PhobGCC screens on the Mac: test overrides read as upstream reads them without YouGame.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdlib>
namespace host::test_env { inline const char* getenv(const char* name) { return std::getenv(name); } }
