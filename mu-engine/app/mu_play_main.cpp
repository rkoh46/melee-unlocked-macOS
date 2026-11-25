// The melee-unlocked engine player for macOS: the app starts it when the melee-unlocked engine is chosen.
//
// melee-unlocked's host services run the game image (an AArch64 Linux ELF, mu-engine/runner) on the main thread,
// paced to the console's 60 Hz. Through the host's frontend hooks (patches/0011) this program gives the game the app's
// controllers (keyboard, SDL gamepads, the GameCube adapter) and plays its sound through the app's audio output; the GX
// command stream goes to the app's decoder and Metal renderer, and the window is the app's SDL3 window. Events are
// pumped at every retrace, as the classic engine does.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host_services.h"
#include "input_script.h"   // melee-unlocked's script format (port/linux, the fetched tree)
#include "mu_elf_bridge.h"
#include "fixedmap.h"
#include "abi_macos.h"
#include "net_host.h"
#include "netplay_bridge.h"
#include "slippi_menus.h"
#include "feature_calls.h"
#include <sstream>
#include "../../port/runtime/host/input_script.h"   // this repository's script format (@scene MODE STATE blocks)
#include "features/present/present.h"

#include "audio.h"
#include "app/game_menu.h"     // ui/app/game_menu.h: the menu interface melee-unlocked's panel (ui/mu_panel.cpp) implements
#include "frontend.h"
#include "fe_mac.h"
#include "ui_metal.h"
#include "gx_core.h"
#include "gx_metal.h"
#include "host.h"
#include "input_config.h"
#include "window.h"

#include <mach-o/dyld.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <ctime>
#include <chrono>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

extern "C" const uint8_t mu_embedded_game[], mu_embedded_game_end[];
extern "C" int32_t mu_frontend_command(uint8_t, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*);   // fe/mods.cpp

namespace fs = std::filesystem;

namespace host {
void mu_shim_set_scene(uint8_t major, uint8_t minor);
void mu_shim_retrace();
void mu_shim_open_log(const std::string& path);
}

