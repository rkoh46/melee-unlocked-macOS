// melee-unlocked's content fingerprint of a mod disc layer (its port/app/source_host.cpp iso_layer_fingerprint, over
// port/runtime/host/layer_fingerprint.h and source_mod_overlay.cpp ModOverlay::add_iso), reimplemented on two disc files so
// the Mac's mod layer (fe/mods.cpp) recognises the packs upstream recognises by it: the 2D-style m-ex build, whose camera and
// HUD codes the game runs natively when MU_GAME_OPTION2_ARCADE_PACK is set.
//
// The same bytes in the same order as upstream: the files the mod disc changes against the retail disc (lower-case paths
// from "/", sorted; a file of the same length and bytes is unchanged; a changed movie over 256 MB is left out), each as
// "path\tlength\n" and its bytes; then the changed runs of main.dol ("dol offset length\n", runs closer than 32 bytes merged,
// over the extent its header gives, up to the retail one's); then "deleted path\n" for each retail file the mod disc lacks.
// SHA-256, lower-case hex. Display and option selection only.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <CommonCrypto/CommonDigest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fe {
namespace {
uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
struct Entry { uint64_t offset = 0, length = 0; };
struct Disc {
  std::ifstream in;
  uint64_t size = 0;
  std::map<std::string, Entry> files;   // lower-case paths from "/"
  std::set<std::string> names;          // files and folders
  uint64_t dol_offset = 0, dol_extent = 0;
  bool read(uint64_t at, void* out, size_t n) {
    if (at + n > size) return false;
    in.clear(); in.seekg(std::streamoff(at)); in.read(static_cast<char*>(out), std::streamsize(n));
    return bool(in);
  }
  bool open(const std::string& path, std::string* error) {
    in.open(path, std::ios::binary);
    if (!in) { *error = "cannot open " + path; return false; }
    in.seekg(0, std::ios::end); size = uint64_t(in.tellg());
    uint8_t h[0x440];
    if (!read(0, h, sizeof h)) { *error = "cannot read the header of " + path; return false; }
    if (std::memcmp(h, "GALE01", 6) && std::memcmp(h, "GTME01", 6) && std::memcmp(h, "GALERM", 6)) { *error = path + " is not a GALE01 disc"; return false; }
    const uint64_t fst_offset = be32(h + 0x424), fst_size = be32(h + 0x428);
    if (fst_size < 12 || fst_size > 2u * 1024u * 1024u || fst_offset + fst_size > size) { *error = "bad filesystem table in " + path; return false; }
    std::vector<uint8_t> fst(fst_size);
    if (!read(fst_offset, fst.data(), fst.size())) { *error = "cannot read the filesystem table of " + path; return false; }
    const uint32_t count = be32(fst.data() + 8);
    if (count < 1 || count > 4096 || uint64_t(count) * 12 > fst_size) { *error = "bad filesystem entries in " + path; return false; }
    const size_t names_start = size_t(count) * 12;
    std::vector<std::pair<uint32_t, std::string>> dirs{{count, ""}};
    for (uint32_t i = 1; i < count; ++i) {
      while (dirs.size() > 1 && i >= dirs.back().first) dirs.pop_back();
      const uint8_t* e = fst.data() + size_t(i) * 12;
      const uint32_t name_at = be32(e) & 0xFFFFFFu;
      if (name_at >= fst_size - names_start) { *error = "bad file name in " + path; return false; }
      const char* s = reinterpret_cast<const char*>(fst.data() + names_start + name_at);
      const char* end = static_cast<const char*>(std::memchr(s, 0, fst_size - names_start - name_at));
      if (!end) { *error = "bad file name in " + path; return false; }
      std::string name(s, end);
      std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
      const std::string full = dirs.back().second + "/" + name;
      names.insert(full);
      if (e[0]) { dirs.push_back({be32(e + 8), full}); continue; }
      files[full] = Entry{be32(e + 4), be32(e + 8)};
    }
    dol_offset = be32(h + 0x420);
    uint8_t dol[0x100];
    if (dol_offset + sizeof dol <= size && read(dol_offset, dol, sizeof dol)) {
      uint64_t extent = sizeof dol;
      for (int i = 0; i < 18; ++i) extent = std::max<uint64_t>(extent, uint64_t(be32(dol + i * 4)) + be32(dol + 0x90 + i * 4));
      dol_extent = extent;
    }
    return true;
  }
};
struct Sha {
  CC_SHA256_CTX c;
  Sha() { CC_SHA256_Init(&c); }
  void update(const void* p, size_t n) { CC_SHA256_Update(&c, p, CC_LONG(n)); }
  void update(const std::string& s) { update(s.data(), s.size()); }
  std::string hex() { uint8_t d[32]; CC_SHA256_Final(d, &c); char t[65]; for (int i = 0; i < 32; ++i) std::snprintf(t + i * 2, 3, "%02x", d[i]); return t; }
};
}  // namespace

