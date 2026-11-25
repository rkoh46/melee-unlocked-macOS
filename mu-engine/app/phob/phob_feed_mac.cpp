// The panel's side of melee-unlocked's PhobGCC menu (phob_bridge.h), against the app's own host.h: while the menu is up it takes
// the controllers the game's poll read (every connected one, the keyboard included), so the panel and the game get none.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host.h"   // port/runtime/host (the app's)
#include "phob_bridge.h"

bool mu_phob_menu_open() { return mu_phob::open(); }
bool mu_phob_menu_feed(host::PadState pads[4]) {
  if (!mu_phob::open()) return false;
  mu_phob::Nav nav;
  for (int i = 0; i < 4; ++i) {
    if (pads[i].err != 0) continue;
    nav.buttons |= pads[i].button;
    if (!nav.stick_x && !nav.stick_y) { nav.stick_x = pads[i].stick_x; nav.stick_y = pads[i].stick_y; }
  }
  mu_phob::set_nav(nav);
  return true;
}
void mu_ui_top_draw(int width, int height) { mu_phob::frame(width, height); }
