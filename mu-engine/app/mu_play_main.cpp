// The melee-unlocked engine player for macOS: the app starts it when the melee-unlocked engine is chosen.
//
// melee-unlocked's host services run the game image (an AArch64 Linux ELF, mu-engine/runner) on the main thread,
// paced to the console's 60 Hz. Through the host's frontend hooks (patches/0011) this program gives the game the app's
// controllers (keyboard, SDL gamepads, the GameCube adapter) and plays its sound through the app's audio output; the GX
// command stream goes to the app's decoder and Metal renderer, and the window is the app's SDL3 window. Events are
// pumped at every retrace, as the classic engine does.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host_services.h"
#include "mu_elf_bridge.h"
#include "fixedmap.h"
#include "abi_macos.h"

#include "audio.h"
#include "game_menu.h"
#include "gx_core.h"
#include "gx_metal.h"
#include "host.h"
#include "input_config.h"
#include "window.h"

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

extern "C" const uint8_t mu_embedded_game[], mu_embedded_game_end[];
extern "C" void mu_host_prewrite(void*, size_t) {}   // no write tracking without online play (rollback.cpp has the real one)

namespace fs = std::filesystem;

namespace host {
void mu_shim_set_scene(uint8_t major, uint8_t minor);
void mu_shim_retrace();
void mu_shim_open_log(const std::string& path);
}

namespace {
std::atomic<bool> g_quit{false};
uint32_t g_frames_limit = 0;
FILE* g_digest = nullptr;   // --digest: the parity runner's per-retrace state rows (tests)

std::string home_dir() { const char* h = std::getenv("HOME"); return h ? h : "."; }

void on_fifo(const uint8_t* data, uint32_t size, void*) {
  // The decoder takes the write-gather pipe's stream; whole words go in at once.
  uint32_t i = 0;
  for (; i + 4 <= size; i += 4)
    gx::write_fifo(uint32_t(data[i]) << 24 | uint32_t(data[i + 1]) << 16 | uint32_t(data[i + 2]) << 8 | data[i + 3], 4);
  for (; i < size; ++i) gx::write_fifo(data[i], 1);
}

void on_retrace(void*) {
  host::mu_shim_retrace();
  host::window_pump();
  if (host::window_closed() || g_quit.load()) linux_host::request_stop(0);
}

void on_frame(const MuStatePod& state, const linux_host::Counters& counters, void*) {
  host::mu_shim_set_scene(uint8_t(state.scene_major), uint8_t(state.scene));
  if (g_digest) {
    std::fprintf(g_digest, "%u,%u,%u,%u,%u", counters.retraces, state.scene_major, state.scene, state.match_frame, state.rng);
    for (const auto& f : state.player)
      std::fprintf(g_digest, ",%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u", f.present, f.stocks, f.action, f.anim_frame, f.pos_x, f.pos_y, f.pos_z,
                   f.vel_x, f.vel_y, f.vel_z, f.percent, f.facing);
    std::fputc('\n', g_digest);
  }
  if (g_frames_limit && counters.retraces >= g_frames_limit) linux_host::request_stop(0);
}
} // namespace

