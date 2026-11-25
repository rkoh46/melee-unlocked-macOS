// The picture frontend's hooks into the host services (patches/0011), answered "not mine" for programs without a
// frontend (the headless runner). Mach-O has no weak undefined references without extra linker flags, so the host's
// weak declarations meet these weak definitions; the app's strong definitions replace them.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mu_host.h"
#include <climits>
#include <cstddef>
#include <cstdint>

extern "C" {
__attribute__((weak)) void mu_frontend_attach(int) {}
__attribute__((weak)) int mu_frontend_pad_read(MuPadStatus*) { return 0; }
__attribute__((weak)) void mu_frontend_pad_rumble(int32_t, int32_t) {}
__attribute__((weak)) void mu_frontend_audio(const int16_t*, uint32_t) {}
__attribute__((weak)) int mu_frontend_log(const char*) { return 0; }
__attribute__((weak)) void mu_frontend_game_log(const char*) {}
__attribute__((weak)) int mu_frontend_option(int, uint32_t*) { return 0; }
__attribute__((weak)) void mu_frontend_disc_ready(void) {}
__attribute__((weak)) int mu_frontend_disc_read(uint64_t, void*, size_t) { return -1; }
__attribute__((weak)) int32_t mu_frontend_command(uint8_t, const uint8_t*, uint32_t, uint8_t*, uint32_t, uint32_t*) { return INT32_MIN; }
__attribute__((weak)) int mu_frontend_jukebox(void) { return 0; }   // patches/0015: the game plays its own music
}
