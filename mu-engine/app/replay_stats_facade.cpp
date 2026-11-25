// See replay_stats_facade.h. melee-unlocked's replay data and stats (port/app/launcher_replay_data.cpp with its
// launcher_replay_stats.inl), its session trace reader and the launcher's whole-session overview (launcher_trace_view.cpp),
// compiled from the fetched tree as the engine's Replay library (fe/features/replay_library.cpp) compiles them.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "replay_stats_facade.h"

#include <cstdio>
#include <ctime>
#include <filesystem>

namespace {
time_t days_from_civil_utc(const std::tm& t) {
  const int y = t.tm_year + 1900 - (t.tm_mon < 2), m = t.tm_mon + 1, d = t.tm_mday;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = unsigned(y - era * 400);
  const unsigned doy = unsigned((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  const long long days = (long long)era * 146097 + (long long)doe - 719468;
  return time_t(days * 86400 + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec);
}
}  // namespace
static inline time_t _mkgmtime(std::tm* t) { return days_from_civil_utc(*t); }
static inline int localtime_s(std::tm* out, const time_t* t) { return localtime_r(t, out) ? 0 : 1; }
#include "launcher_replay_data.cpp"
#include "net_trace_read.cpp"
#include "launcher_trace_view.cpp"
#include "replay_slpz.cpp"          // .slpz (zstd) reading, as the engine's replays.cpp compiles it

namespace mu_replay_stats {
namespace lr = launcher::replay;
int stage_of(const std::string& path) { const lr::Info s = lr::inspect(path, false); return s.valid ? int(s.stage_id) : -1; }
Card read(const std::string& path) {
  Card c;
  const lr::Info s = lr::inspect(path, true);
  if (!s.valid) return c;
  c.ok = s.stats_loaded;
  c.date = lr::display_date(s);
  std::string title;
  for (size_t i = 0; i < s.players.size(); ++i) title += std::string(i ? " vs " : "") + lr::character_name(s.players[i].character);
  c.title = title;
  const double minutes = std::max(1.0, double(s.last_frame + 123)) / 3600.0;
  static const char* const kRows[] = {"Players", "Kills", "Damage done", "Openings per kill", "Neutral wins", "Counter hits",
                                      "Conversions", "L-cancels", "Inputs per minute", "Rolls, spot dodges, air dodges",
                                      "Wavedashes, wavelands, dash dances", "Ledge grabs"};
  for (int row = 0; row < 12 && c.ok; ++row) {
    std::string out;
    for (size_t i = 0; i < s.players.size(); ++i) {
      const lr::Player& p = s.players[i];
      char b[96] = "";
      switch (row) {   // as the engine's Replays page (replay_library.cpp stat_value)
        case 0: std::snprintf(b, sizeof b, "P%d %s%s", p.port, lr::character_name(p.character), int(i) == s.winner ? " (won)" : ""); break;
        case 1: std::snprintf(b, sizeof b, "%d", p.kills); break;
        case 2: std::snprintf(b, sizeof b, "%.0f%%", p.damage_done); break;
        case 3: if (p.kills) std::snprintf(b, sizeof b, "%.1f", double(p.openings) / p.kills); else std::snprintf(b, sizeof b, "-"); break;
        case 4: std::snprintf(b, sizeof b, "%d", p.neutral_wins); break;
        case 5: std::snprintf(b, sizeof b, "%d", p.counter_hits); break;
        case 6: std::snprintf(b, sizeof b, "%d/%d", p.successful_conversions, p.openings); break;
        case 7: if (!s.l_cancel_available) std::snprintf(b, sizeof b, "-");
                else if (p.l_success + p.l_fail) std::snprintf(b, sizeof b, "%d/%d (%d%%)", p.l_success, p.l_success + p.l_fail, 100 * p.l_success / (p.l_success + p.l_fail));
                else std::snprintf(b, sizeof b, "0/0");
                break;
        case 8: std::snprintf(b, sizeof b, "%.0f", double(p.inputs) / minutes); break;
        case 9: std::snprintf(b, sizeof b, "%d %d %d", p.rolls, p.spot_dodges, p.air_dodges); break;
        case 10: std::snprintf(b, sizeof b, "%d %d %d", p.wavedashes, p.wavelands, p.dash_dances); break;
        case 11: std::snprintf(b, sizeof b, "%d", p.ledge_grabs); break;
      }
      if (i) out += " | ";
      out += b;
    }
    c.rows.push_back({kRows[row], out});
  }
  const launcher::trace::Overview view = launcher::trace::load(std::filesystem::path(path));
  if (view.present) {
    c.trace = true;
    for (const auto& b : view.bins) c.bins.push_back({float(b.interval_ms), b.work_ms, float(b.waits), float(b.rollbacks), float(b.flags)});
    char line[160];
    std::snprintf(line, sizeof line, "%.0f s, %zu waits, %zu rollbacks, %zu marks", view.seconds, view.waits, view.rollbacks,
                  view.marks[0] + view.marks[1] + view.marks[2] + view.marks[3]);
    c.trace_summary = line;
  }
  return c;
}
}  // namespace mu_replay_stats