namespace {
using Clock = std::chrono::steady_clock;
std::atomic<bool> g_quit{false};
bool g_play = false;              // play mode: the frontend layer (fe/) is on; scripted runs (tests) leave it off
bool g_net_on = false;
bool g_online_now = false;
uint32_t g_scene_major = ~0u, g_scene_minor = ~0u;
std::vector<std::string> g_restart_args;   // a feature module asked to start the game again with these (the replay viewer's Watch)
std::atomic<bool> g_online_flag{false};
// The performance HUD's history (fe::perf_history): the simulation thread's frame times, newest last.
std::mutex g_perf_mutex;
std::vector<float> g_perf;
bool g_vanilla = false;
uint32_t g_last_match_frame = 0;
double g_last_frame_time = -1;
uint32_t g_frames_limit = 0;
FILE* g_digest = nullptr;   // --digest: the parity runner's per-retrace state rows (tests)

void net_say(const char* format, ...) {
  char line[1024]; va_list args; va_start(args, format); std::vsnprintf(line, sizeof line, format, args); va_end(args);
  host::log("%s", line);
}
// A replay recorded without a Gecko code list (at most its 8-byte terminator) is the retail game's: a Vanilla session or a
// scripted match. It plays back only in a Vanilla session (the replay keeps the option words, not the Vanilla choice),
// so --replay of such a file starts this program as Vanilla. Reads the .slp's raw event stream up to the first frame.
bool replay_is_vanilla(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::vector<uint8_t> d((std::istreambuf_iterator<char>(in)), {});
  static const uint8_t raw_key[] = {'U', 3, 'r', 'a', 'w', '[', '$', 'U', '#', 'l'};
  auto at = std::search(d.begin(), d.end(), std::begin(raw_key), std::end(raw_key));
  if (at == d.end()) return false;
  size_t i = size_t(at - d.begin()) + sizeof raw_key + 4;
  uint16_t sizes[256] = {};
  size_t gecko = 0; bool have_sizes = false;
  while (i < d.size()) {
    const uint8_t cmd = d[i];
    if (cmd == 0x35) {
      if (i + 1 >= d.size()) return false;
      const size_t n = d[i + 1];
      for (size_t k = 0; k + 3 <= n - 1 && i + 2 + k + 2 < d.size(); k += 3) sizes[d[i + 2 + k]] = uint16_t(d[i + 3 + k] << 8 | d[i + 4 + k]);
      i += 1 + n; have_sizes = true; continue;
    }
    if (!have_sizes || cmd == 0x37 || cmd == 0x39) break;   // the first frame: the code list came before it
    if (cmd == 0x10 && i + 1 + 516 <= d.size() && d[i + 1 + 514] == 0x3D) gecko += size_t(d[i + 1 + 512] << 8 | d[i + 1 + 513]);
    if (!sizes[cmd]) return false;
    i += 1 + sizes[cmd];
  }
  return have_sizes && gecko <= 8;
}
std::string home_dir() { const char* h = std::getenv("HOME"); return h ? h : "."; }

void on_fifo(const uint8_t* data, uint32_t size, void*) {
  // The decoder takes the write-gather pipe's stream; whole words go in at once.
  uint32_t i = 0;
  for (; i + 4 <= size; i += 4)
    gx::write_fifo(uint32_t(data[i]) << 24 | uint32_t(data[i + 1]) << 16 | uint32_t(data[i + 2]) << 8 | data[i + 3], 4);
  for (; i < size; ++i) gx::write_fifo(data[i], 1);
}

// Pacing (play mode offline): one retrace per console frame against the wall clock, as the frontend paces it on the
// Switch; a replay's fast forward and seeks turn it off (fe::options.pace). Online, the host paces (the netplay host
// moves its clock).
void pace() {
  static Clock::time_point next{};
  const auto now = Clock::now();
  if (!fe::options.pace) { next = now; return; }
  next += std::chrono::nanoseconds(16683333);   // 60/1.001 Hz
  if (now - next > std::chrono::milliseconds(100) || next - now > std::chrono::milliseconds(100)) next = now;
  std::this_thread::sleep_until(next);
}

void on_retrace(void*) {
  if (g_play && !g_net_on) pace();
  host::mu_shim_retrace();
  host::window_pump();
  if (host::window_closed() || g_quit.load()) linux_host::request_stop(0);
}

void on_frame(const MuStatePod& state, const linux_host::Counters& counters, void*) {
  host::mu_shim_set_scene(uint8_t(state.scene_major), uint8_t(state.scene));
  // The parity runner's per-retrace state row: the digest (--digest) and the netplay host's record of shown states.
  char row[1400]; int n = std::snprintf(row, sizeof row, "%u,%u,%u,%u,%u", counters.retraces, state.scene_major, state.scene, state.match_frame, state.rng);
  for (const auto& f : state.player)
    n += std::snprintf(row + n, sizeof row - size_t(n), ",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u", f.present, f.stocks, f.action, f.anim_frame, f.pos_x,
                       f.pos_y, f.pos_z, f.vel_x, f.vel_y, f.vel_z, f.percent, f.facing);
  row[n++] = '\n';
  if (g_digest) std::fwrite(row, 1, size_t(n), g_digest);
  const bool fighting = state.match_frame && state.match_frame != g_last_match_frame;
  g_last_match_frame = state.match_frame;
  const double now = host::now_seconds();
  net_host::on_retrace(row, size_t(n), fighting, g_last_frame_time < 0 ? -1.0 : (now - g_last_frame_time) * 1000.0);
  g_last_frame_time = now;
  if (g_frames_limit && counters.retraces >= g_frames_limit) linux_host::request_stop(0);
  if (!g_play) return;
  {
    static double last = -1;
    std::lock_guard<std::mutex> lock(g_perf_mutex);
    if (last >= 0) g_perf.push_back(float((now - last) * 1000.0));
    if (g_perf.size() > 600) g_perf.erase(g_perf.begin(), g_perf.begin() + 300);
    last = now;
  }
  if (state.scene_major != g_scene_major || state.scene != g_scene_minor) {
    g_scene_major = state.scene_major; g_scene_minor = state.scene;
    host::log("play: scene %u/%u at retrace %u", g_scene_major, g_scene_minor, counters.retraces);
  }
  // Online as melee-unlocked's host decides it: Slippi's online flow (major scene 8) with the netplay host on. The option
  // words follow (fe/settings.cpp).
  const bool online = g_net_on && state.scene_major == 8;
  if (online != g_online_now) {
    g_online_now = online; g_online_flag.store(online);
    fe::settings_online(online);
    host::log("play: %s at retrace %u: the option words are now the %s ones", online ? "online" : "offline again", counters.retraces, online ? "online" : "player's");
  }
  fe::features::Retrace info;
  info.retrace = counters.retraces; info.scene_major = state.scene_major; info.scene_minor = state.scene;
  info.match_frame = state.match_frame; info.fighting = fighting; info.online = online; info.state = &state;
  fe::features::call_retrace(info);
  if (g_restart_args.empty() && fe::features::call_take_restart(&g_restart_args)) {
    host::log("play: the game starts again with %zu more argument(s) (asked for by a feature module)", g_restart_args.size());
    linux_host::request_stop(0);
  }
}

// The host's slippi_command with the netplay host on: the frontend's commands (fe/mods.cpp, the feature modules) first,
// then the netplay host, whose answers the modules may read.
int32_t (*g_net_command)(uint8_t, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*) = nullptr;
int32_t command(uint8_t c, const uint8_t* payload, uint32_t size, uint8_t* response, uint32_t capacity, uint32_t* response_size) {
  if (response_size) *response_size = 0;
  const int32_t mine = mu_frontend_command(c, payload, size, response, capacity, response_size);
  if (mine != INT32_MIN) return mine;
  if (!g_net_command) return -1;
  const int32_t result = g_net_command(c, payload, size, response, capacity, response_size);
  if (result == 0 && response && response_size) fe::features::call_reply(c, response, *response_size);
  fe::features::call_exchange(c, payload, size, response, response_size ? *response_size : 0, result, false);
  return result;
}
void tables_hook(MuHostApi* host, MuGameApi* game) {
  if (g_play) fe::features::call_tables(host, game);   // first, before anything below wraps the table
  if (!host) return;
  net_host::install(host);
  if (g_net_on && g_play) { g_net_command = host->slippi_command; host->slippi_command = command; }
  // The game's replay events also go to the Discord presence (stage, characters, stocks), after the replays module.
  if (g_net_on && g_play && host->replay_event) {
    static void (*replay_next)(uint8_t, const uint8_t*, uint32_t) = nullptr;
    replay_next = host->replay_event;
    host->replay_event = [](uint8_t c, const uint8_t* p, uint32_t n) { replay_next(c, p, n); netplay_bridge::replay_event(c, p, n); };
  }
  mu_abi_adapt_host_table(host);
}
} // namespace

