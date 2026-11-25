// Two of melee-unlocked's small rows the Mac adds here:
//   - "Reset CPU training options" (upstream's button): every persistent CPU training override (the cpu_* keys of
//     settings.ini: 20XX CPUs, tech, get-up, DI, SDI, no taunts, always L-cancel, no rapid jabs, no transforming) back to the
//     game's own behaviour, on the panel's Computer players page;
//   - the replay viewer's mute (upstream's: while muted the game's sound is not heard; the volume setting is untouched):
//     settings.ini `replaymute`, a Replays page row; mu_play_main.cpp's sound output asks replay_muted().
// SPDX-License-Identifier: GPL-3.0-or-later
#include "features_internal.h"
#include "frontend.h"

#include <atomic>
#include <string>

namespace fe::features {
namespace {
std::atomic<bool> g_replay_mute{false};
constexpr const char* kCpuKeys[] = {"cpu_20xx", "cpu_tech", "cpu_getup", "cpu_di", "cpu_sdi", "cpu_notaunt", "cpu_lcancel", "cpu_nojab", "cpu_notransform"};
std::string cpu_reset_value() {
  int set = 0;
  for (const char* k : kCpuKeys) { const std::string v = settings_get(k); set += !v.empty() && v != "0"; }
  return set ? std::to_string(set) + " set" : "none set";
}
void cpu_reset_press() {
  for (const char* k : kCpuKeys) settings_change(k, "0");
  say("options: CPU training options reset (the game's own computer players again)");
}
bool setting(const std::string& key, const std::string& value) {
  if (key != "replaymute") return false;
  g_replay_mute = value == "1";
  return true;
}
}  // namespace
bool replay_muted() { return g_replay_mute.load(std::memory_order_relaxed) && replay_playing(); }
void register_extras() {
  add_setting(setting);
  add_menu_entry({"Computer players", "Reset CPU training options", cpu_reset_value, nullptr, cpu_reset_press});
  add_menu_entry({"Replays", "Mute the game while watching a replay", [] { return std::string(g_replay_mute ? "On" : "Off"); },
                  [](int) { g_replay_mute = !g_replay_mute; say("replay: mute %s", g_replay_mute ? "on" : "off"); }, nullptr,
                  [](MenuEntry::Write w) { w("replaymute", g_replay_mute ? "1" : "0"); }});
}
}  // namespace fe::features
