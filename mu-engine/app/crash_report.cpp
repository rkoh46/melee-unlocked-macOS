// melee-unlocked's crash report in the Mac launcher, as Melee-NX-NetPlay's frontend/src/diagnostics.cpp has it on the Switch
// (same author): its port/app/launcher_crash_zip.h, launcher_crash_privacy.h and launcher_crash_text.h, compiled from the
// fetched tree (GPL-2.0-or-later). "Save crash report" writes melee_crash_report.zip and its readable companion
// melee_crash_report.md to melee-unlocked/Logs in the app's data folder, every line scrubbed by upstream's rules first (connect
// codes, IP and e-mail addresses, user and computer names, the Slippi name of user.json, full file locations; a line that still
// shows a location is dropped). Nothing is sent anywhere: upstream posts the zip to its own relay, which this app never contacts.
//
// What goes in, by upstream's names: melee_port_crash.txt (the newest macOS crash report of the engine or the app from the last
// week, ~/Library/Logs/DiagnosticReports), melee_port.log (the engine's engine.log with engine.log.host after it),
// melee_port.prev.log (the session before: engine.prev.log, which the engine keeps at each start), session.trace (the marked
// rows of the last day's session traces in the replay folder).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "launcher_crash_text.h"   // fetched tree, port/app (with launcher_crash_zip.h, launcher_crash_privacy.h)

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace mu_crash {
namespace {
namespace fs = std::filesystem;
std::string read_all(const std::string& path) { std::ifstream in(path, std::ios::binary); std::stringstream ss; ss << in.rdbuf(); return ss.str(); }
std::string user_field(const std::string& text, const char* key) {   // the Slippi name and code: only to remove them
  const std::string k = std::string("\"") + key + "\"";
  const size_t at = text.find(k);
  if (at == std::string::npos) return {};
  const size_t q1 = text.find('"', text.find(':', at + k.size()) + 1);
  const size_t q2 = q1 == std::string::npos ? q1 : text.find('"', q1 + 1);
  return q1 == std::string::npos || q2 == std::string::npos ? std::string() : text.substr(q1 + 1, q2 - q1 - 1);
}
// The newest macOS crash report (.ips) of the engine or the app from the last week.
std::string newest_crash_report() {
  const char* home = std::getenv("HOME");
  if (!home) return {};
  std::error_code ec;
  fs::path best; fs::file_time_type best_time{};
  const auto week = fs::file_time_type::clock::now() - std::chrono::hours(24 * 7);
  for (const auto& e : fs::directory_iterator(fs::path(home) / "Library/Logs/DiagnosticReports", ec)) {
    const std::string n = e.path().filename().string();
    if (n.rfind("MeleeUnlockedEngine", 0) != 0 && n.rfind("melee-unlocked-macOS", 0) != 0 && n.rfind("melee_port_mac", 0) != 0) continue;
    const auto t = e.last_write_time(ec);
    if (!ec && t > week && (best.empty() || t > best_time)) { best = e.path(); best_time = t; }
  }
  return best.string();
}
}  // namespace

bool available() { return true; }
bool save(const std::string& support, const std::string& replay_dir, std::string* where, std::string* error) {
  namespace lc = launcher::crash;
  const std::string mu = support + "/melee-unlocked", logs = mu + "/Logs";
  std::vector<lc::File> files;
  // Personal names to remove besides upstream's own (this Mac's user and computer names): the Slippi account's.
  std::vector<std::string> names = lc::environment_names();
  const std::string user = read_all(support + "/Slippi/user.json");
  for (const char* k : {"displayName", "connectCode", "uid"}) { const std::string v = user_field(user, k); if (!v.empty()) names.push_back(v); }
  const auto add = [&](const char* zip_name, std::vector<uint8_t> data, size_t cap) {
    if (data.empty()) return;
    files.emplace_back(zip_name, lc::private_report_text(zip_name, data, data.size() >= cap, names));
  };
  const std::string ips = newest_crash_report();
  if (!ips.empty()) add("melee_port_crash.txt", lc::read_tail(ips, 256 * 1024), 256 * 1024);
  const auto log_pair = [&](const char* zip_name, const std::string& log_path, size_t cap) {
    std::vector<uint8_t> log = lc::read_tail(log_path, cap);
    const std::vector<uint8_t> host = lc::read_tail(log_path + ".host", 512 * 1024);
    if (!host.empty()) { static const char sep[] = "\n---- host.log ----\n"; log.insert(log.end(), sep, sep + sizeof sep - 1); log.insert(log.end(), host.begin(), host.end()); }
    add(zip_name, log, cap + 512 * 1024);
  };
  log_pair("melee_port.log", mu + "/engine.log", 2u * 1024 * 1024);
  log_pair("melee_port.prev.log", mu + "/engine.prev.log", 1u * 1024 * 1024);
  // The last day's session traces (marked rows only), through upstream's own collector.
  if (!replay_dir.empty())
    for (auto& f : lc::collect_logs("/nonexistent-game-dir", "/nonexistent-launcher-dir", {replay_dir}, false))
      if (f.first == "session.trace") files.emplace_back(f.first, lc::private_report_text(f.first, f.second, false, names));
  if (files.empty()) { *error = "There are no logs to put in a report yet."; return false; }
  std::error_code ec;
  fs::create_directories(logs, ec);
  const std::vector<uint8_t> zip = lc::capped_zip(files);
  const std::string zip_path = logs + "/melee_crash_report.zip", md_path = logs + "/melee_crash_report.md";
  const std::string md = lc::make_markdown(files, MELEE_PORT_VERSION, "Source Port (macOS)");
  if (!lc::save_report(zip_path, reinterpret_cast<const char*>(zip.data()), zip.size()) || !lc::save_report(md_path, md.data(), md.size())) {
    *error = "The report could not be written to " + logs + ".";
    return false;
  }
  *where = zip_path;
  return true;
}
}  // namespace mu_crash
