// The two Aurora calls melee-unlocked's settings panel makes on the Switch (Melee-NX-NetPlay patch 0112), answered by
// ui_metal.mm on the Mac.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <imgui.h>

ImTextureID aurora_imgui_add_texture(uint32_t width, uint32_t height, const void* rgba8);
void aurora_imgui_host_gamepad(bool on);
