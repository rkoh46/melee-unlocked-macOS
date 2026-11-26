// A headless melee-unlocked lobby peer for lobby_loopback_test.sh: melee-unlocked's own port/app/launcher_lobby_p2p.cpp, as fetched,
// on mu-engine/lobby_shim. It stands in for a Melee Unlocked PC in the lobby (its launcher with the Slippi layer: both match
// transports in its hello): it joins with that PC's profile fields (Source Port 1.0.5), lists who it sees with their status and
// stocks, and does what the test asks through a command file, one line each:
//   request NAME           a Slippi Direct match request to the player called NAME
//   p2p NAME               a peer-to-peer match request (melee-unlocked's own match) to NAME
// An accepted peer-to-peer match starts its game as melee-unlocked's launcher does: the lobby closes (the game takes over its UDP
// port), the game runs with p2p_arguments(...) (here the headless runner: PEER_GAME, PEER_ISO, PEER_OUT, generated inputs), and the
// lobby opens again on the same port afterwards. Tests only. From Melee-NX-NetPlay's experiments/native-engine/tools/lobby_peer.cpp
// (same author), with upstream's close-and-reopen instead of a separate game port (the test's sandbox allows the lobby ports only).
//   lobby_peer DIRECTORY BOOTSTRAP PORT SECONDS NAME CODE [COMMAND_FILE]
// SPDX-License-Identifier: GPL-3.0-or-later
#include "launcher_lobby_p2p.h"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

namespace fe::mu_lang { std::string by_key(const std::string&) { return {}; } }   // English only
extern "C" FILE* dht_debug;   // jech/dht's own trace (MU_DHT_DEBUG=1, tests)

