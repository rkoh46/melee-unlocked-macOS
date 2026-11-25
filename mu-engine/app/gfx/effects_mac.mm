// The melee-unlocked engine's picture effects on the Mac's Metal renderer (gx::PostHooks, port/runtime/gx/gx_metal.h), as
// melee-unlocked's renderers have them: Dolphin-format texture packs and texture dumping (its texture_pack.cpp), looping menu
// video backgrounds (its video_background.cpp, decoded by AVFoundation: media_apple.mm), RetroArch shader presets (librashader's
// Metal runtime, loaded at run time as melee-unlocked loads its DLL), frame capture bursts (Ctrl+F2: the next 90 presented
// frames as PPM files) and the one-frame glitch detector (its flicker_scan.cpp on a 64 x 64 reduction read back from the GPU).
// Settings: fe/features/graphics.cpp (gfx_live.h). Render thread only, except the folder and list answers for the panel.
// Display only: nothing here reaches game memory.
// SPDX-License-Identifier: GPL-3.0-or-later
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

#include "gx_core.h"
#include "gx_metal.h"
#include "gx_texture.h"
#include "host.h"
#include "frontend.h"
#include "gfx_live.h"
#include "texture_pack.h"
namespace gx::texpack { void prefetch_join(); }   // patches/0130
#include "video_background.h"
#include "video_background_learning.h"
#include "flicker_scan.h"
#include <spawn.h>
#include <sys/wait.h>

// librashader (melee-unlocked's port/third_party/librashader, MPL-2.0 headers from the fetched tree) with its Metal runtime:
// its loader asks dlopen for "librashader.dylib", which is sent to the places the Mac app keeps it (lib_open below).
void* mu_libra_dlopen(const char* name, int flags);
#define LIBRA_RUNTIME_METAL
#define dlopen(name, flags) mu_libra_dlopen(name, flags)
#include "librashader_ld.h"
#undef dlopen

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

std::filesystem::path mu_gfx_folder() {
  if (!fe::options.base_dir.empty()) return fs::path(fe::options.base_dir);
  const char* home = std::getenv("HOME");
  return fs::path(home ? home : ".") / "Library/Application Support/Dashdance/melee-unlocked";
}

