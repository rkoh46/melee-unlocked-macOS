// The engine's picture effects, as melee-unlocked's VIDEO page has them (port/runtime/gx/pc_settings.cpp, its settings.ini
// keys): screen-space ambient occlusion (ssao), Dolphin-format texture packs (customtextures, prefetchtextures, texpackoff,
// dumptextures), looping menu video backgrounds (videobackgrounds), RetroArch shader presets (shaderpreset), HDR output
// (hdroutput, hdrpaperwhite, hdrpeak: EDR on the Mac), the Lab view (labview, look_lab_fighters, look_lab_stage, lab_colour_*), the
// one-frame glitch detector (flickerscan) and frame capture bursts (Ctrl+F2). This module keeps the settings and the panel's
// rows (gfx_live.h); the Metal side is mu-engine/app/gfx (effects_mac.mm, lab_mac.cpp). Display only, offline and online.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "features_internal.h"
#include "frontend.h"
#include "gfx_live.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace fe::gfx {
std::function<void(float, int, int, float, float)> apply_renderer;
void lab_retrace();   // gfx/lab_mac.cpp: the Lab view's frame from the game, on the simulation thread
void lab_settings_changed();
void lab_download();          // the silhouette packs from melee-unlocked's content release (SHA-256 checked)
std::string lab_status();
void lab_end();
}

