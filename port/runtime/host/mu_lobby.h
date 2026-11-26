// melee-unlocked's Multiplayer Lobby in the Mac launcher: its serverless peer lobby (port/app/launcher_lobby_p2p.cpp, compiled from
// the fetched tree as it is, with its jech/dht and Monocypher, on mu-engine/lobby_shim) on a worker thread of its own, as its Win32
// launcher runs it (launcher_lobby.cpp work(): a tick every 50 ms, commands queued, a snapshot for the window); the accepted match
// handed to the game, the game's mailbox while it runs, and the match history. The launcher's Lobby page (mu_launcher.mm) draws it.
// From Melee-NX-NetPlay's frontend/src/lobby.cpp (same author), with upstream's launcher behaviour around a game started as its own
// process (launcher_lobby.cpp game_running, consume_results, p2p_outcome, stop_for_match, restart_after_match).
// Nothing here blocks the launcher's main thread: the DHT, the sockets, the crypto and the file writes run on the lobby's thread.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <nlohmann/json.hpp>

#include <string>
#include <utility>
#include <vector>

namespace mu_lobby {
// The player's lobby profile (upstream's lobby-profile.json, in the launcher's Lobby folder): what the Profile tab edits.
struct Profile {
  std::string name, location;
  std::vector<int> mains;            // one to three character select ids (0..25)
  bool auto_reject = false;          // upstream's "Auto reject invites"
  bool request_sound = true;         // upstream's request chime, and its volume (percent)
  int request_volume = 65;
  int p2p_character = -1, p2p_color = 0;   // the peer-to-peer match's fighter: -1 the first main
};
constexpr int kCharacters = 26;
const char* character_name(int id);   // the character select ids' names, in upstream's order
int color_count(int character);       // upstream's p2p_color_count

// Called once: the Lobby folder (melee-unlocked/Lobby), the Slippi account (name and connect code, empty without one) and whether
// this Mac can play a match now (a disc chosen). Starts the lobby's thread, which stays idle until go_online(). Again later: the
// account and readiness are updated (and announced when online).
void start(const std::string& directory, const std::string& account_name, const std::string& account_code, bool ready);
bool started();
Profile profile();
void set_profile(const Profile& p);  // saved at once; re-announced when online
void go_online();                    // Go Online: the DHT, then the public lobby
void leave();                        // Leave: off the public roster
// One of upstream's lobby actions (request, accept, cancel, chat, friend, friend_code, friend_accept, friend_decline, unfriend,
// available, pm_request, pm_accept, pm_decline, pm, pm_close), queued for the lobby's thread. Its errors come back as the notice.
void command(const std::string& action, const nlohmann::json& data = nlohmann::json::object());
// What the page shows (copied under a lock): upstream's state() plus "phase" (offline, starting, online, held, error), "error",
// "notice" and "notice_seq" (the latest line for the player), "copy_code" (upstream's Copy code after "Can't reach X"), "pings",
// "ready", "account", "port", "playing".
nlohmann::json snapshot();

// An accepted match, taken once by the launcher: the opponent, this player's fighter, and for a peer-to-peer match the game's
// arguments (upstream's p2p_arguments, split into words) and its result file.
struct Match {
  std::string request, code, name, build;
  int character = 2;
  bool p2p = false;
  std::vector<std::string> args;
  std::string result_file;
};
bool take_match(Match* out);
// The game is about to start for this match (upstream's stop_for_match and game_running(true)): a peer-to-peer match first closes
// the lobby, which lets its UDP port go to the game (returns once it is closed, at most 3 s). The game's mailbox is reset.
void game_starting(const Match& m);
// The game process ended (upstream's game_running(false) and restart_after_match): its results go to the history, the lobby opens
// again on the same port and the accepted request is done ("available").
void game_ended();
bool playing();
std::string status_file();           // <Lobby folder>/lobby-game-status.json: the game's --lobby-status-file
// The local match history (upstream's lobby-history.json, newest first) and the wins and losses in it.
nlohmann::json history();
std::pair<int, int> record(const std::string& code = {});
// Tests: a request chime is written to this WAV file (MELEE_LOBBY_CHIME_FILE) instead of played.
void request_chime(int volume);
void stop();                         // at the launcher's end: leave the public lobby, end the thread
}  // namespace mu_lobby