namespace mu_gfx {
namespace {
using fe::gfx::live;

// ---- shaders the effects draw with ----
const char* kSource = R"(
#include <metal_stdlib>
using namespace metal;
struct O { float4 pos [[position]]; float2 uv; };
struct Q { float4 uvrect; };   // uv = uvrect.xy + p * uvrect.zw over the quad
vertex O quad_vs(uint id [[vertex_id]], constant Q& q [[buffer(0)]]) {
  O o; float2 p = float2((id << 1) & 2, id & 2);
  o.pos = float4(p * float2(2, -2) + float2(-1, 1), 0, 1); o.uv = q.uvrect.xy + p * q.uvrect.zw; return o;
}
fragment float4 quad_ps(O i [[stage_in]], texture2d<float> src [[texture(0)]], sampler s [[sampler(0)]]) {
  return float4(src.sample(s, i.uv).rgb, 1.0);
}
// The glitch detector's reduction: each output texel averages its whole footprint of the picture.
struct R { float4 rect; int2 taps; int2 grid; };   // rect: the picture's region in texels (x, y, w, h)
fragment float4 reduce_ps(O i [[stage_in]], constant R& r [[buffer(0)]], texture2d<float> src [[texture(0)]], sampler s [[sampler(0)]]) {
  float2 cell = floor(i.pos.xy);
  float2 size = float2(src.get_width(), src.get_height());
  float2 foot = r.rect.zw / float2(r.grid);
  float4 acc = 0;
  for (int y = 0; y < r.taps.y; ++y)
    for (int x = 0; x < r.taps.x; ++x) {
      float2 t = r.rect.xy + (cell + (float2(x, y) + 0.5) / float2(r.taps)) * foot;
      acc += src.sample(s, t / size);
    }
  return acc / float(r.taps.x * r.taps.y);
}
)";

id<MTLDevice> g_device = nil;
id<MTLLibrary> g_lib = nil;
id<MTLRenderPipelineState> g_quad_efb = nil;   // a picture into the EFB pass (colour RGBA8, depth Depth32Float)
id<MTLRenderPipelineState> g_reduce = nil;     // the 64 x 64 reduction (RGBA8)
id<MTLDepthStencilState> g_no_depth = nil;
id<MTLSamplerState> g_linear = nil;
bool ensure_device(id<MTLDevice> device) {
  if (g_device == device && g_lib) return true;
  g_device = device;
  NSError* error = nil;
  g_lib = [device newLibraryWithSource:[NSString stringWithUTF8String:kSource] options:[MTLCompileOptions new] error:&error];
  if (!g_lib) { host::log("graphics: effect shaders: %s", error.localizedDescription.UTF8String); return false; }
  auto make = [&](const char* ps, bool depth) -> id<MTLRenderPipelineState> {
    MTLRenderPipelineDescriptor* d = [MTLRenderPipelineDescriptor new];
    d.vertexFunction = [g_lib newFunctionWithName:@"quad_vs"];
    d.fragmentFunction = [g_lib newFunctionWithName:[NSString stringWithUTF8String:ps]];
    d.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
    if (depth) d.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    NSError* e = nil;
    id<MTLRenderPipelineState> s = [device newRenderPipelineStateWithDescriptor:d error:&e];
    if (!s) host::log("graphics: pipeline %s: %s", ps, e.localizedDescription.UTF8String);
    return s;
  };
  g_quad_efb = make("quad_ps", true);
  g_reduce = make("reduce_ps", false);
  MTLDepthStencilDescriptor* ds = [MTLDepthStencilDescriptor new];
  ds.depthCompareFunction = MTLCompareFunctionAlways; ds.depthWriteEnabled = NO;
  g_no_depth = [device newDepthStencilStateWithDescriptor:ds];
  MTLSamplerDescriptor* sd = [MTLSamplerDescriptor new];
  sd.minFilter = MTLSamplerMinMagFilterLinear; sd.magFilter = MTLSamplerMinMagFilterLinear;
  sd.sAddressMode = MTLSamplerAddressModeClampToEdge; sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
  g_linear = [device newSamplerStateWithDescriptor:sd];
  return g_quad_efb && g_reduce;
}
// The picture-into-the-EFB pipeline for the pass's sample count (MSAA).
id<MTLRenderPipelineState> quad_efb(id<MTLDevice> device, int samples) {
  static std::unordered_map<int, id<MTLRenderPipelineState>> by_samples;
  if (samples <= 1) return g_quad_efb;
  auto it = by_samples.find(samples);
  if (it != by_samples.end()) return it->second;
  MTLRenderPipelineDescriptor* d = [MTLRenderPipelineDescriptor new];
  d.vertexFunction = [g_lib newFunctionWithName:@"quad_vs"];
  d.fragmentFunction = [g_lib newFunctionWithName:@"quad_ps"];
  d.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
  d.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
  d.rasterSampleCount = samples;
  NSError* e = nil;
  id<MTLRenderPipelineState> state = [device newRenderPipelineStateWithDescriptor:d error:&e];
  by_samples[samples] = state ?: g_quad_efb;
  return by_samples[samples];
}
dispatch_queue_t writer_queue() {   // PPM files and the glitch detector's analysis, in presentation order, off the render thread
  static dispatch_queue_t q = dispatch_queue_create("melee-unlocked.graphics.writer", DISPATCH_QUEUE_SERIAL);
  return q;
}

// ---- texture packs (texture_pack.cpp) ----
std::mutex g_pack_mutex;
std::vector<std::pair<std::string, bool>> g_pack_list;   // for the panel (under g_pack_mutex)
uint32_t g_seen_generation = ~0u;
std::vector<std::string> g_packs_off;
bool g_prefetch_started = false;
uint64_t g_replaced = 0, g_dumped = 0;
void refresh_pack_list() {
  std::vector<std::pair<std::string, bool>> list;
  for (const auto& p : gx::texpack::packs()) list.emplace_back(p.name, p.enabled);
  std::lock_guard<std::mutex> lock(g_pack_mutex);
  g_pack_list = std::move(list);
}
void textures_begin_frame() {
  auto& l = live();
  bool drop = gx::texpack::configure(l.custom_textures.load(std::memory_order_relaxed), l.dump_textures.load(std::memory_order_relaxed));
  const uint32_t generation = l.generation.load(std::memory_order_acquire);
  if (generation != g_seen_generation) {
    g_seen_generation = generation;
    std::vector<std::string> off;
    { std::lock_guard<std::mutex> lock(l.mutex); off = l.packs_off; }
    if (off != g_packs_off) { g_packs_off = off; gx::texpack::set_disabled_packs(off); drop = true; }
    gx::texpack::refresh_packs();
    refresh_pack_list();
  }
  if (gx::texpack::enabled() && l.prefetch_textures.load() && !g_prefetch_started) { g_prefetch_started = true; gx::texpack::prefetch_begin(); }
  if (gx::texpack::take_cosmetics_changed()) drop = true;
  if (drop) { gx::metal_drop_textures(); host::log("graphics: texture packs %s%s", gx::texpack::enabled() ? "on" : "off", gx::texpack::dumping() ? ", dumping the game's textures" : ""); }
}
bool replace(const gx::TextureRef& t, std::vector<std::vector<uint8_t>>& levels, uint32_t* width, uint32_t* height) {
  if (!t.data || !(gx::texpack::enabled() || gx::texpack::dumping() || gx::texpack::cosmetics_enabled())) return false;
  const std::string name = gx::texpack::base_name(t, *t.data);
  if (name.empty()) return false;
  if (gx::texpack::dumping()) {   // every level of the game's own texture, under Dolphin's names
    std::vector<uint8_t> rgba;
    uint32_t lw = t.width, lh = t.height; size_t at = 0;
    for (uint32_t l = 0; l < std::max(1u, t.mip_levels) && lw && lh; ++l) {
      const uint32_t bytes = gx::texture_level_bytes(lw, lh, t.format);
      if (at + bytes > t.data->image.size()) break;
      gx::decode_texture(t.data->image.data() + at, lw, lh, t.format, t.data->palette.data(), t.tlut_format, rgba);
      gx::texpack::dump_level(name, l, rgba.data(), lw, lh);
      at += bytes; lw = std::max(1u, lw / 2); lh = std::max(1u, lh / 2);
    }
    ++g_dumped;
  }
  if (!gx::texpack::has(name)) return false;
  const auto r = gx::texpack::load(name, 3ull << 30);
  gx::texpack::note_lookup(r != nullptr);
  if (!r || !r->width || !r->height) return false;
  levels.clear();
  for (uint32_t l = 0; l < r->levels && l < r->level_offset.size(); ++l) {
    const size_t bytes = size_t(r->level_width[l]) * r->level_height[l] * 4;
    if (r->level_offset[l] + bytes > r->pixels.size()) break;
    levels.emplace_back(r->pixels.begin() + r->level_offset[l], r->pixels.begin() + r->level_offset[l] + bytes);
  }
  *width = r->width; *height = r->height;
  ++g_replaced;
  return !levels.empty();
}

// ---- menu video backgrounds (video_background.cpp) ----
struct VideoSlot { id<MTLTexture> ring[3] = {nil, nil, nil}; int at = 0; uint64_t serial = ~0ull; };
VideoSlot g_video[gx::video_bg::kVideoSlots];
std::shared_ptr<const gx::video_bg::Frame> g_fullscreen;
int g_fullscreen_slot = -1, g_last_dynamic_slot = -1;
unsigned g_leading_untextured = 0;
bool g_background_drawn = false, g_layer_logged[2] = {false, false};
gx::EfbCopy g_screen{};
bool g_have_screen = false;
id<MTLTexture> video_texture(id<MTLDevice> device, int slot, const gx::video_bg::Frame& f) {
  VideoSlot& v = g_video[slot];
  if (v.serial == f.serial && v.ring[v.at]) return v.ring[v.at];
  v.at = (v.at + 1) % 3;   // the GPU may still be reading the last two frames' copies
  __strong id<MTLTexture>& t = v.ring[v.at];
  if (!t || t.width != f.width || t.height != f.height) {
    MTLTextureDescriptor* td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:f.width height:f.height mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead; td.storageMode = MTLStorageModeShared;
    t = [device newTextureWithDescriptor:td];
    if (!t) { gx::video_bg::report_backend_failure(slot, "Metal video texture creation failed"); return nil; }
  }
  // The decoder hands rows bottom first (as Media Foundation does): the texture is filled top first.
  for (uint32_t y = 0; y < f.height; ++y)
    [t replaceRegion:MTLRegionMake2D(0, f.height - 1 - y, f.width, 1) mipmapLevel:0 withBytes:f.bgra.data() + size_t(y) * f.width * 4 bytesPerRow:f.width * 4];
  v.serial = f.serial;
  return t;
}
void video_begin_frame(const gx::Frame& frame) {
  gx::video_bg::set_enabled(live().video_backgrounds.load(std::memory_order_relaxed));
  gx::video_bg::begin_frame(frame.scene_major, frame.scene_minor);
  g_have_screen = false;
  for (const gx::EfbCopy& c : frame.copies) if (c.to_xfb) { g_screen = c; g_have_screen = true; }
  g_fullscreen = gx::video_bg::fullscreen_frame(&g_fullscreen_slot);
  g_leading_untextured = 0; g_background_drawn = false; g_last_dynamic_slot = -1;
}
bool dynamic_wanted() { return gx::video_bg::wants_texture_names(); }
void* dynamic(void* device, void*, const gx::TextureRef& t) {
  if (!t.data) return nullptr;
  int slot = -1;
  const auto f = gx::video_bg::lookup(gx::texpack::base_name(t, *t.data), t.width, t.height, &slot);
  if (!f || slot < 0 || slot > 1 || f->bgra.empty()) return nullptr;
  id<MTLTexture> tex = video_texture((__bridge id<MTLDevice>)device, slot, *f);
  if (tex) g_last_dynamic_slot = slot;
  return (__bridge void*)tex;
}
// The full-screen video layer: on the character select between Melee's hard-coded backdrop draws and its first textured draw
// (upstream's should_insert_fullscreen_layer), on the stage select right after the learned backdrop draw.
bool before_draw(const gx::Frame&, size_t, const gx::DrawCall& dc, void* device, void* encoder, int efb_w, int efb_h, int scale, int samples) {
  if (!g_fullscreen || g_background_drawn || !g_have_screen || g_fullscreen->bgra.empty()) return false;
  bool insert = false;
  if (g_fullscreen_slot == 0) {
    bool textured = false;
    for (const gx::TextureRef& t : dc.textures) textured |= t.used;
    if (!textured) { ++g_leading_untextured; return false; }
    insert = gx::video_bg::learning::should_insert_fullscreen_layer(0, g_leading_untextured, textured);
  } else if (g_fullscreen_slot == 1) insert = g_last_dynamic_slot == 1;
  if (!insert) return false;
  g_background_drawn = true;
  id<MTLDevice> dev = (__bridge id<MTLDevice>)device;
  if (!ensure_device(dev)) return false;
  id<MTLTexture> tex = video_texture(dev, g_fullscreen_slot, *g_fullscreen);
  if (!tex) return false;
  id<MTLRenderCommandEncoder> enc = (__bridge id<MTLRenderCommandEncoder>)encoder;
  const double s = scale;
  const double x = g_screen.src_x * s, y = g_screen.src_y * s, w = g_screen.src_w * s, h = g_screen.src_h * s;
  [enc setRenderPipelineState:quad_efb(dev, samples)];
  [enc setDepthStencilState:g_no_depth];
  [enc setCullMode:MTLCullModeNone];
  [enc setViewport:MTLViewport{x, y, w, h, 0, 1}];
  [enc setScissorRect:MTLScissorRect{NSUInteger(x), NSUInteger(y), NSUInteger(std::min<double>(w, efb_w - x)), NSUInteger(std::min<double>(h, efb_h - y))}];
  const float q[4] = {0, 0, 1, 1};
  [enc setVertexBytes:q length:sizeof q atIndex:0];
  [enc setFragmentTexture:tex atIndex:0];
  [enc setFragmentSamplerState:g_linear atIndex:0];
  [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
  if (!g_layer_logged[g_fullscreen_slot]) {
    g_layer_logged[g_fullscreen_slot] = true;
    host::log("video backgrounds: %s full-screen layer drawn (Metal)", g_fullscreen_slot == 0 ? "CSS" : "SSS");
  }
  return true;
}

// ---- RetroArch shader presets (librashader) ----
void* g_libra_handle = nullptr;
std::string g_libra_path;
std::vector<std::string> library_candidates() {
  std::vector<std::string> out;
  if (const char* env = std::getenv("MELEE_LIBRASHADER")) out.push_back(env);
  char exe[4096]; uint32_t size = sizeof exe;
  if (_NSGetExecutablePath(exe, &size) == 0) {
    const fs::path macos = fs::path(exe).parent_path();
    out.push_back((macos.parent_path() / "Frameworks" / "librashader.dylib").string());   // the app bundle
    out.push_back((macos / "librashader.dylib").string());
  }
  out.push_back((mu_gfx_folder() / "librashader.dylib").string());   // the player's own copy
  out.push_back((mu_gfx_folder() / "Shaders" / "librashader.dylib").string());
  out.push_back("/opt/homebrew/lib/librashader.dylib");
  out.push_back("/usr/local/lib/librashader.dylib");
  return out;
}
}  // namespace
}  // namespace mu_gfx

