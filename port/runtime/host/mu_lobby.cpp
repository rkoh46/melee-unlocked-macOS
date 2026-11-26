// See mu_lobby.h.
//
// What upstream's launcher does around the lobby, and how it is done here:
//   - lobby-profile.json, lobby-peer-identity.json and lobby-history.json in the lobby's folder (here
//     ~/Library/Application Support/Dashdance/melee-unlocked/Lobby): the same files, the same keys. Its developer keys are read
//     too: "url" (a peer address, or "dht://host:port" for a DHT entry point of one's own instead of the public ones: the lobby
//     test's local node) and "peer_port" (a fixed UDP port).
//   - The profile: the display name (the Slippi account's unless one is typed), the Slippi connect code (the account's, not
//     editable, as upstream), a location, three mains, the history's wins and losses; "build" is melee-unlocked's version and the
//     Source Port ("1.0.5:source", as upstream's launchers announce the Source Port: this app runs it), "open" vanilla, "ready" once
//     a disc is chosen. Its options: auto reject invites, the request sound and its volume, the peer-to-peer fighter.
//   - A Slippi Direct match: the game starts with upstream's --lobby-direct CODE --lobby-character N --lobby-status-file PATH
//     (mu-engine/app/fe/features/lobby.cpp), plays one match and ends; the launcher shows its Lobby page again.
//   - A peer-to-peer match (melee-unlocked's own, mu_net): the lobby closes (the game takes over its UDP port), the game starts with
//     p2p_arguments(...) (the engine's --p2p-* mode: mu-engine/runner/p2p_bridge.cpp) and the lobby opens again on the same port.
//   - The history (upstream's record_match, consume_results, p2p_outcome): the game's PATH.results lines for a Direct match, the
//     game's own result file(s) for a peer-to-peer match.
//   - Presence while the game runs: the game's mailbox (PATH, at most 5 s old), else "In game".
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mu_lobby.h"

#include "launcher_lang.h"
#include "launcher_lobby_p2p.h"

#include <AudioToolbox/AudioToolbox.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <sys/stat.h>

#ifndef MU_LOBBY_BUILD
#define MU_LOBBY_BUILD "1.0.5:source"
#endif

namespace host { void log(const char* fmt, ...); }