namespace mu_looks { const gx::LookHooks* hooks(); }   // gx_looks_mac.cpp
namespace fe {
std::function<void(host::RuntimeSettings&)> mac_menu_initial;
std::function<void(const host::RuntimeSettings&, host::MenuChange)> mac_menu_apply;
std::function<void(const MacPicture&)> mac_set_picture;
// The newest `capacity` simulation-thread frame times, oldest first (the overlays' performance graph).
int perf_history(float* sim_ms, int capacity) {
  std::lock_guard<std::mutex> lock(g_perf_mutex);
  const int n = std::min(capacity, int(g_perf.size()));
  for (int i = 0; i < n; ++i) sim_ms[i] = g_perf[g_perf.size() - size_t(n) + size_t(i)];
  return n;
}
}  // namespace fe

// ---- the host services' frontend hooks (patches/0011): the option words, the disc and the commands are fe/settings.cpp's
// and fe/mods.cpp's ----
// The game's poll and the input-suggestion reads (below) never read the controllers at the same time.
std::mutex g_input_mutex;
std::atomic<uint32_t> g_last_poll_retrace{0};
std::atomic<uint64_t> g_last_poll_ns{0};
uint64_t now_ns() { return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count()); }
extern "C" int mu_frontend_pad_read(MuPadStatus out[4]) {
  host::PadState pads[4];
  if (g_net_on) netplay_bridge::suggestion_note_poll();   // a change read from here on is in the next frame's pad (net-0001)
  {
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_last_poll_retrace.store(host::retrace_count(), std::memory_order_relaxed);
    host::input_poll(pads);
  }
  g_last_poll_ns.store(now_ns(), std::memory_order_relaxed);
  for (int i = 0; i < 4; ++i) {
    out[i].button = pads[i].button; out[i].stick_x = pads[i].stick_x; out[i].stick_y = pads[i].stick_y;
    out[i].sub_x = pads[i].sub_x; out[i].sub_y = pads[i].sub_y; out[i].trigger_l = pads[i].trig_l; out[i].trigger_r = pads[i].trig_r;
    out[i].analog_a = pads[i].analog_a; out[i].analog_b = pads[i].analog_b; out[i].err = pads[i].err;
  }
  return 1;
}
extern "C" void mu_frontend_pad_rumble(int32_t port, int32_t on) { host::input_rumble(port, on != 0); }
extern "C" void mu_frontend_audio(const int16_t* rl, uint32_t frames) {
  // The app's output takes the AI DMA layout: big-endian 16-bit samples, two per frame.
  static std::vector<uint8_t> be;
  be.resize(size_t(frames) * 4);
  for (uint32_t i = 0; i < frames * 2; ++i) { const uint16_t s = uint16_t(rl[i]); be[i * 2] = uint8_t(s >> 8); be[i * 2 + 1] = uint8_t(s); }
  host::audio_push(be.data(), be.size());
}

