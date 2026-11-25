// The launcher's Replay Viewer card (port/runtime/host/mu_launcher.mm): one replay's Slippi-Launcher-style stats and its session
// trace overview, read by melee-unlocked's own code (replay_stats_facade.cpp). Keep the declaration in mu_launcher.mm in step.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace mu_replay_stats {
struct Card {
  bool ok = false;
  std::string title, date;                                      // "Fox vs Marth on Battlefield", the recorded date and length
  std::vector<std::pair<std::string, std::string>> rows;        // label, each player's value joined with " | "
  bool trace = false;                                           // a session trace (.trace) beside it
  std::vector<std::array<float, 5>> bins;                       // interval ms, work ms, waits, rollbacks, mark flags
  std::string trace_summary;
};
Card read(const std::string& path);   // slow (parses the whole file): call it off the main thread
int stage_of(const std::string& path);   // the replay's stage id (-1: not a replay this reads); quick
}  // namespace mu_replay_stats
