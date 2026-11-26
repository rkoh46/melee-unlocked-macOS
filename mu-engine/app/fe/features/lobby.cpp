// The game's half of melee-unlocked's Multiplayer Lobby (the launcher's Lobby page, port/runtime/host/mu_lobby.cpp, runs the lobby
// itself): upstream's game options for a lobby match, as its launcher passes them (port/app/launcher.cpp, main.cpp):
//   --lobby-direct CODE --lobby-character N   a Slippi Direct match against CODE as the character select id N
//   --lobby-status-file PATH                  the mailbox the launcher reads while the game runs
// Upstream's Static Recomp boots straight into Direct; this engine (as Melee-NX-NetPlay's app does, frontend/src/lobby.cpp, same
// author) goes from the main menu to Training as that fighter and searches Slippi Direct for the code from there (search.cpp's
// Practice menu search); when the searches meet, Slippi's Direct select, locked in, and the match. When that match is over, or no
// connection came in 90 s (upstream's deadline), the run ends and the launcher shows its Lobby page again.
// The mailbox is upstream's (host.cpp publish_lobby_status, publish_lobby_result): PATH holds {"status":"In game"|"In match",
// "stocks":[n]}, written at most once a second and only when it changes; PATH.results gets one line per game played
// ({"result":"win"|"loss"|"incomplete","winner":N,"end_method":M}) from the game's own end-of-game report (0xBD). A
// peer-to-peer match (the runner's --p2p-* mode) writes the status too; its results are mu_net's own result file.
// The simulation thread only stores what changed: every file is written by this module's own thread.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "features_internal.h"
#include "frontend.h"
#include "host_services.h"                 // melee-unlocked: linux_host::request_stop
#include "netplay_bridge.h"
#include "proai/practice_state.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace fe::features {
namespace {
constexpr int kCharacters = 26;
constexpr const char* kCharacterNames[kCharacters] = {"Captain Falcon", "Donkey Kong", "Fox", "Mr. Game & Watch", "Kirby",
  "Bowser", "Link", "Luigi", "Mario", "Marth", "Mewtwo", "Ness", "Peach", "Pikachu", "Ice Climbers", "Jigglypuff", "Samus",
  "Yoshi", "Zelda", "Sheik", "Falco", "Young Link", "Dr. Mario", "Roy", "Pichu", "Ganondorf"};
const char* character_name(int id) { return id >= 0 && id < kCharacters ? kCharacterNames[id] : "?"; }

struct Args { std::string code, status_file; int character = 2; bool p2p = false; } g_args;
bool g_on = false;

// ---- the mailbox writer (its own thread) ----
std::mutex g_mutex;
std::condition_variable g_wake;
std::string g_status_pending, g_status_written;
std::deque<std::string> g_results;
bool g_writer = false;
void writer() {
  for (;;) {
    std::string status; std::deque<std::string> results;
    {
      std::unique_lock<std::mutex> lock(g_mutex);
      g_wake.wait(lock, [] { return g_status_pending != g_status_written || !g_results.empty(); });
      if (g_status_pending != g_status_written) status = g_status_written = g_status_pending;
      results.swap(g_results);
    }
    if (!status.empty()) {
      const std::string temp = g_args.status_file + ".tmp";
      { std::ofstream f(temp, std::ios::trunc); f << status; }
      std::rename(temp.c_str(), g_args.status_file.c_str());
    }
    if (!results.empty()) {
      std::ofstream f(g_args.status_file + ".results", std::ios::app);
      for (const auto& line : results) f << line << '\n';
    }
  }
}
void post_status(const std::string& text) {
  if (g_args.status_file.empty()) return;
  static auto last = std::chrono::steady_clock::time_point{};
  const auto now = std::chrono::steady_clock::now();
  if (now - last < std::chrono::seconds(1)) return;
  last = now;
  { std::lock_guard<std::mutex> lock(g_mutex); if (text == g_status_pending) return; g_status_pending = text; }
  g_wake.notify_one();
}

// ---- the hand-over to the Direct match (simulation thread) ----
enum class Step { Off, Menu, Training, Searching, Connected, Played, Done };
Step step = Step::Off;
uint32_t ticks = 0, tries = 0;
std::chrono::steady_clock::time_point search_since;
int local_player = -1;
bool was_fighting = false, played = false;
std::mutex hud_mutex;
std::string hud_text;
void set_hud(const std::string& text) { std::lock_guard<std::mutex> lock(hud_mutex); hud_text = text; }
void end_run(int code, const char* why) {
  say("lobby: %s: back to the launcher", why);
  set_hud("");
  step = Step::Done;
  linux_host::request_stop(code);
}

void reply(uint8_t command, const uint8_t* r, uint32_t size) {
  if (!g_on || command != 0xB3 || size < 4) return;
  if (r[0] == 4) local_player = r[3] < 4 ? r[3] : -1;    // CONNECTION_SUCCESS: this player's port
  else if ((r[0] == 0 || r[0] == 5) && step != Step::Played) local_player = -1;
}
// The game's end-of-game report (0xBD: melee-unlocked mu_net result_outbox.cpp's layout, the winner at 13, the end method at 14).
// Upstream's rule (publish_lobby_result): a game ended normally (method 2, GAME!) with a known winner is a win or a loss.
void exchange(uint8_t command, const uint8_t* payload, uint32_t size, const uint8_t*, uint32_t, int32_t, bool resim) {
  if (!g_on || command != 0xBD || resim || !payload || size < 15) return;
  const int winner = int8_t(payload[13]), method = payload[14];
  const int local = g_args.p2p ? netplay_bridge::p2p_local_slot() : local_player;
  const char* outcome = method == 2 && winner >= 0 && winner < 4 && local >= 0 ? (winner == local ? "win" : "loss") : "incomplete";
  say("lobby: game over: %s (winner %d, this player %d, end method %d)", outcome, winner, local, method);
  if (!g_args.p2p && !g_args.status_file.empty()) {   // a peer-to-peer match's result is mu_net's own file
    char line[96];
    std::snprintf(line, sizeof line, "{\"result\":\"%s\",\"winner\":%d,\"end_method\":%d}", outcome, winner, method);
    { std::lock_guard<std::mutex> lock(g_mutex); g_results.push_back(line); }
    g_wake.notify_one();
  }
  if (step == Step::Connected || step == Step::Played) step = Step::Played;
}

void retrace_hook(const Retrace& info) {
  if (!g_on) return;
  const int local = g_args.p2p ? netplay_bridge::p2p_local_slot() : local_player;
  const bool in_match = local >= 0 && local < 4 && info.fighting && info.state && (g_args.p2p || info.scene_major == 8);
  std::string stocks;
  if (in_match) { const int n = int(info.state->player[local].stocks); if (n >= 0 && n <= 99) stocks = std::to_string(n); }
  post_status(std::string("{\"status\":\"") + (in_match ? "In match" : "In game") + "\",\"stocks\":[" + stocks + "]}");
  switch (step) {
    case Step::Off: return;
    case Step::Menu:
      // The main menu (or the title screen) up for a second, as --feature-test training does it.
      ticks = info.scene_major == 1 || info.scene_major == 24 ? ticks + 1 : 0;
      if (ticks == 60) {
        if (training_go(g_args.character, 0, -1, 0, false)) { step = Step::Training; ticks = 0; say("lobby: into Training as %s", character_name(g_args.character)); }
        else if (++tries < 5) ticks = 0;
        else end_run(2, "the game could not go to Training; the lobby match was not started");
      }
      break;
    case Step::Training:
      ticks = info.scene_major == 0x1C && info.scene_minor == 2 ? ticks + 1 : 0;
      if (ticks == 120) {
        proai::search_ask(2, g_args.code.c_str(), 0);
        step = Step::Searching; ticks = 0; search_since = std::chrono::steady_clock::now();
        say("lobby: Slippi Direct search for %s from Training", g_args.code.c_str());
        set_hud("Lobby match: searching Slippi Direct for " + g_args.code);
      }
      break;
    case Step::Searching: {
      const netplay_bridge::SearchInfo s = netplay_bridge::search_info();
      ++ticks;
      if (s.state == 4) {
        step = Step::Connected; set_hud(""); was_fighting = false;
        say("lobby: connected to %s: Slippi Direct's character select", s.opponent.empty() ? g_args.code.c_str() : s.opponent.c_str());
        break;
      }
      if (ticks % 60 == 0 && s.state != 1 && s.state != 2 && s.state != 3) {
        const proai::SearchView v = proai::search_view();
        if (!v.searching && ++tries < 5) { proai::search_ask(2, g_args.code.c_str(), 0); say("lobby: the search had not started (%s); asked again", v.note); }
      }
      // Upstream's lobby boot gives the match 90 seconds to start, then the game ends and the launcher shows again.
      if (std::chrono::steady_clock::now() - search_since > std::chrono::seconds(90)) {
        proai::search_stop();
        end_run(2, ("no Direct connection with " + g_args.code + " in 90 s").c_str());
      }
      break;
    }
    case Step::Connected:
    case Step::Played:
      // Upstream's lobby boot is one match: once it has been played and the fighters stop, the run ends. Only Slippi's online
      // match scene (8/2) counts: the Training match the search ran from is behind this point.
      if (info.fighting && info.scene_major == 8 && info.scene_minor == 2) was_fighting = true;
      else if (was_fighting && info.scene_major == 8 && info.scene_minor != 2) end_run(0, "the lobby match is over");
      break;
    default: break;
  }
}

void hud(std::vector<HudLine>& out) {
  std::lock_guard<std::mutex> lock(hud_mutex);
  if (!hud_text.empty()) out.push_back({hud_text, 0xFFE070FFu});
}
}  // namespace