namespace mu_lobby {
namespace {
using Json = nlohmann::json;
using launcher::lobby::PeerLobby;

constexpr const char* kCharacterNames[kCharacters] = {"Captain Falcon", "Donkey Kong", "Fox", "Mr. Game & Watch", "Kirby",
  "Bowser", "Link", "Luigi", "Mario", "Marth", "Mewtwo", "Ness", "Peach", "Pikachu", "Ice Climbers", "Jigglypuff", "Samus",
  "Yoshi", "Zelda", "Sheik", "Falco", "Young Link", "Dr. Mario", "Roy", "Pichu", "Ganondorf"};

struct State {
  std::mutex mutex;
  std::condition_variable wake, changed;
  bool started = false, stop = false;
  std::string dir, account_name, account_code;
  bool ready = false;
  Json config = Json::object();        // lobby-profile.json as read and written (upstream's keys)
  Json history = Json::array();        // lobby-history.json, newest first
  std::deque<std::pair<std::string, Json>> commands;
  Json snapshot = Json{{"phase", "offline"}};
  std::deque<Match> matches;
  bool hold_wanted = false, held = false;
  std::thread thread;
  bool playing = false;                // a lobby match's game runs
  Match current;
  std::filesystem::file_time_type game_started{};
} g;

std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::stringstream ss; ss << in.rdbuf();
  return ss.str();
}
void save_config_locked() {
  ::mkdir(g.dir.c_str(), 0777);
  std::ofstream out(g.dir + "/lobby-profile.json", std::ios::binary | std::ios::trunc);
  if (out) out << g.config.dump(2);
}
void save_history_locked() {
  const std::string path = g.dir + "/lobby-history.json", temp = path + ".tmp";
  { std::ofstream out(temp, std::ios::binary | std::ios::trunc); if (!out) return; out << g.history.dump(2); }
  std::rename(temp.c_str(), path.c_str());
}
std::pair<int, int> record_locked(const std::string& code = {}) {
  int wins = 0, losses = 0;
  for (const auto& game : g.history) {
    if (!code.empty() && game.value("code", std::string()) != code) continue;
    const std::string r = game.value("result", std::string());
    wins += r == "win"; losses += r == "loss";
  }
  return {wins, losses};
}
// The profile the lobby announces (upstream's profile_config, the build with the Slippi layer).
Json profile_json_locked() {
  Json p = Json::object();
  std::string name = g.config.value("name", std::string());
  if (name.empty()) name = g.account_name;
  p["name"] = name.substr(0, 32);
  p["code"] = g.account_code;
  p["location"] = g.config.value("location", std::string());
  p["build"] = MU_LOBBY_BUILD;
  p["mains"] = g.config.value("mains", Json::array({2, 20, 9}));
  p["ready"] = g.ready;
  p["open"] = Json::array({"vanilla"});
  p["hosted_ok"] = !g.account_code.empty();   // a Slippi account: the Direct match can be started
  p["p2p_ch"] = g.config.value("p2p_ch", -1);  // never sent with the profile: the lobby keeps it for a match setup
  p["p2p_col"] = g.config.value("p2p_col", 0);
  const auto record = record_locked();
  p["wins"] = record.first; p["losses"] = record.second;
  return p;
}
void publish(Json snap) { std::lock_guard<std::mutex> lock(g.mutex); g.snapshot = std::move(snap); }
void set_phase(const std::string& phase, const std::string& error = {}) {
  std::lock_guard<std::mutex> lock(g.mutex);
  g.snapshot["phase"] = phase;
  if (!error.empty()) g.snapshot["error"] = error; else g.snapshot.erase("error");
}
// The game's arguments line of p2p_arguments split into words (double quotes group a path with spaces).
std::vector<std::string> split_arguments(const std::string& line) {
  std::vector<std::string> out; std::string word; bool quoted = false, any = false;
  for (char c : line) {
    if (c == '"') { quoted = !quoted; any = true; continue; }
    if (c == ' ' && !quoted) { if (any) out.push_back(word); word.clear(); any = false; continue; }
    word += c; any = true;
  }
  if (any) out.push_back(word);
  return out;
}
// The game's presence mailbox (upstream: written at most once a second; read when not older than 5 s).
Json game_presence() {
  Json presence = {{"status", "In game"}, {"stocks", Json::array()}};
  std::filesystem::file_time_type started;
  { std::lock_guard<std::mutex> lock(g.mutex); started = g.game_started; }
  std::error_code ec;
  const auto path = std::filesystem::path(g.dir + "/lobby-game-status.json");
  const auto modified = std::filesystem::last_write_time(path, ec);
  if (!ec && modified >= started && decltype(modified)::clock::now() - modified < std::chrono::seconds(5)) {
    const Json live = Json::parse(read_file(path.string()), nullptr, false);
    if (live.is_object()) presence = live;
  }
  return presence;
}

// The lobby's thread: upstream's work() loop, peer mode only.
void work() {
  std::unique_ptr<PeerLobby> peer;
  bool online_wanted = false;
  int held_port = 0;
  std::string notice, copy_code; unsigned notice_seq = 0, shown_seq = 0;
  std::set<std::string> launched, requests_logged, chat_logged, rooms_logged, rejected, chimed;
  const auto add_notice = [&](const std::string& text) { if (!text.empty()) { notice = text; ++notice_seq; } };
  for (;;) {
    std::deque<std::pair<std::string, Json>> batch;
    Json profile; std::string dir, url; int port = 0; bool hold = false, auto_reject = false, sound = true, playing = false; int volume = 65;
    {
      std::unique_lock<std::mutex> lock(g.mutex);
      g.wake.wait_for(lock, std::chrono::milliseconds(peer ? 50 : 500), [] { return g.stop || !g.commands.empty() || g.hold_wanted != g.held; });
      if (g.stop) break;
      profile = profile_json_locked();
      dir = g.dir; url = g.config.value("url", std::string()); port = g.config.value("peer_port", 0);
      auto_reject = g.config.value("auto_reject_invites", false);
      sound = g.config.value("request_sound", true); volume = std::clamp(g.config.value("request_volume", 65), 0, 100);
      hold = g.hold_wanted; playing = g.playing;
      if (!(hold && g.held)) batch.swap(g.commands);
    }
    // Upstream's stop_for_match / restart_after_match: the game takes over this UDP port for a peer-to-peer match.
    if (hold && !g.held) {
      held_port = 0;
      if (peer) {
        held_port = peer->port();
        try {
          peer->presence({{"status", "In game"}, {"stocks", Json::array()}}); peer->presence_now();
          for (const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(600); std::chrono::steady_clock::now() < until;
               std::this_thread::sleep_for(std::chrono::milliseconds(20))) peer->tick();
        } catch (...) {}
        peer.reset();
        host::log("lobby: closed for a match: the game takes UDP port %d", held_port);
      }
      { std::lock_guard<std::mutex> lock(g.mutex); g.held = true; g.snapshot["phase"] = "held"; }
      g.changed.notify_all();
      continue;
    }
    if (hold) continue;
    if (g.held) {
      if (held_port) {
        std::string problem;
        for (int attempt = 0; attempt < 10 && !peer; ++attempt) {
          try { peer = std::make_unique<PeerLobby>(dir, url, held_port); } catch (const std::exception& e) { problem = e.what(); std::this_thread::sleep_for(std::chrono::milliseconds(200)); }
        }
        if (!peer) { try { peer = std::make_unique<PeerLobby>(dir, url, port); } catch (const std::exception& e) { problem = e.what(); } }
        try { if (peer) { if (online_wanted) peer->join(profile); else peer->update_profile(profile); } } catch (const std::exception& e) { problem = e.what(); }
        host::log("lobby: open again on UDP port %d%s%s", peer ? peer->port() : 0, problem.empty() ? "" : ": ", problem.c_str());
      }
      { std::lock_guard<std::mutex> lock(g.mutex); g.held = false; }
      g.changed.notify_all();
    }
    for (auto& c : batch) {
      try {
        if (c.first == "__online") {
          if (profile.value("code", std::string()).empty()) {
            set_phase("error", "Online play needs a Slippi account: sign in on the Play page.");
            host::log("lobby: Go Online refused: no Slippi account on this Mac");
            continue;
          }
          if (!peer) {
            set_phase("starting");
            host::log("lobby: going online (%s)", url.rfind("dht://", 0) == 0 ? "the DHT entry point in lobby-profile.json" : url.empty() ? "the public DHT" : "a peer address in lobby-profile.json");
            peer = std::make_unique<PeerLobby>(dir, url, port);
          }
          peer->join(profile);
          online_wanted = true;
          add_notice("You're online. Select a player to send a match request.");
          host::log("lobby: online on UDP port %d, id %s", peer->port(), peer->id().substr(0, 8).c_str());
        } else if (c.first == "__leave") {
          online_wanted = false;
          if (peer) peer->command("leave");
          host::log("lobby: left the public lobby");
        } else if (c.first == "__profile") {
          if (peer) { if (online_wanted) peer->join(profile); else peer->update_profile(profile); }
        } else if (c.first == "__notice") {
          add_notice(c.second.value("text", std::string()));
        } else if (peer) {
          peer->command(c.first, c.second);
          host::log("lobby: %s", c.first.c_str());
        }
      } catch (const std::exception& e) {
        add_notice(e.what());
        host::log("lobby: %s: %s", c.first.c_str(), e.what());
        if (c.first == "__online") {
          set_phase("error", e.what());
          if (!peer || !peer->visible()) online_wanted = false;
        }
      }
    }
    if (!peer) continue;
    try {
      peer->presence(playing ? game_presence() : Json{{"status", "Online"}, {"stocks", Json::array()}});
      peer->tick();
      Json state = peer->state();
      const std::string self = peer->id();
      // Each of the lobby's own notices once (a refusal, an unanswered request, a delivery, a clock problem). "Can't reach X. Use
      // Slippi Direct with their code": the code is kept for the Copy code button while that line is the notice (upstream).
      const unsigned seq = state.value("notice_seq", 0u);
      if (seq != shown_seq) {
        shown_seq = seq;
        const std::string text = state.value("notice", std::string());
        add_notice(text);
        if (!text.empty()) host::log("lobby: notice: %s", text.c_str());
        const Json notices = state.value("notices", Json::array());
        const Json last = notices.is_array() && !notices.empty() ? notices.back() : Json::object();
        const Json args = last.is_object() ? last.value("args", Json::object()) : Json::object();
        const std::string code = args.is_object() && args.count("code") && args["code"].is_string() ? launcher::lobby::normalize_code(args["code"].get<std::string>()) : std::string();
        copy_code = last.is_object() && last.value("kind", std::string()) == "unreachable" && launcher::lobby::valid_code(code) ? code : std::string();
      }
      // Requests to this player: auto reject (upstream's process_invites cancels each at once) or the request chime.
      for (const auto& r : state.value("requests", Json::array())) {
        const std::string id = r.value("id", std::string());
        if (r.value("state", std::string()) != "pending" || r.value("to", std::string()) != self) continue;
        if (auto_reject) {
          if (!rejected.insert(id).second) continue;
          try { peer->command("cancel", {{"request", id}}); } catch (...) {}
          add_notice("Match request automatically declined.");
          host::log("lobby: match request from %s automatically declined (auto reject invites)", r["peer"].value("name", std::string("?")).c_str());
        } else if (chimed.insert(id).second && sound) request_chime(volume);
      }
      Json pings = Json::object();
      for (const auto& p : peer->pings()) pings[p.first] = p.second;
      Json match;
      while (peer->take_launch(match)) {
        const std::string id = match.value("request", std::string());
        if (id.empty() || !launched.insert(id).second) continue;
        Match m;
        m.request = id; m.code = match.value("code", std::string()); m.name = match.value("name", m.code);
        m.build = match.value("build", std::string());
        m.character = std::clamp(match.value("character", 2), 0, kCharacters - 1);
        if (match.count("p2p")) {
          // Upstream's checks before anything reaches a path or the game's arguments: the request id names the result file, half
          // of the setup came from the other player (p2p_arguments checks every field again).
          const bool plain_id = id.size() == 32 && std::all_of(id.begin(), id.end(), [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'); });
          ::mkdir((dir + "/p2p-results").c_str(), 0777);
          m.result_file = dir + "/p2p-results/" + id + ".json";
          const std::string line = plain_id ? launcher::lobby::p2p_arguments(match["p2p"], dir + "/lobby-peer-identity.json", m.result_file) : std::string();
          if (line.empty()) {
            add_notice(launcher::lang::tr("lobby.p2p.setup_failed"));
            peer->command("available");
            host::log("lobby: an accepted peer-to-peer match could not start: its setup does not hold");
            continue;
          }
          m.p2p = true; m.args = split_arguments(line);
          host::log("lobby: peer-to-peer match accepted against %s: slot %d, stage %d, the game takes UDP port %d", m.name.c_str(),
                    match["p2p"].value("slot", -1), match["p2p"].value("stage", -1), match["p2p"].value("port", 0));
          add_notice(launcher::lang::tr("lobby.p2p.starting", {{"name", m.name}}));
        } else {
          // The code goes to the game as the opponent (upstream's checks in launcher_linux_lobby.inl lobby_launches).
          bool valid = !m.code.empty() && m.code.size() <= 18 && m.code.find('#') != std::string::npos;
          for (char ch : m.code) if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '#')) valid = false;
          if (!valid) {
            add_notice("The match could not start: the opponent's code is not valid.");
            peer->command("available");
            host::log("lobby: an accepted match could not start: the code is not valid");
            continue;
          }
          host::log("lobby: match accepted against %s (%s): Slippi Direct as %s", m.name.c_str(), m.code.c_str(), character_name(m.character));
          add_notice("Match accepted. Starting Slippi Direct against " + m.name + "...");
        }
        std::lock_guard<std::mutex> lock(g.mutex);
        g.matches.push_back(m);
      }
      const int before = [&] { std::lock_guard<std::mutex> lock(g.mutex); return int(g.snapshot.value("players", Json::array()).size()); }();
      for (const auto& m : state.value("messages", Json::array()))   // chat lines from other players, logged by sender only
        if (chat_logged.insert(m.value("id", std::string())).second && m.value("sender", std::string()) != self)
          host::log("lobby: chat message from %s", m.value("name", std::string("?")).c_str());
      for (const auto& room : state.value("private", Json::array())) {   // private rooms: their state and messages, never the text
        const std::string key = room.value("id", std::string()) + ":" + room.value("state", std::string());
        if (rooms_logged.insert(key).second) host::log("lobby: private chat with %s: %s", room.value("name", std::string("?")).c_str(), room.value("state", std::string()).c_str());
        for (const auto& m : room.value("messages", Json::array()))
          if (chat_logged.insert(m.value("id", std::string())).second && m.value("sender", std::string()) != self)
            host::log("lobby: private message from %s", m.value("name", std::string("?")).c_str());
      }
      state["phase"] = state["self"].value("visible", false) ? "online" : "offline";
      state["pings"] = pings;
      state["notice"] = notice; state["notice_seq"] = notice_seq;
      state["copy_code"] = copy_code;
      state["ready"] = profile.value("ready", false);
      state["account"] = profile.value("code", std::string());
      state["port"] = peer->port();
      state["playing"] = playing;
      const int now_players = int(state.value("players", Json::array()).size());
      if (now_players != before) host::log("lobby: %d player%s in the lobby%s", now_players, now_players == 1 ? "" : "s", now_players ? "" : " (looking)");
      for (const auto& r : state.value("requests", Json::array())) {
        const std::string key = r.value("id", std::string()) + ":" + r.value("state", std::string());
        if (requests_logged.insert(key).second)
          host::log("lobby: request %s %s %s (%s%s)", r.value("id", std::string()).substr(0, 8).c_str(), r.value("to", std::string()) == self ? "from" : "to",
                    r["peer"].value("name", std::string("?")).c_str(), r.value("state", std::string()).c_str(),
                    r.value("transport", std::string()) == launcher::lobby::kP2pTransport ? ", peer to peer" : "");
      }
      publish(std::move(state));
    } catch (const std::exception& e) {
      host::log("lobby: tick: %s", e.what());
    }
  }
  if (peer) { try { peer->command("leave"); for (int i = 0; i < 10; ++i) { peer->tick(); std::this_thread::sleep_for(std::chrono::milliseconds(20)); } } catch (...) {} }
}

// What the game's own result file says about a peer-to-peer match (upstream's p2p_outcome: mu_net's result_outbox).
std::string p2p_outcome(const std::string& result_file) {
  if (result_file.empty()) return "incomplete";
  const Json file = Json::parse(read_file(result_file), nullptr, false);
  if (!file.is_object() || !file.count("body") || !file["body"].is_object()) return "incomplete";
  const Json& body = file["body"];
  auto flag = [&](const char* key) { return body.count(key) && body[key].is_boolean() && body[key].get<bool>(); };
  auto number = [](const Json& from, const char* key) { return from.count(key) && from[key].is_number_integer() ? from[key].get<int>() : -1; };
  if (!flag("has_outcome") || flag("desync") || flag("disconnected")) return "incomplete";
  const int slot = number(body, "local_slot"), winner = number(body, "winner");
  if (slot < 0 || slot > 1) return "incomplete";
  if (winner == 0 || winner == 1) return winner == slot ? "win" : "loss";
  if (body.count("players") && body["players"].is_array() && body["players"].size() >= 2 && body["players"][0].is_object() && body["players"][1].is_object()) {
    const int mine = number(body["players"][slot], "stocks"), theirs = number(body["players"][1 - slot], "stocks");
    if (mine >= 0 && theirs >= 0 && mine != theirs) return mine > theirs ? "win" : "loss";
  }
  return "incomplete";
}
void record_match_locked(const std::string& result, int game) {
  char when[32] = {};
  const std::time_t now = std::time(nullptr); std::tm t{}; localtime_r(&now, &t);
  std::strftime(when, sizeof when, "%Y-%m-%d %H:%M", &t);
  g.history.insert(g.history.begin(), Json{{"id", g.current.request + "/" + std::to_string(game)}, {"opponent", g.current.name},
                                           {"code", g.current.code}, {"build", g.current.build}, {"result", result}, {"when", when}, {"game", game},
                                           {"transport", g.current.p2p ? launcher::lobby::kP2pTransport : launcher::lobby::kHostedTransport}});
  if (g.history.size() > 500) g.history.erase(g.history.begin() + 500, g.history.end());
  host::log("lobby: history: game %d against %s: %s", game, g.current.name.c_str(), result.c_str());
}
}  // namespace

