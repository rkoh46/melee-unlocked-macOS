// The console's memory layout and the guest CPU state record the renderer's observer and host.h still name (the
// melee-unlocked engine has no PowerPC CPU: its host passes an empty record).
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>

namespace ppc {

constexpr uint32_t RAM_BASE = 0x80000000u;
constexpr uint32_t RAM_SIZE = 0x01800000u;

union FPR {
  struct { double ps0, ps1; };
  struct { uint64_t u0, u1; };
};

struct Context {
  uint32_t r[32];
  FPR f[32];
  uint8_t cr[8];       // per field: LT=8 GT=4 EQ=2 SO=1
  uint32_t lr, ctr;
  uint32_t ca, so, ov; // XER pieces
  uint32_t fpscr;
  uint32_t gqr[8];
  uint32_t msr;
  uint32_t hid0, hid2, dec;
  uint64_t tb;
  uint32_t spr[1024];  // anything else, by number
  uint32_t call_depth;
  uint32_t backedges;
  uint32_t last_pc;
  uint32_t entry;
  uint32_t trace_pos;
  uint32_t trace[64];
};

}  // namespace ppc