void* mu_libra_dlopen(const char*, int flags) {
  for (const std::string& p : mu_gfx::library_candidates()) {
    std::error_code ec;
    if (!fs::exists(p, ec)) continue;
    if (void* h = ::dlopen(p.c_str(), flags)) { mu_gfx::g_libra_path = p; return h; }
    host::log("shaders: %s could not be loaded: %s", p.c_str(), dlerror());
  }
  return nullptr;
}

namespace mu_gfx {
namespace {
struct Presets {
  bool tried = false;
  libra_instance_t api{};
  bool loaded = false;
  libra_mtl_filter_chain_t chain = nullptr;
  std::string built, failed;
  id<MTLCommandQueue> queue = nil;
  id<MTLTexture> source = nil, output = nil;
  size_t frames = 0;
  std::mutex status_mutex;
  std::string status;
  void set_status(const std::string& s) { std::lock_guard<std::mutex> lock(status_mutex); status = s; }
  std::string error_text(libra_error_t e) {
    if (!e) return {};
    char* text = nullptr;
    std::string out = "librashader error";
    if (api.error_write && api.error_write(e, &text) == 0 && text) { out = text; if (api.error_free_string) api.error_free_string(&text); }
    if (api.error_free) api.error_free(&e);
    return out;
  }
  void release() {
    if (chain && api.mtl_filter_chain_free) api.mtl_filter_chain_free(&chain);
    chain = nullptr; built.clear();
  }
};
Presets g_presets;
fs::path shader_root() { return mu_gfx_folder() / "Shaders"; }
void* picture(const gx::PostFrame& pf) {
  Presets& p = g_presets;
  std::string preset;
  { std::lock_guard<std::mutex> lock(live().mutex); preset = live().shader_preset; }
  if (preset.empty()) { if (p.chain) p.release(); p.set_status(""); p.failed.clear(); return nullptr; }
  if (!p.tried) {
    p.tried = true;
    p.api = librashader_load_instance();
    p.loaded = p.api.instance_loaded && p.api.mtl_filter_chain_create != nullptr;
    if (p.loaded) host::log("shaders: librashader loaded from %s (API %zu)", g_libra_path.c_str(), p.api.instance_api_version());
    else host::log("shaders: librashader.dylib not found (the bundle's Frameworks, or %s)", mu_gfx_folder().c_str());
  }
  if (!p.loaded) { p.set_status("librashader.dylib is missing: put it in the melee-unlocked folder"); return nullptr; }
  if (preset == p.failed) return nullptr;
  id<MTLDevice> device = (__bridge id<MTLDevice>)pf.device;
  if (preset != p.built) {
    p.release();
    const fs::path file = shader_root() / preset;
    std::error_code ec;
    if (!fs::exists(file, ec)) { p.failed = preset; p.set_status("not found in the Shaders folder"); host::log("shaders: %s is not in %s", preset.c_str(), shader_root().c_str()); return nullptr; }
    if (!p.queue) p.queue = [device newCommandQueue];
    libra_shader_preset_t sp = nullptr;
    std::string why = p.error_text(p.api.preset_create(file.string().c_str(), &sp));
    if (why.empty()) {
      filter_chain_mtl_opt_t opt{}; opt.version = LIBRASHADER_CURRENT_VERSION; opt.force_no_mipmaps = false;
      why = p.error_text(p.api.mtl_filter_chain_create(&sp, p.queue, &opt, &p.chain));
    }
    if (sp && p.api.preset_free) p.api.preset_free(&sp);
    if (!why.empty() || !p.chain) {
      p.failed = preset; p.chain = nullptr; p.set_status(why.empty() ? "the preset did not load" : why);
      host::log("shaders: %s did not load: %s", preset.c_str(), why.c_str());
      return nullptr;
    }
    p.built = preset; p.frames = 0; p.set_status("");
    host::log("shaders: %s running", preset.c_str());
  }
  if (!pf.w || !pf.h || !pf.out_w || !pf.out_h) return nullptr;
  if (!p.source || p.source.width != pf.w || p.source.height != pf.h) {
    MTLTextureDescriptor* td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:pf.w height:pf.h mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead; td.storageMode = MTLStorageModePrivate;
    p.source = [device newTextureWithDescriptor:td];
  }
  if (!p.output || p.output.width != pf.out_w || p.output.height != pf.out_h) {
    MTLTextureDescriptor* td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:pf.out_w height:pf.out_h mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget; td.storageMode = MTLStorageModePrivate;
    p.output = [device newTextureWithDescriptor:td];
  }
  id<MTLCommandBuffer> cmd = (__bridge id<MTLCommandBuffer>)pf.command_buffer;
  id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];   // the picture's region of the EFB as the preset's source
  [blit copyFromTexture:(__bridge id<MTLTexture>)pf.color sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(pf.x, pf.y, 0)
             sourceSize:MTLSizeMake(pf.w, pf.h, 1) toTexture:p.source destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
  [blit endEncoding];
  libra_viewport_t vp{0, 0, uint32_t(pf.out_w), uint32_t(pf.out_h)};
  frame_mtl_opt_t opt{}; opt.version = LIBRASHADER_CURRENT_VERSION; opt.frame_direction = 1; opt.total_subframes = 1; opt.current_subframe = 1;
  opt.frames_per_second = 60.0f; opt.brightness_nits = 200.0f;
  const std::string why = p.error_text(p.api.mtl_filter_chain_frame(&p.chain, cmd, p.frames++, p.source, p.output, &vp, nullptr, &opt));
  if (!why.empty()) { p.failed = preset; p.release(); p.set_status(why); host::log("shaders: %s stopped: %s", preset.c_str(), why.c_str()); return nullptr; }
  return (__bridge void*)p.output;
}

// ---- frame capture bursts and the glitch detector ----
gx::FlickerScanner g_scanner;
id<MTLTexture> g_reduce_target = nil;
uint32_t g_burst_index = 0;
int g_test_burst_at = -1, g_test_burst_count = 0;
// Tests (MELEE_TEST_GLITCH_AT=N): presented frame N's reduction gets a bright block, a glitch the detector must report.
int g_test_glitch_at = [] { const char* v = std::getenv("MELEE_TEST_GLITCH_AT"); return v ? std::atoi(v) : -1; }();
void capture_frame(const gx::PostFrame& pf, uint64_t number) {
  id<MTLDevice> device = (__bridge id<MTLDevice>)pf.device;
  id<MTLTexture> src = (__bridge id<MTLTexture>)pf.color;
  if (!pf.w || !pf.h) return;
  const NSUInteger row = NSUInteger(pf.w) * 4;
  id<MTLBuffer> buffer = [device newBufferWithLength:row * pf.h options:MTLResourceStorageModeShared];
  id<MTLCommandBuffer> cmd = (__bridge id<MTLCommandBuffer>)pf.command_buffer;
  id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
  [blit copyFromTexture:src sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(pf.x, pf.y, 0) sourceSize:MTLSizeMake(pf.w, pf.h, 1)
               toBuffer:buffer destinationOffset:0 destinationBytesPerRow:row destinationBytesPerImage:row * pf.h];
  [blit endEncoding];
  const bool bgra = src.pixelFormat == MTLPixelFormatBGRA8Unorm;
  const uint32_t w = pf.w, h = pf.h;
  const std::string path = (mu_gfx_folder() / "capture" / ([&] { char n[40]; std::snprintf(n, sizeof n, "blink_%05llu.ppm", (unsigned long long)number); return std::string(n); })()).string();
  [cmd addCompletedHandler:^(id<MTLCommandBuffer>) {
    dispatch_async(writer_queue(), ^{
      std::error_code ec; fs::create_directories(fs::path(path).parent_path(), ec);
      std::ofstream f(path, std::ios::binary);
      f << "P6\n" << w << ' ' << h << "\n255\n";
      const uint8_t* p = static_cast<const uint8_t*>(buffer.contents);
      std::vector<uint8_t> line(size_t(w) * 3);
      for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
          const uint8_t* px = p + size_t(y) * w * 4 + size_t(x) * 4;
          line[x * 3] = bgra ? px[2] : px[0]; line[x * 3 + 1] = px[1]; line[x * 3 + 2] = bgra ? px[0] : px[2];
        }
        f.write(reinterpret_cast<const char*>(line.data()), std::streamsize(line.size()));
      }
      host::log("captured %s (%ux%u)", path.c_str(), w, h);
    });
  }];
}
void scan_frame(const gx::PostFrame& pf) {
  id<MTLDevice> device = (__bridge id<MTLDevice>)pf.device;
  if (!ensure_device(device) || !pf.w || !pf.h) return;
  constexpr int G = gx::FlickerScanner::kGrid;
  if (!g_reduce_target) {
    MTLTextureDescriptor* td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:G height:G mipmapped:NO];
    td.usage = MTLTextureUsageRenderTarget; td.storageMode = MTLStorageModePrivate;
    g_reduce_target = [device newTextureWithDescriptor:td];
  }
  id<MTLCommandBuffer> cmd = (__bridge id<MTLCommandBuffer>)pf.command_buffer;
  MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
  rp.colorAttachments[0].texture = g_reduce_target;
  rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;
  rp.colorAttachments[0].storeAction = MTLStoreActionStore;
  id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rp];
  struct { float rect[4]; int32_t taps[2]; int32_t grid[2]; } r{{float(pf.x), float(pf.y), float(pf.w), float(pf.h)},
    {std::clamp(int(pf.w / G), 1, 16), std::clamp(int(pf.h / G), 1, 16)}, {G, G}};
  const float q[4] = {0, 0, 1, 1};
  [enc setRenderPipelineState:g_reduce];
  [enc setVertexBytes:q length:sizeof q atIndex:0];
  [enc setFragmentBytes:&r length:sizeof r atIndex:0];
  [enc setFragmentTexture:(__bridge id<MTLTexture>)pf.color atIndex:0];
  [enc setFragmentSamplerState:g_linear atIndex:0];
  [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
  [enc endEncoding];
  id<MTLBuffer> buffer = [device newBufferWithLength:G * G * 4 options:MTLResourceStorageModeShared];
  id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
  [blit copyFromTexture:g_reduce_target sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(G, G, 1)
               toBuffer:buffer destinationOffset:0 destinationBytesPerRow:G * 4 destinationBytesPerImage:G * G * 4];
  [blit endEncoding];
  const uint32_t frame = uint32_t(pf.frame + 1);
  [cmd addCompletedHandler:^(id<MTLCommandBuffer>) {
    dispatch_async(writer_queue(), ^{
      static float grid[G * G];
      const uint8_t* p = static_cast<const uint8_t*>(buffer.contents);
      for (int i = 0; i < G * G; ++i) grid[i] = p[i * 4] * 0.25f + p[i * 4 + 1] * 0.6f + p[i * 4 + 2] * 0.15f;   // upstream's weights
      if (int(frame) == g_test_glitch_at)
        for (int y = 20; y < 30; ++y) for (int x = 20; x < 30; ++x) grid[y * G + x] = std::min(255.0f, grid[y * G + x] + 120.0f);
      g_scanner.push(grid, frame);
    });
  }];
}
bool wants_final() { return live().capture_request.load(std::memory_order_relaxed) > 0 || (g_test_burst_at >= 0); }
void presented(const gx::PostFrame& pf) {
  auto& l = live();
  if (g_test_burst_at >= 0 && pf.frame + 1 >= uint64_t(g_test_burst_at)) { l.capture_request.store(g_test_burst_count); g_test_burst_at = -1; }
  if (const int want = l.capture_request.load(std::memory_order_relaxed); want > 0) {
    if (g_burst_index == 0) host::log("capture: writing the next %d presented frames into %s", want, (mu_gfx_folder() / "capture").c_str());
    gx::PostFrame shot = pf;   // the picture as presented: picture options, sharpening and SSAO applied
    if (pf.final_color) { shot.color = pf.final_color; shot.x = shot.y = 0; }
    capture_frame(shot, pf.frame + 1);
    ++g_burst_index;
    if (l.capture_request.fetch_sub(1) == 1) { g_burst_index = 0; host::log("capture: done"); }
  }
  if (l.flicker_scan.load(std::memory_order_relaxed)) scan_frame(pf);
}

