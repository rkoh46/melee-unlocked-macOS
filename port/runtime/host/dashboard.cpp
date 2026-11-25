// SPDX-License-Identifier: GPL-2.0-or-later
#include "dashboard.h"
#include "slippi_login.h"
#include <cstdio>
#include <cstdlib>

namespace host {
std::vector<GameRow> Dashboard::rows() const {
  std::vector<GameRow> out;
  for (const auto& g : games) {
    GameRow r;
    r.path = g.path;
    std::string vs;
    int mine = -1;
    for (const auto& p : g.players) {
      if (p.type == 3) continue;
      if (!vs.empty()) vs += " vs ";
      vs += slippi::login::character_name(p.character);
      if (!p.name.empty()) vs += " (" + p.name + ")";
      if (!code.empty() && p.code == code) mine = p.port;
    }
    r.title = vs.empty() ? "Match" : vs;
    char b[96];
    const int secs = g.last_frame / 60;
    std::snprintf(b, sizeof b, "%s · %d:%02d · %s", slippi::login::stage_name(g.stage), secs / 60, secs % 60, g.started_at.c_str());
    r.subtitle = b;
    if (g.winner_port > 0) {
      if (mine > 0) { r.win = g.winner_port == mine; r.loss = !r.win; r.result = r.win ? "WIN" : "LOSS"; }
      else { std::snprintf(b, sizeof b, "P%d", g.winner_port); r.result = b; }
    }
    out.push_back(r);
  }
  return out;
}

void dashboard_load_games(const std::string& replay_dir, Dashboard& d, size_t limit) {
  try { d.games = slippi::history::recent_games(replay_dir, limit); } catch (...) { d.games.clear(); }
  const char* sample = std::getenv("MELEE_DASHBOARD_SAMPLE");
  if (!d.games.empty() || !sample || !*sample || *sample == '0') return;
  for (int i = 0; i < 5; ++i) {
    slippi::history::Game g; g.online = true; g.stage = (i % 2) ? 31 : 32; g.last_frame = 60 * (150 + 40 * i); g.winner_port = (i == 1 || i == 3) ? 2 : 1;
    g.started_at = "2026-09-13 20:1" + std::to_string(i);
    slippi::history::Player p1; p1.type = 0; p1.port = 1; p1.character = 2; p1.name = "SAMPLE"; p1.code = "SMPL#123"; p1.stocks = g.winner_port == 1 ? 2 : 0;
    slippi::history::Player p2; p2.type = 0; p2.port = 2; p2.character = (i * 5) % 26; p2.name = "RIVAL"; p2.code = "RIVL#456"; p2.stocks = g.winner_port == 2 ? 1 : 0;
    g.players = {p1, p2}; d.games.push_back(g);
  }
}
}  // namespace host
