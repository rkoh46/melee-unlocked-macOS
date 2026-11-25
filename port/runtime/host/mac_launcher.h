// The launcher (mu_launcher.mm, melee-unlocked's launcher rebuilt for the Mac): pick the disc, a few settings, PLAY.
// What it chose goes to the melee-unlocked engine (MeleeUnlockedEngine in the bundle) as arguments and launcher.ini.
// Nothing from the game ships with the app; the user's own image is used.
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <string>
namespace host {
struct LauncherSettings {
  std::string iso;             // remembered disc image; empty until chosen
  bool widescreen = false;     // the engine's 16:9 code (its settings.ini "widescreen", which the launcher reads and writes)
  bool online = true;          // Slippi Online services
  int online_delay = 2;        // Slippi Online input delay frames (1..9; 0 automatic): each frame adds 16.7 ms; 2 is Slippi's default
  int scale = 0;               // internal resolution multiplier, 0 = auto
  bool vsync = true;           // display sync; off = uncapped presentation
  bool fullscreen = true;
  int volume = 70;             // 0..100
  std::string replay_dir;      // for the Replay Viewer
  std::string slippi_dir;      // where this app keeps its own user.json (native sign-in)
  std::string account_name, account_code;   // current login, if any
  bool account_from_launcher = false;       // login comes from the Slippi Launcher's file
  bool vanilla = false;        // Game Build: Vanilla (the engine plays the game as the disc does: no codes, nothing unlocked)
  uint32_t accent = 0;         // the launcher colour (0xRRGGBB; 0 is melee-unlocked's purple)
};
// Shows the launcher and blocks until PLAY (true) or quit (false). `error` explains why the remembered disc could not be
// used, if that happened.
bool mu_launcher_run(LauncherSettings& settings, const std::string& error);
void mac_show_error(const std::string& title, const std::string& detail);
// The Settings window (mu_settings_window.mm): every melee-unlocked setting the app honours, grouped as melee-unlocked's panel
// groups them. Blocks until closed. `shot` (tests): off-screen, drawn into that PNG, then closed.
void mu_settings_window_run(LauncherSettings& settings, int category, const char* shot);
// The melee-unlocked engine's executable: MELEE_MU_ENGINE (tests), else MeleeUnlockedEngine next to the launcher in the
// bundle's Contents/MacOS; empty when it is missing.
std::string engine_path();
// The engine's settings.ini (melee-unlocked/settings.ini): one "key value" line, read and written in place.
std::string engine_setting(const std::string& key);
void set_engine_setting(const std::string& key, const std::string& value);
void log(const char* fmt, ...);   // the launcher's log (main_mac.cpp): stderr and melee-unlocked/launcher.log
}  // namespace host
