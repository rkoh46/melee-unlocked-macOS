// melee-unlocked's PhobGCC menu (PhobVision, PhobScope, Phob Check) in the Mac engine: what the panel and the ImGui pass ask of
// it (phob_host_mac.cpp), in plain types (the menu's own files compile against melee-unlocked's host.h, the app against its own).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

namespace mu_phob {
struct Nav { uint16_t buttons = 0; int8_t stick_x = 0, stick_y = 0; };
bool open();                                   // the full-screen menu is up
void request(int port, bool scopes_only);      // open it on adapter port `port` (0..3); scopes_only: the Controller test
void set_nav(const Nav& nav);                  // the controllers, as the game's poll read them (the menu takes them)
void frame(int width, int height);             // the ImGui pass, after the panel
}  // namespace mu_phob
