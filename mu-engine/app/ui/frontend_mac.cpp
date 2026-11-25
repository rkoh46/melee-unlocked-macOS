// The parts of Melee-NX-NetPlay's frontend (fe/frontend.h) that its Switch app files (app.cpp, play_main.cpp,
// mods_service.cpp) answer there, for the Mac engine: the log, launcher.ini, the online session and the mod installer.
// See also fe_mac.h and mods_service.h.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "frontend.h"
#include "fe_mac.h"
#include "mods_service.h"
#include "mi_fetch_curl.h"
#include "mods_catalog_json.h"
#include "host_services.h"
#include "../slippi_menus.h"
#include "aurora/aurora.h"
#include "host.h"
#include "window.h"

#include <algorithm>
#include <chrono>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <sys/stat.h>

namespace fe {
Options options;
void say(const char* format, ...) {
  char line[1024]; va_list args; va_start(args, format); std::vsnprintf(line, sizeof line, format, args); va_end(args);
  linux_host::log("%s", line);
}
namespace {
std::mutex g_ini_mutex;
std::string launcher_ini() { return options.out_dir + "/../launcher.ini"; }
std::map<std::string, std::string> read_kv(const std::string& path, std::vector<std::string>* order) {
  std::map<std::string, std::string> out;
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    if (order) order->push_back(line);
    const size_t eq = line.find('=');
    if (eq != std::string::npos) out[line.substr(0, eq)] = line.substr(eq + 1);
  }
  return out;
}
void write_kv(const std::string& path, const std::string& key, const std::string& value) {
  std::vector<std::string> lines;
  read_kv(path, &lines);
  bool found = false;
  for (auto& l : lines) if (l.compare(0, key.size() + 1, key + "=") == 0) { l = key + "=" + value; found = true; }
  if (!found) lines.push_back(key + "=" + value);
  std::ofstream out(path, std::ios::trunc);
  for (const auto& l : lines) out << l << "\n";
}
}  // namespace
// The settings panel reads launcher.ini many times a frame (its appearance, the rows): the file is read again only when it
// changed on disk (checked at most every half second) or after this process wrote it.
namespace {
std::map<std::string, std::string> g_ini_cache;
bool g_ini_cached = false;
std::chrono::steady_clock::time_point g_ini_checked;
struct timespec g_ini_mtime{};
const std::map<std::string, std::string>& ini_locked() {
  const auto now = std::chrono::steady_clock::now();
  if (g_ini_cached && now - g_ini_checked < std::chrono::milliseconds(500)) return g_ini_cache;
  g_ini_checked = now;
  struct stat st{};
  const bool there = stat(launcher_ini().c_str(), &st) == 0;
  if (g_ini_cached && there && st.st_mtimespec.tv_sec == g_ini_mtime.tv_sec && st.st_mtimespec.tv_nsec == g_ini_mtime.tv_nsec) return g_ini_cache;
  g_ini_mtime = there ? st.st_mtimespec : timespec{};
  g_ini_cache = read_kv(launcher_ini(), nullptr);
  g_ini_cached = true;
  return g_ini_cache;
}
}  // namespace
std::string app_ini(const std::string& key, const std::string& fallback) {
  std::lock_guard<std::mutex> lock(g_ini_mutex);
  const auto& kv = ini_locked();
  const auto it = kv.find(key);
  return it == kv.end() ? fallback : it->second;
}
void app_ini_set(const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(g_ini_mutex);
  write_kv(launcher_ini(), key, value);
  g_ini_cached = false;
}
namespace { std::atomic<bool> g_online{false}; }
bool session_online() { return g_online.load(); }
void set_session_online(bool online) { g_online.store(online); }

modinstall::Manager& mod_manager() {
  static modinstall::Manager* manager = [] {
    modinstall::Config c;
    c.mods_dir = options.out_dir + "/Mods"; c.cache_dir = options.out_dir + "/Mods/.cache"; c.disc_path = options.iso;
    c.catalog_json = kModsCatalogJson;
    c.fetcher = modinstall::curl_fetcher();
    c.log = [](const std::string& line) { say("%s", line.c_str()); };
    auto* m = new modinstall::Manager(c);
    // The catalog the launcher last refreshed from melee-unlocked's (mods_facade.cpp keeps it in Mods/.cache): the engine itself
    // never goes online for it.
    std::ifstream in(c.cache_dir + "/mods-catalog-2.json", std::ios::binary);
    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string error;
    if (!json.empty()) say("mods: %s", m->replace_catalog(json, &error) ? "the catalog the launcher refreshed is used" : ("the refreshed catalog was refused (" + error + ")").c_str());
    return m;
  }();
  return *manager;
}
bool mod_downloads_available() { return true; }
std::string mod_profile_next() { return settings_get("mod_profile"); }
bool mod_profile_set(const std::string& profile) { return settings_change("mod_profile", profile); }
std::vector<std::string> mod_profile_choices() {
  std::vector<std::string> out{""};
  for (const auto& p : mod_manager().profiles()) out.push_back(p);
  const std::string now = mod_profile_next();
  if (!now.empty() && std::find(out.begin(), out.end(), now) == out.end()) out.push_back(now);
  return out;
}
}  // namespace fe

// ---- the rest of play mode's frontend, as the Mac engine has it ----
namespace fe {
uint32_t online_delay_frames() { return options.online_delay > 0 ? uint32_t(options.online_delay) : 0u; }   // the launcher's setting
void decoder_sync() {}
double display_refresh_hz() { return host::window_refresh_rate(); }
// The Switch's present hook (Aurora's render loop presents again between frames); the Mac renderer runs the presentation
// loop itself (gx::metal_set_present_loop), so this is only noted.
void set_present_hook(const PresentHook&) {}   // the GX stream is decoded on the simulation thread itself (mu_play_main.cpp on_fifo)
// Slippi's menu files (mods.cpp asks for them): mu-engine/app/slippi_menus.cpp serves the bundle's slippi_sys.
bool slippi_menus_on() { return slippi_menus::active(); }
void slippi_files_build() { slippi_menus::disc_ready(); }
std::vector<std::string> slippi_files_publish() { return {}; }
int slippi_file_read(uint64_t offset, void* out, size_t bytes) { return slippi_menus::read(offset, out, bytes); }
}  // namespace fe

namespace host {
uint64_t now_ns() { return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }
}

// The Switch frontend's picture options (Aurora, patches/0109): the overlays call these when the picture page changes
// (fe/overlay_draw.cpp apply_picture: the picture first, then the shape and the internal resolution).
namespace { std::mutex g_pic_mutex; fe::MacPicture g_pic; }
void aurora_set_picture(const AuroraPicture* p) {
  if (!p) return;
  std::lock_guard<std::mutex> lock(g_pic_mutex);
  g_pic.brightness = p->brightness; g_pic.contrast = p->contrast; g_pic.vibrance = p->vibrance; g_pic.sharpness = p->sharpness; g_pic.crt = int(p->crt); g_pic.crt_curve = p->crt_curve;
  if (fe::mac_set_picture) fe::mac_set_picture(g_pic);
}
namespace aurora::window {
void set_frame_buffer_aspect(float aspect) {
  std::lock_guard<std::mutex> lock(g_pic_mutex);
  g_pic.aspect = aspect;   // 0: the whole window; the renderer takes 0 as the game's own shape
  if (fe::mac_set_picture) fe::mac_set_picture(g_pic);
}
void set_frame_buffer_scale(float scale) {
  std::lock_guard<std::mutex> lock(g_pic_mutex);
  g_pic.scale = int(scale);
  if (fe::mac_set_picture) fe::mac_set_picture(g_pic);
}
}
