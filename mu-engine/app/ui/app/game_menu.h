// The in-game menu interface as melee-unlocked's settings panel (../mu_panel.cpp) implements it in the engine: the app's
// port/runtime/host/game_menu.h with the console rows and the host pages of Melee-NX-NetPlay's frontend (its game_menu.h
// and patches 0104 and 0111). Only the engine's own sources include it; window_sdl.cpp keeps the app's header, whose
// calls (menu_frame, menu_touch, menu_toggle, menu_capturing, menu_overlay) are the same.
//
// In-game settings menu and performance HUD, drawn over the game (the decomp-style overlay that
// Ship of Harkinian and friends have): open it with L+R+Start held for half a second on any
// controller, F1 or Escape on the keyboard, or the MENU button next to the touch controls. While it
// is open the game receives neutral inputs. Changes apply immediately and are saved for next time.
// A Controls page configures each controller and the keyboard, with a step-by-step remap.
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <vector>
#include <string>
#include "host.h"
#include "overlay.h"
#include <functional>
#include <string>

namespace host {
struct RuntimeSettings {
  int scale = 0;                 // 0 auto, 1..8
  int anisotropy = 16;           // 1, 4, 16
  float sharpness = 0.0f;        // 0..1
  int upscaler = 0;              // 0 off, 1 MetalFX spatial (balanced), 2 (quality); Metal only, ignored where unsupported
  bool widescreen = false;       // takes full effect on the next launch (the game's own code table reads it at boot)
  bool vsync = true;
  int volume = 70;               // 0..100
  float overlay_opacity = 1.0f;  // touch controls
  float overlay_scale = 1.0f;
  bool hud = false;              // performance HUD
  bool fullscreen = false;       // macOS
  int online_delay = 2;          // Slippi Online input delay frames (1..9), used from the next match; each frame adds 16.7 ms
  int filter = 0;                // console: screen filter (0 smooth, 1 area, 2 CRT, 3 vibrant)
  int msaa = 1;                  // console: antialiasing samples (1 or 4), next launch
  bool full_speed = true;        // console: leave out a picture when the game falls behind, so it never slows down
  int stick_range = 100;         // console: GameCube stick value at the gate (80..120; a real controller reads ~100)
  int trigger_range = 4;         // console: GameCube analog trigger scale (2..8; higher reaches full shield sooner)
};
enum class MenuChange { Graphics, Volume, TouchControls, Fullscreen, Hud, Widescreen, OnlineDelay, Filter, Msaa, FullSpeed, Trigger, Anisotropy, Stick };
// Console: the Watch replays page lists these (newest first) and calls `watch` with the chosen one.
void menu_open_replays();                  // opens the menu on the Watch replays page
void menu_close();
void menu_sync_fullscreen(bool on);       // mumac: the window's full screen changed outside the panel
void menu_set_notice(const std::string& text);   // a line in place of the Watch replays page's hint
void menu_set_replays(std::function<std::vector<std::string>()> list, std::function<void(const std::string&)> watch);
// `apply` runs on the simulation thread whenever the player changes a value.
void menu_init(const RuntimeSettings& initial, std::function<void(const RuntimeSettings&, MenuChange)> apply);
bool menu_is_open();
bool menu_capturing();                     // a remap is waiting for an input: Escape belongs to it, not to the menu toggle
bool menu_changed();                       // any value changed since menu_init (for saving at exit)
RuntimeSettings menu_settings();
void menu_toggle();                        // from the event thread (keyboard)
// Per frame from input_poll: handles the open combo and navigation, neutralises pads while open.
void menu_frame(PadState pads[4]);
// Touch: returns true when the point hit the menu (or its MENU button) and was consumed.
bool menu_touch(float px, float py);
// Text and shapes for the open menu and the HUD; appended to `out`.
void menu_overlay(OverlayFrame& out, int window_w, int window_h, bool touch_controls_visible);
// ---- pages and rows a host adds (Melee-NX-NetPlay patches 0104 and 0111) ----
struct MenuExtraItem {
  std::string label, value;      // an empty value makes an action row (A)
  bool enabled = true;           // false: greyed and inert (for example locked during online play)
  bool next_start = false;       // marked " *": takes effect at the next start
};
struct MenuExtraPage {
  std::string title;             // the row on the main page and the page's title bar
  std::function<std::vector<MenuExtraItem>()> items;   // called under the menu's lock, on either thread: must not call back into the menu
  std::function<bool(int item, int dir)> change;       // dir -1/+1 left/right, 0 the A button; outside the lock; true: a saved value changed
};
void menu_set_extra_pages(std::vector<MenuExtraPage> pages);   // listed on the main page before Controls
void menu_hide(MenuChange what);                                // a main-page row this host has no use for
void menu_set_label(MenuChange what, const std::string& label); // another label for a main-page row
void menu_set_quit(std::function<void()> quit);                 // what Quit does (default: exit(0))
void menu_set_hud_extra(std::function<std::string()> text);     // appended to the performance HUD's line
void menu_set_controls_live(bool live);                         // the Controls page lists controllers (default: as built)
void menu_open_extra(const std::string& title);                 // opens the menu on the host page with this title
}  // namespace host
