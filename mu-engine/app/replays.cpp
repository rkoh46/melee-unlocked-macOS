// Slippi replays in the melee-unlocked engine: every match recorded as a .slp, and playback of a .slp through the engine.
// As melee-unlocked does it (its host: port/app/source_host.cpp, h_replay_event / h_replay_finished for recording,
// h_replay_start / h_replay_frame for playback; port/runtime/hle/native_replay_stream.* writes the file,
// port/third_party/slippilib reads one; its game: shim/mu_replay.c). Adapted from Melee-NX-NetPlay's
// experiments/native-engine/frontend/src/features/replays.cpp (same author): the recording, the playback and the fidelity
// check; its viewer controls, takeover and coach are not here yet.
//
// Recording: the game sends Slippi's events itself (replay_event): Game Start, then the pre-frame and post-frame events of
// every frame, Game End; the Gecko list is the host's to add (melee-unlocked's native_recording_codes.h, generated from the
// fetched tree). Offline and online matches alike. Written when the match ends, on a writer thread, to the replay folder
// (the same one the classic engine writes to) as Game_YYYYMMDDTHHMMSS.slp.
//
// Playback (--replay FILE): the game boots straight into the replay's match and asks for each frame's recorded inputs; the
// option words are the recorded ones. The game's own post-frame records are compared with the file's: the end of the run
// says how many differ (0 is the bar).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "replays.h"
#include "host_services.h"

#include "native_replay_stream.h"
#include "SlippiGame.h"
#if __has_include("native_recording_codes.h")
#include "native_recording_codes.h"
#define MU_HAVE_RECORDING_CODES 1
#else
#define MU_HAVE_RECORDING_CODES 0
#endif

#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unistd.h>

