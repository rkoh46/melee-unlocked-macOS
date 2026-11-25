// Dear ImGui on the engine's Metal renderer: one context, drawn last in the present pass (gx::metal_set_ui_pass) with
// imgui_impl_metal (fetched by mu-engine/fetch.sh at melee-unlocked's ImGui version). melee-unlocked's settings panel
// (mu_panel.cpp) is the only thing drawn in it. The panel is driven by the game's controllers, the keyboard included
// (the app's keyboard layout makes it a controller), as the menu's poll hands them over (app_menu_frame); the mouse
// position and buttons come from SDL for anything that wants a pointer.
// SPDX-License-Identifier: GPL-3.0-or-later
#import <Metal/Metal.h>

#include "ui_metal.h"
#include "mu_ui.h"
#include "frontend.h"
#include "aurora/imgui.h"

#include <imgui.h>
#include "imgui_impl_metal.h"
#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>

namespace {
id<MTLDevice> g_device = nil;
bool g_ready = false;
double g_last = 0.0;

double now_seconds() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

void ensure_context(id<MTLDevice> device) {
  if (g_ready) return;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;     // nothing of ImGui's own is saved: the panel keeps its settings in the app's files
  io.LogFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;
  io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
  g_device = device ?: MTLCreateSystemDefaultDevice();
  ImGui_ImplMetal_Init(g_device);
  fe::mu_ui::fonts_init();
  g_ready = true;
}

// The pointer, in the window's points (SDL's mouse state; the panel itself ignores it while it is shown).
void feed_mouse(ImGuiIO& io, float scale) {
  float x = 0, y = 0;
  const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&x, &y);
  SDL_Window* focus = SDL_GetMouseFocus();
  if (!focus) { io.AddMousePosEvent(-FLT_MAX, -FLT_MAX); return; }
  const float density = SDL_GetWindowPixelDensity(focus);
  io.AddMousePosEvent(x * density / scale, y * density / scale);
  io.AddMouseButtonEvent(0, (buttons & SDL_BUTTON_LMASK) != 0);
  io.AddMouseButtonEvent(1, (buttons & SDL_BUTTON_RMASK) != 0);
}

// One ImGui frame at w x h pixels; drawn when there is an encoder (a hidden window only runs the frame).
void ui_frame(void* encoder, void* pass, void* command_buffer, int w, int h) {
  if (w <= 0 || h <= 0) return;
  id<MTLRenderCommandEncoder> enc = (__bridge id<MTLRenderCommandEncoder>)encoder;
  ensure_context(enc ? enc.device : nil);
  ImGuiIO& io = ImGui::GetIO();
  // The panel is laid out for a window about 720 points high: whole multiples of that keep its pictures sharp.
  const float scale = std::max(1.0f, std::floor(float(h) / 700.0f));
  io.DisplaySize = ImVec2(float(w) / scale, float(h) / scale);
  io.DisplayFramebufferScale = ImVec2(scale, scale);
  const double t = now_seconds();
  io.DeltaTime = g_last > 0.0 ? float(std::clamp(t - g_last, 1.0 / 1000.0, 0.1)) : 1.0f / 60.0f;
  g_last = t;
  feed_mouse(io, scale);
  if (pass) ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor*)pass);
  ImGui::NewFrame();
  fe::overlay_draw(int(io.DisplaySize.x), int(io.DisplaySize.y));   // the feature modules' overlays, the replay viewer, the Practice menu
  fe::mu_ui::panel_draw(int(io.DisplaySize.x), int(io.DisplaySize.y));
  ImGui::Render();
  if (enc) ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (__bridge id<MTLCommandBuffer>)command_buffer, enc);
}
}  // namespace

namespace mu_ui_metal {
gx::UiPass pass() { return ui_frame; }
}

// The calls mu_panel.cpp makes into Aurora on the Switch, answered here.
ImTextureID aurora_imgui_add_texture(uint32_t width, uint32_t height, const void* rgba8) {
  if (!g_device) g_device = MTLCreateSystemDefaultDevice();
  MTLTextureDescriptor* td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
  td.usage = MTLTextureUsageShaderRead;
  id<MTLTexture> texture = [g_device newTextureWithDescriptor:td];
  [texture replaceRegion:MTLRegionMake2D(0, 0, width, height) mipmapLevel:0 withBytes:rgba8 bytesPerRow:width * 4];
  return ImTextureID(uintptr_t((__bridge_retained void*)texture));   // kept for the life of the program, as Aurora's are
}
void aurora_imgui_host_gamepad(bool) {}   // the gamepad keys come from the app's controllers only (there is no other source here)
