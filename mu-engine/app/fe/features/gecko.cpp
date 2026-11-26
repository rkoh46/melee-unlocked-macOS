// melee-unlocked's player Gecko codes on the Source Port (its GECKO CODES page, "Your codes"), from Melee-NX-NetPlay's
// frontend/src/features/gecko.cpp (same author): upstream's user_gecko.cpp, compiled from the fetched tree (fe/patches/0122),
// with the game image's own writer and reader (shim/mu_gecko.c: mu_user_gecko_write and mu_user_gecko_read, found by name,
// mu-engine/patches/0016).
//   - The codes are read from <the engine's folder>/GeckoCodes.ini (~/Library/Application Support/Dashdance/melee-unlocked) (Dolphin's format: [Gecko], "$Name", "*note", "XXXXXXXX YYYYYYYY"); a
//     code is switched on the panel's GECKO CODES page; the choice is kept in settings.ini as upstream keeps it (`geckocode NAME`
//     lines and `geckochosen 1`; until then the file's [Gecko_Enabled] section decides).
//   - What runs is upstream's rule for the Source Port: every byte a code writes or reads must land in one game variable the
//     native game keeps with the console's layout (user_gecko_targets.h): types 00-06, the serial write 08, the pointer forms
//     10-18, 42/4A/44/4C, the ifs 20-2E with E2 and E0, the Gecko registers 80-88. Refused, with the reason on the row, as
//     upstream refuses them: PowerPC injection (C0, C2, C6), pointer loads from game memory (40, 48), loops and jumps (60-68),
//     register compares (A0-AE), writes outside those variables, Slippi's own required codes and its lag reduction. Upstream
//     also runs a code that rewrites a game function into "return N at once" by overwriting the native function's entry; this
//     engine does not rewrite the game image's code yet, so such a code is refused here with that reason.
//   - A code made only of patches the game already carries (the General Codes, widescreen, screen shake...) is shown as built in.
//   - Offline only, as upstream: nothing runs online (the game's writer itself refuses during a network match), in replay
//     playback or in a vanilla-only session; every code is off until the player switches it on.
//   - Codes agreed online (settings.ini gecko_online 1, off by default; upstream has the protocol and keeps it off), from
//     Melee-NX-NetPlay 0.4.8 so the two apps agree: in a Direct 1v1 between two builds whose players both turned it on (the build
//     message's flag 0x40, net/patches/net-0014; this app or Melee-NX-NetPlay on the Switch), each side offers its switched-on
//     codes that can run online (melee-unlocked's gecko_session.cpp and gecko_online.cpp from the fetched tree, fe/patches/0141);
//     the other player ticks the ones it accepts on GECKO CODES > Online; the match runs every code one side offered and the other
//     accepted, decided at its first frame from the two locks (sent before each selection), so both games decide alike. They run
//     through the game's agreed writer (mu_user_gecko_write_agreed) at every simulated frame's input exchange (0xB0), rollback
//     frames included, so a re-simulated frame sees what the first run saw. Against Slippi Dolphin, melee-unlocked or a build
//     without it: nothing is sent and no code runs.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "features_internal.h"
#include "frontend.h"
#include "user_gecko.h"          // melee-unlocked port/runtime/host (the fetched tree)
#include "gecko_session.h"       // its online session (build-time copy with fe/patches/0141) and messages (gecko_online.h)
#include "netplay_bridge.h"      // set_gecko_hooks: the netplay object's calls (net/patches/net-0014)
#include "gameplay_policy.h"
#include "mu_host.h"            // MuHostApi: the disc_entrynum hook below

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace linux_host { void* fe_game_symbol(const char* name); }   // mu-engine/patches/0016

// user_gecko.cpp's Static Recomp path (console memory writes, functions run from RAM): never reached here, because code
// patches are not allowed (user_gecko::set_code_patches_allowed stays false); defined as refusals so the copy links.
namespace host {
void wr8(uint32_t, uint8_t) {}
void wr16(uint32_t, uint16_t) {}
void wr32(uint32_t, uint32_t) {}
}
namespace ppc { bool redirect_function_at(uint32_t) { return false; } }

