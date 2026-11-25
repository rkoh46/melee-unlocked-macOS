// What the Mac engine adds to Melee-NX-NetPlay's frontend interface (fe/frontend.h). See frontend_mac.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>

namespace fe {
void set_session_online(bool online);   // the engine, when the netplay host starts and stops
void mac_controls_load(const std::string& launcher_ini);   // controls_mac.cpp
void mac_controls_save();
}

#include "app/game_menu.h"
#include <functional>
namespace fe {
// fe/app_menu.cpp's in-game menu setup asks these (mu_play_main.cpp sets them): the starting values of the Mac's own
// rows, and what a change does to the window and the renderer.
extern std::function<void(host::RuntimeSettings&)> mac_menu_initial;
extern std::function<void(const host::RuntimeSettings&, host::MenuChange)> mac_menu_apply;
void app_menu_setup();
// The overlays' picture options (fe/overlay_draw.cpp through the Aurora calls in frontend_mac.cpp), any thread.
struct MacPicture { float brightness = 1, contrast = 1, vibrance = 1, sharpness = 0; int crt = 0; float crt_curve = 1; float aspect = -1; int scale = -1; };
extern std::function<void(const MacPicture&)> mac_set_picture;
}
