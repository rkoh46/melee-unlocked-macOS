// Host services the renderer, window, controllers, menu and sound output call. The melee-unlocked engine answers them
// (mu-engine/app/host_shim.cpp and the frontend layer); console addresses map to its memory.
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "ppc.h"

namespace host {

struct Options {
  std::string iso;
  std::string state_trace;        // optional per-retrace CPU/RAM/ARAM verification CSV
  std::string log_file;           // console log copy (default melee_port.log in the working directory)
  uint32_t frames = 0;           // stop after N retraces (0 = run until exit)
  bool fast = false;             // no real-time pacing
  bool trace_calls = false;      // log HLE calls
  bool quiet = false;
  uint64_t time_base = 0;        // preset timebase (0 = derive from wall clock like Dolphin)
  bool time_base_set = false;    // explicit preset, including zero, for deterministic validation
  int volume = 0;                // audio output volume percent (0 = muted, the development default)
  double hang_watch = 0.0;       // seconds without a retrace before the guest is declared hung (0 = off)
  std::string sys_dir = "port/slippi_sys";   // Slippi Sys folder (code tables, GameFiles served over the EXI device)
  std::string replay_dir = "replays";        // where .slp recordings are written
  std::string card_dir = "User/GC/CardA";    // memory card slot A as a folder of .gci files
  std::string audio_dump;        // optional WAV file receiving everything the AI DMA plays
  bool offline = false;          // explicitly disables online services in portable launchers
  std::string profile_dir;       // explicit profile location; portable CLI never discovers one
  std::string cache_dir;         // explicit isolated output/cache location for portable launchers
  bool trace_scenes = false;     // raw state-machine transitions for native acceptance evidence
};

extern Options options;
extern uint8_t* ram;             // 24 MB guest RAM
extern uint8_t* aram;            // 16 MB audio RAM (host side)
extern ppc::Context* cpu;

// ---- logging ----
void log(const char* fmt, ...);
[[noreturn]] void die(const char* fmt, ...);

// ---- guest memory (host side, big-endian) ----
uint32_t rd32(uint32_t addr);
uint8_t rd8(uint32_t addr);
void wr32(uint32_t addr, uint32_t v);
void wr16(uint32_t addr, uint16_t v);
void wr8(uint32_t addr, uint8_t v);
uint8_t* ptr(uint32_t addr, uint32_t bytes = 1); // checks the complete RAM span

// ---- disc ----
bool disc_read(uint32_t offset, void* dst, uint32_t size);

// ---- events (interrupt delivery at guest wait points) ----
void set_pe_finish_pending();
void set_pe_token_pending(uint16_t token);
void request_exit(int code);
int exit_code();
uint32_t retrace_count();

// ---- simulation-thread cost accounting (per retrace; logged when a frame exceeds 20 ms) ----
enum SimCost { SIM_DVD, SIM_AX, SIM_JUKEBOX, SIM_EXI, SIM_SNAPSHOT, SIM_QUEUE, SIM_OBSERVE, SIM_RECORD, SIM_RENDER, SIM_TEXTURE, SIM_PUMP, SIM_GPUWAIT, SIM_DRAWABLE, SIM_SAVESTATE, SIM_COST_COUNT };
void sim_cost_add(int slot, double seconds);
double last_sim_frame_ms();            // work time of the most recent simulation frame (sleep excluded)
uint64_t late_frame_count();           // simulation frames that took longer than one 60 Hz period

// ---- time ----
constexpr uint64_t TB_HZ = 40500000ull;   // bus clock / 4
constexpr uint64_t TB_PER_FRAME = TB_HZ / 60;
// Retrace pacing multiplier (Slippi Online nudges it by up to 1% to keep peers in step).
void set_emulation_speed(double speed);
// Display phase lock. The renderer reports, per presented frame, how long the XFB copy took
// to reach the panel; the retrace grid then drifts (at most 0.25 ms per frame, the 60 Hz
// average is untouched) so frames are finished just before a refresh instead of at a random
// phase, which removes most of the wait-for-vblank latency. Reporting the display period
// lets the lock recognise a wrap (a frame that missed its refresh by a hair).
void present_feedback(double latency_ms, double display_period_ms);
// The same policy for any 60 Hz thread (the renderer): `computation_ms` is its typical work per frame.
void thread_realtime(const char* name, double computation_ms, double constraint_ms = 12.0);
// A system notification when the app is not in front (macOS app bundles only; a no-op elsewhere).
void notify_local(const std::string& title, const std::string& body);
// A short note for the performance HUD when the device adds latency the app cannot remove (Low Power Mode, Bluetooth
// audio); empty when nothing applies.
const char* latency_warning();
// Round-trip time to the first remote player in an online match, from Slippi's pad acks; -1 when not connected.
void set_online_ping_ms(int ms);
int online_ping_ms();
// iPhone, iPad, Vision Pro: logs the audio buffer and route the system actually granted (call after opening the device).
void audio_session_report();
double now_seconds();
struct SimCostScope { int slot; double t0; explicit SimCostScope(int s) : slot(s), t0(now_seconds()) {} ~SimCostScope() { sim_cost_add(slot, now_seconds() - t0); } };


// ---- input ----
struct PadState { uint16_t button; int8_t stick_x, stick_y, sub_x, sub_y; uint8_t trig_l, trig_r, analog_a, analog_b; int8_t err; };
void input_poll(PadState out[4]);
// GameCube controller adapter (WUP-028 over WinUSB): fills plugged ports, returns their mask.
uint32_t gcadapter_poll(PadState out[4]);
struct GcAdapterStatus { uint32_t ports = 0; int interval_ms = 0; double report_hz = 0.0; };   // ports: bit per plugged controller
bool gcadapter_status(GcAdapterStatus& out);   // false when no adapter is open
void gcadapter_rumble(int port, bool on);
// PADRecalibrate support: re-capture the adapter's stick/trigger neutrals (-1 = every port).
void gcadapter_recalibrate(int port);
// Rumble routed by game port (the controller that feeds that port, fallback included) or by
// local player: during an online match the game's ports are match slots, not sockets.
void input_rumble(int game_port, bool on);
void gcadapter_shutdown();

}  // namespace host


// Unwind a normal game stop so renderer threads can drain and join.
struct ExitRequested { int code; };