namespace gecko_session { void set_online_codes(bool on); }   // fe/patches/0141

namespace fe::features {
namespace {
std::vector<std::string> g_enabled_names;    // settings.ini geckocode lines
std::atomic<bool> g_online{false};           // settings.ini gecko_online 1: codes agreed online with another such build
std::vector<std::string> g_ticked;           // the opponent's codes this player ticked (simulation thread)
std::vector<std::vector<uint8_t>> g_outgoing;   // gecko_session's messages, waiting for the netplay object (simulation thread)
std::mutex g_online_mutex;
std::vector<GeckoRow> g_online_rows;         // GECKO CODES > Online as the panel shows it (render thread reads a copy)
size_t g_running_codes = 0;
bool g_chosen = false;                       // settings.ini geckochosen 1
bool g_loaded = false;
std::atomic<bool> g_running{false};          // the codes ran in the last frame (for the log)
std::mutex g_rows_mutex;
std::vector<GeckoRow> g_rows;                // what the panel shows (render thread reads a copy)

std::string file_path() { return options.out_dir + "/GeckoCodes.ini"; }

void publish_rows() {
  std::vector<GeckoRow> rows;
  for (const user_gecko::Code& c : user_gecko::codes()) {
    GeckoRow r;
    r.name = c.name;
    r.enabled = c.enabled;
    const char* builtin = user_gecko::native_equivalent(c);
    r.supported = c.supported && !builtin;
    r.reason = builtin ? std::string("Built in: ") + builtin + " is the switch that applies here" : c.supported ? std::string() : "Cannot run on the Source Port: this code " + c.reason;
    rows.push_back(std::move(r));
  }
  std::lock_guard<std::mutex> lock(g_rows_mutex);
  g_rows.swap(rows);
}

// settings.ini keeps the switched-on codes as upstream does: one `geckocode NAME` line each, and `geckochosen 1`.
void persist() {
  const std::string path = settings_path();
  if (path.empty()) return;
  std::vector<std::string> lines;
  { std::ifstream in(path); std::string l; while (std::getline(in, l)) if (l.rfind("geckocode ", 0) != 0 && l.rfind("geckochosen ", 0) != 0 && l.rfind("gecko_online ", 0) != 0) lines.push_back(l); }
  for (const user_gecko::Code& c : user_gecko::codes()) if (c.enabled || c.wanted) lines.push_back("geckocode " + c.name);
  lines.push_back("geckochosen 1");
  if (g_online) lines.push_back("gecko_online 1");
  const std::string tmp = path + ".tmp";
  if (FILE* f = std::fopen(tmp.c_str(), "w")) {
    for (const auto& l : lines) std::fprintf(f, "%s\n", l.c_str());
    std::fclose(f);
    std::rename(tmp.c_str(), path.c_str());
  }
}

bool setting(const std::string& key, const std::string& value) {
  if (key == "gecko_online") { g_online = value == "1"; gecko_session::set_online_codes(g_online); return true; }
  if (key == "geckocode") { if (!value.empty()) g_enabled_names.push_back(value); return true; }
  if (key == "geckochosen") { g_chosen = value == "1"; return true; }
  return false;
}

// The game image's writer and reader. On the Mac its symbols are found only once the host has finished loading the image,
// after the tables hook: bound there when they are there, else at the first frame.
bool g_bound = false;
void bind_native() {
  if (g_bound) return;
  void* w = linux_host::fe_game_symbol("mu_user_gecko_write");
  void* r = linux_host::fe_game_symbol("mu_user_gecko_read");
  if (!w) return;
  g_bound = true;
  user_gecko::set_native_writer(reinterpret_cast<user_gecko::NativeWrite>(w));
  user_gecko::set_native_reader(reinterpret_cast<user_gecko::NativeRead>(r));
  user_gecko::set_agreed_writer(reinterpret_cast<user_gecko::NativeWrite>(linux_host::fe_game_symbol("mu_user_gecko_write_agreed")),
                                reinterpret_cast<user_gecko::NativeRead>(linux_host::fe_game_symbol("mu_user_gecko_read_agreed")));
}

void load_codes() {
  if (g_loaded) return;
  g_loaded = true;
  host::gameplay::vanilla_only_session = settings_vanilla_only();
  user_gecko::set_code_patches_allowed(false);   // the Source Port: game variables only
  user_gecko::set_sources_allowed(!settings_vanilla_only());
  bind_native();
  user_gecko::load(file_path(), g_enabled_names, g_chosen);
  int supported = 0, on = 0;
  for (const auto& c : user_gecko::codes()) { supported += c.supported; on += c.enabled; }
  if (!user_gecko::codes().empty())
    say("gecko: %zu codes in %s, %d can run on the Source Port, %d switched on%s", user_gecko::codes().size(), file_path().c_str(), supported, on,
        settings_vanilla_only() ? " (a vanilla-only session: none runs)" : "");
  publish_rows();
}

// The codes once more whenever the game opens a disc file (the host's disc_entrynum). A VS match's scene start refills the
// start data and loads and builds the fighters inside one frame on the Mac (the disc answers at once), so a code that sets a
// start-data value (a fighter's model scale) would be overwritten before the frame's pass and never reach the fighters; on a
// console or a PC the loads span frames and the per-frame pass gets there first. Applied here, the value is in place when the
// fighter files open, just before the fighters read it. Same rule as the frame pass: offline, never in a replay or a vanilla
// session, only with a code switched on.
std::atomic<bool> g_may_run{false};
int32_t (*g_entrynum_next)(const char*) = nullptr;
int32_t entrynum_with_codes(const char* path) {
  if (g_may_run.load(std::memory_order_relaxed) && user_gecko::any_enabled()) {
    user_gecko::apply();
    static int logged = 0;
    if (logged++ < 3) say("gecko: player codes applied as %s opens", path ? path : "a file");
  }
  return g_entrynum_next ? g_entrynum_next(path) : -1;
}
// GECKO CODES > Online, as the panel shows it: the switch, the session's line, and while the opponent's offer waits for an
// answer one row per code (ticked or not, with the known codes table's description or the sender's name) and the answers.
void publish_online() {
  std::vector<GeckoRow> rows;
  GeckoRow sw; sw.key = "@online"; sw.name = "Offer and accept codes in Direct (melee-unlocked-macOS or Melee-NX-NetPlay)"; sw.enabled = g_online; sw.supported = !online();
  sw.reason = "Off by default. In a Direct match against a melee-unlocked-macOS or Melee-NX-NetPlay player who turned this on too, your "
              "switched-on codes are offered and theirs asked about; a match runs only the codes both of you agreed to. Never with Slippi "
              "Dolphin or melee-unlocked.";
  rows.push_back(sw);
  const std::string status = gecko_session::status();
  if (!status.empty()) { GeckoRow st; st.key = "@status"; st.name = status; st.supported = false; st.info = true; rows.push_back(st); }
  if (gecko_session::asking()) {
    for (const auto& in : gecko_session::incoming()) {
      GeckoRow r; r.key = "@tick:" + in.id;
      r.name = "Accept: " + in.name + " (" + in.kind + ")";
      r.enabled = std::find(g_ticked.begin(), g_ticked.end(), in.id) != g_ticked.end();
      r.supported = in.runnable;
      r.reason = in.runnable ? in.what : "Cannot run here: " + in.why_not;
      rows.push_back(r);
    }
    GeckoRow ok; ok.key = "@answer"; ok.name = "Use the ticked codes this match"; ok.supported = true; ok.action = true; rows.push_back(ok);
    GeckoRow none; none.key = "@none"; none.name = "No codes"; none.supported = true; none.action = true; rows.push_back(none);
  }
  std::lock_guard<std::mutex> lock(g_online_mutex);
  g_online_rows.swap(rows);
}
void collect_outgoing() { for (auto& m : gecko_session::take_outgoing()) g_outgoing.push_back(std::move(m)); }
void answer_ticked(bool none) {
  gecko_session::answer(none ? std::vector<std::string>{} : g_ticked);
  g_ticked.clear();
  collect_outgoing();
  publish_online();
}

// Every simulated frame's input exchange (0xB0, rollback frames included; the response's first byte 1 runs the frame, 4 runs it
// and one more, 2 holds it back, 3 is a disconnect): the agreed codes, at the same point of every frame on both games.
void exchange(uint8_t command, const uint8_t*, uint32_t, const uint8_t* response, uint32_t response_size, int32_t result, bool) {
  if (command != 0xB0 || result < 0 || response_size < 1 || (response[0] != 1 && response[0] != 4)) return;
  const auto& codes = gecko_session::match_codes();
  if (!codes.empty() && !replay_playing()) { bind_native(); user_gecko::run_agreed(codes); }
}

void register_net_hooks();
void tables(MuHostApi* host, MuGameApi*) {
  if (!options.play) return;
  load_codes();
  publish_online();
  register_net_hooks();
  if (host && host->disc_entrynum && !g_entrynum_next && user_gecko::any_enabled()) { g_entrynum_next = host->disc_entrynum; host->disc_entrynum = entrynum_with_codes; }
}

// Once per frame, as upstream applies them each frame (it does so at the pad read; here right after the frame, the same
// point of every frame). Offline only.
void on_retrace(const Retrace& info) {
  // Tests (app_gecko_test.sh, app_online_menus_test.sh with MU_GECKO_PROBE=1): every 10 s, P1's model scale in the VS start data as
  // the game image hands it back (online through its agreed reader, which also reads during a network match).
  static const bool probe = std::getenv("MU_GECKO_PROBE") != nullptr;
  if (probe && info.retrace % 600 == 0) {
    using Read = int (*)(unsigned, unsigned char*, unsigned);
    static const Read read = reinterpret_cast<Read>(linux_host::fe_game_symbol("mu_user_gecko_read"));
    static const Read read_agreed = reinterpret_cast<Read>(linux_host::fe_game_symbol("mu_user_gecko_read_agreed"));
    const Read r = info.online && read_agreed ? read_agreed : read;
    unsigned char b[4] = {};
    const int got = r ? r(0x804805B0u, b, 4) : 0;
    say("gecko probe: retrace %u, VS start data P1 model scale %02X%02X%02X%02X (read %d%s)", info.retrace, b[0], b[1], b[2], b[3], got,
        info.online ? ", online" : "");
  }
  if (!g_loaded) return;
  bind_native();   // also with no code of its own on: the opponent's agreed codes are checked and run through it
  if (!user_gecko::any_enabled()) return;
  const bool run = !info.online && !replay_playing() && !vanilla();
  g_may_run.store(run, std::memory_order_relaxed);
  if (run) user_gecko::apply(); else user_gecko::suspend_function_patches();
  if (run != g_running.exchange(run)) say("gecko: player codes %s", run ? "run (offline)" : "suspended (online, a replay or a vanilla session)");
}
}  // namespace

// The panel (mu_panel.cpp): the rows, and a switch from its queue (simulation thread).
std::vector<GeckoRow> gecko_rows() { std::lock_guard<std::mutex> lock(g_rows_mutex); return g_rows; }
std::string gecko_file() { return file_path(); }
std::vector<GeckoRow> gecko_online_rows() { std::lock_guard<std::mutex> lock(g_online_mutex); return g_online_rows; }
void gecko_toggle(const std::string& name) {
  if (name == "@online") {
    if (online()) return;   // from the next connection
    g_online = !g_online; gecko_session::set_online_codes(g_online);
    say("gecko: codes online %s (Direct, with melee-unlocked-macOS or Melee-NX-NetPlay; from the next connection)", g_online ? "on" : "off");
    persist(); publish_online();
    return;
  }
  if (name.rfind("@tick:", 0) == 0) {
    const std::string id = name.substr(6);
    auto it = std::find(g_ticked.begin(), g_ticked.end(), id);
    if (it == g_ticked.end()) g_ticked.push_back(id); else g_ticked.erase(it);
    publish_online();
    return;
  }
  if (name == "@answer" || name == "@none") { answer_ticked(name == "@none"); return; }
  load_codes();
  for (user_gecko::Code& c : user_gecko::codes()) {
    if (c.name != name) continue;
    if (!c.supported || user_gecko::native_equivalent(c)) return;
    c.enabled = !c.enabled; c.wanted = c.enabled;
    say("gecko: \"%s\" %s", c.name.c_str(), c.enabled ? "on (offline)" : "off");
    break;
  }
  persist();
  publish_rows();
}

void register_gecko() {
  add_exchange(exchange);
  add_setting(setting);
  add_tables(tables);
  add_retrace(on_retrace);
}
}  // namespace fe::features

