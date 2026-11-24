// Slippi's online menus in the melee-unlocked engine. The game shows them when its host sets the Slippi-menus option
// bit and serves Slippi's copies of thirteen disc files in place of the disc's: ten are the disc's own file with
// Slippi's VCDIFF patch applied, the others Slippi's files. The patches and files come from the bundle's slippi_sys
// folder (port/slippi_sys); the decoder is port/runtime/host/vcdiff.cpp. Nothing here talks to a server.
// Adapted from Melee-NX-NetPlay's experiments/native-engine/frontend/src/slippi_files.cpp (same author).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "slippi_menus.h"
#include "host_services.h"
#include "vcdiff.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace linux_host {   // patches/0011: the host's disc table
int32_t fe_disc_find(const char* path, uint32_t* start, uint32_t* length);
int32_t fe_disc_set(const char* path, uint32_t start, uint32_t length);
bool fe_disc_read_base(uint32_t offset, void* out, uint32_t bytes);
}

namespace slippi_menus {
namespace {
constexpr uint32_t kBase = 0x60000000u, kEnd = 0x70000000u;    // as upstream: past the disc
const char* const kNames[] = {"MnMaAll.usd", "MnMaAll.dat", "SdMenu.usd", "SdMenu.dat", "MnSlMap.usd", "MnSlMap.dat", "SdSlChr.usd", "SdSlChr.dat",
                              "MnExtAll.usd", "MnExtAll.dat", "slpCSS.dat", "GameSetup_gui.dat", "SlippiCSS_gui.dat"};
struct SystemFile { std::string path; uint32_t start; std::vector<uint8_t> data; };
std::vector<SystemFile> files;
std::string sys_dir;
bool on = false;

std::string lower(std::string text) { for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c))); return text; }
bool read_file(const std::string& path, std::vector<uint8_t>& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return true;
}
bool disc_file(const std::string& name, std::vector<uint8_t>& out) {
  uint32_t start = 0, length = 0;
  if (linux_host::fe_disc_find(("/" + lower(name)).c_str(), &start, &length) < 0 || !length) return false;
  out.resize(length);
  return linux_host::fe_disc_read_base(start, out.data(), length);
}
std::vector<uint8_t> build(const std::string& name) {
  std::vector<uint8_t> out, diff, source;
  const std::string base = sys_dir + "/GameFiles/GALE01/";
  if (read_file(base + name, out)) return out;
  if (!read_file(base + name + ".diff", diff)) return {};
  std::string error;
  if (!disc_file(name, source) || !host::vcdiff_decode(source.data(), source.size(), diff.data(), diff.size(), out, &error)) {
    linux_host::log("slippi menus: cannot apply %s.diff (%s)", name.c_str(), source.empty() ? "the file is not on the disc" : error.c_str());
    out.clear();
  }
  return out;
}
}  // namespace

void set_sys_dir(const std::string& dir) { sys_dir = dir; }
bool active() { return on; }

// With the retail disc's table loaded: build the files and serve them over the disc's.
void disc_ready() {
  if (sys_dir.empty()) return;
  files.clear();
  uint32_t next = kBase;
  for (const char* name : kNames) {
    std::vector<uint8_t> data = build(name);
    const bool optional = !std::strcmp(name, "SlippiCSS_gui.dat") || !std::strcmp(name, "slpCSS.dat");   // the game takes whichever of the two exists
    if (data.empty() && optional) continue;
    if (data.empty()) { linux_host::log("slippi menus: off (cannot build %s)", name); files.clear(); return; }
    const uint32_t size = uint32_t(data.size());
    if (uint64_t(next) + size > kEnd) { linux_host::log("slippi menus: off (the files exceed their range)"); files.clear(); return; }
    files.push_back({"/" + lower(name), next, std::move(data)});
    next += (size + 0xFFFFu) & ~0xFFFFu;
  }
  uint32_t replaced = 0, added = 0;
  for (const auto& file : files) {
    if (linux_host::fe_disc_find(file.path.c_str(), nullptr, nullptr) >= 0) ++replaced; else ++added;
    linux_host::fe_disc_set(file.path.c_str(), file.start, uint32_t(file.data.size()));
  }
  on = !files.empty();
  if (on) linux_host::log("slippi menus: on, %u disc files replaced by Slippi's copies, %u added", replaced, added);
}

int read(uint64_t offset, void* out, size_t bytes) {
  if (offset < kBase || offset >= kEnd || files.empty()) return -1;
  for (const auto& file : files) {
    if (offset < file.start || offset >= uint64_t(file.start) + ((file.data.size() + 0xFFFFu) & ~size_t(0xFFFFu))) continue;
    const uint64_t at = offset - file.start;
    const size_t real = at < file.data.size() ? size_t(std::min<uint64_t>(bytes, file.data.size() - at)) : 0;
    std::memcpy(out, file.data.data() + at, real);
    std::memset(static_cast<uint8_t*>(out) + real, 0, bytes - real);
    return 1;
  }
  return 0;
}
}  // namespace slippi_menus
