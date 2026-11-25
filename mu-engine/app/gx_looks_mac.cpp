// melee-unlocked's looks on the Mac's Metal renderer: the look presets' surfaces, the Lightweight ladder, Visual effects,
// the look's background and True widescreen, decided per draw (gx::LookHooks, port/runtime/gx/gx_metal.h). Display only:
// the renderer changes what it draws, never game memory, so the simulation, online play and replays are the same at
// every look.
//
// Adapted from Melee-NX-NetPlay's frontend/patches/0116-gx-looks.patch (its Aurora bridge part), which adapts
// melee-unlocked v1.0.5's rules (fetched tree: port/runtime/gx/gx_retro.h, the lightweight and effects parts of gx_core.h,
// the tint in gx_shader.cpp fill_ps_constants, build_projection's True widescreen). Here each draw carries its whole
// register state (DrawCall), so the rules read it directly; the draw owner tokens (0xF1 fighter slot, 0xF2 skinned, 0xF7
// stage map tag in DrawCall::viewport_x) come from the decoder copy (mu-engine/app/CMakeLists.txt).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gx_core.h"
#include "gx_metal.h"
#include "looks_live.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

namespace mu_looks {
namespace {
using fe::looks::Surface;
using gx::DrawCall;
using gx::Frame;
constexpr uint8_t kEffectOwner = 0xFE, kModelEffectOwner = 0xFC, kGameplayOwner = 0xFD;
constexpr float kBackgroundDepth = 60.0f;
struct Filter {
  bool on = false, in_match = false, have_fighters = false, backdrop_off = false, wide = false;
  int level = 0, effects = 0;
  uint32_t projection[6] = {};
  float fighter_far = 0;
  uint8_t fighter = 0, stage = 0, menus = 0, colours = 0;
  bool unfiltered = false;
};
// Render thread only.
Filter g_filter;
uint64_t g_ns = 0, g_skipped = 0, g_drawn = 0, g_tinted = 0, g_wired = 0;

// melee-unlocked's frame_in_match (fetched tree port/runtime/gx/gx_core.h, GPL-2.0-or-later), newer than the app decoder's:
// replay playback (debug VS, 0x0E minor 1), Event Match and the 1P modes' own scene numbering count as matches.
bool in_match(const Frame& f) {
  const uint8_t major = f.scene_major, minor = f.scene_minor;
  if (major == 0x08) return minor == 2;
  if (major == 0x0E) return minor == 1;
  if (major == 0x2B) return minor == 1;
  if (major == 0x03) return minor < 0x68 && (minor & 7) == 1;
  if (major == 0x04) {
    static constexpr uint8_t kFights[] = {0x01, 0x03, 0x09, 0x0A, 0x11, 0x12, 0x19, 0x1B, 0x21, 0x23, 0x25,
                                          0x29, 0x2B, 0x31, 0x3A, 0x3B, 0x41, 0x49, 0x51, 0x53, 0x59, 0x5C};
    for (uint8_t id : kFights) if (minor == id) return true;
    return false;
  }
  if (major == 0x05) return minor < 0x68;
  if (major == 0x0F || (major >= 0x20 && major <= 0x26)) return minor == 1;
  const bool match_mode = major == 0x02 || (major >= 0x10 && major <= 0x13) || major == 0x1B || major == 0x1C || (major >= 0x1D && major <= 0x1F);
  return match_mode && minor >= 2;
}
inline uint8_t stage_map(const DrawCall& d) { return uint8_t(d.viewport_x); }   // the 0xF7 tag (decoder copy)
inline bool world(const Filter& e, const DrawCall& st) {
  return e.in_match && st.xf_regs[0x26] == 0 && std::memcmp(e.projection, &st.xf_regs[0x20], sizeof e.projection) == 0;
}
// The draw's vertex positions in view space (x, y, z per vertex) from the decoded vertices.
bool view_positions(const Frame& f, const DrawCall& d, std::vector<float>& out) {
  out.clear();
  if (d.first_vertex + size_t(d.vertex_count) > f.vertices.size()) return false;
  for (uint32_t i = 0; i < d.vertex_count; ++i) {
    const gx::Vertex& v = f.vertices[d.first_vertex + i];
    const uint32_t row = (d.components & gx::VB_HAS_POSMTXIDX) ? v.posmtx & 63 : d.matrix_index_a & 63;
    if (row + 2 >= 64) return false;
    const float* m = &d.posMatrices[row * 4];
    for (int r = 0; r < 3; ++r) {
      const float c = m[r * 4] * v.pos[0] + m[r * 4 + 1] * v.pos[1] + m[r * 4 + 2] * v.pos[2] + m[r * 4 + 3];
      if (!std::isfinite(c)) return false;
      out.push_back(c);
    }
  }
  return true;
}
// lightweight_filter_build: the world camera (a fighter's projection) and the play plane's depth.
Filter build(const Frame& f) {
  Filter e;
  const fe::looks::Live& l = fe::looks::live();
  e.level = fe::looks::load(l.lightweight); e.effects = fe::looks::load(l.effects);
  e.fighter = fe::looks::load(l.fighter); e.stage = fe::looks::load(l.stage); e.menus = fe::looks::load(l.menus);
  e.colours = fe::looks::load(l.colours); e.unfiltered = l.unfiltered.load(std::memory_order_relaxed);
  e.backdrop_off = e.level >= 2 || fe::looks::load(l.arcade_backdrop) != 0;
  if (e.level >= 3) e.effects = std::max(e.effects, 2);
  // True widescreen: never on the opening movie (upstream's frame_has_widenable_scene).
  e.wide = l.true_widescreen.load(std::memory_order_relaxed) && f.scene_major != 0x18;
  fe::looks::live().scene_major.store(f.scene_major, std::memory_order_relaxed);
  e.on = fe::looks::active() || e.menus || e.wide || l.flash_on.load(std::memory_order_relaxed);   // the L-cancel flash too
  if (!e.on || !in_match(f)) return e;
  const DrawCall* first = nullptr; bool have = false;
  struct Model { uint32_t projection[6]; float z; };
  static std::vector<Model> models; models.clear();
  for (const auto& cmd : f.commands) {
    if (cmd.kind != gx::FrameCommand::Draw) continue;
    const DrawCall& d = f.draws[cmd.index];
    if (d.xf_regs[0x26] != 0) continue;
    if (!first) { first = &d; std::memcpy(e.projection, &d.xf_regs[0x20], sizeof e.projection); }
    if (!d.skinned) continue;
    if (!have && d.owner_player < 6) { have = true; std::memcpy(e.projection, &d.xf_regs[0x20], sizeof e.projection); }
    const uint32_t row = d.matrix_index_a & 63;
    if (row + 2 >= 64) continue;
    Model m; std::memcpy(m.projection, &d.xf_regs[0x20], sizeof m.projection); m.z = d.posMatrices[(row + 2) * 4 + 3];
    if (std::isfinite(m.z)) models.push_back(m);
  }
  if (!first) return e;
  e.in_match = true;
  for (const auto& m : models) {
    if (std::memcmp(m.projection, e.projection, sizeof e.projection) != 0) continue;
    if (!e.have_fighters || m.z > e.fighter_far) e.fighter_far = m.z;
    e.have_fighters = true;
  }
  return e;
}
// lightweight_background: a stage piece wholly behind the play plane, a dome around the camera, or scenery the game names.
bool background(const Filter& e, const Frame& f, const DrawCall& d) {
  const uint8_t tag = stage_map(d);
  if (e.have_fighters && tag != 0xFF && (tag & 0x80)) return true;
  if (!e.have_fighters || d.owner_player != 0xFF || d.skinned) return false;
  static std::vector<float> p;
  if (!view_positions(f, d, p) || p.empty()) return false;
  const bool band = e.level >= 5;
  const bool tight = tag != 0xFF && (tag & 0x40);
  const float limit = e.fighter_far - (tight ? 8.0f : band ? 20.0f : kBackgroundDepth);
  const float front = e.fighter_far + 30.0f;
  float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
  const size_t n = p.size() / 3;
  for (size_t i = 0; i < n; ++i)
    for (int r = 0; r < 3; ++r) { lo[r] = std::min(lo[r], p[i * 3 + r]); hi[r] = std::max(hi[r], p[i * 3 + r]); }
  if (lo[0] < 0 && hi[0] > 0 && lo[1] < 0 && hi[1] > 0 && lo[2] < 0 && hi[2] > 0) return true;   // around the camera
  bool seen = false;
  for (size_t i = 0; i < n; ++i) {
    const float x = p[i * 3], y = p[i * 3 + 1], z = p[i * 3 + 2];
    if (z >= 0 || std::abs(x) > -z * 1.5f || std::abs(y) > -z * 1.5f) continue;   // out of view
    seen = true;
    if (z >= limit && (!band || z <= front)) return false;   // something visible is on the play plane
  }
  if (seen) return true;
  if (hi[2] < limit || (band && lo[2] > front)) return true;
  return hi[0] - lo[0] > 3000.0f || hi[1] - lo[1] > 3000.0f || hi[2] - lo[2] > 3000.0f;
}
// DrawCall::frustum_backdrop upstream: one quad with an identity model-view filling the camera's view (a sky plane).
bool frustum_backdrop(const Frame& f, const DrawCall& d) {
  if (d.primitive != 0x80 || d.vertex_count != 4 || d.xf_regs[0x26] != 0 || (d.components & gx::VB_HAS_POSMTXIDX) || (d.bp.zmode() & 0x10)) return false;
  const uint32_t row = d.matrix_index_a & 63;
  if (row + 3 > 64) return false;
  static const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
  for (int i = 0; i < 12; ++i) if (std::fabs(d.posMatrices[row * 4 + i] - identity[i]) > 1e-5f) return false;
  static std::vector<float> v;
  if (!view_positions(f, d, v) || v.size() != 12) return false;
  float proj[4]; std::memcpy(proj, &d.xf_regs[0x20], sizeof proj);
  for (int i = 0; i < 4; ++i) {
    const float x0 = v[i * 3], y0 = v[i * 3 + 1], z0 = v[i * 3 + 2];
    if (!(z0 < 0.0f)) return false;
    const float x = (proj[0] * x0 + proj[1] * z0) / -z0, y = (proj[2] * y0 + proj[3] * z0) / -z0;
    if (std::fabs(std::fabs(x) - 1.0f) > 0.01f || std::fabs(std::fabs(y) - 1.0f) > 0.01f) return false;
  }
  return true;
}
// skip_for_effects (Visual effects).
bool effect(const Filter& e, const DrawCall& d) {
  if (d.owner_player == kGameplayOwner || d.owner_player < 6) return false;
  if (e.effects <= 0 || !(d.bp.blendmode() & 1) || !world(e, d)) return false;
  const uint32_t zmode = d.bp.zmode();
  if (zmode & 0x10) return false;
  const bool additive = ((d.bp.blendmode() >> 5) & 7) == 1;
  if (d.owner_player == kModelEffectOwner) return e.effects >= 3 && additive;
  if (additive || (e.effects >= 2 && !(zmode & 1))) return true;
  return e.effects >= 3 && d.owner_player == kEffectOwner;
}
// ---- True widescreen (melee-unlocked gx_shader.cpp build_projection, set_true_widescreen): at 16:9 the x row of a
// widened projection is scaled by 219/320. Widened: the world's perspective cameras and the P1/P2 tag layer. Not
// widened, as upstream: render-to-texture views, the screen flash and its wipes, corner-pinned perspective layers,
// backdrop planes that fill the frustum and the orthographic 2D and shadow layers.
constexpr float kWiden = 219.0f / 320.0f;
inline const float* proj(const DrawCall& d) { return reinterpret_cast<const float*>(&d.xf_regs[0x20]); }
bool authored_fullscreen(const float* p) {
  if (p[1] != 0.0f || p[3] != 0.0f || p[0] <= 0.0f || p[2] <= 0.0f) return false;
  if (std::fabs(p[2] - 1.7320508f) > 0.001f) return false;
  return std::fabs(p[2] / p[0] - 4.0f / 3.0f) < 0.001f;
}
bool authored_screen_pinned(const float* p) {
  if (p[0] <= 0.0f || p[2] <= 0.0f) return false;
  if (std::fabs(std::fabs(p[1]) - 1.0f) > 0.001f || std::fabs(std::fabs(p[3]) - 1.0f) > 0.001f) return false;
  return std::fabs(p[2] / p[0] - 4.0f / 3.0f) < 0.001f;
}
bool nametag_layer(const float* p) {
  return std::fabs(p[0] - 2.0f / 640.0f) < 1e-6f && std::fabs(p[1] + 1.0f) < 1e-4f && std::fabs(p[2] - 2.0f / 480.0f) < 1e-6f &&
         std::fabs(p[3] - 1.0f) < 1e-4f && std::fabs(p[4] + 1.0f / 32767.9f) < 2e-6f;
}
bool widen(const Frame& f, const DrawCall& d) {
  const float* p = proj(d);
  if (d.xf_regs[0x26] != 0) return nametag_layer(p);
  const float* vp = reinterpret_cast<const float*>(&d.xf_regs[0x1A]);
  if (!(std::fabs(vp[0]) >= 320.0f && std::fabs(vp[1]) >= 240.0f)) return false;
  return !authored_fullscreen(p) && !authored_screen_pinned(p) && !frustum_backdrop(f, d);
}

uint64_t since(std::chrono::steady_clock::time_point t0) {
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count());
}
bool begin_frame(const Frame& f) {
  const auto t0 = std::chrono::steady_clock::now();
  g_filter = build(f);
  g_ns = since(t0);
  return g_filter.on;
}
void decide_draw(const Frame& f, const DrawCall& d, gx::LookDecision* out);
void decide(const Frame& f, size_t, const DrawCall& d, gx::LookDecision* out) {
  const auto t0 = std::chrono::steady_clock::now();
  decide_draw(f, d, out);
  g_ns += since(t0);
}
void decide_draw(const Frame& f, const DrawCall& d, gx::LookDecision* out) {
  const Filter& e = g_filter;
  const bool in_world = world(e, d);
  out->widen = e.wide && widen(f, d);
  out->widen_by = kWiden;
  if (e.in_match && effect(e, d)) { out->skip = true; ++g_skipped; return; }
  if (in_world && e.backdrop_off) {
    if (d.owner_player == 0xFF && !d.skinned && fe::looks::load(fe::looks::live().arcade_backdrop) && frustum_backdrop(f, d)) { out->skip = true; ++g_skipped; return; }
    if (background(e, f, d)) { out->skip = true; ++g_skipped; return; }
  }
  if (e.in_match) ++g_drawn;
  const bool owned_effect = d.owner_player == kGameplayOwner || d.owner_player == kModelEffectOwner;
  // look_surface: fighters (skinned models of the world camera), the stage (the rest of it), menus (everything else).
  Surface s = Surface::Textured;
  if (!owned_effect) {
    if (in_world) s = Surface(d.skinned ? e.fighter : e.stage);
    else s = Surface(e.menus);
  }
  // Lightweight 3 and up: no textures on world draws (one white texel), except blended draws that write no depth.
  const bool blended_nodepth = (d.bp.blendmode() & 1) && !(d.bp.zmode() & 0x10);
  if (s == Surface::Textured && e.level >= 3 && in_world && !owned_effect && !blended_nodepth) s = Surface::White;
  out->wire = in_world && !owned_effect && (e.level >= 5 || s == Surface::Wire);
  if (out->wire) ++g_wired;
  if (s == Surface::Wire) s = Surface::Textured;
  out->mark = uint8_t(s) | ((e.unfiltered && s != Surface::Textured) ? 8 : 0);
  // The L-cancel flash (Melee-NX-NetPlay frontend/patches/0120; upstream gx_shader.cpp's player tint): the flashing slot's
  // skinned model of the perspective camera takes the colour (fe/features/overlays.cpp sets it), not its shadow pass or
  // effects, over every look.
  if (d.skinned && d.owner_player < 6 && d.xf_regs[0x26] == 0) {
    const uint32_t flash = fe::looks::live().flash[d.owner_player].load(std::memory_order_relaxed);
    if (flash) {
      out->tint = true;
      out->rgb[0] = float((flash >> 16) & 0xFF) / 255.0f; out->rgb[1] = float((flash >> 8) & 0xFF) / 255.0f; out->rgb[2] = float(flash & 0xFF) / 255.0f;
      ++g_tinted; fe::looks::live().flashed.fetch_add(1, std::memory_order_relaxed);
      return;
    }
  }
  // The look's port colours (fighters' models) and its white stage (opaque stage pieces): one plain colour, light and shade kept.
  if (e.colours == 1 && d.skinned && d.owner_player < 4 && d.xf_regs[0x26] == 0) {
    out->tint = true; std::memcpy(out->rgb, fe::looks::port_colour(d.owner_player), sizeof out->rgb);
  } else if (in_world && !d.skinned && !(d.bp.blendmode() & 1) && Surface(e.stage) == Surface::White) {
    out->tint = true; out->rgb[0] = 0.93f; out->rgb[1] = 0.94f; out->rgb[2] = 0.96f;
  }
  if (out->tint) ++g_tinted;
}
uint32_t clear_colour(const Frame& f, uint32_t argb) { return fe::looks::clear_colour(in_match(f), argb); }
bool treat(uint8_t mark, std::vector<uint8_t>& px, uint32_t& w, uint32_t& h) {
  const bool done = fe::looks::treat(Surface(mark & 7), px, w, h);
  if (done) fe::looks::live().treated_textures.fetch_add(1, std::memory_order_relaxed);
  return done;
}
void end_frame(const Frame&) {
  auto& l = fe::looks::live();
  if (!g_filter.in_match) { g_skipped = g_drawn = g_tinted = g_wired = 0; return; }
  l.frames.fetch_add(1, std::memory_order_relaxed); l.rule_ns.fetch_add(g_ns, std::memory_order_relaxed);
  l.skipped.fetch_add(g_skipped, std::memory_order_relaxed); l.drawn.fetch_add(g_drawn, std::memory_order_relaxed);
  l.tinted.fetch_add(g_tinted, std::memory_order_relaxed); l.wired.fetch_add(g_wired, std::memory_order_relaxed);
  g_skipped = g_drawn = g_tinted = g_wired = 0;
}
}  // namespace

// The hooks the Metal renderer calls (installed by mu_play_main.cpp in play mode).
const gx::LookHooks* hooks() {
  static gx::LookHooks h{begin_frame, decide, clear_colour, treat, end_frame};
  return &h;
}
}  // namespace mu_looks