const char* character_name(int id) { return id >= 0 && id < kCharacters ? kCharacterNames[id] : "?"; }
int color_count(int character) { return launcher::lobby::p2p_color_count(character); }

void start(const std::string& directory, const std::string& account_name, const std::string& account_code, bool ready) {
  std::lock_guard<std::mutex> lock(g.mutex);
  const bool changed = g.account_name != account_name || g.account_code != account_code || g.ready != ready;
  g.account_name = account_name; g.account_code = account_code; g.ready = ready;
  if (g.started) {
    if (changed) { g.commands.emplace_back("__profile", Json::object()); g.wake.notify_all(); }
    return;
  }
  g.started = true; g.dir = directory;
  std::error_code ec; std::filesystem::create_directories(g.dir, ec);
  const Json cfg = Json::parse(read_file(g.dir + "/lobby-profile.json"), nullptr, false);
  g.config = cfg.is_object() ? cfg : Json::object();
  g.config["mode"] = "peer";
  g.config.erase("hosted_ok");   // upstream: whether an account was signed in when the profile was saved says nothing about now
  if (!g.config.count("mains") || !g.config["mains"].is_array() || g.config["mains"].empty()) g.config["mains"] = Json::array({2, 20, 9});
  const Json history = Json::parse(read_file(g.dir + "/lobby-history.json"), nullptr, false);
  if (history.is_array()) g.history = history;
  // The identity before the thread starts (upstream calls identity_key first): a damaged file is reported by the lobby itself.
  const std::string key = launcher::lobby::identity_key(g.dir);
  host::log("lobby: folder %s, identity %s, %zu games in the history", g.dir.c_str(), key.empty() ? "not readable" : (key.substr(0, 8) + "...").c_str(), g.history.size());
  g.thread = std::thread(work);
}
bool started() { std::lock_guard<std::mutex> lock(g.mutex); return g.started; }
Profile profile() {
  std::lock_guard<std::mutex> lock(g.mutex);
  Profile p;
  p.name = g.config.value("name", std::string());
  p.location = g.config.value("location", std::string());
  for (const auto& m : g.config.value("mains", Json::array())) if (m.is_number_integer()) p.mains.push_back(m.get<int>());
  p.auto_reject = g.config.value("auto_reject_invites", false);
  p.request_sound = g.config.value("request_sound", true);
  p.request_volume = std::clamp(g.config.value("request_volume", 65), 0, 100);
  p.p2p_character = std::clamp(g.config.value("p2p_ch", -1), -1, kCharacters - 1);
  p.p2p_color = std::max(0, g.config.value("p2p_col", 0));
  return p;
}
void set_profile(const Profile& p) {
  std::lock_guard<std::mutex> lock(g.mutex);
  g.config["name"] = p.name.substr(0, 32); g.config["location"] = p.location.substr(0, 48);
  Json mains = Json::array();
  for (int m : p.mains) if (m >= 0 && m < kCharacters && std::find(mains.begin(), mains.end(), m) == mains.end() && mains.size() < 3) mains.push_back(m);
  if (mains.empty()) mains.push_back(2);
  g.config["mains"] = mains;
  g.config["auto_reject_invites"] = p.auto_reject;
  g.config["request_sound"] = p.request_sound;
  g.config["request_volume"] = std::clamp(p.request_volume, 0, 100);
  const int ch = std::clamp(p.p2p_character, -1, kCharacters - 1);
  const int first = mains[0].get<int>();
  g.config["p2p_ch"] = ch;
  g.config["p2p_col"] = p.p2p_color >= 0 && p.p2p_color < launcher::lobby::p2p_color_count(ch >= 0 ? ch : first) ? p.p2p_color : 0;
  save_config_locked();
  g.commands.emplace_back("__profile", Json::object());
  g.wake.notify_all();
}
void go_online() { std::lock_guard<std::mutex> lock(g.mutex); g.commands.emplace_back("__online", Json::object()); g.wake.notify_all(); }
void leave() { std::lock_guard<std::mutex> lock(g.mutex); g.commands.emplace_back("__leave", Json::object()); g.wake.notify_all(); }
void command(const std::string& action, const Json& data) { std::lock_guard<std::mutex> lock(g.mutex); g.commands.emplace_back(action, data); g.wake.notify_all(); }
Json snapshot() { std::lock_guard<std::mutex> lock(g.mutex); return g.snapshot; }
Json history() { std::lock_guard<std::mutex> lock(g.mutex); return g.history; }
std::pair<int, int> record(const std::string& code) { std::lock_guard<std::mutex> lock(g.mutex); return record_locked(code); }
bool take_match(Match* out) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (g.matches.empty()) return false;
  *out = g.matches.front(); g.matches.pop_front();
  return true;
}
std::string status_file() { std::lock_guard<std::mutex> lock(g.mutex); return g.dir + "/lobby-game-status.json"; }
bool playing() { std::lock_guard<std::mutex> lock(g.mutex); return g.playing; }
void game_starting(const Match& m) {
  if (m.p2p) {
    // Upstream's stop_for_match: the lobby answers a little longer, says "In game", and lets the socket go before the game binds it.
    std::unique_lock<std::mutex> lock(g.mutex);
    g.hold_wanted = true;
    g.wake.notify_all();
    g.changed.wait_for(lock, std::chrono::seconds(3), [] { return g.held; });
  }
  std::lock_guard<std::mutex> lock(g.mutex);
  g.current = m; g.playing = true;
  std::error_code ec;
  std::filesystem::remove(g.dir + "/lobby-game-status.json.results", ec);
  g.game_started = std::filesystem::file_time_type::clock::now();
  host::log("lobby: the game starts the lobby match against %s: %s", m.name.c_str(), m.p2p ? "melee-unlocked's peer-to-peer match" :
            ("Slippi Direct for " + m.code + " as " + character_name(m.character)).c_str());
}
void game_ended() {
  std::unique_lock<std::mutex> lock(g.mutex);
  if (!g.playing) return;
  int games = 0;
  // The game's results mailbox: one line per game (a Slippi Direct match).
  {
    std::istringstream in(read_file(g.dir + "/lobby-game-status.json.results"));
    std::string line;
    while (std::getline(in, line)) {
      const Json report = Json::parse(line, nullptr, false);
      if (!report.is_object()) continue;
      const std::string result = report.value("result", std::string());
      if (result == "win" || result == "loss" || result == "incomplete") record_match_locked(result, ++games);
    }
  }
  // A peer-to-peer match has no result mailbox: the game's own result file says who won; further games of the same session are
  // "<id>.g2.json", "<id>.g3.json"... beside the first (source_p2p.cpp names them), until one is missing.
  if (games == 0 && g.current.p2p) {
    const std::string first = g.current.result_file;
    record_match_locked(p2p_outcome(first), ++games);
    if (first.size() > 5) for (int game = 2; game <= 127; ++game) {
      const std::string next = first.substr(0, first.size() - 5) + ".g" + std::to_string(game) + ".json";
      std::error_code ec;
      if (!std::filesystem::exists(next, ec)) break;
      record_match_locked(p2p_outcome(next), ++games);
    }
  }
  if (games) save_history_locked();
  g.playing = false;
  g.hold_wanted = false;   // upstream's restart_after_match: the lobby opens again on the same port
  g.commands.emplace_back("__profile", Json::object());   // the record announced (upstream: own_profile with wins and losses)
  g.commands.emplace_back("available", Json::object());   // upstream's launcher after its game: the accepted request is done
  g.commands.emplace_back("__notice", Json{{"text", games ? "Match ended. Results saved in local history." : "The match did not start."}});
  g.wake.notify_all();
  g.changed.wait_for(lock, std::chrono::seconds(4), [] { return !g.held; });
}
void stop() {
  {
    std::lock_guard<std::mutex> lock(g.mutex);
    if (!g.started || g.stop) return;
    g.stop = true;
  }
  g.wake.notify_all();
  if (g.thread.joinable()) g.thread.join();
}

