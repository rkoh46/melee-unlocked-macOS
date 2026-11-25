// Two of melee-unlocked's small rows the Mac adds here:
//   - "Reset CPU training options" (upstream's button): every persistent CPU training override (the cpu_* keys of
//     settings.ini: 20XX CPUs, tech, get-up, DI, SDI, no taunts, always L-cancel, no rapid jabs, no transforming) back to the
//     game's own behaviour, on the panel's Computer players page;
//   - the replay viewer's mute (upstream's: while muted the game's sound is not heard; the volume setting is untouched):
//     settings.ini `replaymute`, a Replays page row; mu_play_main.cpp's sound output asks replay_muted();
//   - "Restart game" (upstream's footer menu, with its confirmation): A, then A again within 3 s; the engine starts again with
//     the same arguments (replays.cpp's restart request, mu_play_main.cpp); offline only;
//   - the Arcade look preset and reset (upstream's two buttons): every Arcade-look switch on (with the grid) or off at once;
//   - 20XX TE's pair link (upstream's te_follow_ours / ours_follow_te): with the TE save's features on, TE's "Disable screen
//     rumble" and this app's "No screen shake" are one switch: whichever changes, the other follows (it settles once, from TE's,
//     when first linked); TE's Lock settings keeps TE's side as it is. TE's input display is drawn by the overlays' TE pads.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "features_internal.h"
#include "frontend.h"
#include "mu_host.h"
#include "feature_calls.h"     // call_setting: the overlays module's arcade_combo_counter

#include <atomic>
#include <chrono>
#include <string>