namespace replays {
namespace {
std::string folder;
bool save = true;
uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
void say(const char* format, ...) __attribute__((format(printf, 1, 2)));
void say(const char* format, ...) {
  char line[1024]; va_list args; va_start(args, format); std::vsnprintf(line, sizeof line, format, args); va_end(args);
  linux_host::log("%s", line);
}

// ---- writer thread ----
std::mutex write_mutex;
std::condition_variable write_cv;
std::deque<std::pair<std::string, std::vector<uint8_t>>> write_queue;
std::thread writer;
bool writer_stop = false;
void writer_main() {
  std::unique_lock<std::mutex> lock(write_mutex);
  while (true) {
    write_cv.wait(lock, [] { return writer_stop || !write_queue.empty(); });
    if (write_queue.empty()) { if (writer_stop) return; continue; }
    auto job = std::move(write_queue.front());
    write_queue.pop_front();
    lock.unlock();
    std::string path;
    FILE* out = nullptr;
    for (unsigned suffix = 0; suffix < 1000 && !out; ++suffix) {     // two matches in one second get _1, _2
      path = folder + "/" + job.first + (suffix ? "_" + std::to_string(suffix) : std::string()) + ".slp";
      if (access(path.c_str(), F_OK) == 0) continue;
      out = std::fopen(path.c_str(), "wb");
    }
    bool ok = false;
    if (out) { ok = std::fwrite(job.second.data(), 1, job.second.size(), out) == job.second.size(); ok = std::fclose(out) == 0 && ok; }
    say("replay: %s %s (%zu bytes)", ok ? "recording written to" : "WRITE FAILED for", path.c_str(), job.second.size());
    lock.lock();
  }
}
void queue_write(std::string name, std::vector<uint8_t> bytes) {
  std::error_code ec; std::filesystem::create_directories(folder, ec);
  std::lock_guard<std::mutex> lock(write_mutex);
  if (!writer.joinable()) writer = std::thread(writer_main);
  write_queue.emplace_back(std::move(name), std::move(bytes));
  write_cv.notify_all();
}

// ---- recording ----
slippi::NativeReplayStream stream;
bool written = true;
uint32_t online_games = 0;
void write_recording() {
  if (written || !stream.active()) return;
  written = true;
  if (!save) return;
  std::vector<uint8_t> file = stream.encode();
  if (file.empty()) { say("replay: recording failed: %s", stream.error().c_str()); return; }
  const std::time_t start = stream.start_time() ? std::time_t(stream.start_time()) : std::time(nullptr);
  std::tm tm{}; localtime_r(&start, &tm);
  char name[64]; std::strftime(name, sizeof name, "Game_%Y%m%dT%H%M%S", &tm);
  queue_write(name, std::move(file));
}

// ---- playback ----
struct Playback {
  std::string path;
  std::unique_ptr<Slippi::SlippiGame> game;
  MuReplayStart start{};
  uint32_t words[3] = {};
  std::vector<uint8_t> game_start;
  std::map<std::tuple<int32_t, uint8_t, uint8_t>, std::vector<uint8_t>> post;   // the file's post-frame records
  uint64_t compared = 0, differ = 0; int32_t first_differ = INT32_MIN; int32_t frame = 0;
  std::map<std::tuple<int32_t, uint8_t, uint8_t>, bool> seen_differ;
  bool finished = false;
  bool vanilla = false;   // recorded without a code list: the retail game (a Vanilla session or a scripted match)
} pb;
bool playing = false;
const MuHostApi* g_host = nullptr;

std::vector<uint8_t> read_file(const std::string& path) {
  std::vector<uint8_t> d;
  if (FILE* f = std::fopen(path.c_str(), "rb")) {
    uint8_t block[65536]; size_t n;
    while ((n = std::fread(block, 1, sizeof block, f)) > 0) d.insert(d.end(), block, block + n);
    std::fclose(f);
  }
  return d;
}
// Every event of the file's raw element.
template <typename Fn> bool each_event(const std::vector<uint8_t>& d, Fn fn) {
  static const char tag[] = "raw[$U#l";
  auto it = std::search(d.begin(), d.end(), tag, tag + 8);
  if (it == d.end()) return false;
  size_t i = size_t(it - d.begin()) + 8 + 4;
  if (i + 2 > d.size() || d[i] != 0x35 || d[i + 1] < 1 || (d[i + 1] - 1) % 3) return false;
  int32_t sizes[256]; std::fill(std::begin(sizes), std::end(sizes), -1);
  for (size_t k = 0; k < size_t((d[i + 1] - 1) / 3); ++k) {
    const size_t e = i + 2 + 3 * k;
    if (e + 3 > d.size()) return false;
    sizes[d[e]] = int32_t(uint32_t(d[e + 1]) << 8 | d[e + 2]);
  }
  const uint32_t raw_size = be32(d.data() + i - 4);
  const size_t end = raw_size && raw_size <= d.size() - i ? i + raw_size : d.size();
  for (size_t pos = i + 1 + d[i + 1]; pos < end;) {
    const uint8_t command = d[pos++];
    if (sizes[command] < 0 || size_t(sizes[command]) > end - pos) return false;
    fn(command, d.data() + pos, size_t(sizes[command]));
    pos += size_t(sizes[command]);
  }
  return true;
}
uint32_t read_word(const std::vector<uint8_t>& bytes, int word) {   // muOptions, muOptions2, muOptions3
  static const char k1[] = "U\x09muOptionsl", k2[] = "U\x0AmuOptions2l", k3[] = "U\x0AmuOptions3l";
  const char* key = word == 2 ? k3 : word == 1 ? k2 : k1;
  const size_t len = word == 0 ? sizeof k1 - 1 : sizeof k2 - 1;
  const auto at = std::search(bytes.begin(), bytes.end(), key, key + len);
  if (at == bytes.end() || size_t(bytes.end() - at) < len + 4) return 0;
  return be32(&*(at + std::ptrdiff_t(len)));
}
// The replay's gameplay codes the native game knows, by injection address (upstream's replay_codes, the same table).
uint32_t replay_codes(const std::vector<uint8_t>& list, uint8_t* ps_frozen_toggle, size_t* unknown_out) {
  constexpr uint32_t kUcfShared = 0x80000000u;
  struct Known { uint32_t address, code; };
  static const Known known[] = {
      {0x800D65EC, MU_REPLAY_CODE_UCF084}, {0x800C9A44, kUcfShared}, {0x8006B460, MU_REPLAY_CODE_UCF084}, {0x8008E54C, MU_REPLAY_CODE_UCF084},
      {0x8009A0B8, MU_REPLAY_CODE_UCF084}, {0x800998A4, kUcfShared}, {0x80093294, MU_REPLAY_CODE_UCF084}, {0x800908F4, kUcfShared},
      {0x8016E510, MU_REPLAY_CODE_NEUTRAL_SPAWN}, {0x801239A8, MU_REPLAY_CODE_FREEZE_GLITCH}, {0x801C154C, MU_REPLAY_CODE_INIT_STAGE_DATA},
      {0x80068EEC, MU_REPLAY_CODE_INIT_PLAYER_DATA}, {0x8006A880, MU_REPLAY_CODE_OFFSCREEN_DAMAGE}, {0x800D4C1C, MU_REPLAY_CODE_DEAD_UP_FALL},
      {0x800D4D68, MU_REPLAY_CODE_DEAD_UP_FALL}, {0x80080E80, MU_REPLAY_CODE_DEAD_UP_FALL}, {0x8021AAE4, MU_REPLAY_CODE_FD_BG_SEED},
      {0x801C65C8, MU_REPLAY_CODE_PS_ZERO_BUFFER}, {0x801D4760, MU_REPLAY_CODE_PS_IS_VALID}, {0x801D457C, MU_REPLAY_CODE_PS_FROZEN_CHECK},
      {0x801D4578, MU_REPLAY_CODE_PS_FROZEN_CHECK}, {0x801D14C8, MU_REPLAY_CODE_PS_PRELOAD}, {0x801D45EC, MU_REPLAY_CODE_PS_PRELOAD},
      {0x801D460C, MU_REPLAY_CODE_PS_PRELOAD}, {0x801D4610, MU_REPLAY_CODE_PS_PRELOAD}, {0x801D4724, MU_REPLAY_CODE_PS_PRELOAD},
      {0x801D4F14, MU_REPLAY_CODE_PS_PRELOAD}, {0x800165AC, MU_REPLAY_CODE_PS_FILE_LOAD}, {0x801CD37C, MU_REPLAY_CODE_CASTLE_WIND_RESET},
      {0x801D69B0, MU_REPLAY_CODE_KONGO_LOG_RESET}, {0x8008653C, MU_REPLAY_CODE_WHISPY_FIX}, {0x800AC5B8, MU_REPLAY_CODE_NANA_DETERMINISM},
      {0x801D24FC, MU_REPLAY_CODE_PS_MONITOR}, {0x800DB880, MU_REPLAY_CODE_PREVENT_WOBBLING}, {0x800DBBD4, MU_REPLAY_CODE_PREVENT_WOBBLING},
      {0x8008F090, MU_REPLAY_CODE_PREVENT_WOBBLING},
      {0x80179378, 0}, {0x8017900C, 0}, {0x8017A890, 0}, {0x8000568C, 0}, {0x802F84C8, 0},
  };
  uint32_t codes = 0; size_t unknown = 0, dashback = 0, shield_drop = 0;
  for (size_t idx = 0; idx + 8 <= list.size();) {
    const uint8_t type = list[idx] & 0xFE;
    const uint32_t address = (be32(&list[idx]) & 0x01FFFFFFu) | 0x80000000u;
    const uint32_t word = be32(&list[idx + 4]);
    size_t len = 8;
    if (type == 0xC0 || type == 0xC2) len = 8 + size_t(word) * 8;
    else if (type == 0x08) len = 16;
    else if (type == 0x06) len = 8 + ((word + 7) & ~7u);
    if (list[idx] == 0xFF || idx + len > list.size()) break;
    const Known* k = nullptr;
    for (const auto& e : known) if (e.address == address) { k = &e; break; }
    if (!k) ++unknown;
    else {
      codes |= k->code;
      if (address == 0x800C9A44) dashback = len;
      if (address == 0x800998A4) shield_drop = len;
      if (address == 0x801D457C && len >= 8 + 12) *ps_frozen_toggle = list[idx + 8 + 8];
      if (address == 0x801D4578 && len == 32 && list[idx + 0x18] == 0x4E && list[idx + 0x1B] == 0x20) *ps_frozen_toggle = 1;
    }
    idx += len;
  }
  if (codes & kUcfShared) {
    codes &= ~kUcfShared;
    if (!(codes & MU_REPLAY_CODE_UCF084)) codes |= dashback == 352 && shield_drop == 312 ? MU_REPLAY_CODE_UCF08 : MU_REPLAY_CODE_UCF_OTHER;
  }
  *unknown_out = unknown;
  return codes;
}

bool open_replay(const std::string& path) {
  const std::vector<uint8_t> bytes = read_file(path);
  if (bytes.empty()) { say("replay: cannot read %s", path.c_str()); return false; }
  pb = Playback{};
  pb.path = path;
  for (int w = 0; w < 3; ++w) pb.words[w] = read_word(bytes, w);
  each_event(bytes, [&](uint8_t command, const uint8_t* p, size_t n) {
    if (command == 0x36 && pb.game_start.empty()) pb.game_start.assign(p, p + n);
    else if (command == 0x38 && n >= 6) pb.post[{int32_t(be32(p)), p[4], p[5]}] = std::vector<uint8_t>(p, p + n);
  });
  pb.game = Slippi::SlippiGame::FromFile(path);
  if (!pb.game || !pb.game->AreSettingsLoaded() || pb.game_start.empty()) { say("replay: %s is not a Slippi replay this build reads", path.c_str()); pb = Playback{}; return false; }
  Slippi::GameSettings* s = pb.game->GetSettings();
  std::array<uint32_t, Slippi::GAME_INFO_HEADER_SIZE> header = s->header;
  for (int i = 0; i < 4; ++i) {          // Dolphin's prepareGameInfo: Sheik or Zelda in the header
    if (!pb.game->DoesPlayerExist(int8_t(i))) continue;
    const uint8_t ext = s->players[uint8_t(i)].characterId;
    if (ext == 0x12 || ext == 0x13) header[size_t(24 + 9 * i)] = (header[size_t(24 + 9 * i)] & 0x00FFFFFFu) | uint32_t(ext) << 24;
  }
  for (int i = 0; i < Slippi::GAME_INFO_HEADER_SIZE; ++i)
    for (int b = 0; b < 4; ++b) pb.start.game_info[4 * i + b] = uint8_t(header[size_t(i)] >> (24 - 8 * b));
  pb.start.random_seed = s->randomSeed;
  pb.start.is_pal = s->isPAL;
  const auto version = pb.game->GetVersion();
  pb.start.preload_ps = version[0] > 1 || (version[0] == 1 && version[1] > 2);
  pb.start.frozen_ps = s->isFrozenPS;
  size_t unknown = 0;
  pb.start.codes = replay_codes(s->geckoCodes, &pb.start.ps_frozen_toggle, &unknown);
  pb.vanilla = s->geckoCodes.size() <= 8;
  say("replay: %s, Slippi %s, frames %d..%d, stage %u, codes %08X (%zu others not native), options %08X %08X %08X, %zu post-frame records",
      path.c_str(), pb.game->GetVersionString().c_str(), Slippi::GAME_FIRST_FRAME, pb.game->GetLatestIndex(), unsigned(s->stage),
      pb.start.codes, unknown, pb.words[0], pb.words[1], pb.words[2], pb.post.size());
  return true;
}

const MuReplayStart* h_replay_start() { return playing ? &pb.start : nullptr; }
void h_replay_frame(int32_t frame, MuReplayFrame* out) {
  *out = MuReplayFrame{};
  pb.frame = frame;
  if (!playing || !pb.game->DoesFrameExist(frame)) { out->result = MU_REPLAY_TERMINATE; return; }
  Slippi::FrameData* f = pb.game->GetFrame(frame);
  out->result = MU_REPLAY_CONTINUE;
  out->seed_exists = f->randomSeedExists ? 1 : 0;
  out->seed = f->randomSeed;
  for (uint8_t port = 0; port < 4; ++port)
    for (int follower = 0; follower < 2; ++follower) {
      const auto& source = follower ? f->followers : f->players;
      auto it = source.find(port);
      if (it == source.end()) continue;
      const Slippi::PlayerFrameData& d = it->second;
      MuReplayCharacter& c = out->character[port][follower];
      c.present = 1; c.random_seed = d.randomSeed;
      c.joystick_x = d.joystickX; c.joystick_y = d.joystickY; c.cstick_x = d.cstickX; c.cstick_y = d.cstickY;
      c.trigger = d.trigger; c.buttons = d.buttons;
      c.x = d.locationX; c.y = d.locationY; c.facing = d.facingDirection;
      c.action_state = d.animation; c.percent = d.percent;
      c.joystick_x_raw = d.joystickXRaw; c.joystick_y_raw = d.joystickYRaw; c.cstick_x_raw = d.cstickXRaw; c.cstick_y_raw = d.cstickYRaw;
    }
}
int32_t h_replay_stock_steal(int32_t frame, int32_t port) {
  if (!playing || !pb.game->DoesFrameExist(frame)) return 0;
  return pb.game->GetFrame(frame)->players.count(uint8_t(port)) ? 1 : 0;
}
void playback_event(uint8_t command, const uint8_t* payload, uint32_t size) {
  if (command != 0x38 || size < 6) return;
  const auto key = std::make_tuple(int32_t(be32(payload)), payload[4], payload[5]);
  const auto it = pb.post.find(key);
  if (it == pb.post.end()) return;
  ++pb.compared;
  const size_t n = std::min<size_t>(it->second.size(), size);
  if (std::memcmp(it->second.data(), payload, n) != 0 && !pb.seen_differ[key]) {
    pb.seen_differ[key] = true;
    if (pb.differ < 4) {
      size_t at = 0; while (at < n && it->second[at] == payload[at]) ++at;
      say("replay: post-frame record of frame %d port %u%s differs from the file at byte %zu (%02X, file %02X)", std::get<0>(key),
          unsigned(std::get<1>(key)), std::get<2>(key) ? " (follower)" : "", at, payload[at], it->second[at]);
    }
    if (!pb.differ++) pb.first_differ = std::get<0>(key);
  }
}
void h_replay_event(uint8_t command, const uint8_t* payload, uint32_t size) {
  if (playing) { playback_event(command, payload, size); return; }
  if (command == 0x36) {
    write_recording();                            // a recording never finished (a reset): kept as it is
    std::vector<uint8_t> start(payload, payload + size);
    const bool net = size >= 0x2F8 && (start[0x1A3] == 0x08 || start[0x2BD] != 0);
    std::vector<uint8_t> codes;
#if MU_HAVE_RECORDING_CODES
    if (net) {
      codes.assign(std::begin(native_online_recording_codes), std::end(native_online_recording_codes));
      const uint32_t game = ++online_games;
      for (int b = 0; b < 4; ++b) start[size_t(0x2F0 + b)] = uint8_t(game >> (24 - 8 * b));
    } else if (!(g_host && (g_host->game_options() & MU_GAME_OPTION_VANILLA))) {
      codes.assign(std::begin(native_recording_codes), std::end(native_recording_codes));
    } else
#endif
      codes = {0xFF, 0, 0, 0, 0, 0, 0, 0};
    written = false;
    stream = slippi::NativeReplayStream{};
    const bool vanilla = g_host && (g_host->game_options() & MU_GAME_OPTION_VANILLA);
    stream.set_feature_options(net || vanilla || !g_host ? 0u : g_host->game_options() & 0x00007FF0u);
    stream.set_feature_options2(net || !g_host || !g_host->game_options2 ? 0u : g_host->game_options2());
    stream.set_feature_options3(net || !g_host || !g_host->game_options3 ? 0u : g_host->game_options3());
    if (!stream.begin_slippi(start, codes, int64_t(std::time(nullptr)))) say("replay: cannot start the recording: %s", stream.error().c_str());
    else say("replay: recording %s match", net ? "an online" : "an offline");
    return;
  }
  if (!stream.active() || stream.ended()) return;
  if (!stream.append(command, payload, size)) say("replay: cannot record event %02X: %s", command, stream.error().c_str());
  if (command == 0x39) write_recording();   // at GAME!, on the writer thread
}
void report_check(const char* how) {
  say("replay: playback %s at frame %d: %llu post-frame records compared with the file, %llu differ%s", how, pb.frame,
      (unsigned long long)pb.compared, (unsigned long long)pb.differ, pb.differ ? (" (first at frame " + std::to_string(pb.first_differ) + ")").c_str() : "");
}
void h_replay_finished() {
  if (!playing) { write_recording(); return; }
  if (pb.finished) return;
  pb.finished = true;
  report_check("finished");
  linux_host::request_stop(0);
}
}  // namespace

void configure(const std::string& replay_folder, bool save_matches) { folder = replay_folder; save = save_matches; }
bool start_playback(const std::string& path) {
  playing = open_replay(path);
  return playing;
}
bool active() { return playing; }
bool word(int which, uint32_t* value) {
  if (!playing || which < 0 || which > 2) return false;
  *value = pb.words[which] | (which == 0 && pb.vanilla ? MU_GAME_OPTION_VANILLA : 0u);
  return true;
}
void install(MuHostApi* host) {
  g_host = host;
  // The game records an offline VS match only when MELEE_SOURCE_RECORD is "1" (shim/mu_replay.c reads it with the C
  // library's getenv, which the loader binds to this program's); online matches are always recorded.
  setenv("MELEE_SOURCE_RECORD", save && !playing ? "1" : "0", 1);
  host->replay_start = h_replay_start;
  host->replay_frame = h_replay_frame;
  host->replay_stock_steal = h_replay_stock_steal;
  host->replay_event = h_replay_event;
  host->replay_finished = h_replay_finished;
  say("replay: %s; recordings go to %s%s", playing ? "playing a replay" : save ? "recording every match" : "not recording", folder.c_str(),
      MU_HAVE_RECORDING_CODES ? "" : " (this build has no Slippi code list: recordings carry an empty one)");
}
void finish() {
  if (!playing) write_recording();
  else if (!pb.finished) report_check("stopped");
  { std::unique_lock<std::mutex> lock(write_mutex); writer_stop = true; write_cv.notify_all(); }
  if (writer.joinable()) writer.join();
  writer_stop = false;
}
}  // namespace replays
