// netplay_bridge.h on this repository's Slippi netplay (port/runtime/hle: slippi_online, slippi_net with ENet). Built
// into one relocatable object with everything but netplay_bridge::* made local (mu-engine/build.sh net_object), because
// the app's netplay code and melee-unlocked's host services share names (host::log, host::options and others).
// Adapted from Melee-NX-NetPlay's experiments/native-engine/runner/netplay_bridge.cpp. The online module is a copy of
// port/runtime/hle with mu-engine/patches/net-*.patch (kept searches, search info, same-router fallback, slow first frame,
// Slippi's services: reports, profile refresh, replay uploads). What it does not have (the Switch builds' build message:
// port colours, shield colours, pick previews) answers "none".
// SPDX-License-Identifier: GPL-3.0-or-later
#include "netplay_bridge.h"
#include "slippi_online.h"
#include "slippi_net.h"
#include "host.h"
#include "slippi_report.h"
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

namespace {
uint32_t (*g_retrace_count)() = nullptr; void (*g_log_time)(uint64_t) = nullptr; void (*g_set_speed)(double) = nullptr;
std::string (*g_replay_path)() = nullptr;
std::atomic<double> g_speed{1.0};
std::atomic<int> g_ping_ms{0}; std::atomic<bool> g_ping_seen{false};
}

// ---- what the app's netplay code expects from the rest of the app --------------------------------------------------
namespace host {
Options options;
void log(const char* format, ...) {
  char line[1024];
  va_list args; va_start(args, format); std::vsnprintf(line, sizeof line, format, args); va_end(args);
  const auto t = std::chrono::steady_clock::now();
  std::fprintf(stderr, "[net] %s\n", line);
  if (g_log_time) g_log_time(uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t).count()));
}
// The device's savestates of guest memory: the native engine's state is saved by net_host's store (0xB1/0xB2 never reach
// the device), so these only keep the device's unused savestate objects empty.
static uint8_t g_nothing[64];
uint8_t* ptr(uint32_t, uint32_t) { return g_nothing; }
uint8_t rd8(uint32_t) { return 0; }
uint16_t rd16(uint32_t) { return 0; }
uint32_t rd32(uint32_t) { return 0; }
uint32_t retrace_count() { return g_retrace_count ? g_retrace_count() : 0; }
double now_seconds() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void sim_cost_add(int, double) {}
void set_emulation_speed(double speed) {
  if (g_speed.exchange(speed, std::memory_order_relaxed) != speed && g_set_speed) g_set_speed(speed);
}
void set_online_ping_ms(int ms) { g_ping_ms.store(ms, std::memory_order_relaxed); g_ping_seen.store(true, std::memory_order_relaxed); }
int online_ping_ms() { return g_ping_ms.load(std::memory_order_relaxed); }
void input_mark_match_start() {}
void notify_local(const std::string&, const std::string&) {}
}

// Game reports, rank lookups, the profile refresh and replay uploads are the real reporter (slippi_report.cpp, never with a
// local peer: slippi_online.cpp reporting_enabled). The replay path comes from the engine's replay writer, asked on the
// reporter's thread only when Slippi's reply wants the upload (mu-engine/patches/net-0004). Discord presence: not here.
namespace slippi {
const std::string& last_replay_path() { static std::string path; path = g_replay_path ? g_replay_path() : std::string(); return path; }
namespace discord { void set_menus() {} void set_local_port(int) {} void set_searching(const std::string&) {} void set_opponent(const std::string&, const std::string&, int) {} }
}

