// melee-unlocked's Lab view on the Mac (its lab_view.cpp from the fetched tree, gfx/gen_gfx.sh): the match drawn the way
// Slippi Lab draws a replay (silhouettes, stick figures, capsules or blobs on a plain stage), over the game picture in the
// engine's ImGui pass. The Source Port path upstream takes: the game image's geometry snapshot (mu_look_snapshot_v1,
// mu_look_items_v1, mu_look_extras_v1, found with mu-engine/patches/0016's fe_game_symbol) read at the end of each retrace
// on the simulation thread. Read only: it cannot change the match. Settings: fe/features/graphics.cpp (labview, Ctrl+F3).
// The silhouette packs (Lab/<character>.lab) are optional: the stick figure, capsule and blob looks draw from the fighters'
// hurtboxes and need nothing. As Melee-NX-NetPlay 0.4.8 (its src/lab.cpp, same author) and upstream (host/updater.cpp
// lab_packs_download) do, they come from melee-unlocked's content release when the silhouette look finds none, or from the
// Picture page's "Lab fighters download" row: the zip checked against its pinned SHA-256 (fe_proai_fetch_verified, the app's
// libcurl), its 27 <internal id>.lab packs unpacked with the mod installer's archive reader, the zip deleted.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "lab_view.h"
#include "mu_look_snapshot.h"   // melee-unlocked port/runtime/abi
#include "gfx_live.h"
#include "gfx_host_shim.h"

#include "mi_archive.h"   // mu-engine/modinstall

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// The app's download (fe/proai/proai_fetch_curl.cpp, SHA-256 checked).
__attribute__((weak)) bool fe_proai_fetch_verified(const std::string& url, const std::string& sha256, const std::string& dest,
                                                   std::atomic<uint64_t>* progress, std::string* why);

namespace linux_host { void* fe_game_symbol(const char* name); }   // mu-engine/patches/0016

std::string mu_lab_dir() { return (mu_gfx_folder() / "Lab").string(); }
bool mu_lab_spectate() { return false; }   // upstream's room match window: not on the Mac
float mu_lab_picture_aspect() { return 4.0f / 3.0f; }