void begin_frame(const gx::Frame& frame) {
  static bool once = false;
  if (!once) {
    once = true;
    if (const char* b = std::getenv("MELEE_CAPTURE_BURST")) {   // tests: FRAME,COUNT presented frames from FRAME
      int at = 0, count = 0;
      if (std::sscanf(b, "%d,%d", &at, &count) == 2 && count > 0) { g_test_burst_at = at; g_test_burst_count = count; }
    }
  }
  textures_begin_frame();
  video_begin_frame(frame);
}
}  // namespace

const gx::PostHooks* hooks() {
  static gx::PostHooks h = [] {
    gx::PostHooks p;
    p.begin_frame = begin_frame;
    p.replace = replace;
    p.dynamic_wanted = dynamic_wanted;
    p.dynamic = dynamic;
    p.before_draw = before_draw;
    p.picture = picture;
    p.presented = presented;
    p.wants_final = wants_final;
    return p;
  }();
  return &h;
}
void report() {
  gx::texpack::prefetch_join();
  if (gx::texpack::enabled() || g_dumped) { gx::texpack::report(); host::log("graphics: %llu textures replaced, %llu dumped", (unsigned long long)g_replaced, (unsigned long long)g_dumped); }
  g_presets.release();
}
}  // namespace mu_gfx

