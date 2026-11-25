// What the Mac engine adds to Melee-NX-NetPlay's frontend interface (fe/frontend.h). See frontend_mac.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace fe {
void set_session_online(bool online);   // the engine, when the netplay host starts and stops
}

#include "app/game_menu.h"
#include <functional>
namespace fe {
// fe/app_menu.cpp's in-game menu setup asks these (mu_play_main.cpp sets them): the starting values of the Mac's own
// rows, and what a change does to the window and the renderer.
extern std::function<void(host::RuntimeSettings&)> mac_menu_initial;
extern std::function<void(const host::RuntimeSettings&, host::MenuChange)> mac_menu_apply;
void app_menu_setup();
}
