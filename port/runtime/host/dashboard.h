// The launcher's Replay Viewer model: the newest games in the replay folder as rows to lay out.
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "slippi_history.h"
#include <string>
#include <vector>

namespace host {
struct GameRow { std::string title, subtitle, result, path; bool win = false, loss = false; };   // path: the .slp file, empty for sample rows
struct Dashboard {
  std::string name, code;   // whose games: WIN and LOSS from this player's side
  std::vector<slippi::history::Game> games;
  std::vector<GameRow> rows() const;   // recent games, newest first
};
// The newest `limit` games in replay_dir (blocking; run off the UI thread). MELEE_DASHBOARD_SAMPLE=1 adds sample games when
// there are none (screenshots without a replay folder).
void dashboard_load_games(const std::string& replay_dir, Dashboard& d, size_t limit);
}  // namespace host