namespace netplay_bridge {
bool has_services() { return true; }
const char* ranked_refusal() { return "Ranked is not available in this build. Play Unranked, Direct or Teams."; }

static void write_user(const Setup& setup, const char* uid, const char* code, const char* name) {
  mkdir(setup.user_dir.c_str(), 0777);
  if (FILE* out = std::fopen((setup.user_dir + "/user.json").c_str(), "w")) {
    std::fprintf(out, "{\"uid\":\"%s-%d\",\"playKey\":\"test-only\",\"connectCode\":\"%s#%03d\",\"displayName\":\"%s %d\"}\n", uid,
                 setup.local_index, code, setup.local_index + 1, name, setup.local_index + 1);
    std::fclose(out);
  }
}

bool start(const Setup& setup, std::string* error) {
  g_retrace_count = setup.retrace_count; g_log_time = setup.log_time; g_set_speed = setup.set_speed; g_replay_path = setup.replay_path;
  auto& peer = slippi::Matchmaking::local_peer;
  auto& config = slippi::online::config();
  config.offline = false; config.user_dir = setup.user_dir; config.delay = setup.delay_frames;
  config.reply_370 = true;   // melee-unlocked's engine reads Slippi 3.7.0's ONLINE_INPUTS reply
  host::options.cache_dir = setup.user_dir; host::options.iso = setup.iso_path;
  if (setup.services) {
    // Matchmaking: the app's own client (ENet). Tests use a made-up user and the loopback stand-in
    // (tools/mac/mock_matchmaking, MELEE_MM_LOOPBACK_PORT); a player's own sign-in otherwise.
    if (setup.fake_user) {
      write_user(setup, "mock-test", "MOCK", "Mock");
      // A made-up user never reaches Slippi's services: without the tests' loopback stand-in its requests go to the discard
      // port on 127.0.0.1, where nothing answers.
      if (!std::getenv("MELEE_SERVICES_LOOPBACK_PORT")) setenv("MELEE_SERVICES_LOOPBACK_PORT", "9", 1);
    }
    peer.enabled = false;
  } else {
    // Two instances peer directly: nothing leaves the machine and the made-up identity opens nothing.
    write_user(setup, "local-test", "TEST", "Local");
    peer.enabled = true; peer.local_index = setup.local_index; peer.local_port = setup.local_port;
    peer.remote_ip = setup.remote_ip; peer.remote_port = setup.remote_port;
  }
  slippi::online::init();
  if (!slippi::online::available()) { if (error) *error = "the online module is not available in this build"; return false; }
  return true;
}
void shutdown() { slippi::online::shutdown(); }
bool handle(uint8_t command, const uint8_t* payload, uint32_t size, std::vector<uint8_t>* reply) {
  return slippi::online::handle(command, payload, size, *reply);
}
bool request_match(int mode, const std::string& connect_code, uint8_t character, uint8_t color) {
  if (mode <= 0 || mode > 4) return false;                  // Ranked is refused; see ranked_refusal()
  std::vector<uint8_t> ignored;
  // CMD_SET_MATCH_SELECTIONS: team, character, colour, character chosen, stage (2 bytes), stage option 3 (random from the
  // allowed list), mode, alternate stage flag.
  const uint8_t selections[9] = {0, character, color, 1, 0, 0, 3, uint8_t(mode), 0};
  slippi::online::handle(0xB5, selections, sizeof selections, ignored);
  uint8_t find[19] = {uint8_t(mode)};                       // mode, then the opponent's code (Shift-JIS; codes are ASCII)
  std::memcpy(find + 1, connect_code.data(), std::min<size_t>(18, connect_code.size()));
  ignored.clear();
  slippi::online::handle(0xB4, find, sizeof find, ignored);
  return true;
}
uint64_t rollbacks_seen_by_device() { return slippi::online::rollback_count(); }
void set_online_prefs(const OnlinePrefs&) {}
uint32_t port_colour_bits() { return 0; }
uint32_t shield_color(int) { return 0; }
SearchInfo search_info() {
  const slippi::online::SearchInfo s = slippi::online::search_info();
  SearchInfo out;
  out.state = s.state; out.mode = s.mode; out.locked = s.locked; out.character = s.character; out.color = s.color; out.kept = s.kept;
  out.remote_ready = s.remote_ready; out.elapsed_ms = s.elapsed_ms; out.error = s.error; out.opponent = s.opponent;
  out.code = s.code; out.remote_character = s.remote_character; out.remote_color = s.remote_color;
  return out;
}
void keep_search(bool kept) { slippi::online::keep_search(kept); }
bool search_kept() { return slippi::online::search_kept(); }
void set_chat(int mode, bool) { slippi::online::config().chat = std::clamp(mode, 0, 2); }
int local_slot() { return slippi::online::local_player_slot(); }
void end_kept_search(const char* why) { slippi::online::end_kept_search(why); }
FrameTrace frame_trace() { FrameTrace t; slippi::online::frame_trace(&t.skip, &t.advanced, &t.offset_us); return t; }
double emulation_speed() { return g_speed.load(std::memory_order_relaxed); }
int ping_ms() { return g_ping_seen.load(std::memory_order_relaxed) ? g_ping_ms.load(std::memory_order_relaxed) : -1; }
void set_delay(int frames, bool) { if (frames >= 1 && frames <= 9) slippi::online::config().delay = frames; }
}
