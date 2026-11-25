// The controller setup of Melee-NX-NetPlay's frontend (fe/controls.cpp: bindings beyond the button remap, box mode,
// inversion, radius, snapback, trigger click and cap, profiles, Dolphin import) on the Mac engine's input. The app's
// own input (port/runtime/host/window_sdl.cpp) has already read the GameCube adapter (IOKit) and the SDL gamepads and
// applied the CONTROLS page's remap and deadzones; this adds the rest per port, as the Switch's app_pad_postprocess does.
// Called from the menu's poll (mu_panel.cpp app_menu_frame), simulation thread. Setups are kept in launcher.ini as
// padx.<id>, as on the Switch.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "frontend.h"
#include "host.h"
#include "input_config.h"
#include "window.h"
#include <SDL3/SDL.h>

#include <fstream>
#include <string>

namespace fe {
namespace {
std::string guid_of(SDL_Gamepad* pad) {
  char text[64] = {};
  SDL_GUIDToString(SDL_GetJoystickGUIDForID(SDL_GetGamepadID(pad)), text, sizeof text);
  return text;
}
}  // namespace

// True when the pads must not reach the menu as they are (a binding is being captured).
bool mac_controls_frame(host::PadState pads[4]) {
  if (!options.play) return false;
  host::PadState raw[4];
  for (int i = 0; i < 4; ++i) raw[i] = pads[i];
  if (controls_active()) {
    host::GcAdapterStatus adapter{};
    const uint32_t adapter_ports = (host::gcadapter_status(adapter) ? adapter.ports : 0) | controls_test_adapter_ports();
    const host::ControllerMap defaults = host::ControllerMap::defaults();
    for (int port = 0; port < 4; ++port) {
      host::PadState& p = pads[port];
      if (p.err != 0) continue;
      if (adapter_ports & (1u << port)) { controls_apply_adapter(port, &p, 0); continue; }
      auto* pad = static_cast<SDL_Gamepad*>(host::input_port_gamepad(port));
      if (!pad) continue;
      const std::string guid = guid_of(pad);
      const host::ControllerConfig* config = host::controller_config_for(guid);
      const host::PadState before = raw[port];
      controls_apply_sdl(guid, pad, &before, &p, config ? config->map.trigger_press : defaults.trigger_press, 0);
    }
  }
  return controls_capture_frame(raw);
}

// launcher.ini's padx.<id> lines (start), and the setups as they are now (exit).
void mac_controls_load(const std::string& launcher_ini) {
  std::ifstream in(launcher_ini);
  std::string line;
  while (std::getline(in, line)) {
    if (line.rfind("padx.", 0) != 0) continue;
    const size_t eq = line.find('=');
    if (eq != std::string::npos) controls_load(line.substr(5, eq - 5), line.substr(eq + 1));
  }
}
void mac_controls_save() {
  controls_saved([](const std::string& id, const std::string& text) { app_ini_set("padx." + id, text); });
}
}  // namespace fe