void register_lobby() {
  const auto& rest = options.rest;
  for (size_t i = 0; i + 1 < rest.size(); ++i) {
    if (rest[i] == "--lobby-direct") g_args.code = rest[i + 1];
    else if (rest[i] == "--lobby-character") g_args.character = std::clamp(std::atoi(rest[i + 1].c_str()), 0, kCharacters - 1);
    else if (rest[i] == "--lobby-status-file") g_args.status_file = rest[i + 1];
  }
  for (const auto& a : rest) if (a.rfind("--p2p-", 0) == 0) g_args.p2p = true;
  // upstream's check of the code the launcher passes (launcher_linux_lobby.inl lobby_launches)
  bool valid = !g_args.code.empty() && g_args.code.size() <= 18 && g_args.code.find('#') != std::string::npos;
  for (char ch : g_args.code) if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '#')) valid = false;
  if (!g_args.code.empty() && !valid) { say("lobby: --lobby-direct %s is not a connect code: ignored", g_args.code.c_str()); g_args.code.clear(); }
  g_on = !g_args.code.empty() || (!g_args.status_file.empty() && g_args.p2p);
  if (!g_on) return;
  if (!g_args.status_file.empty() && !g_writer) { g_writer = true; std::thread(writer).detach(); }
  if (!g_args.code.empty()) {
    step = Step::Menu;
    set_hud("Lobby match: going to Training to search for " + g_args.code);
    say("lobby: a lobby match: Training as %s, then Slippi Direct for %s", character_name(g_args.character), g_args.code.c_str());
  } else say("lobby: a peer-to-peer lobby match; status to %s", g_args.status_file.c_str());
  add_retrace(retrace_hook);
  add_reply(reply);
  add_exchange(exchange);
  add_hud(hud);
}
}  // namespace fe::features
