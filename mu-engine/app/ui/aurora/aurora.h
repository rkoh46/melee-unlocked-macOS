// The picture options the frontend's overlays set through Aurora on the Switch (Melee-NX-NetPlay patch 0109), answered
// by ui_metal.mm on the Mac: the internal resolution and sharpening go to the Metal renderer; brightness, contrast,
// vibrance, the CRT look and the picture shape are not drawn by it yet (logged).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>

typedef struct {
  float brightness, contrast, vibrance, sharpness;
  uint32_t crt;
  float crt_curve;
} AuroraPicture;
void aurora_set_picture(const AuroraPicture* picture);
namespace aurora::window {
void set_frame_buffer_aspect(float aspect);
void set_frame_buffer_scale(float scale);
}
