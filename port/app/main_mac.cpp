// The app's main program: melee-unlocked's launcher (mu_launcher.mm), then the melee-unlocked engine (MeleeUnlockedEngine,
// next to this executable in the bundle), which replaces this process with the launcher's choices.
//
// launcher.ini (~/Library/Application Support/Dashdance) is shared with the engine: the launcher writes the keys it owns
// and keeps every other line (the engine's controller setups, screen filter, HUD and the rest); the engine reads its
// controller settings and Discord choice there. Settings written by the earlier static-recompilation engine are read the
// same way: its "Static Recomp" Game Build becomes the Source Port, its widescreen choice moves to the engine's settings.ini
// once, and its memory card (User/GC/CardA) is copied to the engine's card (melee-unlocked/Card A) while that has no saves.
// SPDX-License-Identifier: GPL-2.0-or-later
#include "input_config.h"
#include "mac_launcher.h"
#include "slippi_login.h"

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <ctime>

#include <mach-o/dyld.h>
#include <unistd.h>

#ifndef MELEE_PORT_VERSION
#define MELEE_PORT_VERSION "dev"
#endif

namespace fs = std::filesystem;

namespace {
std::FILE* g_log = nullptr;

std::string home_dir() {
  const char* home = std::getenv("HOME");
  return home && *home ? home : ".";
}

// The Slippi Launcher keeps the netplay Dolphin's user.json here on macOS: a player signed in there plays online with it.
std::string find_slippi_user_dir() {
  const fs::path support = fs::path(home_dir()) / "Library/Application Support";
  for (const fs::path candidate : {support / "com.project-slippi.dolphin/netplay/User/Slippi",
                                   support / "com.project-slippi.dolphin/netplay-beta/User/Slippi",
                                   support / "com.project-slippi.dolphin/Slippi",
                                   support / "Slippi Launcher/netplay/User/Slippi"}) {
    std::error_code ec;
    if (fs::is_regular_file(candidate / "user.json", ec)) return candidate.string();
  }
  return "";
}

// launcher.ini as lines, kept in order so a rewrite changes only the keys the launcher owns.
struct Ini {
  std::vector<std::string> lines;
  std::map<std::string, std::string> values;
  void read(const fs::path& path) {
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
      lines.push_back(line);
      const size_t eq = line.find('=');
      if (eq != std::string::npos) values[line.substr(0, eq)] = line.substr(eq + 1);
    }
  }
  const char* get(const std::string& key) const { const auto it = values.find(key); return it == values.end() ? nullptr : it->second.c_str(); }
  void set(const std::string& key, const std::string& value) {
    values[key] = value;
    for (auto& l : lines) if (l.compare(0, key.size() + 1, key + "=") == 0) { l = key + "=" + value; return; }
    lines.push_back(key + "=" + value);
  }
  void write(const fs::path& path) const {
    std::ofstream out(path, std::ios::trunc);
    for (const auto& l : lines) out << l << "\n";
  }
};

void save_launcher_ini(const fs::path& path, const host::LauncherSettings& s) {
  Ini ini;
  ini.read(path);
  ini.set("iso", s.iso);
  ini.set("online", s.online ? "1" : "0");
  ini.set("online_delay", s.online_delay <= 0 ? "auto" : std::to_string(s.online_delay));
  ini.set("scale", std::to_string(s.scale));
  ini.set("vsync", s.vsync ? "1" : "0");
  ini.set("fullscreen", s.fullscreen ? "1" : "0");
  ini.set("volume", std::to_string(s.volume));
  ini.set("engine", s.vanilla ? "2" : "0");
  ini.set("accent", std::to_string(s.accent));
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  ini.write(path);
}

fs::path support_dir() { return fs::path(home_dir()) / "Library/Application Support/Dashdance"; }

bool has_gci(const fs::path& dir) {
  std::error_code ec;
  for (const auto& e : fs::directory_iterator(dir, ec)) if (e.path().extension() == ".gci") return true;
  return false;
}
fs::path engine_settings_path() { return support_dir() / "melee-unlocked/settings.ini"; }
}  // namespace