// ---- the host services' frontend hooks (patches/0011) ----
extern "C" int mu_frontend_pad_read(MuPadStatus out[4]) {
  host::PadState pads[4];
  host::input_poll(pads);
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

int main(int argc, char** argv) {
  mu_abi_install_crash_report();
  std::string iso, profile_dir, log_file, cache_dir, capture_path, script;
  int volume = 70, scale = 0;
  uint32_t capture_frame = 0;
  bool fullscreen = false, hidden = false, vsync = true;
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
    else if (a == "--hidden") hidden = true;
    else if (a == "--no-vsync") vsync = false;
    else if (a == "--script") script = next();
    else if (a == "--digest") g_digest = std::fopen(next().c_str(), "w");
    else if (a == "--frames") g_frames_limit = uint32_t(std::strtoul(next().c_str(), nullptr, 0));
    else if (a == "--capture") { capture_path = next(); capture_frame = uint32_t(std::strtoul(next().c_str(), nullptr, 0)); }
    else if (a == "--help") {
      std::printf("MeleeUnlockedEngine --iso PATH [--profile-dir DIR] [--log-file F] [--cache-dir DIR] [--volume 0..100]\n"
                  "  [--scale auto|1..8] [--fullscreen] [--no-vsync] [--frames N] [--capture FILE.ppm FRAME] [--script FILE]\n");
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

  // Controller settings the dashboard keeps (launcher.ini): the keyboard layout and each controller's mapping and port.
  {
    std::ifstream in(support / "launcher.ini");
    std::string line;
    while (std::getline(in, line)) {
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
      auto value = [&](const char* key) -> const char* { size_t n = std::strlen(key); return line.compare(0, n, key) == 0 ? line.c_str() + n : nullptr; };
      if (const char* v = value("keyboard=")) host::set_keyboard_map(host::KeyboardMap::parse(v));
      else if (const char* v = value("controller.")) {
        std::string rest = v; size_t eq = rest.find('='), bar = rest.find('|', eq == std::string::npos ? 0 : eq);
        if (eq != std::string::npos && bar != std::string::npos) {
          host::ControllerConfig c; c.guid = rest.substr(0, eq); c.port = std::clamp(std::atoi(rest.c_str() + eq + 1), 0, 4);
          c.map = host::ControllerMap::parse(rest.substr(bar + 1)); host::upsert_controller_config(c);
        }
      }
    }
  }

  gx::MetalOptions gfx;
  gfx.efb_scale = scale; gfx.vsync = vsync; gfx.cache_dir = cache_dir;
  gfx.capture_path = capture_path; gfx.capture_frame = capture_frame;
  void* layer = host::window_create(1280, 960, L"melee-unlocked-macOS", !hidden);
  int client_w = 0, client_h = 0;
  host::window_client_size(&client_w, &client_h);
  gx::Backend* backend = gx::create_metal_backend(layer, client_w, client_h, gfx);
  host::window_set_resize_callback([backend](int w, int h) { gx::metal_resize(backend, w, h); });
  if (fullscreen) host::window_set_fullscreen(true);
  gx::metal_set_overlay(backend, host::game_overlay);
  host::window_input_init();
  gx::init(backend);
  const bool audio = host::audio_open(volume, nullptr, true);
  if (!audio) host::log("audio: device unavailable, continuing silent");

  linux_host::Config config;
  config.iso_path = iso;
  config.game_path = "(embedded)";
  config.log_path = log_file + ".host";
  config.card_dir = (mu_dir / "Card A").string();
  config.pace = true;
  config.headless = true;          // the host's own SDL window, pads and sound are not built: this program brings them
  config.no_gc_adapter = true;
  if (!script.empty()) {        // a scripted match (tests): the same pads, seed and clock as the parity runner
    config.input_script = script; config.vanilla_game = true;
    config.has_rng_seed = true; config.rng_seed = 0x12345678; config.has_time_base = true; config.time_base = 0;
  }
  mac_elf::set_embedded_image(mu_embedded_game, size_t(mu_embedded_game_end - mu_embedded_game));
  linux_host::set_video_sinks(on_fifo, on_retrace, nullptr);
  int code = 0;
  std::string error;
  if (!linux_host::initialize(config, &error)) {
    host::log("melee-unlocked engine: cannot start: %s", error.c_str());
    std::fprintf(stderr, "cannot start: %s\n", error.c_str());
    code = 1;
  } else {
    linux_host::set_frame_observer(on_frame, nullptr);
    code = linux_host::run();
    linux_host::set_frame_observer(nullptr, nullptr);
    const auto& c = linux_host::counters();
    host::log("melee-unlocked engine: stopped with code %d after %u retraces, %llu FIFO bytes, %llu audio blocks", code, c.retraces,
              (unsigned long long)c.fifo_bytes, (unsigned long long)c.audio_blocks);
  }
  linux_host::shutdown();
  if (g_digest) std::fclose(g_digest);
  fixedmap_release_all();
  gx::init(nullptr);
  { uint64_t silent_ms = 0; const uint64_t gaps = host::audio_underruns(&silent_ms);
    host::log("audio: %llu frames played, %llu blocks dropped, %llu gaps (%llu ms held)", (unsigned long long)host::audio_pushed_frames(),
              (unsigned long long)host::audio_dropped_blocks(), (unsigned long long)gaps, (unsigned long long)silent_ms); }
  if (audio) host::audio_close();
  host::gcadapter_shutdown();
  host::log("metal: %llu frames presented", (unsigned long long)gx::metal_frames_presented(backend));
  host::window_set_resize_callback(nullptr);
  gx::metal_set_overlay(backend, nullptr);
  delete backend;
  host::window_destroy();
  return code;
}