namespace {
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
pid_t start_game(const std::vector<std::string>& p2p) {
  const char* game = std::getenv("PEER_GAME"); const char* iso = std::getenv("PEER_ISO"); const char* out = std::getenv("PEER_OUT");
  if (!game || !iso || !out) { std::printf("peer: no game to start (PEER_GAME, PEER_ISO, PEER_OUT)\n"); return -1; }
  std::vector<std::string> args = {game, "--iso", iso, "--out", out, "--frames", std::getenv("PEER_FRAMES") ? std::getenv("PEER_FRAMES") : "20000", "--net-inputs", "3"};
  for (const auto& a : p2p) args.push_back(a);
  std::string shown;
  for (const auto& a : args) shown += " " + a;
  std::printf("peer: starting the game:%s\n", shown.c_str()); std::fflush(stdout);
  const pid_t pid = fork();
  if (pid == 0) {
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    const std::string log = std::string(out) + "/console.log";
    if (FILE* f = std::freopen(log.c_str(), "w", stdout)) { (void)f; dup2(fileno(stdout), 2); }
    execv(game, argv.data());
    _exit(127);
  }
  return pid;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 7) { std::fprintf(stderr, "usage: lobby_peer DIRECTORY BOOTSTRAP PORT SECONDS NAME CODE [COMMAND_FILE]\n"); return 2; }
  const std::string dir = argv[1], bootstrap = argv[2];
  const int port = std::atoi(argv[3]), seconds = std::atoi(argv[4]);
  const std::string commands = argc > 7 ? argv[7] : "";
  if (std::getenv("MU_DHT_DEBUG")) dht_debug = stderr;
  try {
    auto lobby = std::make_unique<launcher::lobby::PeerLobby>(dir, bootstrap, port);
    nlohmann::json profile = {{"name", argv[5]}, {"code", argv[6]}, {"location", "PC stand-in"}, {"build", MU_PEER_BUILD},
                              {"mains", {9, 2, 20}}, {"ready", true}, {"open", {"vanilla"}}};
    lobby->join(profile);
    std::printf("peer: online as %s (%s), build %s, id %s, UDP port %d\n", argv[5], argv[6], MU_PEER_BUILD, lobby->id().substr(0, 8).c_str(), lobby->port());
    std::fflush(stdout);
    std::string last; size_t done = 0; unsigned shown_notice = 0;
    std::set<std::string> logged;
    pid_t game = -1;
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < end) {
      if (game > 0) {   // the game holds the lobby's port: the lobby is closed until it ends (upstream's stop_for_match)
        int status = 0;
        if (waitpid(game, &status, WNOHANG) == game) {
          std::printf("peer: the game ended (%d)\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1); std::fflush(stdout);
          game = 0;
          for (int attempt = 0; attempt < 20 && !lobby; ++attempt) {
            try { lobby = std::make_unique<launcher::lobby::PeerLobby>(dir, bootstrap, port); } catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(200)); }
          }
          if (!lobby) { std::printf("peer: the lobby did not open again\n"); return 1; }
          lobby->join(profile); lobby->command("available");
          std::printf("peer: the lobby is open again on UDP port %d\n", lobby->port()); std::fflush(stdout);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        continue;
      }
      lobby->presence({{"status", "Online"}, {"stocks", nlohmann::json::array()}});
      lobby->tick();
      const auto state = lobby->state();
      std::string now;
      for (const auto& p : state.value("players", nlohmann::json::array()))
        now += "[" + p.value("name", std::string("?")) + " " + p.value("code", std::string()) + " " + p.value("build", std::string()) + " " +
               p.value("status", std::string()) + " stocks " + p.value("stocks", nlohmann::json::array()).dump() + " p2p " +
               (p.value("p2p", false) ? "yes" : "no") + "] ";
      if (now != last) { std::printf("peer: %zu player(s): %s\n", state.value("players", nlohmann::json::array()).size(), now.c_str()); std::fflush(stdout); last = now; }
      if (state.value("notice_seq", 0u) != shown_notice) {
        shown_notice = state.value("notice_seq", 0u);
        std::printf("peer: notice: %s\n", state.value("notice", std::string()).c_str()); std::fflush(stdout);
      }
      for (const auto& r : state.value("requests", nlohmann::json::array())) {
        const std::string key = r.value("id", std::string()) + r.value("state", std::string());
        if (logged.insert(key).second) { std::printf("peer: request %s to %s: %s\n", r.value("id", std::string()).substr(0, 8).c_str(),
                                                     r["peer"].value("name", std::string("?")).c_str(), r.value("state", std::string()).c_str()); std::fflush(stdout); }
      }
      if (!commands.empty()) {   // the test's commands, each line once
        std::ifstream in(commands); std::string line; size_t n = 0;
        while (std::getline(in, line)) {
          if (n++ < done) continue;
          ++done;
          const size_t space = line.find(' ');
          const std::string verb = line.substr(0, space), name = space == std::string::npos ? "" : line.substr(space + 1);
          std::string target;
          for (const auto& p : state.value("players", nlohmann::json::array())) if (p.value("name", std::string()) == name) target = p.value("id", std::string());
          try {
            if (target.empty()) throw std::runtime_error("no player called " + name);
            nlohmann::json data = {{"target", target}, {"mode", "vanilla"}};
            if (verb == "p2p") data["transport"] = launcher::lobby::kP2pTransport;
            lobby->command("request", data);
            std::printf("peer: %s request sent to %s\n", verb == "p2p" ? "peer-to-peer" : "Slippi Direct", name.c_str());
          } catch (const std::exception& e) { std::printf("peer: %s %s: %s\n", verb.c_str(), name.c_str(), e.what()); }
          std::fflush(stdout);
        }
      }
      nlohmann::json launch;
      while (lobby->take_launch(launch)) {
        if (!launch.count("p2p")) { std::printf("peer: a Slippi Direct match was accepted (no game here)\n"); continue; }
        const std::string out = std::getenv("PEER_OUT") ? std::getenv("PEER_OUT") : ".";
        const std::string line = launcher::lobby::p2p_arguments(launch["p2p"], dir + "/lobby-peer-identity.json", out + "/p2p-result.json");
        std::printf("peer: peer-to-peer match accepted against %s: %s\n", launch.value("name", std::string("?")).c_str(), line.empty() ? "SETUP REFUSED" : "starting the game");
        std::fflush(stdout);
        if (!line.empty() && game <= 0) {
          // upstream's stop_for_match: "In game" to the players known, a moment to say it, then the port goes to the game
          lobby->presence({{"status", "In game"}, {"stocks", nlohmann::json::array()}}); lobby->presence_now();
          for (int i = 0; i < 30; ++i) { lobby->tick(); std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
          lobby.reset();
          game = start_game(split_arguments(line));
          break;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (lobby) { lobby->command("leave"); for (int i = 0; i < 5; ++i) { lobby->tick(); std::this_thread::sleep_for(std::chrono::milliseconds(20)); } }
    if (game > 0) { kill(game, SIGTERM); waitpid(game, nullptr, 0); }
  } catch (const std::exception& e) {
    std::printf("peer: %s\n", e.what());
    return 1;
  }
  return 0;
}