namespace fe::gfx {
namespace {
void (*g_snapshot)(MuLookSnapshot*) = nullptr;
void (*g_items)(MuLookItems*) = nullptr;
void (*g_extras)(MuLookExtras*) = nullptr;
bool g_looked = false;

// ---- the fighter packs: melee-unlocked's content release (from Melee-NX-NetPlay's src/lab.cpp) ----
constexpr int kLabPacks = 27;
constexpr const char* kLabUrl = "https://github.com/hero88go/melee-unlocked/releases/download/content-1/MeleeUnlocked-Lab.zip";
constexpr const char* kLabSha256 = "6fcb60586ee0b73a42b3b7521546b554d3b105299bc97d74720b849075febb75";
std::thread lab_fetch;
std::atomic<int> lab_fetch_state{0};   // 0 idle, 1 running, 2 done, 3 failed
std::atomic<uint64_t> lab_fetch_bytes{0};
std::mutex lab_fetch_mutex;
std::string lab_fetch_message;
void lab_message(const std::string& m) {
  { std::lock_guard<std::mutex> lock(lab_fetch_mutex); lab_fetch_message = m; }
  lab::set_notice(m);
}
bool packs_present(const std::string& dir) {
  std::error_code ec;
  for (int i = 0; i < kLabPacks; ++i) if (!std::filesystem::exists(std::filesystem::path(dir) / (std::to_string(i) + ".lab"), ec)) return false;
  return true;
}
void fetch_packs(std::string dir) {
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  const std::string zip = dir + "/MeleeUnlocked-Lab.zip";
  // Tests only: MU_LAB_URL names a stand-in for the release on the loopback (with MELEE_PRO_AI_TEST_MIRROR, which lets the app's
  // download take plain HTTP there), and MU_LAB_SHA256 that stand-in's hash; both are ignored for any other address.
  const char* test_url = std::getenv("MU_LAB_URL");
  const bool test = test_url && std::string(test_url).rfind("http://127.0.0.1:", 0) == 0 && std::getenv("MELEE_PRO_AI_TEST_MIRROR");
  const char* test_sha = std::getenv("MU_LAB_SHA256");
  const std::string url = test ? test_url : kLabUrl, sha = test && test_sha ? test_sha : kLabSha256;
  std::string why;
  host::log("lab view: downloading the fighters (about 85 MB) from %s", test ? url.c_str() : "melee-unlocked's content release");
  lab_message("Downloading the Lab view's fighters (about 85 MB)...");
  if (!fe_proai_fetch_verified(url, sha, zip, &lab_fetch_bytes, &why)) {
    lab_message("Lab view fighters: download failed (" + why + ")");
    host::log("lab view: download failed: %s", why.c_str());
    lab_fetch_state.store(3);
    return;
  }
  lab_message("Unpacking the Lab view's fighters...");
  std::string error;
  auto archive = modinstall::open_archive(zip, &error);
  int placed = 0;
  if (archive) {
    const auto& entries = archive->entries();
    for (size_t i = 0; i < entries.size(); ++i) {
      const std::string& name = entries[i].name;
      const size_t slash = name.find_last_of('/');
      const std::string base = slash == std::string::npos ? name : name.substr(slash + 1);
      if (entries[i].directory || base.size() < 5 || base.substr(base.size() - 4) != ".lab") continue;
      auto in = archive->open(i, &error);
      if (!in) break;
      const std::string out_path = dir + "/" + base, part = out_path + ".part";
      FILE* out = std::fopen(part.c_str(), "wb");
      if (!out) { error = "cannot write " + part; break; }
      std::vector<uint8_t> buffer(1 << 16);
      long n = 0; bool ok = true;
      while ((n = in->read(buffer.data(), buffer.size())) > 0) ok = ok && std::fwrite(buffer.data(), 1, size_t(n), out) == size_t(n);
      ok = std::fclose(out) == 0 && ok && n == 0;
      if (!ok) { std::remove(part.c_str()); error = in->error().empty() ? "cannot write " + out_path : in->error(); break; }
      std::remove(out_path.c_str());
      std::rename(part.c_str(), out_path.c_str());
      ++placed;
    }
  }
  std::remove(zip.c_str());
  if (!packs_present(dir)) {
    lab_message("Lab view fighters: the download is incomplete (" + (error.empty() ? std::to_string(placed) + " packs" : error) + ")");
    host::log("lab view: unpacking failed: %s (%d packs placed)", error.c_str(), placed);
    lab_fetch_state.store(3);
    return;
  }
  host::log("lab view: %d fighter packs installed in %s", placed, dir.c_str());
  lab_message("");
  lab::packs_changed();
  lab_fetch_state.store(2);
}
}  // namespace

void lab_download() {
  if (lab_fetch_state.load() == 1) return;
  if (lab_fetch.joinable()) lab_fetch.join();
  if (!&fe_proai_fetch_verified) { lab_message("This build has no downloads"); lab_fetch_state.store(3); return; }
  lab_fetch_state.store(1);
  lab_fetch = std::thread(fetch_packs, mu_lab_dir());
}
std::string lab_status() {
  const int state = lab_fetch_state.load();
  if (state == 1) return "Downloading: " + std::to_string(lab_fetch_bytes.load() >> 20) + " MB";
  if (state == 3) { std::lock_guard<std::mutex> lock(lab_fetch_mutex); return lab_fetch_message; }
  return packs_present(mu_lab_dir()) ? std::string("Installed") : std::string("Not installed (about 85 MB)");
}
void lab_end() { if (lab_fetch.joinable()) lab_fetch.join(); }

void lab_settings_changed() {
  auto& l = live();
  lab::set_style(l.lab_fighters.load(), l.lab_stage.load());
  uint32_t players[4];
  for (int i = 0; i < 4; ++i) players[i] = l.lab_colour[i].load();
  lab::set_colors(players, l.lab_background.load(), l.lab_stage_colour.load());
  lab::set_enabled(l.lab_view.load());
}

// Simulation thread, once per retrace: the frame the view draws.
void lab_retrace() {
  if (!live().lab_view.load(std::memory_order_relaxed) && !lab::styles_on()) return;
  if (!g_looked) {
    g_looked = true;
    g_snapshot = reinterpret_cast<void (*)(MuLookSnapshot*)>(linux_host::fe_game_symbol("mu_look_snapshot_v1"));
    g_items = reinterpret_cast<void (*)(MuLookItems*)>(linux_host::fe_game_symbol("mu_look_items_v1"));
    g_extras = reinterpret_cast<void (*)(MuLookExtras*)>(linux_host::fe_game_symbol("mu_look_extras_v1"));
    host::log("lab view: the game's geometry snapshot %s", g_snapshot ? "found" : "is missing from the game image (the view stays off)");
  }
  // The silhouette look found no pack: fetched once a run, as upstream does (MELEE_LAB_NO_DOWNLOAD=1 never fetches).
  static std::atomic<bool> missing{false};
  static const bool wired = (lab::set_missing_packs_handler([](const std::string&) { missing.store(true); }), true);
  (void)wired;
  static bool tried = false;
  if (missing.load() && !tried && live().lab_fighters.load() == 0 && !std::getenv("MELEE_LAB_NO_DOWNLOAD")) { tried = true; lab_download(); }
  if (!g_snapshot || !lab::active()) return;
  static MuLookSnapshot snapshot; static MuLookItems items; static MuLookExtras extras;   // large: kept off the stack
  snapshot = MuLookSnapshot{};
  g_snapshot(&snapshot);
  if (g_items) { items = MuLookItems{}; g_items(&items); }
  if (g_extras) { extras = MuLookExtras{}; g_extras(&extras); }
  static const std::array<std::string, 4> names{};
  lab::native_frame(snapshot, g_items ? &items : nullptr, g_extras ? &extras : nullptr, names.data());
}

bool lab_packs_present() {
  std::error_code ec;
  return std::filesystem::exists(std::filesystem::path(mu_lab_dir()) / "2.lab", ec);
}
}  // namespace fe::gfx

// The engine's ImGui pass (ui/ui_metal.mm), before the overlays and the panel: the view covers the window during a match.
void mu_gfx_imgui_draw(int width, int height) {
  lab::draw(fe::gfx::live().lab_view.load(std::memory_order_relaxed), float(width), float(height));
}