// Slippi 3.7.0's input suggestions (net-0001, as the Switch's play_io.cpp mu_net_suggest_read): the netplay object's sampler
// thread reads the local controller about a thousand times a second during an online game, as the game will get it at its
// next poll, in wire form. The app's whole input path (GameCube adapter, SDL gamepads, keyboard, the player's remap and
// controller setup on their own filter lane), never at the same time as the game's poll; nothing while melee-unlocked's
// panel is open or hands the controllers back. A controller script (tests) has nothing new between polls, except with
// MU_SUGGEST_LEAD_MS=N: then a read N ms before the next poll returns what the script gives that poll (scene-format scripts).
namespace fe { thread_local bool suggestion_read = false; bool panel_holds_pads(); }
namespace {
std::string g_play_script_path;
std::atomic<uint32_t> g_script_match_start{0}, g_script_scene{0};
int suggest_read(int port, uint8_t out[8]) {
  if (!g_play || port < 0 || port > 3) return 0;
  static const bool generated = std::getenv("MELEE_ONLINE_TEST_INPUTS") != nullptr;   // the game's own test inputs play
  if (generated) return 0;
  host::PadState pads[4];
  if (!g_play_script_path.empty()) {
    struct Lead {
      bool on = false; double seconds = 0; host::InputScript script;
      Lead() {
        const char* v = std::getenv("MU_SUGGEST_LEAD_MS");
        const double ms = v && *v ? std::atof(v) : 0.0;
        if (ms <= 0.0 || ms >= 16.0) return;
        std::ifstream file(g_play_script_path);
        on = file && script.load(file); seconds = ms / 1000.0;
      }
    };
    static Lead lead;
    if (!lead.on) return 0;
    std::lock_guard<std::mutex> lock(g_input_mutex);
    const uint32_t polled = g_last_poll_retrace.load(std::memory_order_relaxed);
    const bool ahead = double(now_ns() - g_last_poll_ns.load(std::memory_order_relaxed)) * 1e-9 >= 1.0 / 60.0 - lead.seconds;
    const uint32_t scene = g_script_scene.load(std::memory_order_relaxed);
    const auto sample = lead.script.sample(ahead ? polled + 1 : polled, g_script_match_start.load(std::memory_order_relaxed), uint8_t(scene >> 8), uint8_t(scene));
    for (int i = 0; i < 4; ++i) {
      pads[i] = {}; pads[i].err = sample[i].connected ? 0 : -1; pads[i].button = sample[i].buttons;
      pads[i].stick_x = sample[i].sx; pads[i].stick_y = sample[i].sy; pads[i].sub_x = sample[i].cx; pads[i].sub_y = sample[i].cy;
    }
  } else {
    std::lock_guard<std::mutex> lock(g_input_mutex);
    if (fe::panel_holds_pads()) return 0;
    fe::suggestion_read = true;
    host::input_poll(pads);
    fe::suggestion_read = false;
  }
  const host::PadState& p = pads[port];
  if (p.err != 0) return 0;
  out[0] = uint8_t(p.button >> 8); out[1] = uint8_t(p.button);
  out[2] = uint8_t(p.stick_x); out[3] = uint8_t(p.stick_y); out[4] = uint8_t(p.sub_x); out[5] = uint8_t(p.sub_y);
  out[6] = p.trig_l; out[7] = p.trig_r;
  return 1;
}
}  // namespace
extern "C" int mu_net_suggest_setting(void);   // fe/features/options.cpp: settings.ini input_suggestions (default on)

