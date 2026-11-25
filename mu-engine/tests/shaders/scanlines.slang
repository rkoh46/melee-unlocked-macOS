#version 450
// A small scanline and aperture-mask look for testing the engine's RetroArch shader presets (mu-engine/tools/app_graphics_test.sh).
// Written for melee-unlocked-macOS. SPDX-License-Identifier: GPL-3.0-or-later

layout(push_constant) uniform Push {
  vec4 SourceSize;
  vec4 OutputSize;
  uint FrameCount;
  float LINES;
  float STRENGTH;
} params;

#pragma parameter LINES "Scanlines across the picture" 240.0 120.0 480.0 30.0
#pragma parameter STRENGTH "Scanline strength" 0.45 0.0 1.0 0.05

layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; } global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
void main() {
  gl_Position = global.MVP * Position;
  vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
void main() {
  vec3 c = texture(Source, vTexCoord).rgb;
  float s = sin(vTexCoord.y * params.LINES * 3.14159265);
  float line = 1.0 - params.STRENGTH * (1.0 - s * s);
  int column = int(mod(vTexCoord.x * params.OutputSize.x, 3.0));
  vec3 mask = column == 0 ? vec3(1.0, 0.8, 0.8) : column == 1 ? vec3(0.8, 1.0, 0.8) : vec3(0.8, 0.8, 1.0);
  FragColor = vec4(c * line * mask * 1.15, 1.0);
}
