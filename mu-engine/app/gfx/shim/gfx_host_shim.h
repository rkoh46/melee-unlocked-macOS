// Pre-included into melee-unlocked's texture_pack.cpp, video_background.cpp and flicker_scan.cpp (the fetched tree's copies,
// mu-engine/app/gfx/gen_gfx.sh): the host functions they call that this app's host.h does not declare.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <exception>
#include <new>
#include "host.h"   // port/runtime/host: log, now_seconds
namespace host {
// melee-unlocked's host.h optional_work: a worker's body whose failure ends only that piece of work.
template <class Body> void optional_work(const char* what, Body&& body) noexcept {
  try { body(); }
  catch (const std::bad_alloc&) { log("%s: not enough memory; skipped", what); }
  catch (const std::exception& e) { log("%s: stopped (%s)", what, e.what()); }
  catch (...) { log("%s: stopped", what); }
}
namespace test_env { inline const char* getenv(const char* name) { return std::getenv(name); } }
}  // namespace host
// The folders players drop texture packs, menu videos and border art into, and where captures go (gfx/effects_mac.mm):
// <Application Support>/Dashdance/melee-unlocked.
#include <filesystem>
std::filesystem::path mu_gfx_folder();