int main(int argc, char** argv) {
  mu_abi_install_crash_report();
  std::string iso, profile_dir, log_file, cache_dir, capture_path, script, sys_dir, replay, replay_dir, play_script;
  bool record = true, script_codes = false, audio_device = true, script_clock = false;
  net_host::Options net;
  bool offline = false;
  int volume = 70, scale = 0;
  uint32_t capture_frame = 0;
  bool fullscreen = false, hidden = false, vsync = true, vanilla = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
    if (a == "--iso") iso = next();
    else if (a == "--profile-dir") profile_dir = next();
    else if (a == "--log-file") log_file = next();
    else if (a == "--cache-dir") cache_dir = next();
    else if (a == "--volume") volume = std::clamp(std::atoi(next().c_str()), 0, 100);
    else if (a == "--scale") { const std::string v = next(); scale = v == "auto" ? 0 : std::clamp(std::atoi(v.c_str()), 0, 8); }
    else if (a == "--fullscreen") fullscreen = true;
    else if (a == "--vanilla") vanilla = true;
    else if (a == "--hidden") hidden = true;
    else if (a == "--no-vsync") vsync = false;
    else if (a == "--script") script = next();
    else if (a == "--play-script") play_script = next();   // tests: a controller script through the app's input layer, in play mode
    else if (a == "--sys-dir") sys_dir = next();
    else if (a == "--offline") offline = true;
    else if (a == "--replay") replay = next();
    else if (a == "--replay-dir") replay_dir = next();
    else if (a == "--no-record") record = false;
    else if (a == "--no-audio-device") audio_device = false;
    else if (a == "--script-real-clock") script_clock = true;   // tests: a scripted match on the real clock and seed (as a player's)   // tests: mix and count the sound, open no output device
    else if (a == "--script-codes") script_codes = true;   // tests: a scripted match with melee-unlocked's codes on (not the retail game)
    else if (net_host::parse_option(a, next, &net)) {}
    else if (a == "--digest") g_digest = std::fopen(next().c_str(), "w");
    else if (a == "--frames") g_frames_limit = uint32_t(std::strtoul(next().c_str(), nullptr, 0));
    else if (a == "--capture") { capture_path = next(); capture_frame = uint32_t(std::strtoul(next().c_str(), nullptr, 0)); }
    else if (a == "--help") {
      std::printf("MeleeUnlockedEngine --iso PATH [--profile-dir DIR] [--log-file F] [--cache-dir DIR] [--volume 0..100]\n"
                  "  [--scale auto|1..8] [--fullscreen] [--no-vsync] [--frames N] [--capture FILE.ppm FRAME] [--script FILE]\n"
                  "  [--sys-dir DIR] [--offline] [--online unranked|direct:CODE|teams|party] [--user-dir DIR] [--net-delay N]\n"
                  "  [--replay FILE.slp] [--replay-dir DIR] [--no-record]\n");
      return 0;
    } else if (!a.empty() && a[0] != '-' && iso.empty()) iso = a;
  }
  if (iso.empty()) if (const char* env = std::getenv("MELEE_ISO")) iso = env;
  if (iso.empty()) { std::fprintf(stderr, "no disc image: pass --iso PATH\n"); return 2; }
  const fs::path support = profile_dir.empty() ? fs::path(home_dir()) / "Library/Application Support/Dashdance" : fs::path(profile_dir);
  const fs::path mu_dir = support / "melee-unlocked";
  std::error_code ec;
  fs::create_directories(mu_dir / "Card A", ec);
  if (log_file.empty()) log_file = (mu_dir / "engine.log").string();
  host::mu_shim_open_log(log_file);
  if (cache_dir.empty()) { cache_dir = (mu_dir / "shaders").string(); fs::create_directories(cache_dir, ec); }
  // Play mode: everything but the scripted runs (tests), which play exactly as the parity runner does.
  g_play = script.empty();
  if (!replay.empty()) {
    offline = true; script.clear(); g_play = true;
    if (!vanilla && replay_is_vanilla(replay)) { vanilla = true; host::log("replay: recorded without a code list (the retail game): playing it as Vanilla"); }
  }
  g_vanilla = vanilla || (!script.empty() && !script_codes);   // a scripted match plays the retail game, as the parity runner does
  // Slippi's files: the bundle's slippi_sys (Contents/Resources next to Contents/MacOS), or --sys-dir.
  if (sys_dir.empty()) {
    char exe[4096]; uint32_t size = sizeof exe;
    if (_NSGetExecutablePath(exe, &size) == 0) {
      const fs::path bundled = fs::path(exe).parent_path().parent_path() / "Resources" / "slippi_sys";
      if (fs::is_directory(bundled, ec)) sys_dir = bundled.string();
    }
  }
  if (!script.empty() || !replay.empty()) sys_dir.clear();   // scripted runs and replays: no Slippi menus
  slippi_menus::set_sys_dir(sys_dir);
  // Online: the player's own Slippi sign-in (support/Slippi/user.json, as the classic engine) unless the launcher said
  // offline; a search starts from the game's Online Play page. --online without menus (tests) asks for a match at boot.
  const bool menus_online = !offline && !vanilla && script.empty() && !sys_dir.empty() && net.online.empty() && net.net.empty();
  if (menus_online) { net.online = "unranked"; net.from_menus = true; if (net.user_dir.empty()) net.user_dir = (support / "Slippi").string(); }

  // Controller settings the dashboard keeps (launcher.ini): the keyboard layout and each controller's mapping and port.
  bool discord_on = true;
  {
    std::ifstream in(support / "launcher.ini");
    std::string line;
    while (std::getline(in, line)) {
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
      auto value = [&](const char* key) -> const char* { size_t n = std::strlen(key); return line.compare(0, n, key) == 0 ? line.c_str() + n : nullptr; };
      if (const char* v = value("keyboard=")) host::set_keyboard_map(host::KeyboardMap::parse(v));
      else if (const char* v = value("discord=")) discord_on = *v == '1';             // the classic app's Discord setting (default on)
      else if (const char* v = value("discord_rank=")) net.discord_rank = *v == '1';
      else if (const char* v = value("controller.")) {
        std::string rest = v; size_t eq = rest.find('='), bar = rest.find('|', eq == std::string::npos ? 0 : eq);
        if (eq != std::string::npos && bar != std::string::npos) {
          host::ControllerConfig c; c.guid = rest.substr(0, eq); c.port = std::clamp(std::atoi(rest.c_str() + eq + 1), 0, 4);
          c.map = host::ControllerMap::parse(rest.substr(bar + 1)); host::upsert_controller_config(c);
        }
      }
    }
  }

  // The frontend layer (fe/, Melee-NX-NetPlay's play mode): settings.ini (melee-unlocked's game options), the replays
  // (recorded to the folder the app shares with the classic engine; --replay plays one with the viewer), the mod profile
  // chosen for this start, Training, the overlays and Fight a pro.
  std::string profile_card;
  if (g_play) {
    fe::options.play = true; fe::options.app = true;
    fe::options.out_dir = mu_dir.string(); fe::options.base_dir = mu_dir.string(); fe::options.iso = iso;
    fe::options.offline = offline; fe::options.online_delay = net.delay;
    for (int i = 1; i < argc; ++i) fe::options.rest.push_back(argv[i]);
    fe::settings_app_defaults(!sys_dir.empty() && !vanilla);
    fe::settings_load((mu_dir / "settings.ini").string());
    fe::settings_set_quiet("replay_dir", replay_dir.empty() ? (support / "Replays").string() : replay_dir);
    if (!record) fe::settings_set_quiet("savereplays", "0");
    if (vanilla) fe::settings_set_quiet("vanilla", "1");
    if (sys_dir.empty()) fe::settings_set_quiet("slippi_menus", "0");
    // A mod profile is for offline play: a session that can go online plays the retail files (as on the Switch).
    const bool can_go_online = !offline && !vanilla && !sys_dir.empty();
    if (can_go_online && !fe::settings_profile().empty())
      host::log("play: mod profile \"%s\" is not loaded: this session can go online (start offline to play it; its look-only costumes stay)", fe::settings_profile().c_str());
    profile_card = fe::mods_configure(can_go_online || vanilla ? std::string() : fe::settings_profile());
    // A session that can go online: skins in their online form, and of a mod profile only its look-only costumes (MU's pack
    // skin rule, fe/features/cosmetics.cpp), as on the Switch.
    if (can_go_online) { fe::mods_session_can_go_online(); if (!fe::settings_profile().empty()) fe::mods_configure_costumes(fe::settings_profile()); }
  }

  gx::MetalOptions gfx;
  gfx.efb_scale = scale; gfx.vsync = vsync; gfx.cache_dir = cache_dir;
  gfx.capture_path = capture_path; gfx.capture_frame = capture_frame;
  gfx.capture_ui = std::getenv("MELEE_CAPTURE_UI") != nullptr;   // screenshots of the settings panel (MELEE_MENU_OPEN)
  // Display-rate presentation and sub-frames (settings.ini subframe / fps, or --frame-mode / --fps): off by default.
  if (g_play)
    gx::metal_set_present_loop([](gx::Backend& b, gx::FrameQueue& q, const std::atomic<bool>& stop, gx::PresentRender render) {
      fe::present::loop(b, q, stop, [] {}, render);
    }, [] { return fe::present::active(); });
  void* layer = host::window_create(1280, 960, L"melee-unlocked-macOS", !hidden);
  int client_w = 0, client_h = 0;
  host::window_client_size(&client_w, &client_h);
  gx::Backend* backend = gx::create_metal_backend(layer, client_w, client_h, gfx);
  host::window_set_resize_callback([backend](int w, int h) { gx::metal_resize(backend, w, h); });
  if (fullscreen) host::window_set_fullscreen(true);
  gx::metal_set_overlay(backend, host::game_overlay);
  // melee-unlocked's settings panel (L + R + Start, or F1 / Escape): drawn with ImGui over the picture; what it changes
  // applies here, on the simulation thread.
  if (!g_play) { fe::options.out_dir = mu_dir.string(); fe::options.iso = iso; }
  fe::mac_menu_initial = [=](host::RuntimeSettings& rs) {
    rs.scale = scale; rs.vsync = vsync; rs.volume = volume; rs.fullscreen = fullscreen; rs.online_delay = net.delay;
  };
  // The renderer's options change from the panel (simulation thread) and the overlays' picture page (render thread).
  static std::mutex gfx_mutex;
  fe::mac_menu_apply = [backend, &gfx](const host::RuntimeSettings& s, host::MenuChange what) {
    switch (what) {
      case host::MenuChange::Volume: host::audio_set_volume(s.volume); break;
      case host::MenuChange::Fullscreen: host::window_set_fullscreen(s.fullscreen); break;
      case host::MenuChange::Graphics: case host::MenuChange::Anisotropy: case host::MenuChange::Filter: {
        std::lock_guard<std::mutex> lock(gfx_mutex);
        gfx.efb_scale = s.scale; gfx.vsync = s.vsync; gfx.anisotropy = s.anisotropy; gfx.filter = s.filter;
        gx::metal_set_options(backend, gfx);
        break;
      }
      default: break;
    }
  };
  fe::mac_set_picture = [backend, &gfx](const fe::MacPicture& p) {
    std::lock_guard<std::mutex> lock(gfx_mutex);
    gfx.brightness = p.brightness; gfx.contrast = p.contrast; gfx.vibrance = p.vibrance; gfx.crt = p.crt; gfx.crt_curve = p.crt_curve;
    // The overlays' Auto shape is 4:3: the renderer's own choice (4:3, or 16:9 with the widescreen code). 0 fills the window.
    if (p.aspect >= 0) gfx.picture_aspect = std::fabs(p.aspect - 4.0f / 3.0f) < 1e-3f ? 0.0f : p.aspect <= 0.0f ? 99.0f : p.aspect;
    if (p.scale > 0) gfx.efb_scale = p.scale;   // Auto keeps the launcher's choice
    gx::metal_set_options(backend, gfx);
  };
  if (g_play) fe::mac_controls_load((support / "launcher.ini").string());   // padx.<id>: the controller setups (fe/controls.cpp)
  if (g_play) fe::app_menu_setup();   // melee-unlocked's options, the feature modules' pages (fe/app_menu.cpp)
  if (const int f = host::menu_settings().filter) { gfx.filter = f; gx::metal_set_options(backend, gfx); }   // the screen filter launcher.ini keeps
  else { host::RuntimeSettings rs; fe::mac_menu_initial(rs); host::menu_init(rs, fe::mac_menu_apply); }
  gx::metal_set_ui_pass(mu_ui_metal::pass());
  // melee-unlocked's looks (fe/features/looks.cpp settings, mu-engine/app/gx_looks_mac.cpp rules): play mode only, so the
  // scripted reference runs draw exactly the game's picture.
  if (g_play) gx::metal_set_look_hooks(mu_looks::hooks());
  host::window_input_init();
  // --play-script: a controller script in melee-unlocked's format (the parity scripts': @scene major:minor latched once
  // seen, @match on the game's match frame), fed through the app's input path so the menu and the chords see it. A script
  // with "@scene MODE STATE" blocks (two hex bytes; several blocks, @rearm) is this repository's format
  // (port/runtime/host/input_script.cpp), as Melee-NX-NetPlay's frontend tests write them (mu-engine/tests/online).
  static linux_host::InputScript s_play_script;
  static host::InputScript s_scene_script;
  static uint32_t s_scene_match_start = 0;
  bool scene_format = false;
  if (!play_script.empty()) {
    std::ifstream in(play_script);
    std::string line;
    while (std::getline(in, line)) {
      std::istringstream words(line); std::string first, a, b;
      if ((words >> first >> a >> b) && first == "@scene") { scene_format = true; break; }
    }
  }
  g_play_script_path = play_script;
  if (scene_format) {
    std::ifstream in(play_script);
    std::string why;
    if (!s_scene_script.load(in, &why)) { std::fprintf(stderr, "cannot read %s: %s\n", play_script.c_str(), why.c_str()); return 2; }
    host::input_set_source([](host::PadState out[4]) {
      MuStatePod state{};
      linux_host::state_snapshot(&state);
      // @match entries count from the match's start; outside a match there is none (0), so the scene blocks drive the pads.
      if (state.match_frame == 0) s_scene_match_start = 0;
      else if (!s_scene_match_start) s_scene_match_start = std::max<uint32_t>(1, host::retrace_count() - state.match_frame);
      g_script_match_start.store(s_scene_match_start, std::memory_order_relaxed);   // input suggestions' look ahead (tests)
      g_script_scene.store(uint32_t(state.scene_major & 0xFF) << 8 | (state.scene & 0xFF), std::memory_order_relaxed);
      const auto pads = s_scene_script.sample(host::retrace_count(), s_scene_match_start, uint8_t(state.scene_major), uint8_t(state.scene));
      for (unsigned port = 0; port < pads.size(); ++port) {
        out[port].err = pads[port].connected ? 0 : -1;
        out[port].button = pads[port].buttons;
        out[port].stick_x = pads[port].sx; out[port].stick_y = pads[port].sy;
        out[port].sub_x = pads[port].cx; out[port].sub_y = pads[port].cy;
      }
      return true;
    });
  } else if (!play_script.empty()) {
    std::string why;
    if (!s_play_script.load(play_script, &why)) { std::fprintf(stderr, "cannot read %s: %s\n", play_script.c_str(), why.c_str()); return 2; }
    host::input_set_source([](host::PadState out[4]) {
      MuStatePod state{};
      linux_host::state_snapshot(&state);
      MuPadStatus mu[4] = {};
      for (auto& p : mu) p.err = -1;
      if (!s_play_script.apply(host::retrace_count(), state, mu)) return false;
      for (int i = 0; i < 4; ++i) {
        out[i].button = mu[i].button; out[i].stick_x = mu[i].stick_x; out[i].stick_y = mu[i].stick_y; out[i].sub_x = mu[i].sub_x;
        out[i].sub_y = mu[i].sub_y; out[i].trig_l = mu[i].trigger_l; out[i].trig_r = mu[i].trigger_r; out[i].analog_a = mu[i].analog_a;
        out[i].analog_b = mu[i].analog_b; out[i].err = mu[i].err;
      }
      return true;
    });
  }
  gx::init(backend);
  const bool audio = host::audio_open(volume, nullptr, audio_device);
  if (!audio) host::log("audio: device unavailable, continuing silent");

  linux_host::Config config;
  config.iso_path = iso;
  config.game_path = "(embedded)";
  config.log_path = log_file + ".host";
  config.card_dir = profile_card.empty() ? (mu_dir / "Card A").string() : profile_card;
  config.pace = true;
  config.headless = true;          // the host's own SDL window, pads and sound are not built: this program brings them
  config.no_gc_adapter = true;
  config.vanilla_game = vanilla;   // Vanilla: melee-unlocked as the disc plays (MU_GAME_OPTION_VANILLA)
  if (g_play) config.vanilla_game = fe::settings_vanilla();
  if (!script.empty()) {        // a scripted match (tests): the same pads, seed and clock as the parity runner
    config.input_script = script; config.vanilla_game = !script_codes;
    if (!script_clock) { config.has_rng_seed = true; config.rng_seed = 0x12345678; config.has_time_base = true; config.time_base = 0; }
  }
  mac_elf::set_embedded_image(mu_embedded_game, size_t(mu_embedded_game_end - mu_embedded_game));
  bool net_on = false;
  if (net_host::wanted(net) && (menus_online ? std::filesystem::exists(net.user_dir + "/user.json") || net.fake_user : true)) {
    net.out_dir = (mu_dir / "net").string(); fs::create_directories(net.out_dir, ec);
    net.iso_path = iso; net.embedded_image = true;
    if (&fe::features::online_replay_for_report) net_host::set_replay_path_hook(fe::features::online_replay_for_report);   // Slippi's replay upload
    net.discord = discord_on && menus_online;   // Discord Rich Presence for the player's own sign-in (never a test's)
    if (g_play) netplay_bridge::set_suggestion_hooks(suggest_read, mu_net_suggest_setting);   // Slippi 3.7.0's input suggestions, sent
    if (const int start = net_host::start(net, net_say)) {
      host::log("online: the netplay host could not start (%d); playing offline", start);
    } else {
      net_on = true; g_net_on = true;
      fe::set_session_online(true);
      config.input_script.clear();
      host::log("online: on (user folder %s)%s", net.user_dir.c_str(), menus_online ? "; a search starts from the game's Online Play page" : "");
    }
  } else if (menus_online) host::log("online: no Slippi sign-in in %s: the Online Play page will ask to log in", net.user_dir.c_str());
  // The game's calendar from the local clock in play mode (Melee-NX-NetPlay play_main, upstream h_boot_time: saves and Training
  // Mode CE exports were otherwise dated January 1, 2000). Scripted runs keep their fixed clock.
  if (g_play && !config.has_time_base) {
    const std::time_t now = std::time(nullptr); std::tm t{}; localtime_r(&now, &t);
    const int64_t local_seconds = int64_t(timegm(&t));
    if (local_seconds > 946684800) { config.has_time_base = true; config.time_base = uint64_t(local_seconds - 946684800) * uint64_t(MU_TB_HZ); }
  }
  if (g_play && !net_on) config.pace = false;   // paced here (on_retrace), so the replay viewer can fast-forward
  linux_host::set_tables_hook(tables_hook);
  linux_host::set_video_sinks(on_fifo, on_retrace, nullptr);
  int code = 0;
  std::string error;
  if (!linux_host::initialize(config, &error)) {
    host::log("melee-unlocked engine: cannot start: %s", error.c_str());
    std::fprintf(stderr, "cannot start: %s\n", error.c_str());
    code = 1;
  } else {
    if (g_play) fe::settings_report();
    linux_host::set_frame_observer(on_frame, nullptr);
    code = linux_host::run();
    if (g_play) fe::features::call_run_end();   // the game stopped; its memory is still mapped
    linux_host::set_frame_observer(nullptr, nullptr);
    const auto& c = linux_host::counters();
    host::log("melee-unlocked engine: stopped with code %d after %u retraces, %llu FIFO bytes, %llu audio blocks", code, c.retraces,
              (unsigned long long)c.fifo_bytes, (unsigned long long)c.audio_blocks);
  }
  // The sound stops with the game: shutting the netplay host down takes a moment, and the output would only run dry.
  { uint64_t silent_ms = 0; const uint64_t gaps = host::audio_underruns(&silent_ms);
    host::log("audio: %llu frames played, %llu blocks dropped, %llu gaps (%llu ms held)", (unsigned long long)host::audio_pushed_frames(),
              (unsigned long long)host::audio_dropped_blocks(), (unsigned long long)gaps, (unsigned long long)silent_ms); }
  if (audio) host::audio_close();
  net_host::stop_tracking();
  linux_host::shutdown();
  if (net_on) { net_host::report(); net_host::shutdown(); }
  if (g_digest) std::fclose(g_digest);
  fixedmap_release_all();
  gx::init(nullptr);
  host::gcadapter_shutdown();
  host::log("metal: %llu frames presented", (unsigned long long)gx::metal_frames_presented(backend));
  host::window_set_resize_callback(nullptr);
  gx::metal_set_overlay(backend, nullptr);
  gx::metal_set_ui_pass(nullptr);
  if (host::menu_changed()) {   // what the panel changed is the launcher's next start too (launcher.ini, the keys it reads)
    const host::RuntimeSettings s = host::menu_settings();
    fe::app_ini_set("volume", std::to_string(s.volume)); fe::app_ini_set("scale", std::to_string(s.scale));
    fe::app_ini_set("vsync", s.vsync ? "1" : "0"); fe::app_ini_set("fullscreen", s.fullscreen ? "1" : "0");
    fe::app_ini_set("hud", s.hud ? "1" : "0"); fe::app_ini_set("online_delay", std::to_string(std::clamp(s.online_delay, 1, 9)));
    for (const host::ControllerConfig& c : host::controller_configs())   // the CONTROLS page: remaps, deadzones, rumble
      fe::app_ini_set("controller." + c.guid, std::to_string(c.port) + "|" + c.map.serialize());
    fe::app_ini_set("keyboard", host::keyboard_map().serialize());
  }
  if (g_play) fe::mac_controls_save();
  delete backend;
  host::window_destroy();
  if (!g_restart_args.empty()) {   // the replay viewer's Watch: the game takes a replay only at boot, so this program starts again
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
      const std::string a = argv[i];
      if (a == "--replay" && i + 1 < argc) { ++i; continue; }   // the earlier replay's choice is replaced
      args.push_back(a);
    }
    for (const auto& a : g_restart_args) args.push_back(a);
    std::vector<char*> cargs;
    for (auto& a : args) cargs.push_back(a.data());
    cargs.push_back(nullptr);
    char exe[4096]; uint32_t size = sizeof exe;
    if (_NSGetExecutablePath(exe, &size) == 0) execv(exe, cargs.data());
    host::log("play: could not start again: %s", std::strerror(errno));
  }
  return code;
}
