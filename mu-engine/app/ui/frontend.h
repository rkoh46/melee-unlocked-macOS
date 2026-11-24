// What melee-unlocked's settings panel (mu_panel.cpp, from Melee-NX-NetPlay's frontend) and its Mods screens use from
// the Switch frontend, answered by the Mac engine (frontend_mac.cpp): the log, the app's folder, launcher.ini's keys and
// the next start's mod profile.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>

namespace fe {
struct Options { std::string out_dir; std::string iso; };
extern Options options;                       // out_dir: the engine's folder (Application Support/Dashdance/melee-unlocked)
void say(const char* format, ...) __attribute__((format(printf, 1, 2)));
std::string app_ini(const std::string& key, const std::string& fallback);   // launcher.ini (the app's settings file)
void app_ini_set(const std::string& key, const std::string& value);
bool settings_change(const std::string& key, const std::string& value);     // mod_profile: the next start's profile
std::string settings_get(const std::string& key);
void settings_write_key(const std::string& key, const std::string& value);   // the same as settings_change here
void settings_set_quiet(const std::string& key, const std::string& value);   // nothing more to do: settings_get reads the file
bool session_online();                       // the netplay host is on (Slippi Online): the mod profile is locked
void set_session_online(bool online);        // the engine, when the netplay host starts and stops
}
