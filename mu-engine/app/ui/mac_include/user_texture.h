// The Practice menu's own pictures (the game's font from the disc, stock icons) as ImTextureData. On the Switch the
// frontend's ImGui (Aurora's 1.91) lacks that type and fe/proai/include/user_texture.h supplies a stand-in; here ImGui is
// melee-unlocked's 1.92, which has it: a texture is registered with ImGui once, and the Metal back end uploads it.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "imgui.h"
#include "imgui_internal.h"

namespace gx {
inline void keep_user_texture(ImTextureData* tex, ImGuiContext*& registered_in) {
  if (!tex || tex->Width <= 0 || registered_in == ImGui::GetCurrentContext()) return;
  ImGui::RegisterUserTexture(tex);
  registered_in = ImGui::GetCurrentContext();
}
}  // namespace gx
