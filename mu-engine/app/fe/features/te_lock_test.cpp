// --feature-test te-lock (verification runs, mu-engine/tools/app_te_test.sh lock): 20XX TE's "lock settings" rule as the game's
// TE menu meets it through command 0xF8 (fe/mods.cpp): with the lock on, a change from TE's menu is refused; turning the lock
// off in the menu is taken; then a change goes through. Driven from the first match frame of a session with the TE save as its
// profile; read and logged, nothing else. SPDX-License-Identifier: GPL-3.0-or-later
#include "features_internal.h"
#include "mu_host.h"
#include "frontend.h"

#include <cstdint>
#include <cstring>

extern "C" int32_t mu_frontend_command(uint8_t command, const uint8_t* payload, uint32_t size, uint8_t* response, uint32_t capacity, uint32_t* response_size);
namespace fe::features {
namespace {
bool done = false;
bool f8(const uint32_t* words, uint32_t* w1, uint32_t* w2) {   // words null: only read them
  uint8_t in[8], out[8]; uint32_t got = 0;
  for (int i = 0; words && i < 2; ++i) for (int b = 0; b < 4; ++b) in[i * 4 + b] = uint8_t(words[i] >> (24 - 8 * b));
  if (mu_frontend_command(0xF8, words ? in : nullptr, words ? 8 : 0, out, sizeof out, &got) != 0 || got != 8) return false;
  *w1 = uint32_t(out[0]) << 24 | uint32_t(out[1]) << 16 | uint32_t(out[2]) << 8 | out[3];
  *w2 = uint32_t(out[4]) << 24 | uint32_t(out[5]) << 16 | uint32_t(out[6]) << 8 | out[7];
  return true;
}
void retrace(const Retrace& info) {
  if (done || info.retrace < 120 || !feature_test("te-lock")) return;
  done = true;
  uint32_t w1 = 0, w2 = 0;
  if (!f8(nullptr, &w1, &w2)) { say("te-lock: command 0xF8 was not answered (no TE save in this profile?)"); return; }
  say("te-lock: words %08X %08X, lock %s", w1, w2, (w2 & MU_GAME_OPTION2_TE_LOCK_SETTINGS) ? "on" : "off");
  const uint32_t change[2] = {w1 ^ MU_GAME_OPTION_TE_FIXED_CAMERA, w2};
  uint32_t a1, a2; f8(change, &a1, &a2);
  say("te-lock: TE's menu asks for fixed camera %s: %s", (change[0] & MU_GAME_OPTION_TE_FIXED_CAMERA) ? "on" : "off", a1 == w1 ? "refused (locked)" : "taken");
  const uint32_t unlock[2] = {a1, a2 & ~MU_GAME_OPTION2_TE_LOCK_SETTINGS};
  uint32_t b1, b2; f8(unlock, &b1, &b2);
  say("te-lock: TE's menu turns the lock off: lock now %s", (b2 & MU_GAME_OPTION2_TE_LOCK_SETTINGS) ? "on" : "off");
  const uint32_t again[2] = {b1 ^ MU_GAME_OPTION_TE_FIXED_CAMERA, b2};
  uint32_t c1, c2; f8(again, &c1, &c2);
  say("te-lock: the same change again: %s", c1 != b1 ? "taken (unlocked)" : "refused");
}
}  // namespace
void register_te_lock_test() { add_retrace(retrace); }
}  // namespace fe::features