std::string iso_layer_fingerprint(const std::string& base_iso, const std::string& mod_iso, std::string* error) {
  std::string why;
  Disc base, mod;
  if (!base.open(base_iso, &why) || !mod.open(mod_iso, &why)) { if (error) *error = why; return {}; }
  std::vector<uint8_t> a(65536), b(65536);
  struct Changed { std::string path; Entry e; };
  std::vector<Changed> changed;
  for (const auto& [path, e] : mod.files) {
    const auto it = base.files.find(path);
    bool identical = false;
    if (it != base.files.end() && it->second.length == e.length) {
      identical = true;
      for (uint64_t pos = 0; pos < e.length; pos += a.size()) {
        const size_t n = size_t(std::min<uint64_t>(a.size(), e.length - pos));
        if (!mod.read(e.offset + pos, a.data(), n) || !base.read(it->second.offset + pos, b.data(), n)) { if (error) *error = "cannot compare " + path; return {}; }
        if (std::memcmp(a.data(), b.data(), n)) { identical = false; break; }
      }
    }
    if (identical) continue;
    if (e.length > 256u * 1024u * 1024u) {
      if (path.size() > 4 && path.compare(path.size() - 4, 4, ".mth") == 0) continue;
      if (error) *error = "replacement file too large: " + path;
      return {};
    }
    changed.push_back({path, e});
  }
  // std::map already orders paths as upstream's sort does (byte order).
  Sha total;
  for (const auto& c : changed) {
    total.update(c.path + "\t" + std::to_string(c.e.length) + "\n");
    for (uint64_t pos = 0; pos < c.e.length; pos += a.size()) {
      const size_t n = size_t(std::min<uint64_t>(a.size(), c.e.length - pos));
      if (!mod.read(c.e.offset + pos, a.data(), n)) { if (error) *error = "cannot read " + c.path; return {}; }
      total.update(a.data(), n);
    }
  }
  if (base.dol_extent && mod.dol_extent && mod.dol_offset + mod.dol_extent <= mod.size && mod.dol_extent <= 64u * 1024u * 1024u) {
    const uint64_t common = std::min(mod.dol_extent, base.dol_extent);
    int64_t run_start = -1; uint64_t last = 0;
    std::vector<std::pair<uint64_t, uint64_t>> runs;
    for (uint64_t pos = 0; pos < common; pos += a.size()) {
      const size_t n = size_t(std::min<uint64_t>(a.size(), common - pos));
      if (!mod.read(mod.dol_offset + pos, a.data(), n) || !base.read(base.dol_offset + pos, b.data(), n)) break;
      for (size_t k = 0; k < n; ++k) {
        if (a[k] == b[k]) continue;
        const uint64_t at = pos + k;
        if (run_start >= 0 && at - last > 32) { runs.push_back({uint64_t(run_start), last + 1 - uint64_t(run_start)}); run_start = -1; }
        if (run_start < 0) run_start = int64_t(at);
        last = at;
      }
    }
    if (run_start >= 0) runs.push_back({uint64_t(run_start), last + 1 - uint64_t(run_start)});
    for (const auto& r : runs) total.update("dol " + std::to_string(uint32_t(r.first)) + " " + std::to_string(uint32_t(r.second)) + "\n");
  }
  for (const auto& [path, e] : base.files)
    if (!mod.names.count(path)) total.update("deleted " + path + "\n");
  return total.hex();
}
}  // namespace fe

#ifdef MU_LAYER_FINGERPRINT_MAIN
// mu-engine/tools/layer_fingerprint_test.sh: layer_fingerprint BASE.iso MOD.iso
int main(int argc, char** argv) {
  if (argc != 3) { std::fprintf(stderr, "usage: layer_fingerprint BASE.iso MOD.iso\n"); return 2; }
  std::string error;
  const std::string f = fe::iso_layer_fingerprint(argv[1], argv[2], &error);
  if (f.empty()) { std::fprintf(stderr, "%s\n", error.c_str()); return 1; }
  std::printf("%s\n", f.c_str());
  return 0;
}
#endif
