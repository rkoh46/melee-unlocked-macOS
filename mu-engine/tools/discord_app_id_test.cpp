// The Discord Application ID setting (launcher.ini discord_app_id, handed to the presence as MELEE_DISCORD_APP_ID): the
// presence starts with that id; one that is not digits is refused and the app's own id is used. Run with TMPDIR pointing at
// an empty folder, so no Discord app is ever reached (the IPC socket is looked for there).
//   mu-engine/tools/discord_app_id_test.sh
// SPDX-License-Identifier: GPL-3.0-or-later
#include "discord_rpc.h"
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
static std::string g_lines;
namespace host {
void log(const char* fmt, ...) {
  char line[1024]; va_list a; va_start(a, fmt); std::vsnprintf(line, sizeof line, fmt, a); va_end(a);
  g_lines += line; g_lines += "\n"; std::printf("%s\n", line);
}
}
int main(int argc, char** argv) {
  const std::string want = argc > 1 ? argv[1] : "";
  slippi::discord::start(false);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  slippi::discord::stop();
  const bool ok = g_lines.find("application " + want) != std::string::npos;
  std::printf("%s: expected application %s\n", ok ? "PASS" : "FAIL", want.c_str());
  return ok ? 0 : 1;
}
