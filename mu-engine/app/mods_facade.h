// The Mac launcher's view of the mod installer (mu-engine/modinstall): mods_facade.cpp, linked into the launcher
// (port/runtime/host/mu_launcher.mm).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>

namespace mu_mods {
struct Row {
  std::string id, name, version, note, line;
  std::string page;          // the mod's official download page
  std::string found;         // its download, found on disk (mods/Downloads or ~/Downloads); empty when none
  int state = 0;             // 0 not installed, 1 installed, 2 update available, 3 working
  int percent = -1;
  bool automatic = false;    // Install downloads it; otherwise the player puts the file in Downloads
  bool file_ready = false, failed = false;
};
void configure(const std::string& mods_dir, const std::string& disc_path);
std::vector<Row> rows();
bool install(const std::string& id, std::string* why);
bool install_file(const std::string& id, const std::string& path, std::string* why);   // Choose File
bool remove(const std::string& id, std::string* why);
bool busy();
void cancel();
std::string catalog_error();
std::string downloads_dir();
}
