// Offline test of the installer's link-only path (melee-unlocked's "Open download page" + "Add file..."): a mod the catalog
// does not let us fetch is found in a watched folder (the Mac's ~/Downloads) by its file name, browser copies and
// MediaFire's underscores included, and installed from where it is after the SHA-256 check; a chosen file is installed the
// same way; a file that is not the verified download is refused and nothing is installed. No network, no disc: the mod is a
// save (.gci), so no vanilla disc is needed.
//   mu-engine/tools/mod_watch_test.sh
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mod_install.h"
#include "mi_hash.h"
#include "mi_io.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace modinstall;
static int checks = 0, failed = 0;
#define CHECK(cond) do { ++checks; if (!(cond)) { ++failed; std::printf("FAILED line %d: %s\n", __LINE__, #cond); } } while (0)

static std::string install_and_wait(Manager& m, const std::string& id, const std::string& chosen, bool* ok) {
  std::string why;
  *ok = chosen.empty() ? m.install(id, &why) : m.install_file(id, chosen, &why);
  if (!*ok) return why;
  m.wait();
  const ModStatus s = m.status(id);
  *ok = !s.failed && s.state == State::Installed;
  return s.line;
}

int main(int argc, char** argv) {
  const std::string root = argc > 1 ? argv[1] : "/tmp/mod_watch_test";
  std::system(("rm -rf '" + root + "'").c_str());
  const std::string mods = join(root, "mods"), downloads = join(root, "Downloads"), elsewhere = join(root, "elsewhere");
  make_dirs(mods); make_dirs(downloads); make_dirs(elsewhere);
  std::string payload(8192, '\0');
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = char((i * 131 + 7) & 0xFF);
  const std::string probe = join(root, "probe.gci");
  write_text(probe, payload);
  const std::string sha = sha256_file(probe);
  const std::string catalog = R"({"version":2,"mods":[{"id":"linked","name":"Linked Mod","page":"https://example.org/linked",
    "repo":"someone/linked","tag":"1","asset_pattern":"LINKED MOD - PATCH 1.0.gci","kind":"gci","one_click":false,
    "version":"1.0","url":"https://www.mediafire.com/file/x/LINKED_MOD_-_PATCH_1.0.gci/file","sha256":")" + sha + R"(",
    "policy":"link_only"}]})";
  Config c;
  c.mods_dir = mods; c.disc_path = join(root, "none.iso"); c.catalog_json = catalog; c.watch_dirs = {downloads};
  Manager m(c);
  CHECK(m.catalog_ok());

  // Nothing downloaded yet: not ready, and Install says where to get it.
  ModStatus s = m.status("linked");
  CHECK(!s.automatic && !s.file_ready && s.found_path.empty());
  bool ok = false;
  std::string line = install_and_wait(m, "linked", "", &ok);
  std::printf("nothing downloaded: %s\n", line.c_str());
  CHECK(!ok && line.find("https://example.org/linked") != std::string::npos);

  // An unfinished download is not taken; MediaFire's name with a browser copy number is.
  write_text(join(downloads, "LINKED_MOD_-_PATCH_1.0.gci.crdownload"), payload.substr(0, 100));
  CHECK(m.status("linked").found_path.empty());
  const std::string copy = join(downloads, "LINKED_MOD_-_PATCH_1.0 (1).gci");
  write_text(copy, payload);
  s = m.status("linked");
  std::printf("found: %s\n", s.found_path.c_str());
  CHECK(s.file_ready && s.found_path == copy);
  line = install_and_wait(m, "linked", "", &ok);
  std::printf("install from Downloads: %s\n", line.c_str());
  CHECK(ok && file_exists(join(join(mods, "linked"), "LINKED MOD - PATCH 1.0.gci")));
  CHECK(file_exists(copy));   // the player's file stays where it is
  CHECK(m.remove("linked"));

  // A file with the right name but other bytes: refused, nothing installed.
  remove_file(copy);
  std::string bad = payload; bad[0] ^= 1;
  write_text(join(downloads, "LINKED MOD - PATCH 1.0.gci"), bad);
  CHECK(m.status("linked").file_ready);
  line = install_and_wait(m, "linked", "", &ok);
  std::printf("wrong bytes: %s\n", line.c_str());
  CHECK(!ok && line.find("not Linked Mod's verified download") != std::string::npos && m.status("linked").state == State::NotInstalled);
  remove_file(join(downloads, "LINKED MOD - PATCH 1.0.gci"));

  // Choose File: any name, anywhere, checked by its bytes.
  const std::string chosen = join(elsewhere, "renamed.gci");
  write_text(chosen, payload);
  line = install_and_wait(m, "linked", chosen, &ok);
  std::printf("chosen file: %s\n", line.c_str());
  CHECK(ok && m.status("linked").state == State::Installed && file_exists(chosen));
  CHECK(m.remove("linked"));
  write_text(chosen, bad);
  line = install_and_wait(m, "linked", chosen, &ok);
  std::printf("chosen wrong file: %s\n", line.c_str());
  CHECK(!ok && m.status("linked").state == State::NotInstalled);

  std::system(("rm -rf '" + root + "'").c_str());
  std::printf("%d checks, %d failed\n", checks, failed);
  return failed ? 1 : 0;
}
