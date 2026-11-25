// melee-unlocked's host.h includes its ppc.h (x86 intrinsics) for the CPU context type; its PhobGCC menu never touches the CPU,
// so on the Mac (arm64) the type is only declared.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace ppc { struct Context; }
// host.h's SimCostScope reads the x86 time stamp counter; nothing here constructs one.
inline uint64_t __rdtsc() { return 0; }
