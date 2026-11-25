// The engine's picture effects as the settings left them (features/graphics.cpp writes them, on the simulation thread;
// mu-engine/app/gfx/effects_mac.mm reads them on the render thread): ambient occlusion, texture packs, menu video
// backgrounds, the shader preset, the Lab view, the glitch detector and frame captures. Display only: none of it reaches
// the game, so it may differ between two players online.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace fe::gfx {
struct Live {
  std::atomic<float> ssao{0.0f};                 // ssao 0..1
  std::atomic<bool> custom_textures{false};      // customtextures: Dolphin-format texture packs
  std::atomic<bool> prefetch_textures{true};     // prefetchtextures: decode every replacement before the game starts
  std::atomic<bool> dump_textures{false};        // dumptextures: the game's textures as PNGs under Dump/Textures/GALE01
  std::atomic<bool> video_backgrounds{false};    // videobackgrounds: VideoBackgrounds/css.mp4 and sss.mp4
  std::atomic<bool> flicker_scan{false};         // flickerscan: the one-frame glitch detector
  std::atomic<bool> lab_view{false};             // labview: the match as Slippi Lab draws it (Ctrl+F3)
  std::atomic<int> lab_fighters{0}, lab_stage{0};   // look_lab_fighters 0..3, look_lab_stage 0..3
  std::atomic<uint32_t> lab_colour[4] = {};      // lab_colour_p1..p4: 0 the look's own, else 0x1000000 | RGB
  std::atomic<uint32_t> lab_background{0}, lab_stage_colour{0};
  std::atomic<int> msaa{1};                      // launcher.ini msaa (upstream's): 1 off, 2, 4, 8 samples on the EFB pass
  std::atomic<int> hdr{0};                       // hdroutput: 0 off, 1 the display's extended range (EDR)
  std::atomic<float> hdr_paper_white{203.0f}, hdr_peak{1000.0f};   // hdrpaperwhite, hdrpeak (nits)
  std::atomic<int> capture_request{0};           // frames still to write (Ctrl+F2: 90)
  std::atomic<uint32_t> generation{0};           // bumped by every change (the render side re-reads the strings)
  std::mutex mutex;                              // the strings below
  std::string shader_preset;                     // shaderpreset: a .slangp path relative to the Shaders folder; empty off
  std::vector<std::string> packs_off;            // texpackoff: the texture packs switched off
};
inline Live& live() { static Live l; return l; }
inline void changed() { live().generation.fetch_add(1, std::memory_order_release); }

// The render side's answers (gfx/effects_mac.mm), for the settings rows.
std::vector<std::pair<std::string, bool>> texture_packs();       // installed packs and whether each is on
std::vector<std::string> shader_presets();                      // .slangp files under Shaders (relative paths)
std::string shader_status();                                    // why the chosen preset is not running; empty when it is
std::string video_status();                                     // the menu videos' state, one line
bool lab_packs_present();                                       // the Lab view's silhouette packs are installed
// Shows a folder in the Finder: 0 TexturePacks, 1 Dump/Textures/GALE01, 2 VideoBackgrounds, 3 Shaders, 4 capture,
// 5 flicker, 6 Lab.
void open_folder(int which);
// The renderer options the effects need (play mode sets it: mu_play_main.cpp): SSAO strength and HDR output.
extern std::function<void(float ssao, int msaa, int hdr, float paper_white, float peak)> apply_renderer;
}  // namespace fe::gfx