// ---- the panel's answers (fe/gfx_live.h) ----
namespace fe::gfx {
std::vector<std::pair<std::string, bool>> texture_packs() {
  std::lock_guard<std::mutex> lock(mu_gfx::g_pack_mutex);
  return mu_gfx::g_pack_list;
}
std::vector<std::string> shader_presets() {
  std::vector<std::string> out;
  const fs::path root = mu_gfx::shader_root();
  std::error_code ec;
  if (!fs::is_directory(root, ec)) return out;
  for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end; it != end && !ec; it.increment(ec)) {
    if (!it->is_regular_file(ec)) continue;
    const std::string ext = it->path().extension().string();
    if (ext == ".slangp") out.push_back(fs::relative(it->path(), root, ec).generic_string());
    if (out.size() >= 2000) break;
  }
  std::sort(out.begin(), out.end());
  return out;
}
std::string shader_status() { std::lock_guard<std::mutex> lock(mu_gfx::g_presets.status_mutex); return mu_gfx::g_presets.status; }
std::string video_status() {
  std::string css = gx::video_bg::status(0), sss = gx::video_bg::status(1);
  if (css.empty() && sss.empty()) return "Open the folder";
  return "CSS: " + (css.empty() ? std::string("-") : css) + "; SSS: " + (sss.empty() ? std::string("-") : sss);
}
void open_folder(int which) {
  switch (which) {
    case 0: gx::texpack::open_packs_folder(); return;
    case 1: gx::texpack::open_dump_folder(); return;
    case 2: gx::video_bg::open_folder(); return;
    default: break;
  }
  static const char* const names[] = {"", "", "", "Shaders", "capture", "flicker", "Lab"};
  if (which < 3 || which > 6) return;
  const fs::path folder = mu_gfx_folder() / names[which];
  std::error_code ec; fs::create_directories(folder, ec);
  if (std::getenv("MELEE_NO_FINDER")) return;   // tests: never open a Finder window
  const std::string path = folder.string();
  pid_t pid = 0;
  const char* argv[] = {"/usr/bin/open", path.c_str(), nullptr};
  if (posix_spawn(&pid, "/usr/bin/open", nullptr, nullptr, const_cast<char* const*>(argv), nullptr) == 0) waitpid(pid, nullptr, 0);
}
}  // namespace fe::gfx
