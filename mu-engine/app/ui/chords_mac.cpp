// The controller chords of Melee-NX-NetPlay's frontend (its controls.cpp controls_chords), for the Mac engine: Start +
// D-pad Down + Z opens or closes melee-unlocked's settings panel, Start + D-pad Up + Z the Practice menu in offline
// Training (features/proai.cpp) or the panel's Training section elsewhere; in Training a lone Start waits a moment for
// the rest of a chord. The chord rules are melee-unlocked's own (port/runtime/host/settings_chord.h, the fetched tree).
// Called from the menu's poll (mu_panel.cpp app_menu_frame), simulation thread.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host.h"
#include "app/game_menu.h"
#include "frontend.h"
#include "settings_chord.h"
#include "mu_host.h"

#include <cstdint>

namespace linux_host { bool state_snapshot(MuStatePod* out); }   // the host services (fetched tree)
namespace fe::features {
__attribute__((weak)) bool practice_menu_chord(int port);
__attribute__((weak)) bool practice_menu_open();
__attribute__((weak)) void practice_menu_pads(MuPadStatus pads[4]);
}

namespace fe {
void mac_chords(host::PadState pads[4]) {
  if (!options.play) return;
  static bool settings_down[4] = {}, practice_down[4] = {};
  static int start_waited[4] = {};
  MuStatePod state{};
  linux_host::state_snapshot(&state);
  const bool training = state.scene_major == 0x1C && !session_online();
  for (int i = 0; i < 4; ++i) {
    const bool connected = pads[i].err == 0;
    const uint16_t raw = connected ? uint16_t(pads[i].button) : 0;
    uint16_t buttons = uint16_t(pads[i].button);
    const bool settings = host::settings_shortcut::poll_z_start(connected, buttons, settings_down[i]);
    const bool practice = host::settings_shortcut::poll_practice(connected, buttons, practice_down[i]);
    if (settings) {
      if (host::menu_is_open()) host::menu_close(); else host::menu_toggle();
      say("controls: Start + D-pad Down + Z on port %d: the menu", i + 1);
    } else if (practice) {
      if (!host::menu_is_open() && &features::practice_menu_chord && features::practice_menu_chord(i)) {
        say("controls: Start + D-pad Up + Z on port %d: the Practice menu", i + 1);
      } else {
        if (host::menu_is_open()) host::menu_close(); else host::menu_open_extra("Training");
        say("controls: Start + D-pad Up + Z on port %d: the Training page", i + 1);
      }
    }
    const bool practice_open = &features::practice_menu_open && features::practice_menu_open();
    if (connected) host::settings_shortcut::hold_lone_start(training && !host::menu_is_open() && !practice_open, settings_down[i] || practice_down[i], raw, buttons, start_waited[i]);
    pads[i].button = buttons;
  }
  // While the Practice menu is open its controller is the menu's and the game gets neutral pads (features/proai.cpp).
  if (&features::practice_menu_pads) {
    MuPadStatus mu[4];
    for (int i = 0; i < 4; ++i) {
      mu[i].button = uint16_t(pads[i].button); mu[i].stick_x = pads[i].stick_x; mu[i].stick_y = pads[i].stick_y;
      mu[i].sub_x = pads[i].sub_x; mu[i].sub_y = pads[i].sub_y; mu[i].trigger_l = pads[i].trig_l; mu[i].trigger_r = pads[i].trig_r;
      mu[i].analog_a = pads[i].analog_a; mu[i].analog_b = pads[i].analog_b; mu[i].err = pads[i].err;
    }
    features::practice_menu_pads(mu);
    for (int i = 0; i < 4; ++i) {
      pads[i].button = mu[i].button; pads[i].stick_x = mu[i].stick_x; pads[i].stick_y = mu[i].stick_y;
      pads[i].sub_x = mu[i].sub_x; pads[i].sub_y = mu[i].sub_y; pads[i].trig_l = mu[i].trigger_l; pads[i].trig_r = mu[i].trigger_r;
      pads[i].analog_a = mu[i].analog_a; pads[i].analog_b = mu[i].analog_b;
    }
  }
}
}  // namespace fe