// ---- net/patches/net-0014: the netplay object's calls (simulation thread: inside the game's slippi_command), handed over through
// netplay_bridge::set_gecko_hooks (the object's own mu_net_gecko_* are local to it and forward here) ----
namespace fe::features {
namespace {
size_t copy_out(const std::vector<uint8_t>& m, uint8_t* out, size_t capacity) {
  if (m.empty() || m.size() > capacity) return 0;
  std::copy(m.begin(), m.end(), out);
  return m.size();
}
int net_wanted() { return g_online.load() && !settings_vanilla_only() ? 1 : 0; }
void net_connected(int consent) {
  g_ticked.clear(); g_outgoing.clear(); g_running_codes = 0;
  gecko_session::connected(consent != 0);
  collect_outgoing();
  say("gecko: online session with %s", consent ? "a player whose build takes codes (Direct)" : "no code exchange (not Direct, or the other build does not take codes)");
  publish_online();
}
void net_disconnected() { gecko_session::disconnected(); g_ticked.clear(); g_outgoing.clear(); publish_online(); }
void net_receive(const uint8_t* data, size_t size) {
  gecko_session::receive(data, size);
  // Tests only: MU_TEST_GECKO_ACCEPT=all ticks every code the opponent offers that can run here and answers at once.
  static const char* test_accept = std::getenv("MU_TEST_GECKO_ACCEPT");
  if (test_accept && std::string(test_accept) == "all" && gecko_session::asking()) {
    g_ticked.clear();
    for (const auto& in : gecko_session::incoming()) if (in.runnable) g_ticked.push_back(in.id);
    say("gecko: test: accepting the opponent's %zu codes", g_ticked.size());
    answer_ticked(false);
  }
  collect_outgoing();
  publish_online();
}
size_t net_take(uint8_t* out, size_t capacity) {
  if (g_outgoing.empty()) return 0;
  const size_t n = copy_out(g_outgoing.front(), out, capacity);
  g_outgoing.erase(g_outgoing.begin());
  return n;
}
int net_take_resend() { return gecko_session::take_resend() ? 1 : 0; }
size_t net_lock(uint8_t* out, size_t capacity) { return copy_out(gecko_session::lock_message(), out, capacity); }
void net_match_started() {
  gecko_session::match_started();
  g_running_codes = gecko_session::match_codes().size();
  say("gecko: this online match runs %zu agreed codes", g_running_codes);
  publish_online();
}
void register_net_hooks() {
  static bool done = false;
  if (done) return;
  done = true;
  netplay_bridge::GeckoHooks h;
  h.wanted = net_wanted; h.connected = net_connected; h.disconnected = net_disconnected; h.receive = net_receive;
  h.take = net_take; h.take_resend = net_take_resend; h.lock = net_lock; h.match_started = net_match_started;
  netplay_bridge::set_gecko_hooks(h);
}
}  // namespace
}  // namespace fe::features
