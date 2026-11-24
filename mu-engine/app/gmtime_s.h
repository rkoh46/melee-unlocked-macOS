// native_replay_stream.cpp (melee-unlocked) uses the Windows name; upstream's Linux build gets it from its compat layer.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cerrno>
#include <ctime>
static inline int gmtime_s(struct tm* out, const time_t* t) { return gmtime_r(t, out) ? 0 : errno; }