namespace fe::features {
namespace {
std::atomic<bool> g_replay_mute{false}, g_restart{false};
double g_restart_armed = -10.0;
double now_s() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
std::string restart_value() {
  if (online()) return "Not online (leave the online mode first)";
  return now_s() - g_restart_armed < 3.0 ? "Press A again to restart" : "Press A";
}
void restart_press() {
  if (online()) return;
  if (now_s() - g_restart_armed < 3.0) { g_restart = true; say("options: Restart the game confirmed"); }
  else g_restart_armed = now_s();
}
constexpr const char* kArcade[] = {"arcade_hitboxes", "arcade_projectile_hitboxes", "arcade_hitboxes_behind", "arcade_camera_2d", "arcade_backdrop"};
void arcade_set(bool on) {
  for (const char* k : kArcade) settings_change(k, on ? (std::string(k) == "arcade_backdrop" ? "2" : "1") : "0");
  // The combo counter too, as upstream's set_all: a key of the overlays module, set there and written here.
  fe::features::call_setting("arcade_combo_counter", on ? "1" : "0"); fe::settings_write_key("arcade_combo_counter", on ? "1" : "0");
  say("options: Arcade look %s", on ? "preset: every switch on, the grid behind the stage" : "reset: every switch off");
}
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
// --feature-test extras (verification runs): at retrace 120 the Arcade preset and reset, then Restart the game pressed twice;
// the restarted engine (--restarted on its command line) does nothing more.
void test_retrace(const Retrace& info) {
  static bool done = false;
  if (done || info.retrace < 120 || !feature_test("extras")) return;
  done = true;
  for (const auto& a : options.rest) if (a == "--restarted") { say("extras test: running again after Restart the game"); return; }
  arcade_set(true);
  say("extras test: after the preset: %s %s %s %s %s", settings_get("arcade_hitboxes").c_str(), settings_get("arcade_projectile_hitboxes").c_str(),
      settings_get("arcade_hitboxes_behind").c_str(), settings_get("arcade_camera_2d").c_str(), settings_get("arcade_backdrop").c_str());
  arcade_set(false);
  say("extras test: after the reset: %s %s %s %s %s", settings_get("arcade_hitboxes").c_str(), settings_get("arcade_projectile_hitboxes").c_str(),
      settings_get("arcade_hitboxes_behind").c_str(), settings_get("arcade_camera_2d").c_str(), settings_get("arcade_backdrop").c_str());
  restart_press(); say("extras test: Restart the game: %s", restart_value().c_str()); restart_press();
}
// The pair link, on the simulation thread after each frame (offline only; nothing changes online).
bool g_linked = false, g_last_te = false, g_last_ours = false;
void te_pair(const Retrace& info) {
  if (info.online || !mods_te_owned() || !(settings_te_word(1) & MU_GAME_OPTION_TE)) { g_linked = false; return; }
  uint32_t& w2 = settings_te_word(2);
  const bool te = (w2 & MU_GAME_OPTION2_TE_NO_SCREEN_RUMBLE) != 0, ours = settings_get("noscreenshake") == "1";
  if (!g_linked) {   // settles once, from TE's side
    g_linked = true;
    if (te != ours) { settings_change("noscreenshake", te ? "1" : "0"); say("options: TE's Disable screen rumble and No screen shake linked (%s)", te ? "on" : "off"); }
    g_last_te = te; g_last_ours = te;
    return;
  }
  if (te != g_last_te) {
    if (ours != te) settings_change("noscreenshake", te ? "1" : "0");
    say("options: TE's Disable screen rumble %s: No screen shake follows", te ? "on" : "off");
  } else if (ours != g_last_ours && !(w2 & MU_GAME_OPTION2_TE_LOCK_SETTINGS)) {
    w2 = ours ? (w2 | MU_GAME_OPTION2_TE_NO_SCREEN_RUMBLE) : (w2 & ~MU_GAME_OPTION2_TE_NO_SCREEN_RUMBLE);
    settings_state_changed();
    say("options: No screen shake %s: TE's Disable screen rumble follows", ours ? "on" : "off");
  }
  g_last_te = (w2 & MU_GAME_OPTION2_TE_NO_SCREEN_RUMBLE) != 0; g_last_ours = settings_get("noscreenshake") == "1";
}
// --feature-test te-pair: with the TE profile, at retrace 150 this app's No screen shake is switched on; at 200 TE's bit off.
void te_pair_test(const Retrace& info) {
  if (!feature_test("te-pair") || !mods_te_owned()) return;
  if (info.retrace == 150) { settings_change("noscreenshake", "1"); say("te-pair test: No screen shake switched on"); }
  if (info.retrace == 152) say("te-pair test: TE's Disable screen rumble is %s", (settings_te_word(2) & MU_GAME_OPTION2_TE_NO_SCREEN_RUMBLE) ? "on" : "off");
  if (info.retrace == 200) { settings_te_word(2) &= ~MU_GAME_OPTION2_TE_NO_SCREEN_RUMBLE; say("te-pair test: TE's Disable screen rumble switched off (as its menu would)"); }
  if (info.retrace == 202) say("te-pair test: No screen shake is %s", settings_get("noscreenshake") == "1" ? "on" : "off");
}
}  // namespace
bool restart_requested() { return g_restart.exchange(false); }
bool replay_muted() { return g_replay_mute.load(std::memory_order_relaxed) && replay_playing(); }
void register_extras() {
  add_setting(setting);
  add_retrace(test_retrace);
  add_retrace(te_pair_test);
  add_retrace(te_pair);
  add_menu_entry({"Computer players", "Reset CPU training options", cpu_reset_value, nullptr, cpu_reset_press});
  add_menu_entry({"Game options", "Restart the game", restart_value, nullptr, restart_press});
  add_menu_entry({"Picture and HUD", "Arcade look: every switch on", [] { return std::string(); }, nullptr, [] { arcade_set(true); }});
  add_menu_entry({"Picture and HUD", "Arcade look: reset", [] { return std::string(); }, nullptr, [] { arcade_set(false); }});
  add_menu_entry({"Replays", "Mute the game while watching a replay", [] { return std::string(g_replay_mute ? "On" : "Off"); },
                  [](int) { g_replay_mute = !g_replay_mute; say("replay: mute %s", g_replay_mute ? "on" : "off"); }, nullptr,
                  [](MenuEntry::Write w) { w("replaymute", g_replay_mute ? "1" : "0"); }});
}
}  // namespace fe::features