// Upstream's request_chime: 660 Hz then 880 Hz, 0.3 s at 22050 Hz, at the player's volume, on its own thread (AudioToolbox here).
void request_chime(int volume) {
  if (volume <= 0) return;
  std::thread([volume] {
    constexpr int sample_rate = 22050, frames = sample_rate * 3 / 10;
    std::array<int16_t, frames> samples{};
    for (int i = 0; i < frames; ++i) {
      const double time = double(i) / sample_rate;
      const double frequency = i < frames / 2 ? 660.0 : 880.0;
      const double envelope = std::min(1.0, time * 35.0) * std::min(1.0, (frames - i) / 1800.0);
      samples[i] = int16_t(std::sin(time * frequency * 6.283185307179586) * envelope * 7500 * volume / 100);
    }
    // Tests (the player's headset stays quiet): the chime goes to a WAV file instead.
    if (const char* file = std::getenv("MELEE_LOBBY_CHIME_FILE")) {
      std::ofstream out(file, std::ios::binary | std::ios::trunc);
      auto u32 = [&](uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };
      auto u16 = [&](uint16_t v) { out.write(reinterpret_cast<const char*>(&v), 2); };
      out.write("RIFF", 4); u32(36 + sizeof samples); out.write("WAVEfmt ", 8); u32(16); u16(1); u16(1); u32(sample_rate); u32(sample_rate * 2); u16(2); u16(16);
      out.write("data", 4); u32(sizeof samples); out.write(reinterpret_cast<const char*>(samples.data()), sizeof samples);
      host::log("lobby: request sound written to %s (volume %d%%)", file, volume);
      return;
    }
    AudioStreamBasicDescription format{};
    format.mSampleRate = sample_rate; format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mChannelsPerFrame = 1; format.mBitsPerChannel = 16; format.mBytesPerFrame = 2; format.mFramesPerPacket = 1; format.mBytesPerPacket = 2;
    AudioQueueRef queue = nullptr;
    if (AudioQueueNewOutput(&format, [](void*, AudioQueueRef, AudioQueueBufferRef) {}, nullptr, nullptr, nullptr, 0, &queue) != noErr) return;
    AudioQueueBufferRef buffer = nullptr;
    if (AudioQueueAllocateBuffer(queue, sizeof samples, &buffer) == noErr) {
      std::memcpy(buffer->mAudioData, samples.data(), sizeof samples);
      buffer->mAudioDataByteSize = sizeof samples;
      if (AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr) == noErr && AudioQueueStart(queue, nullptr) == noErr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(420));
        AudioQueueStop(queue, true);
      }
    }
    AudioQueueDispose(queue, true);
    host::log("lobby: request sound played (volume %d%%)", volume);
  }).detach();
}
}  // namespace mu_lobby
