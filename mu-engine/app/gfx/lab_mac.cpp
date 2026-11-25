// melee-unlocked's Lab view on the Mac (its lab_view.cpp from the fetched tree, gfx/gen_gfx.sh): the match drawn the way
// Slippi Lab draws a replay (silhouettes, stick figures, capsules or blobs on a plain stage), over the game picture in the
// engine's ImGui pass. The Source Port path upstream takes: the game image's geometry snapshot (mu_look_snapshot_v1,
// mu_look_items_v1, mu_look_extras_v1, found with mu-engine/patches/0016's fe_game_symbol) read at the end of each retrace
// on the simulation thread. Read only: it cannot change the match. Settings: fe/features/graphics.cpp (labview, Ctrl+F3).
// The silhouette packs (Lab/<character>.lab, upstream's own download) are optional: the stick figure, capsule and blob looks
// draw from the fighters' hurtboxes and need nothing.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "lab_view.h"
#include "mu_look_snapshot.h"   // melee-unlocked port/runtime/abi
#include "gfx_live.h"
#include "gfx_host_shim.h"

#include <array>
#include <filesystem>
#include <string>

namespace linux_host { void* fe_game_symbol(const char* name); }   // mu-engine/patches/0016

std::string mu_lab_dir() { return (mu_gfx_folder() / "Lab").string(); }
bool mu_lab_spectate() { return false; }   // upstream's room match window: not on the Mac
float mu_lab_picture_aspect() { return 4.0f / 3.0f; }

namespace fe::gfx {
namespace {
void (*g_snapshot)(MuLookSnapshot*) = nullptr;
void (*g_items)(MuLookItems*) = nullptr;
void (*g_extras)(MuLookExtras*) = nullptr;
bool g_looked = false;
}  // namespace

void lab_settings_changed() {
  auto& l = live();
  lab::set_style(l.lab_fighters.load(), l.lab_stage.load());
  uint32_t players[4];
  for (int i = 0; i < 4; ++i) players[i] = l.lab_colour[i].load();
  lab::set_colors(players, l.lab_background.load(), l.lab_stage_colour.load());
  lab::set_enabled(l.lab_view.load());
}

// Simulation thread, once per retrace: the frame the view draws.
void lab_retrace() {
  if (!live().lab_view.load(std::memory_order_relaxed) && !lab::styles_on()) return;
  if (!g_looked) {
    g_looked = true;
    g_snapshot = reinterpret_cast<void (*)(MuLookSnapshot*)>(linux_host::fe_game_symbol("mu_look_snapshot_v1"));
    g_items = reinterpret_cast<void (*)(MuLookItems*)>(linux_host::fe_game_symbol("mu_look_items_v1"));
    g_extras = reinterpret_cast<void (*)(MuLookExtras*)>(linux_host::fe_game_symbol("mu_look_extras_v1"));
    host::log("lab view: the game's geometry snapshot %s", g_snapshot ? "found" : "is missing from the game image (the view stays off)");
  }
  if (!g_snapshot || !lab::active()) return;
  static MuLookSnapshot snapshot; static MuLookItems items; static MuLookExtras extras;   // large: kept off the stack
  snapshot = MuLookSnapshot{};
  g_snapshot(&snapshot);
  if (g_items) { items = MuLookItems{}; g_items(&items); }
  if (g_extras) { extras = MuLookExtras{}; g_extras(&extras); }
  static const std::array<std::string, 4> names{};
  lab::native_frame(snapshot, g_items ? &items : nullptr, g_extras ? &extras : nullptr, names.data());
}

bool lab_packs_present() {
  std::error_code ec;
  return std::filesystem::exists(std::filesystem::path(mu_lab_dir()) / "2.lab", ec);
}
}  // namespace fe::gfx

// The engine's ImGui pass (ui/ui_metal.mm), before the overlays and the panel: the view covers the window during a match.
void mu_gfx_imgui_draw(int width, int height) {
  lab::draw(fe::gfx::live().lab_view.load(std::memory_order_relaxed), float(width), float(height));
}
