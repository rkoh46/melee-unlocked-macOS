// See frontend.h and mods_service.h.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "frontend.h"
#include "mods_service.h"
#include "mi_fetch_curl.h"
#include "mods_catalog_json.h"
#include "host_services.h"

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>

namespace fe {
Options options;
void say(const char* format, ...) {
  char line[1024]; va_list args; va_start(args, format); std::vsnprintf(line, sizeof line, format, args); va_end(args);
  linux_host::log("%s", line);
}
namespace {
std::mutex g_ini_mutex;
std::string launcher_ini() { return options.out_dir + "/../launcher.ini"; }
std::string settings_file() { return options.out_dir + "/settings.ini"; }
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
std::string app_ini(const std::string& key, const std::string& fallback) {
  std::lock_guard<std::mutex> lock(g_ini_mutex);
  const auto kv = read_kv(launcher_ini(), nullptr);
  const auto it = kv.find(key);
  return it == kv.end() ? fallback : it->second;
}
void app_ini_set(const std::string& key, const std::string& value) { std::lock_guard<std::mutex> lock(g_ini_mutex); write_kv(launcher_ini(), key, value); }
std::string settings_get(const std::string& key) {
  std::lock_guard<std::mutex> lock(g_ini_mutex);
  const auto kv = read_kv(settings_file(), nullptr);
  const auto it = kv.find(key);
  return it == kv.end() ? std::string() : it->second;
}
bool settings_change(const std::string& key, const std::string& value) { std::lock_guard<std::mutex> lock(g_ini_mutex); write_kv(settings_file(), key, value); return true; }
void settings_write_key(const std::string& key, const std::string& value) { settings_change(key, value); }
void settings_set_quiet(const std::string&, const std::string&) {}
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
    return new modinstall::Manager(c);
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
