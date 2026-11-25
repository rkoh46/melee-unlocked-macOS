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
  int msaa = 1;              // multisample anti-aliasing on the EFB pass: 1 (off), 2, 4 or 8 samples (melee-unlocked's msaa),
                             // resolved at the end of each pass; 1 leaves the renderer exactly as it was
  float ssao = 0.0f;         // screen-space ambient occlusion strength 0..1 (melee-unlocked's ssao): contact shading from the
                             // EFB depth in the present pass; 0 = off (the present pass is then unchanged)
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

// Picture effects of the melee-unlocked engine (mu-engine/app/gfx): texture packs, menu video backgrounds, shader presets,
// frame captures and the glitch detector. Called on the render thread with Metal objects as void* (id<MTL...>, borrowed
// for the call). Display only. None set (the default) leaves the renderer as it is.
struct PostFrame {
  void* device = nullptr;          // id<MTLDevice>
  void* command_buffer = nullptr;  // id<MTLCommandBuffer> of this frame
  void* color = nullptr;           // id<MTLTexture>: the EFB colour (RGBA8), or what `picture` returned
  void* depth = nullptr;           // id<MTLTexture>: the EFB depth (Depth32Float, 1 = near), readable
  uint32_t x = 0, y = 0, w = 0, h = 0;   // the picture's region in `color`, in pixels
  uint32_t out_w = 0, out_h = 0;   // the size it is presented at, in pixels
  int scale = 1;                   // the EFB's internal-resolution multiplier
  uint64_t frame = 0;              // frames presented so far
  void* final_color = nullptr;     // `presented` only, when wants_final says so: the picture with the present pass's picture
                                   // options and SSAO applied (RGBA8, w x h, at the internal resolution)
};
struct PostHooks {
  void (*begin_frame)(const Frame& frame) = nullptr;
  // A texture pack's replacement for a game texture: RGBA8 levels (level 0 first, each half the last); false: the game's.
  // Asked once per texture the renderer caches.
  bool (*replace)(const TextureRef& t, std::vector<std::vector<uint8_t>>& levels, uint32_t* width, uint32_t* height) = nullptr;
  // A changing texture in place of a game texture (a menu video): an id<MTLTexture> the hook keeps, asked on every use
  // while `dynamic_wanted` says so; nullptr: the game's.
  bool (*dynamic_wanted)() = nullptr;
  void* (*dynamic)(void* device, void* command_buffer, const TextureRef& t) = nullptr;
  // Before a draw of the EFB pass (a video layer under the menu's foreground): may draw into `encoder`
  // (id<MTLRenderCommandEncoder>, colour RGBA8 + Depth32Float attachments at efb_w x efb_h, `samples` per pixel); true: it drew.
  bool (*before_draw)(const Frame& frame, size_t command, const DrawCall& dc, void* device, void* encoder, int efb_w, int efb_h,
                      int scale, int samples) = nullptr;   // samples: the pass's sample count (MSAA)
  // The picture to present instead of the EFB region (a shader preset's output, any size; drawn to the picture's rectangle
  // with the present pass's sampling); nullptr: the EFB as usual.
  void* (*picture)(const PostFrame& f) = nullptr;
  // After the frame's present pass is encoded, before the EFB is cleared: the picture as presented (`color`, region).
  void (*presented)(const PostFrame& f) = nullptr;
  bool (*wants_final)() = nullptr;   // this frame's `presented` needs final_color (a capture)
};
void metal_set_post_hooks(const PostHooks* hooks);   // the pointer is kept: a static object
// Drops every cached game texture at the next frame (a texture pack switched on or off).
void metal_drop_textures();
}  // namespace gx
