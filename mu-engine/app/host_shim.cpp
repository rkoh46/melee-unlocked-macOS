// The app-side host services the decoder, renderer, window, controllers, menu and sound output call (port/runtime/host
// host.h), answered for the melee-unlocked engine: console addresses map to its memory (MU_HIGH_BASE + address), the
// log is this program's, and the guest-only services the classic engine has are absent.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host.h"
#include "host_services.h"
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <cstdlib>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <mutex>
#include <pthread.h>
#include "overlay.h"
#include "window.h"

namespace host {
extern const uint32_t mu_gx_ram_size = 0x04000000u;   // main memory (40 MiB) and the game image behind it
namespace {
uint8_t g_scene_major = 0, g_scene_minor = 0;
uint32_t g_retraces = 0;
FILE* g_log = nullptr;
std::mutex g_log_mutex;
const auto g_start = std::chrono::steady_clock::now();
}
uint8_t mu_scene_major() { return g_scene_major; }
uint8_t mu_scene_minor() { return g_scene_minor; }
void mu_shim_set_scene(uint8_t major, uint8_t minor) { g_scene_major = major; g_scene_minor = minor; }
void mu_shim_retrace() { ++g_retraces; }
void mu_shim_open_log(const std::string& path) { g_log = std::fopen(path.c_str(), "w"); }

Options options;
uint8_t* ram = reinterpret_cast<uint8_t*>(linux_host::mem1_base);
uint8_t* aram = nullptr;
ppc::Context* cpu = nullptr;

// host::log is the melee-unlocked host's (host_services.cpp): its lines come to mu_frontend_log below.
uint32_t retrace_count() { return g_retraces; }
double now_seconds() { return std::chrono::duration<double>(std::chrono::steady_clock::now() - g_start).count(); }

// Console addresses: the cached and uncached segments and physical addresses all land in MU's memory.
uint8_t* ptr(uint32_t addr, uint32_t bytes) {
  uint8_t* p = linux_host::memory_pointer(addr, bytes ? bytes : 1);
  if (!p) die("console address %08X+%X is outside the game's memory", addr, bytes);
  return p;
}
uint8_t rd8(uint32_t addr) {
  // The scene bytes the app's input scripts key on (@scene, the classic game's 0x80479D30 / 0x80479D33): melee-unlocked's
  // native game keeps its globals elsewhere, so the frame observer's scene answers for them.
  if (addr == 0x80479D30u) return g_scene_major;
  if (addr == 0x80479D33u) return g_scene_minor;
  const uint8_t* p = linux_host::memory_pointer(addr, 1);
  return p ? *p : 0;
}
[[noreturn]] void die(const char* format, ...) {
  char line[1024];
  va_list args; va_start(args, format); std::vsnprintf(line, sizeof line, format, args); va_end(args);
  log("fatal: %s", line);
  std::abort();
}

namespace { std::atomic<bool> g_exit{false}; std::atomic<int> g_exit_code{0}; }
void request_exit(int code) { g_exit_code = code; g_exit = true; linux_host::request_stop(code); }
int exit_code() { return g_exit_code.load(); }

// The GameCube's GPU interrupts: the native game's GX library does not wait for them.
void set_pe_finish_pending() {}
void set_pe_token_pending(uint16_t) {}

// Timing the classic engine reports in its HUD and log; the melee-unlocked host keeps its own.
void sim_cost_add(int, double) {}
double last_sim_frame_ms() { return 0; }
uint64_t late_frame_count() { return 0; }
void present_feedback(double, double) {}
int online_ping_ms() { return -1; }
const char* latency_warning() { return ""; }
void audio_session_report() {}
void haptic_tap(bool) {}

void input_rumble(int game_port, bool on) {
  if (game_port < 0 || game_port > 3) return;
  gcadapter_rumble(game_port, on);
  window_gamepad_rumble(game_port, on);
}

// As the classic engine does it (port/runtime/host/host.cpp): the render thread asks for a real-time slot.
void thread_realtime(const char* name, double computation_ms, double constraint_ms) {
  if (const char* e = std::getenv("MELEE_REALTIME")) if (*e == '0') return;
  mach_timebase_info_data_t tb; mach_timebase_info(&tb);
  const double ns_per_tick = (double)tb.numer / (double)tb.denom;
  auto ticks = [&](double ms) { return (uint32_t)(ms * 1e6 / ns_per_tick); };
  thread_time_constraint_policy_data_t policy;
  policy.period = ticks(16.667); policy.computation = ticks(computation_ms); policy.constraint = ticks(constraint_ms); policy.preemptible = TRUE;
  const kern_return_t kr = thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&policy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
  log("%s thread: real-time scheduling %s", name, kr == KERN_SUCCESS ? "on" : "unavailable");
}
}  // namespace host

// audio_sdl.cpp's device thread calls the music player here, after the game's sound (at the player's volume) is in `out`:
// melee-unlocked's jukebox when Use custom music is on (fe/features/audio.cpp), else nothing (the game plays its own songs).
namespace host { int audio_volume(); }
extern "C" __attribute__((weak)) void fe_jukebox_mix(int16_t* out, size_t frames, int volume);
namespace slippi { namespace jukebox { void mix(short* out, unsigned long frames) { if (fe_jukebox_mix) fe_jukebox_mix(out, frames, host::audio_volume()); } } }

// Every log line of the host services, the decoder and the renderer: this program's log file and stderr.
extern "C" int mu_frontend_log(const char* line) {
  const double t = host::now_seconds();
  std::lock_guard<std::mutex> lock(host::g_log_mutex);
  std::fprintf(stderr, "[%8.3f] %s\n", t, line);
  if (host::g_log) { std::fprintf(host::g_log, "[%8.3f] %s\n", t, line); std::fflush(host::g_log); }
  return 1;
}
