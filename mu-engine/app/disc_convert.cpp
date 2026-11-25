// RVZ and WIA discs in the Mac app's engine, as Melee-NX-NetPlay's frontend/src/app.cpp app_disc_convert has it (same author):
// melee-unlocked's port/app/disc_rvz.cpp and the bzip2 it vendors, compiled from the fetched tree (GPL-2.0-or-later; LZMA from
// the LZMA SDK objects the mod installer links, Zstandard from melee-unlocked's vendored zstd). Upstream's launcher turns a
// dropped .rvz or .wia into the plain disc image once; here the engine does it at start when the disc it is given is one: the
// image is checked first (a GameCube disc, GALE01), the result is GALE01.iso beside it, or in the engine's own folder when that
// folder cannot be written (written as .part and renamed when complete, upstream's rule), and the compressed file is left as it
// was. Later starts find the plain image and use it at once.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "disc_rvz.h"   // fetched tree, port/app

#include <chrono>
#include <filesystem>
#include <string>
#include <unistd.h>

namespace host { void log(const char* format, ...) __attribute__((format(printf, 1, 2))); }

bool mu_disc_convert(std::string* iso, const std::string& own_folder) {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path given = fs::u8path(*iso);
  if (!fs::is_regular_file(given, ec)) return false;
  if (disc_rvz::detect(given) == disc_rvz::Format::NotCompressed) return true;   // a plain image
  fs::path folder = given.parent_path();
  if (folder.empty() || ::access(folder.c_str(), W_OK) != 0) folder = fs::u8path(own_folder);
  const fs::path target = folder / "GALE01.iso";
  if (fs::is_regular_file(target, ec) && disc_rvz::detect(target) == disc_rvz::Format::NotCompressed) {
    host::log("disc: %s was converted before: playing %s", iso->c_str(), target.u8string().c_str());
    *iso = target.u8string();
    return true;
  }
  disc_rvz::Info info;
  std::string error;
  if (!disc_rvz::probe(given, &info, &error)) { host::log("disc: %s cannot be read as a disc image: %s", iso->c_str(), error.c_str()); return false; }
  if (!info.gamecube || info.game_id != "GALE01") {
    host::log("disc: %s is %s (%s), not Super Smash Bros. Melee NTSC", iso->c_str(), info.game_id.c_str(), info.gamecube ? "GameCube" : "not a GameCube disc");
    return false;
  }
  host::log("disc: converting %s (%s, %s compression, revision %d) into %s (%.2f GB), once", iso->c_str(),
            info.format == disc_rvz::Format::Rvz ? "RVZ" : "WIA", info.compression.c_str(), info.revision, target.u8string().c_str(),
            double(info.iso_size) / 1e9);
  const auto t0 = std::chrono::steady_clock::now();
  int last_tenth = -1;
  const bool ok = disc_rvz::convert(given, target, [&](uint64_t done, uint64_t total) {
    const int tenth = total ? int(done * 10 / total) : 0;
    if (tenth != last_tenth) { last_tenth = tenth; host::log("disc: converting: %d%%", tenth * 10); }
    return true;
  }, &error);
  const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  if (!ok) { host::log("disc: the conversion failed after %.0f s: %s", seconds, error.c_str()); return false; }
  host::log("disc: %s written in %.0f s", target.u8string().c_str(), seconds);
  *iso = target.u8string();
  return true;
}