namespace host {
void log(const char* fmt, ...) {
  char line[2048];
  va_list args; va_start(args, fmt); std::vsnprintf(line, sizeof line, fmt, args); va_end(args);
  std::fprintf(stderr, "%s\n", line);
  if (g_log) { std::fprintf(g_log, "%s\n", line); std::fflush(g_log); }
}

std::string engine_path() {
  if (const char* e = std::getenv("MELEE_MU_ENGINE")) return access(e, X_OK) == 0 ? e : "";
  char path[4096]; uint32_t size = sizeof path;
  if (_NSGetExecutablePath(path, &size) != 0) return "";
  const fs::path candidate = fs::path(path).parent_path() / "MeleeUnlockedEngine";
  return access(candidate.c_str(), X_OK) == 0 ? candidate.string() : "";
}

// The engine's settings.ini ("key value" lines, # comments): the launcher's Widescreen row and the Mods page's profile.
std::string engine_setting(const std::string& key) {
  std::ifstream in(engine_settings_path());
  std::string line, out;
  while (std::getline(in, line)) {
    if (line.rfind(key + " ", 0) != 0) continue;
    out = line.substr(key.size() + 1);
    const size_t hash = out.find('#');
    if (hash != std::string::npos && key != "mod_profile") out.erase(hash);
    while (!out.empty() && (out.back() == '\r' || out.back() == ' ' || out.back() == '\t')) out.pop_back();
  }
  return out;
}
void set_engine_setting(const std::string& key, const std::string& value) {
  std::vector<std::string> lines; bool found = false;
  { std::ifstream in(engine_settings_path()); std::string line; while (std::getline(in, line)) lines.push_back(line); }
  for (auto& l : lines) {
    size_t at = 0;
    while (at < l.size() && (l[at] == '#' || l[at] == ' ')) ++at;   // the template's commented-out line takes the value
    if (l.compare(at, key.size() + 1, key + " ") == 0 && !found) { l = key + " " + value; found = true; }
  }
  if (!found) lines.push_back(key + " " + value);
  std::error_code ec;
  fs::create_directories(engine_settings_path().parent_path(), ec);
  std::ofstream out(engine_settings_path(), std::ios::trunc);
  for (const auto& l : lines) out << l << "\n";
}
}  // namespace host

namespace host {
// The engine's arguments for the launcher's choices (PLAY, and a Multiplayer Lobby match with its own options added).
std::vector<std::string> engine_arguments(const LauncherSettings& settings) {
  const fs::path support = support_dir();
  const std::string volume_text = std::to_string(settings.volume), scale_text = settings.scale > 0 ? std::to_string(settings.scale) : "auto";
  std::vector<std::string> args = {"--iso", settings.iso, "--profile-dir", support.string(), "--volume", volume_text, "--scale", scale_text};
  if (settings.fullscreen) args.push_back("--fullscreen");
  if (!settings.vsync) args.push_back("--no-vsync");
  if (settings.vanilla) args.push_back("--vanilla");
  // Online: the input delay; offline when Slippi Online is off; the Slippi Launcher's sign-in when this app has none of its own.
  args.push_back("--net-delay"); args.push_back(settings.online_delay <= 0 ? "auto" : std::to_string(std::clamp(settings.online_delay, 1, 9)));
  if (!settings.online) args.push_back("--offline");
  if (settings.replay_dir != (support / "Replays").string()) { args.push_back("--replay-dir"); args.push_back(settings.replay_dir); }
  if (settings.account_from_launcher) { const std::string dir = find_slippi_user_dir(); if (!dir.empty()) { args.push_back("--user-dir"); args.push_back(dir); } }
  return args;
}
}  // namespace host