namespace fe::features {
namespace {
using fe::gfx::live;
using Write = MenuEntry::Write;

std::string on_off(bool on) { return on ? "On" : "Off"; }
std::string pct(float v) { return std::to_string(int(std::lround(v * 100.0f))) + "%"; }
std::string fmt(float v) { char b[32]; std::snprintf(b, sizeof b, "%g", v); return b; }
uint32_t colour_value(const std::string& v) {
  if (v.empty() || v == "0" || v == "-") return 0;
  const std::string hex = v[0] == '#' ? v.substr(1) : v;
  return 0x1000000u | (uint32_t(std::strtoul(hex.c_str(), nullptr, 16)) & 0xFFFFFFu);
}
std::string colour_text(uint32_t c) { if (!(c & 0x1000000u)) return "0"; char b[16]; std::snprintf(b, sizeof b, "%06X", c & 0xFFFFFFu); return b; }

void push_renderer() {
  auto& l = live();
  if (fe::gfx::apply_renderer) fe::gfx::apply_renderer(l.ssao.load(), l.msaa.load(), l.hdr.load(), l.hdr_paper_white.load(), l.hdr_peak.load());
}

bool setting(const std::string& key, const std::string& value) {
  auto& l = live();
  const int n = std::atoi(value.c_str());
  if (key == "ssao") l.ssao.store(std::clamp(float(std::atof(value.c_str())), 0.0f, 1.0f));
  else if (key == "customtextures") l.custom_textures.store(value == "1");
  else if (key == "prefetchtextures") l.prefetch_textures.store(value != "0");
  else if (key == "dumptextures") l.dump_textures.store(value == "1");
  else if (key == "texpackoff") {   // one line per pack upstream; the Mac writes them on one line, joined by '|'
    std::lock_guard<std::mutex> lock(l.mutex);
    std::stringstream names(value); std::string name;
    while (std::getline(names, name, '|')) if (!name.empty() && std::find(l.packs_off.begin(), l.packs_off.end(), name) == l.packs_off.end()) l.packs_off.push_back(name);
  }
  else if (key == "videobackgrounds") l.video_backgrounds.store(value != "0");
  else if (key == "shaderpreset") { std::lock_guard<std::mutex> lock(l.mutex); l.shader_preset = value == "-" ? std::string() : value; }
  else if (key == "flickerscan") l.flicker_scan.store(value == "1");
  else if (key == "labview") l.lab_view.store(value == "1");
  else if (key == "look_lab_fighters") l.lab_fighters.store(std::clamp(n, 0, 3));
  else if (key == "look_lab_stage") l.lab_stage.store(std::clamp(n, 0, 3));
  else if (key.rfind("lab_colour_p", 0) == 0 && key.size() == 13 && key[12] >= '1' && key[12] <= '4') l.lab_colour[key[12] - '1'].store(colour_value(value));
  else if (key == "lab_colour_background") l.lab_background.store(colour_value(value));
  else if (key == "lab_colour_stage") l.lab_stage_colour.store(colour_value(value));
  else if (key == "hdroutput") l.hdr.store(std::clamp(n, 0, 1));
  else if (key == "hdrpaperwhite") l.hdr_paper_white.store(std::clamp(float(std::atof(value.c_str())), 80.0f, 500.0f));
  else if (key == "hdrpeak") l.hdr_peak.store(std::clamp(float(std::atof(value.c_str())), 400.0f, 10000.0f));
  else return false;
  fe::gfx::changed();
  return true;
}

void write_all(Write w) {
  auto& l = live();
  w("ssao", fmt(l.ssao.load()));
  w("customtextures", l.custom_textures.load() ? "1" : "0");
  w("prefetchtextures", l.prefetch_textures.load() ? "1" : "0");
  w("dumptextures", l.dump_textures.load() ? "1" : "0");
  w("videobackgrounds", l.video_backgrounds.load() ? "1" : "0");
  w("flickerscan", l.flicker_scan.load() ? "1" : "0");
  w("labview", l.lab_view.load() ? "1" : "0");
  w("look_lab_fighters", std::to_string(l.lab_fighters.load()));
  w("look_lab_stage", std::to_string(l.lab_stage.load()));
  w("hdroutput", std::to_string(l.hdr.load()));
  w("hdrpaperwhite", fmt(l.hdr_paper_white.load()));
  w("hdrpeak", fmt(l.hdr_peak.load()));
  std::string preset, off;
  { std::lock_guard<std::mutex> lock(l.mutex); preset = l.shader_preset; for (const auto& p : l.packs_off) off += (off.empty() ? "" : "|") + p; }
  w("shaderpreset", preset.empty() ? "-" : preset);
  w("texpackoff", off.empty() ? "-" : off);
}
void edited() { fe::gfx::changed(); push_renderer(); fe::gfx::lab_settings_changed(); }

// The texture pack the "Pack" row points at.
size_t g_pack = 0;
std::string pack_row() {
  const auto packs = fe::gfx::texture_packs();
  if (packs.empty()) return "None installed";
  g_pack %= packs.size();
  return packs[g_pack].first + (packs[g_pack].second ? " (on)" : " (off)");
}
void toggle_pack() {
  const auto packs = fe::gfx::texture_packs();
  if (packs.empty()) return;
  g_pack %= packs.size();
  auto& l = live();
  std::lock_guard<std::mutex> lock(l.mutex);
  const std::string& name = packs[g_pack].first;
  auto it = std::find(l.packs_off.begin(), l.packs_off.end(), name);
  if (it == l.packs_off.end()) l.packs_off.push_back(name); else l.packs_off.erase(it);
}
std::string preset_row() {
  std::lock_guard<std::mutex> lock(live().mutex);
  const std::string& p = live().shader_preset;
  if (p.empty()) return "Off";
  const size_t slash = p.find_last_of('/');
  return slash == std::string::npos ? p : p.substr(slash + 1);
}
void step_preset(int d) {
  std::vector<std::string> list = fe::gfx::shader_presets();
  list.insert(list.begin(), std::string());
  std::lock_guard<std::mutex> lock(live().mutex);
  size_t i = 0;
  while (i < list.size() && list[i] != live().shader_preset) ++i;
  const size_t count = list.size();
  live().shader_preset = list[((i % count) + (d < 0 ? count - 1 : 1)) % count];
}
const char* const kLabFighters[] = {"Silhouettes", "Stick figures", "Capsules", "Blobs"};
const char* const kLabStage[] = {"As drawn", "Outline", "Blocks", "Hidden"};

void retrace(const Retrace&) {
  static bool pushed = false;
  if (!pushed) { pushed = true; live().msaa.store(std::clamp(std::atoi(app_ini("msaa", "1").c_str()), 1, 8)); push_renderer(); fe::gfx::lab_settings_changed(); }   // the settings file's values, once the renderer is up
  fe::gfx::lab_retrace();
}
void run_end() {
  fe::gfx::lab_end();   // a download in flight finishes first
  auto& l = live();
  say("graphics: ssao %g, texture packs %s%s, menu videos %s, shader preset %s, Lab view %s, glitch detector %s, HDR %s",
      l.ssao.load(), on_off(l.custom_textures.load()).c_str(), l.dump_textures.load() ? " (dumping)" : "", on_off(l.video_backgrounds.load()).c_str(),
      preset_row().c_str(), on_off(l.lab_view.load()).c_str(), on_off(l.flicker_scan.load()).c_str(), on_off(l.hdr.load() != 0).c_str());
}
}  // namespace

void register_graphics() {
  add_setting(setting);
  add_retrace(retrace);
  add_run_end(run_end);
  // melee-unlocked's VIDEO page rows, on the panel's Picture section.
  add_menu_entry({"Picture", "Anti-aliasing (MSAA)", [] { const int m = live().msaa.load(); return m <= 1 ? std::string("Off") : std::to_string(m) + "x"; },
                  [](int d) {
                    static const int steps[] = {1, 2, 4, 8};
                    int i = 0; while (i < 3 && steps[i] < live().msaa.load()) ++i;
                    i = std::clamp(i + (d < 0 ? -1 : 1), 0, 3);
                    live().msaa.store(steps[i]); app_ini_set("msaa", std::to_string(steps[i])); edited();
                  }, nullptr, nullptr});
  add_menu_entry({"Picture", "Ambient occlusion (SSAO)", [] { const float a = live().ssao.load(); return a <= 0.0f ? std::string("Off") : pct(a); },
                  [](int d) { auto& a = live().ssao; a.store(std::clamp(std::round((a.load() + 0.1f * d) * 10.0f) / 10.0f, 0.0f, 1.0f)); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "HDR output (extended range)", [] { return on_off(live().hdr.load() != 0); },
                  [](int) { live().hdr.store(live().hdr.load() ? 0 : 1); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Paper white", [] { return std::to_string(int(live().hdr_paper_white.load())) + " nits"; },
                  [](int d) { auto& a = live().hdr_paper_white; a.store(std::clamp(a.load() + 20.0f * d, 80.0f, 500.0f)); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Peak brightness", [] { return std::to_string(int(live().hdr_peak.load())) + " nits"; },
                  [](int d) { auto& a = live().hdr_peak; a.store(std::clamp(a.load() + 100.0f * d, 400.0f, 10000.0f)); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "Shader preset (RetroArch)", preset_row, [](int d) { step_preset(d); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Preset status", [] { const std::string s = fe::gfx::shader_status(); return s.empty() ? std::string("Running") : s; },
                  nullptr, nullptr, nullptr});
  add_menu_entry({"Picture", "  Open the Shaders folder", [] { return std::string(""); }, nullptr, [] { fe::gfx::open_folder(3); }, nullptr});
  add_menu_entry({"Picture", "Texture packs", [] { return on_off(live().custom_textures.load()); },
                  [](int) { live().custom_textures.store(!live().custom_textures.load()); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Pack", pack_row, [](int d) { g_pack += d < 0 ? size_t(-1) : 1; }, [] { toggle_pack(); edited(); }, write_all});
  add_menu_entry({"Picture", "  Load every texture before the game", [] { return on_off(live().prefetch_textures.load()); },
                  [](int) { live().prefetch_textures.store(!live().prefetch_textures.load()); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Dump the game's textures (pack authors)", [] { return on_off(live().dump_textures.load()); },
                  [](int) { live().dump_textures.store(!live().dump_textures.load()); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Open the TexturePacks folder", [] { return std::string(""); }, nullptr, [] { fe::gfx::open_folder(0); }, nullptr});
  add_menu_entry({"Picture", "Menu video backgrounds", [] { return on_off(live().video_backgrounds.load()); },
                  [](int) { live().video_backgrounds.store(!live().video_backgrounds.load()); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Videos", [] { return fe::gfx::video_status(); }, nullptr, [] { fe::gfx::open_folder(2); }, nullptr});
  add_menu_entry({"Picture", "Lab view (Ctrl+F3)", [] { return on_off(live().lab_view.load()); },
                  [](int) { live().lab_view.store(!live().lab_view.load()); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Lab fighters", [] { return std::string(kLabFighters[std::clamp(live().lab_fighters.load(), 0, 3)]); },
                  [](int d) { live().lab_fighters.store((live().lab_fighters.load() + (d < 0 ? 3 : 1)) % 4); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "  Lab fighters download (silhouettes)", [] { return fe::gfx::lab_status(); }, nullptr, [] { fe::gfx::lab_download(); }, nullptr});
  add_menu_entry({"Picture", "  Lab stage", [] { return std::string(kLabStage[std::clamp(live().lab_stage.load(), 0, 3)]); },
                  [](int d) { live().lab_stage.store((live().lab_stage.load() + (d < 0 ? 3 : 1)) % 4); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "One-frame glitch detector", [] { return on_off(live().flicker_scan.load()); },
                  [](int) { live().flicker_scan.store(!live().flicker_scan.load()); edited(); }, nullptr, write_all});
  add_menu_entry({"Picture", "Capture the next 90 frames (Ctrl+F2)", [] { const int n = live().capture_request.load(); return n > 0 ? std::to_string(n) + " to go" : std::string(""); },
                  nullptr, [] { live().capture_request.store(90); }, nullptr});
  add_menu_entry({"Picture", "  Open the capture folder", [] { return std::string(""); }, nullptr, [] { fe::gfx::open_folder(4); }, nullptr});
}
}  // namespace fe::features
