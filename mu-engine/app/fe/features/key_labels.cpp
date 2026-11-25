// Keyboard key names in the game's prompts (melee-unlocked's command 0xDF, mu_host_button_label in the game image's
// shim/mu_slippi_css.c: "Hold E to disconnect" for a keyboard player instead of the GameCube button): the payload is the port
// and the PAD bit (big-endian 16), the reply the key's name. Answered for port 1 when the keyboard is the only controller (the
// app's keyboard plays on port 1, window_sdl.cpp); otherwise no answer, and the game keeps the GameCube names.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "features_internal.h"
#include "frontend.h"
#include "input_config.h"
#include "window.h"

#include <cstdint>
#include <string>
#include <vector>

namespace fe::features {
namespace {
int control_of(uint16_t bit) {   // the GameCube PAD bit -> the app's GcControl
  switch (bit) {
    case 0x0100: return host::GC_CTL_A;    case 0x0200: return host::GC_CTL_B;    case 0x0400: return host::GC_CTL_X;
    case 0x0800: return host::GC_CTL_Y;    case 0x0010: return host::GC_CTL_Z;    case 0x0040: return host::GC_CTL_L;
    case 0x0020: return host::GC_CTL_R;    case 0x1000: return host::GC_CTL_START; case 0x0008: return host::GC_CTL_DUP;
    case 0x0004: return host::GC_CTL_DDOWN; case 0x0001: return host::GC_CTL_DLEFT; case 0x0002: return host::GC_CTL_DRIGHT;
    default: return -1;
  }
}
int button_label(uint8_t, const uint8_t* p, uint32_t n, std::vector<uint8_t>& reply) {
  if (n != 3 || p[0] != 0) return 0;
  const int control = control_of(uint16_t(p[1] << 8 | p[2]));
  if (control < 0 || !host::window_list_controllers().empty()) return 0;
  const int scancode = host::keyboard_map().key[control];
  if (!scancode) return 0;
  const std::string name = host::key_name(scancode);
  if (name.empty() || name.size() > 32) return 0;
  reply.assign(name.begin(), name.end());
  static int logged = 0;
  if (logged++ < 4) say("keys: the game's prompt names %s as \"%s\" (keyboard on port 1)", host::kGcControlNames[control], name.c_str());
  return 1;
}
}  // namespace
void register_key_labels() { add_command(0xDF, button_label); }
}  // namespace fe::features
