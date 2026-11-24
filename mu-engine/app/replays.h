// Slippi replays in the melee-unlocked engine (replays.cpp): recording every match, and playback through the engine.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "mu_host.h"
#include <cstdint>
#include <string>

namespace replays {
void configure(const std::string& folder, bool save_matches);
bool start_playback(const std::string& path);   // before the host starts; false when the file cannot be played
bool active();                                   // a replay is playing
bool word(int which, uint32_t* value);           // the recorded option words (0..2) while a replay plays
void install(MuHostApi* host);                   // the host's replay entries; from the tables hook
void finish();                                   // after the run: the last recording written, the fidelity line
}