int main(int argc, char** argv) {
  std::string iso_arg;
  bool choose_disc = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--version") { std::printf("%s (melee-unlocked-macOS launcher)\n", MELEE_PORT_VERSION); return 0; }
    else if (a == "--help" || a == "-h") {
      std::printf("melee-unlocked-macOS [--iso <Melee NTSC 1.02 image>] [--choose-disc]\n"
                  "  without --iso, or with --choose-disc, the launcher opens; --iso alone starts the game with that disc\n");
      return 0;
    }
    else if (a == "--iso" && i + 1 < argc) iso_arg = argv[++i];
    else if (a == "--choose-disc") choose_disc = true;
    else if (!a.empty() && a[0] != '-' && iso_arg.empty()) iso_arg = a;   // drag-and-drop / open-with launch
    else if (a.rfind("-psn_", 0) == 0) continue;                           // Finder's process serial number
    else { std::fprintf(stderr, "unknown option %s (see --help)\n", a.c_str()); return 2; }
  }
  if (iso_arg.empty()) { if (const char* env = std::getenv("MELEE_ISO")) iso_arg = env; }

  const fs::path support = support_dir();
  std::error_code ec;
  for (const char* old_name : {"iSlippi", "Shine", "MeleeUnlocked"}) {   // carry settings, saves and replays over from earlier names
    const fs::path previous = fs::path(home_dir()) / "Library/Application Support" / old_name;
    if (fs::is_directory(previous, ec) && !fs::exists(support, ec)) fs::rename(previous, support, ec);
  }
  fs::create_directories(support / "melee-unlocked", ec);
  {   // launcher.log, as upstream's: kept across starts (why a Play started no game), begun again once it passes 256 KB
    const fs::path log_path = support / "melee-unlocked/launcher.log";
    const bool big = fs::exists(log_path, ec) && fs::file_size(log_path, ec) > 256 * 1024;
    g_log = std::fopen(log_path.c_str(), big ? "w" : "a");
    if (g_log && !big) std::fputs("\n", g_log);
  }
  {
    char when[32]; const std::time_t now = std::time(nullptr); std::tm t{}; localtime_r(&now, &t);
    std::strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", &t);
    host::log("melee-unlocked-macOS %s: launcher, %s", MELEE_PORT_VERSION, when);
  }

  // Remembered launcher settings (launcher.ini).
  const fs::path remembered = support / "launcher.ini";
  host::LauncherSettings settings;
  {
    Ini ini;
    ini.read(remembered);
    if (const char* v = ini.get("iso")) settings.iso = v;
    if (const char* v = ini.get("online")) settings.online = *v == '1';
    if (const char* v = ini.get("online_delay")) settings.online_delay = std::strcmp(v, "auto") == 0 ? 0 : std::clamp(std::atoi(v), 1, 9);
    if (const char* v = ini.get("scale")) settings.scale = std::clamp(std::atoi(v), 0, 8);
    if (const char* v = ini.get("vsync")) settings.vsync = *v != '0';
    if (const char* v = ini.get("fullscreen")) settings.fullscreen = *v == '1';
    if (const char* v = ini.get("volume")) settings.volume = std::clamp(std::atoi(v), 0, 100);
    // engine=2 is Vanilla; 0 and the earlier engine's 1 (Static Recomp, removed) are the Source Port.
    if (const char* v = ini.get("engine")) settings.vanilla = std::atoi(v) == 2;
    if (const char* v = ini.get("accent")) settings.accent = uint32_t(std::strtoul(v, nullptr, 10)) & 0xFFFFFFu;
    if (const char* v = ini.get("keyboard")) host::set_keyboard_map(host::KeyboardMap::parse(v));   // the PLAY page's keyboard hint
    // The earlier engine kept widescreen in launcher.ini; the engine's own setting is settings.ini's widescreen line.
    const std::string engine_widescreen = host::engine_setting("widescreen");
    if (engine_widescreen.empty() && ini.get("widescreen") && *ini.get("widescreen") == '1') {
      host::set_engine_setting("widescreen", "1");
      host::log("launcher: widescreen moved from launcher.ini to the engine's settings.ini");
    }
    settings.widescreen = host::engine_setting("widescreen") == "1";
  }
  // The earlier engine's memory card: both are folders of .gci files (Dolphin's layout). Copied, never moved.
  {
    const fs::path old_card = support / "User/GC/CardA", card = support / "melee-unlocked/Card A";
    if (has_gci(old_card) && !has_gci(card)) {
      fs::create_directories(card, ec);
      int copied = 0;
      for (const auto& e : fs::directory_iterator(old_card, ec))
        if (e.path().extension() == ".gci" && fs::copy_file(e.path(), card / e.path().filename(), fs::copy_options::skip_existing, ec)) ++copied;
      host::log("launcher: %d save file%s copied from the earlier engine's memory card to melee-unlocked/Card A", copied, copied == 1 ? "" : "s");
    }
  }
  // Slippi sign-in: this app's own user.json (native sign-in) wins over the Slippi Launcher's file.
  settings.slippi_dir = (support / "Slippi").string();
  {
    slippi::login::Account account;
    if (slippi::login::read_user_file(settings.slippi_dir, account)) {
      settings.account_name = account.display_name; settings.account_code = account.connect_code;
    } else if (const std::string launcher_dir = find_slippi_user_dir(); !launcher_dir.empty() && slippi::login::read_user_file(launcher_dir, account)) {
      settings.account_name = account.display_name; settings.account_code = account.connect_code; settings.account_from_launcher = true;
    }
  }
  settings.replay_dir = (support / "Replays").string();
  {   // melee-unlocked's replay folder choice (launcher.ini replaydir; the Settings window's Replay folder): empty is Replays
    Ini ini; ini.read(remembered);
    if (const char* v = ini.get("replaydir"); v && *v && fs::is_directory(v, ec)) settings.replay_dir = v;
  }

  const std::string engine = host::engine_path();
  if (engine.empty()) {
    host::mac_show_error("The game engine is missing", "This copy of melee-unlocked-macOS has no MeleeUnlockedEngine next to the launcher. "
                                                       "Run the install line from the README again to rebuild it.");
    return 1;
  }
  if (iso_arg.empty() || choose_disc) {
    std::string previous_error;
    if (!settings.iso.empty() && !fs::is_regular_file(settings.iso, ec)) { previous_error = "The last disc image is no longer at " + settings.iso; settings.iso.clear(); }
    if (!iso_arg.empty()) {
      if (fs::is_regular_file(iso_arg, ec)) settings.iso = iso_arg;
      else previous_error = "There is no disc image at " + iso_arg;
    }
    if (!host::mu_launcher_run(settings, previous_error) || settings.iso.empty()) return 0;
    save_launcher_ini(remembered, settings);
  } else {
    settings.iso = iso_arg;
  }

  // The engine replaces this process, with the launcher's choices.
  std::vector<std::string> args = host::engine_arguments(settings);
  args.insert(args.begin(), engine);
  std::vector<char*> argv_engine;
  for (std::string& a : args) argv_engine.push_back(a.data());
  argv_engine.push_back(nullptr);
  host::log("launcher: starting the engine (%s)", engine.c_str());
  if (g_log) { std::fclose(g_log); g_log = nullptr; }
  execv(engine.c_str(), argv_engine.data());
  const int why = errno;
  if ((g_log = std::fopen((support / "melee-unlocked/launcher.log").c_str(), "a"))) host::log("launcher: the engine did not start: %s", std::strerror(why));
  host::mac_show_error("The game could not start", std::string(std::strerror(why)) + "\n\n" + engine);
  return 1;
}
