// Native Metal renderer for decoded GX frames: the D3D12 backend's design (EFB render
// target at an integer scale, Dolphin-derived integer-math shaders, EFB copies kept as
// GPU textures, letterboxed present) on Metal, for macOS, iOS and visionOS.
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "gx_core.h"
#include "overlay.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace gx {
struct MetalOptions {
  int efb_scale = 0;         // internal resolution multiplier; 0 = auto (integer scale covering the window)
  bool vsync = true;
  bool widescreen = false;   // present at 16:9 (Slippi widescreen code on)
  float sharpness = 0.0f;    // 0..1 contrast-adaptive sharpening in the present pass
  int anisotropy = 16;
  int ssaa = 1;              // 2 = 4x supersampling
  int upscaler = 0;          // 0 = off, 1 = MetalFX spatial (balanced), 2 = MetalFX spatial (quality): the EFB
                             // is rendered at half the window's auto scale and reconstructed to full size at
                             // present; ignored (plain blit) where MetalFX is unsupported or downscaling
  std::string capture_path;  // write a PPM of the presented EFB region at capture_frame
  uint32_t capture_frame = 0;
  uint32_t capture_every = 0;
  bool capture_ui = false;    // the capture also has the UI pass (metal_set_ui_pass) drawn over it
  // Picture options (melee-unlocked's picture page): all off by default, the present pass is then unchanged.
  float brightness = 1.0f, contrast = 1.0f, vibrance = 1.0f;   // 1 = as rendered
  int filter = 0;            // 0 smooth, 1 area (sharp box), 2 CRT scanlines, 3 vibrant
  int crt = 0;               // the CRT look: 0 off, 1 scanlines and mask, 2 with curvature
  float crt_curve = 1.0f;    // its curvature strength
  float picture_aspect = 0;  // the picture's shape (width / height); 0 = the game's own (4:3, or 16:9 with widescreen)
  std::string cache_dir;     // pipelines.bin lives here: every pipeline seen, precompiled in the background next launch
};

// `layer` is a CAMetalLayer*; `w`/`h` the drawable size in pixels.
Backend* create_metal_backend(void* layer, int w, int h, const MetalOptions& options);
void metal_resize(Backend* backend, int w, int h);
void metal_set_options(Backend* backend, const MetalOptions& options);
// Caps on the internal-resolution multiplier, applied to both auto and explicit scales and re-evaluated
// every frame: `device_cap` for the device class (phones: 2), `thermal_cap` from the thermal state
// (serious: 2, critical: 1). 0 = no cap.
void metal_scale_caps(int device_cap, int thermal_cap);
uint64_t metal_frames_presented(Backend* backend);
// Drawn over every presented frame (touch controls); the provider runs on the render thread.
using OverlayProvider = std::function<bool(host::OverlayFrame&)>;
void metal_set_overlay(Backend* backend, OverlayProvider provider);
// Drawn last in the present pass, over the picture and the overlay (melee-unlocked's ImGui panel): the provider gets the
// pass's MTLRenderCommandEncoder, MTLRenderPassDescriptor and MTLCommandBuffer and the drawable's size in pixels, on the
// render thread. One per process; nullptr removes it.
using UiPass = std::function<void(void* encoder, void* pass, void* command_buffer, int width, int height)>;
void metal_set_ui_pass(UiPass pass);
// Display-rate presentation (sub-frames): when `active` says so as the render thread starts, `loop` runs the render
// thread instead of its own loop, taking frames from `queue` and drawing each (or a sub-frame pose of it) with `render`.
// Set before create_metal_backend. Off by default.
class FrameQueue;
using PresentRender = void (*)(Backend& backend, const Frame& frame, const DrawMatrices* overrides);
using PresentLoop = std::function<void(Backend& backend, FrameQueue& queue, const std::atomic<bool>& stop, PresentRender render)>;
void metal_set_present_loop(PresentLoop loop, std::function<bool()> active);
// Per-draw look rules (melee-unlocked's looks, Lightweight, Visual effects, True widescreen; supplied by the
// melee-unlocked engine's mu-engine/app/gx_looks_mac.cpp). Display only: they change what the renderer draws, never
// game memory. Called on the render thread; none set (the default) leaves every draw as the game made it.
struct LookDecision {
  bool skip = false;      // not drawn
  bool wire = false;      // triangles drawn as lines
  bool tint = false;      // one plain colour, light and shade kept (rgb)
  bool widen = false;     // the projection's x row scaled by `widen_by` (True widescreen)
  float rgb[3] = {};
  float widen_by = 1.0f;
  uint8_t mark = 0;       // bits 0-2: the textures' surface treatment (looks_live.h Surface), bit 3: nearest-texel sampling
};
struct LookHooks {
  bool (*begin_frame)(const Frame& frame) = nullptr;   // false: no rule this frame (decide is not called)
  void (*decide)(const Frame& frame, size_t draw, const DrawCall& dc, LookDecision* out) = nullptr;
  uint32_t (*clear_colour)(const Frame& frame, uint32_t argb) = nullptr;   // the EFB clear (the look's background)
  // Treats level 0 of a decoded RGBA8 texture for a mark in place; false: the game's own texture is used.
  bool (*treat)(uint8_t mark, std::vector<uint8_t>& rgba, uint32_t& width, uint32_t& height) = nullptr;
  void (*end_frame)(const Frame& frame) = nullptr;
};
void metal_set_look_hooks(const LookHooks* hooks);   // the pointer is kept: a static object
}  // namespace gx
