// BCryptGenRandom for melee-unlocked's peer lobby: the system's random source (lobby_shim.cpp: getentropy on macOS). Keys and nonces come from it, so a failure is reported, never papered over.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "windows.h"

#define BCRYPT_USE_SYSTEM_PREFERRED_RNG 0x00000002
long BCryptGenRandom(void* algorithm, PUCHAR buffer, ULONG size, ULONG flags);   // 0 or a negative status
