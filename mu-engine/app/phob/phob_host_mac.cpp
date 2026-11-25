// The host side melee-unlocked's PhobGCC menu (its gx/phob_menu.cpp, compiled from the fetched tree by phob/gen_phob.sh) asks
// for, on the Mac: each adapter port's PhobGCC settings, live controller and recent reports from fe/phob_adapter.cpp (the
// IOKit reader's reports, gc_adapter_iokit.cpp), the PhobGCC folder and named setups (its phob_store.cpp), and the menu's
// frame in the engine's ImGui pass with the controllers the panel hands over (phob_bridge.h). Compiled against melee-unlocked's
// host.h, like the menu.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host.h"          // melee-unlocked port/runtime/host (the fetched tree)
#include "phob_menu.h"
#include "phob_bridge.h"
#include "phob_adapter.h"  // mu-engine/app/fe
#include "frontend.h"

#include <imgui.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <mutex>
#include <vector>

namespace host {
void gcadapter_phob_set(int port, const phob::Settings& settings) { fe::phob_adapter::set_settings(port, settings); }
phob::Settings gcadapter_phob_get(int port) { return fe::phob_adapter::settings(port); }
bool gcadapter_phob_raw(int port, phob::HardwareInput& in) { return fe::phob_adapter::raw(port, &in); }
int gcadapter_scope(int port, GCScopeSample* out, int max) {
  static_assert(sizeof(GCScopeSample) == sizeof(fe::phob_adapter::ScopeSample), "the scope sample layouts must agree");
  std::vector<fe::phob_adapter::ScopeSample> samples(size_t(std::max(max, 0)));
  const int n = fe::phob_adapter::scope(port, samples.data(), max);
  for (int i = 0; i < n; ++i) {
    const auto& s = samples[size_t(i)]; GCScopeSample& o = out[i];
    o.time_us = s.time_us; o.raw_ax = s.raw_ax; o.raw_ay = s.raw_ay; o.raw_cx = s.raw_cx; o.raw_cy = s.raw_cy;
    o.ax = s.ax; o.ay = s.ay; o.cx = s.cx; o.cy = s.cy; o.raw_l = s.raw_l; o.raw_r = s.raw_r; o.l = s.l; o.r = s.r;
    o.raw_buttons = s.raw_buttons; o.buttons = s.buttons; o.hw_ax = s.hw_ax; o.hw_ay = s.hw_ay; o.hw_cx = s.hw_cx; o.hw_cy = s.hw_cy;
  }
  return n;
}
double gcadapter_poll_rate_hz() { return fe::phob_adapter::report_rate_hz(); }
void gcadapter_test_script_start() {}   // upstream's scripted adapter (tests with the adapter off): not on the Mac
std::string profile_clean_name(const std::string& name) {
  std::string out;
  for (char c : name) if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_' || c == '.') out += c;
  while (!out.empty() && (out.back() == ' ' || out.back() == '.')) out.pop_back();
  while (!out.empty() && out.front() == ' ') out.erase(out.begin());
  return out.size() > 48 ? out.substr(0, 48) : out;
}
}  // namespace host

namespace mu_phob {
namespace {
std::mutex g_mutex;
Nav g_nav;
bool g_armed = false, g_store_loaded = false;
std::atomic<bool> g_open{false};
}  // namespace
bool open() { return g_open.load(std::memory_order_relaxed); }
void request(int port, bool scopes_only) {
  if (!g_store_loaded) { g_store_loaded = true; host::phob_store_load(fe::options.out_dir + "/settings.ini"); }   // PhobGCC/ beside settings.ini
  gx::phob_native_request(std::clamp(port, 0, 3), scopes_only);
  { std::lock_guard<std::mutex> lock(g_mutex); g_armed = false; }
  g_open.store(true);
  host::log("phob: the PhobGCC menu opened on adapter port %d%s", port + 1, scopes_only ? " (controller test)" : "");
}
void set_nav(const Nav& nav) { std::lock_guard<std::mutex> lock(g_mutex); g_nav = nav; if (!nav.buttons) g_armed = true; }
void frame(int width, int height) {
  if (!g_open.load()) return;
  if (!gx::phob_native_open()) { g_open.store(false); return; }
  gx::PhobNativeIn in;
  in.x0 = 0; in.y0 = 0; in.x1 = float(width); in.y1 = float(height);
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    in.buttons = g_nav.buttons; in.stick_x = g_nav.stick_x; in.stick_y = g_nav.stick_y; in.armed = g_armed;
  }
  in.time = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  const gx::PhobNativeOut out = gx::phob_native_frame(in);
  if (out.back_to_panel || out.close_all) { gx::phob_native_close(); g_open.store(false); host::log("phob: the PhobGCC menu closed"); }
}
}  // namespace mu_phob
