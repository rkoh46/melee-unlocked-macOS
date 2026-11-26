// The launcher's Settings window: every melee-unlocked setting the Mac app honours, in melee-unlocked's own groups and order
// (its settings panel's categories VIDEO, AUDIO, GAME, CONTROLS, OVERLAYS, CUSTOMIZE, GECKO CODES, MODS and their sections),
// with native controls that show their label and value. melee-unlocked's launcher opens its settings panel as a window of
// its own from the rail's Settings; this is that window for the Mac, before the game starts.
//
// Each row is a key the engine reads: settings.ini ("key value", melee-unlocked/settings.ini, read by fe/settings.cpp and the
// feature modules), launcher.ini ("key=value", read by the engine at start: mu_play_main.cpp and the panel) or one of the
// launcher's own choices (LauncherSettings, passed to the engine as arguments). The in-game panel edits the same files, so
// a change here and one there are the same change. docs/SETTINGS_PARITY.md lists every melee-unlocked setting and where it
// is here; the ones that cannot apply on the Mac are not shown.
// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>
#include "mac_launcher.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {
std::string support() { const char* h = std::getenv("HOME"); return std::string(h ? h : ".") + "/Library/Application Support/Dashdance"; }
std::string ini_get(const std::string& key, bool* found) {
  std::ifstream in(support() + "/launcher.ini");
  std::string line, out; *found = false;
  while (std::getline(in, line)) {
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    if (line.compare(0, key.size() + 1, key + "=") == 0) { out = line.substr(key.size() + 1); *found = true; }
  }
  return out;
}
void ini_put(const std::string& key, const std::string& value) {
  std::vector<std::string> lines; bool found = false;
  { std::ifstream in(support() + "/launcher.ini"); std::string line; while (std::getline(in, line)) lines.push_back(line); }
  for (auto& l : lines) if (l.compare(0, key.size() + 1, key + "=") == 0) { l = key + "=" + value; found = true; }
  if (!found) lines.push_back(key + "=" + value);
  std::ofstream out(support() + "/launcher.ini", std::ios::trunc);
  for (const auto& l : lines) out << l << "\n";
}
bool engine_has(const std::string& key) {
  std::ifstream in(support() + "/melee-unlocked/settings.ini");
  std::string line;
  while (std::getline(in, line)) if (line.rfind(key + " ", 0) == 0) return true;
  return false;
}

enum Store { E, L, S, B };              // settings.ini, launcher.ini, the launcher's choices, a community_codes bit
enum Kind { TOGGLE, CHOICE, SLIDER, TEXT, PATH, NOTE, BUTTONS };   // BUTTONS: titles are buttons, values their actions
struct Row {
  const char* cat; const char* sec; const char* label; const char* key; Store store; Kind kind;
  std::vector<const char*> titles = {}, values = {};      // CHOICE
  double lo = 0, hi = 1, step = 1, mult = 1; const char* fmt = "%.0f";   // SLIDER: shown as fmt(value * mult)
  const char* def = "0";
  const char* hint = nullptr;
  unsigned bit = 0;                                       // B
};
const std::vector<const char*> kOnOff = {"Off", "On"};
const std::vector<Row>& rows() {
  static const std::vector<Row> r = {
    // ---- VIDEO ----
    {"Video", "Display", "Start full screen", "fullscreen", S, TOGGLE},
    {"Video", "Display", "VSync", "vsync", S, TOGGLE, {}, {}, 0, 1, 1, 1, "%.0f", "1", "Off presents frames as soon as they are ready."},
    {"Video", "Frame rate", "Frame rate cap", "fps", E, CHOICE, {"Match the display", "60", "120", "144", "165", "240"}, {"0", "60", "120", "144", "165", "240"}, 0, 1, 1, 1, "", "0"},
    {"Video", "Frame rate", "Sub-frame animation", "subframe", E, CHOICE, {"Off", "Predict ahead", "Interpolate"}, {"0", "1", "2"}, 0, 1, 1, 1, "", "0",
     "Draws in-between pictures on a display faster than 60 Hz. The game itself still runs at 60."},
    {"Video", "Widescreen", "Widescreen (16:9 code)", "widescreen", S, TOGGLE},
    {"Video", "Widescreen", "True widescreen", "truewidescreen", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "Widens the camera itself instead of stretching the 4:3 picture."},
    {"Video", "Widescreen", "Widescreen shape", "truewsaspect", E, CHOICE, {"16:9", "16:10", "Custom", "Fit the window"}, {"0", "1", "2", "3"}, 0, 1, 1, 1, "", "0",
     "With True widescreen: the shape the world is widened to, from 4:3 up to 64:27."},
    {"Video", "Widescreen", "Custom shape width", "truewscustomw", E, SLIDER, {}, {}, 1, 100, 1, 1, "%.0f", "21"},
    {"Video", "Widescreen", "Custom shape height", "truewscustomh", E, SLIDER, {}, {}, 1, 100, 1, 1, "%.0f", "9"},
    {"Video", "Window size", "Window size", "window", E, CHOICE, {"Follow window", "640×480", "960×720", "1280×960", "1440×1080", "1600×1200", "1920×1440"},
     {"follow", "640x480", "960x720", "1280x960", "1440x1080", "1600x1200", "1920x1440"}, 0, 1, 1, 1, "", "follow",
     "The size the game's window opens at. It stays resizable."},
    {"Video", "Resolution and filtering", "Picture shape", "aspect", E, CHOICE, {"Auto", "73:60 (console)", "4:3", "16:9", "Stretched"}, {"0", "1", "2", "3", "4"}},
    {"Video", "Resolution and filtering", "Internal resolution", "scale", S, CHOICE, {"Auto", "1× (640×480)", "2×", "3×", "4×"}, {"0", "1", "2", "3", "4"}},
    {"Video", "Resolution and filtering", "Anti-aliasing (MSAA)", "msaa", L, CHOICE, {"Off", "2×", "4×", "8×"}, {"1", "2", "4", "8"}, 0, 1, 1, 1, "", "1"},
    {"Video", "Resolution and filtering", "Anisotropic filtering", "anisotropy", L, CHOICE, {"Off", "4×", "16×"}, {"1", "4", "16"}, 0, 1, 1, 1, "", "16"},
    {"Video", "Resolution and filtering", "Screen filter", "filter", L, CHOICE, {"Smooth", "Area", "CRT", "Vibrant"}, {"0", "1", "2", "3"}},
    {"Video", "Picture", "Brightness", "brightness", E, SLIDER, {}, {}, 0.5, 1.5, 0.05, 100, "%.0f%%", "1"},
    {"Video", "Picture", "Contrast", "contrast", E, SLIDER, {}, {}, 0.5, 1.5, 0.05, 100, "%.0f%%", "1"},
    {"Video", "Picture", "Vibrance", "vibrance", E, SLIDER, {}, {}, 0, 2, 0.1, 100, "%.0f%%", "1"},
    {"Video", "Picture", "Sharpening", "sharpness", E, SLIDER, {}, {}, 0, 1, 0.1, 100, "%.0f%%", "0"},
    {"Video", "Picture", "Ambient occlusion", "ssao", E, SLIDER, {}, {}, 0, 1, 0.1, 100, "%.0f%%", "0"},
    {"Video", "Get mod looks", "Look", "look", E, CHOICE, {"Off", "Light", "Lighter", "Diet Classic (mod)", "Diet 64 (mod)", "Crystal (original colours)", "Crystal (player colours)", "Wireframe", "Potato",
     "64 Style (built-in)", "Silhouettes", "Classic Style (built-in)"}, {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"}, 0, 1, 1, 1, "", "",
     "A preset sets every part below."},
    {"Video", "Get mod looks", "Crystal fighter colours", "look_colours", E, CHOICE, {"Original mod colours", "Player colours (red, blue, yellow, green)"}, {"0", "1"}, 0, 1, 1, 1, "", "0",
     "With the Crystal look: the mod's own colours, or each fighter in its port's colour."},
    {"Video", "Get mod looks", "Look of the fighters", "look_fighter", E, CHOICE, {"Textured", "64 style", "Flat colour", "White", "Posterize", "Wire"}, {"0", "1", "2", "3", "4", "5"}},
    {"Video", "Get mod looks", "Look of the stage", "look_stage", E, CHOICE, {"Textured", "64 style", "Flat colour", "White", "Posterize", "Wire"}, {"0", "1", "2", "3", "4", "5"}},
    {"Video", "Get mod looks", "Look of the menus", "look_menus", E, CHOICE, {"Textured", "64 style", "Flat colour", "White", "Posterize"}, {"0", "1", "2", "3", "4"}},
    {"Video", "Get mod looks", "Port colours on fighters", "look_colours", E, TOGGLE},
    {"Video", "Get mod looks", "Background", "look_background", E, CHOICE, {"The stage's own", "Black", "Plain colour"}, {"0", "1", "2"}},
    {"Video", "Get mod looks", "Sharp texels", "look_unfiltered", E, TOGGLE},
    {"Video", "Get mod looks", "Plain background colour", "look_background_rgb", E, CHOICE, {"Black", "Night blue", "Grey", "Green screen", "Blue screen", "White"},
     {"000000", "1E2430", "404040", "00B140", "0047BB", "FFFFFF"}, 0, 1, 1, 1, "", "000000"},
    {"Video", "Low-detail modes", "Lightweight", "lightweight", E, CHOICE, {"Off", "1", "2", "3", "4", "5"}, {"0", "1", "2", "3", "4", "5"}, 0, 1, 1, 1, "", "0",
     "Fewer details for slower Macs: each step drops more."},
    {"Video", "Low-detail modes", "Visual effects", "effects", E, CHOICE, {"Full", "Reduced", "Minimal", "No decorative particles"}, {"0", "1", "2", "3"}},
    {"Video", "Low-detail modes", "Low-poly fighters", "lowpoly", E, TOGGLE},
    {"Video", "Arcade look", "Every Arcade row", "arcade", E, BUTTONS, {"Arcade preset", "Reset"}, {"arcade:on", "arcade:off"}},
    {"Video", "Arcade look", "Show hitboxes", "arcade_hitboxes", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "Offline only."},
    {"Video", "Arcade look", "Projectile hitboxes", "arcade_projectile_hitboxes", E, TOGGLE},
    {"Video", "Arcade look", "Hitboxes behind fighters", "arcade_hitboxes_behind", E, TOGGLE},
    {"Video", "Arcade look", "2D camera", "arcade_camera_2d", E, TOGGLE},
    {"Video", "Arcade look", "Behind the stage", "arcade_backdrop", E, CHOICE, {"The stage", "Plain colour", "Grid"}, {"0", "1", "2"}},
    {"Video", "Arcade look", "Combo counter", "arcade_combo_counter", E, TOGGLE},
    {"Video", "Arcade look", "Background colour", "arcade_background_rgb", E, CHOICE, {"Night blue", "Black", "Grey", "Green screen", "Blue screen", "White"},
     {"1E2430", "000000", "404040", "00B140", "0047BB", "FFFFFF"}, 0, 1, 1, 1, "", "1E2430"},
    {"Video", "Arcade look", "Grid colour", "arcade_grid_rgb", E, CHOICE, {"White", "Grey", "Cyan", "Green", "Pink"}, {"FFFFFF", "808080", "40E0FF", "40FF80", "FF60C0"}, 0, 1, 1, 1, "", "FFFFFF"},
    {"Video", "HDR", "HDR output", "hdroutput", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "Extended range on displays that have it."},
    {"Video", "HDR", "Paper white", "hdrpaperwhite", E, SLIDER, {}, {}, 80, 500, 20, 1, "%.0f nits", "203"},
    {"Video", "HDR", "Peak brightness", "hdrpeak", E, SLIDER, {}, {}, 400, 10000, 100, 1, "%.0f nits", "1000"},
    {"Video", "Border art", "Border art", "borderart", E, CHOICE, {"None", "Grid"}, {"-", "grid"}, 0, 1, 1, 1, "", "-"},
    {"Video", "Border art", "Shadow at the picture's edge", "borderedge", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Video", "Built-in CRT", "CRT look", "crt_filter", E, CHOICE, {"Off", "Studio monitor", "Home television"}, {"0", "1", "2"}},
    {"Video", "Built-in CRT", "Screen curve", "crt_curve", E, SLIDER, {}, {}, 0, 3, 0.25, 100, "%.0f%%", "1"},
    {"Video", "HUD and texture packs", "Texture packs", "customtextures", E, TOGGLE},
    {"Video", "HUD and texture packs", "Load every texture before the game", "prefetchtextures", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Video", "HUD and texture packs", "Dump the game's textures", "dumptextures", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "For pack authors."},
    {"Video", "Animated menu backgrounds", "Menu video backgrounds", "videobackgrounds", E, TOGGLE},
    {"Video", "Advanced", "Lab view", "labview", E, TOGGLE},
    {"Video", "Advanced", "Lab fighters", "look_lab_fighters", E, CHOICE, {"Silhouettes", "Stick figures", "Capsules", "Blobs"}, {"0", "1", "2", "3"}},
    {"Video", "Advanced", "Lab stage", "look_lab_stage", E, CHOICE, {"As drawn", "Outline", "Blocks", "Hidden"}, {"0", "1", "2", "3"}},
    {"Video", "Advanced", "One-frame glitch detector", "flickerscan", E, TOGGLE},
    {"Video", "Advanced", "Lossless adapter lag test (next start only)", "lagtestnext", E, TOGGLE},
    // ---- AUDIO ----
    {"Audio", "Volume", "Volume", "volume", S, SLIDER, {}, {}, 0, 100, 5, 1, "%.0f%%", "70"},
    {"Audio", "Volume", "Music volume", "music", E, SLIDER, {}, {}, 0, 100, 10, 1, "%.0f%%", "100"},
    {"Audio", "Sound", "Mute voices", "mute_voices", E, TOGGLE},
    {"Audio", "Sound", "Faster sound effects", "audio_early_keyon", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "Offline only."},
    {"Audio", "Sound", "Console low-pass on voices", "dsp_lowpass", E, TOGGLE},
    {"Audio", "Sound", "Custom music", "musicpacks", E, TOGGLE},
    {"Audio", "Sound", "Menu sounds", "menusounds", L, TOGGLE},
    {"Audio", "Output", "Audio buffer", "audio_mode", E, CHOICE, {"Automatic", "Low latency"}, {"0", "1"}},
    {"Audio", "Output", "Buffer size (low latency)", "audio_buffer_ms", E, SLIDER, {}, {}, 5, 120, 5, 1, "%.0f ms", "30"},
    // ---- GAME ----
    {"Game", "In-game overlay", "Open the settings when the game starts", "autoopenoverlay", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "F1 or Esc opens them at any time."},
    {"Game", "Input", "Background input", "background_input", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1", "Controllers keep playing while another window is in front."},
    {"Game", "Input", "Zelda starts as Sheik", "zelda_as_sheik", E, TOGGLE},
    {"Game", "Input", "Squad Strike in VS", "squad_strike", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "Z on the character select adds fighters. Offline."},
    {"Game", "Language", "Game language", "gamelanguage", E, CHOICE, {"The save's", "Japanese", "English"}, {"0", "1", "2"}},
    {"Game", "Language", "Everything unlocked", "unlock_all", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Language", "Results screen", "results_screen", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Input delay", "Online input delay", "online_delay", S, CHOICE, {"Automatic", "1 frame", "2 frames", "3 frames", "4 frames", "5 frames", "6 frames", "7 frames", "8 frames", "9 frames"},
     {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"}, 0, 1, 1, 1, "", "2", "Each frame adds 16.7 ms. 2 is Slippi's default."},
    {"Game", "Input delay", "Offline input delay", "offline_delay", E, TOGGLE},
    {"Game", "Input delay", "Offline delay", "offline_delay_frames", E, SLIDER, {}, {}, 1, 9, 1, 1, "%.0f frames", "4"},
    {"Game", "Online", "Slippi Online", "online", S, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Online", "Input suggestions", "input_suggestions", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Online", "Quick chat", "quickchat", E, CHOICE, {"On", "Direct only", "Off"}, {"0", "1", "2"}},
    {"Game", "Online", "Play while searching", "play_while_searching", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1", "Hold L+R on the select to play Training until a match is found."},
    {"Game", "Online", "Leaving the select while searching", "keep_search", E, CHOICE, {"Ask me", "Keep searching", "Stop the search"}, {"0", "1", "2"}},
    {"Game", "Online", "Warm-up room", "warmup", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Online", "Live pick previews", "pick_previews", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Online", "After a Direct or Teams session: Training", "online_training_after", E, TOGGLE},
    {"Game", "Online", "After an online session: Training's select", "online_return_training", E, TOGGLE},
    {"Game", "Online", "Pause in Direct", "directpause", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Online", "Port colour (first choice)", "portcolour1", E, CHOICE, {"None", "Red", "Blue", "Yellow", "Green"}, {"-1", "0", "1", "2", "3"}, 0, 1, 1, 1, "", "-1"},
    {"Game", "Online", "Port colour (second choice)", "portcolour2", E, CHOICE, {"None", "Red", "Blue", "Yellow", "Green"}, {"-1", "0", "1", "2", "3"}, 0, 1, 1, 1, "", "-1"},
    {"Game", "Online", "Opponent's shield colour", "shield_color_online", E, CHOICE, {"Their own", "Red", "Blue", "Yellow", "Green", "White", "Purple"},
     {"0", "FF3030", "3070FF", "FFD030", "30D060", "FFFFFF", "A050FF"}},
    {"Game", "Online", "Skip in Unranked (1)", "blockcode1", E, TEXT, {}, {}, 0, 1, 1, 1, "", "", "A connect code, such as CODE#123."},
    {"Game", "Online", "Skip in Unranked (2)", "blockcode2", E, TEXT, {}, {}, 0, 1, 1, 1, "", ""},
    {"Game", "Online", "Skip in Unranked (3)", "blockcode3", E, TEXT, {}, {}, 0, 1, 1, 1, "", ""},
    {"Game", "Online", "Network overlay", "netoverlay", E, TOGGLE},
    {"Game", "Online", "Session trace beside replays", "session_trace", E, CHOICE, {"Off", "Marked matches", "Every match"}, {"0", "1", "2"}, 0, 1, 1, 1, "", "1"},
    {"Game", "Discord", "Discord presence", "discord", L, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1", "Shows what you are playing in Discord, through the Discord app on this Mac."},
    {"Game", "Discord", "Show your rank", "discord_rank", L, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Discord", "Application ID", "discord_app_id", L, TEXT, {}, {}, 0, 1, 1, 1, "", "", "Empty uses this app's own Discord application and its artwork. Digits only."},
    {"Game", "Replays", "Save every match", "savereplays", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Replays", "Compress replays (.slpz)", "compressreplays", E, TOGGLE},
    {"Game", "Replays", "Replay folder", "replaydir", L, PATH, {}, {}, 0, 1, 1, 1, "", ""},
    {"Game", "Replays", "Takeover: a try lasts", "replay_try_length", E, CHOICE, {"Until the combo drops", "10 seconds", "20 seconds", "No limit"}, {"0", "1", "2", "3"}, 0, 1, 1, 1, "", "3"},
    {"Game", "Replays", "Takeover: the other players", "replay_opponent", E, CHOICE, {"Smart defence", "No DI or techs", "Practice CPU", "Follow the recording"}, {"0", "1", "2", "3"}},
    {"Game", "Replays", "Coach tips", "replay_tips", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Replays", "Sort", "replay_sort", E, CHOICE, {"Most recent", "Least recent", "Longest game", "Shortest game"}, {"0", "1", "2", "3"}},
    {"Game", "Replays", "Hide short games", "replay_hide_short", E, TOGGLE},
    {"Game", "Replays", "Show it as it was played", "replayasplayed", E, TOGGLE},
    {"Game", "Replays", "Mute the game while watching", "replaymute", E, TOGGLE},
    {"Game", "Replays", "Spectator server", "slippi_spectator", E, TOGGLE},
    {"Game", "Replays", "Spectator server port", "slippi_spectator_port", E, TEXT, {}, {}, 0, 1, 1, 1, "", "51441"},
    {"Game", "Stages", "Frozen Pokémon Stadium", "stadium_frozen", E, TOGGLE},
    {"Game", "Stages", "Frozen Final Destination", "freeze_fd", E, TOGGLE},
    {"Game", "Stages", "20XX computer players", "cpu_20xx", E, TOGGLE},
    {"Game", "Stages", "Computer players: tech", "cpu_tech", E, CHOICE, {"Normal", "In place", "Roll forward", "Roll back", "Miss", "Random"}, {"0", "1", "2", "3", "4", "5"}},
    {"Game", "Stages", "Computer players: get-up", "cpu_getup", E, CHOICE, {"Normal", "Stand", "Roll forward", "Roll back", "Attack", "Random"}, {"0", "1", "2", "3", "4", "5"}},
    {"Game", "Stages", "Computer players: DI", "cpu_di", E, CHOICE, {"Normal", "None", "Random", "Survival"}, {"0", "1", "2", "3"}},
    {"Game", "Stages", "Computer players: smash DI", "cpu_sdi", E, CHOICE, {"Normal", "None", "Random", "Against the hit", "With the hit", "Up", "Down"}, {"0", "1", "2", "3", "4", "5", "6"}},
    {"Game", "Stages", "Computer players: no taunts", "cpu_notaunt", E, TOGGLE},
    {"Game", "Stages", "Computer players: always L-cancel", "cpu_lcancel", E, TOGGLE},
    {"Game", "Stages", "Computer players: no rapid jabs", "cpu_nojab", E, TOGGLE},
    {"Game", "Stages", "Computer players: no transforming", "cpu_notransform", E, TOGGLE},
    {"Game", "Stage effects", "Fountain of Dreams reflections", "fodreflections", E, TOGGLE},
    {"Game", "Stage effects", "Character select background keeps playing", "cssbackgroundfree", E, TOGGLE},
    {"Game", "L-cancel", "Automatic L-cancel", "autolcancel", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "Offline, and in Direct when every player offers it."},
    {"Game", "HUD", "PAL stock icons", "palstockicons", E, TOGGLE},
    {"Game", "HUD", "No screen shake", "noscreenshake", E, TOGGLE},
    {"Game", "HUD", "Screen shake", "screenshake", E, SLIDER, {}, {}, 0, 200, 10, 1, "%.0f%%", "100"},
    {"Game", "HUD", "Stock icons size", "stockhudscale", E, SLIDER, {}, {}, 75, 175, 25, 1, "%.0f%%", "100"},
    {"Game", "HUD", "Damage numbers size", "damagehudscale", E, SLIDER, {}, {}, 75, 175, 25, 1, "%.0f%%", "100"},
    {"Game", "HUD", "Player tags stay up", "playertags", E, TOGGLE},
    {"Game", "HUD", "Hide KO explosions", "hide_ko_explosions", E, TOGGLE},
    {"Game", "HUD", "Hide HUD emblems", "hide_hud_emblems", E, TOGGLE},
    {"Game", "HUD", "Training HUD", "training_hud", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Game", "Shield colours", "P1 shield", "shield_colorp1", E, CHOICE, {"Port colour", "Red", "Blue", "Yellow", "Green", "White", "Purple"}, {"0", "FF3030", "3070FF", "FFD030", "30D060", "FFFFFF", "A050FF"}},
    {"Game", "Shield colours", "P2 shield", "shield_colorp2", E, CHOICE, {"Port colour", "Red", "Blue", "Yellow", "Green", "White", "Purple"}, {"0", "FF3030", "3070FF", "FFD030", "30D060", "FFFFFF", "A050FF"}},
    {"Game", "Shield colours", "P3 shield", "shield_colorp3", E, CHOICE, {"Port colour", "Red", "Blue", "Yellow", "Green", "White", "Purple"}, {"0", "FF3030", "3070FF", "FFD030", "30D060", "FFFFFF", "A050FF"}},
    {"Game", "Shield colours", "P4 shield", "shield_colorp4", E, CHOICE, {"Port colour", "Red", "Blue", "Yellow", "Green", "White", "Purple"}, {"0", "FF3030", "3070FF", "FFD030", "30D060", "FFFFFF", "A050FF"}},
    {"Game", "Community codes", "HUD fades behind fighters", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x1000},
    {"Game", "Community codes", "Respawn platform colours", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x2000},
    {"Game", "Community codes", "Nana's percent", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x4000},
    {"Game", "Community codes", "No GO! at the start", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x10000},
    {"Game", "Community codes", "Hitbox colours by id", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x20000},
    {"Game", "Community codes", "Hitbox order", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x40000},
    {"Game", "Community codes", "Hitbox angles", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x80000},
    {"Game", "Community codes", "Full Fountain of Dreams reflections", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x100000},
    {"Game", "Community codes", "V-cancel sound", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x200000},
    {"Game", "Community codes", "Instant Melee (offline)", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x400000},
    {"Game", "Community codes", "Free pause camera (offline)", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x800000},
    {"Game", "Community codes", "No fighter shadows", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x1000000},
    {"Game", "Community codes", "Boot to character select (offline)", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x2000000},
    {"Game", "Community codes", "Centred 2-player HUD", "community_codes", B, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", nullptr, 0x4000000},
    {"Game", "Training", "Save states and recording on the D-pad", "lab", E, TOGGLE},
    {"Game", "Training", "Loop the recording", "lab_loop", E, TOGGLE},
    {"Game", "Training", "Collision bubbles", "lab_bubbles", E, CHOICE, {"Off", "Over the model", "Bubbles only"}, {"0", "1", "2"}},
    {"Game", "Training", "Inputs and frame advantage", "lab_overlay", E, TOGGLE},
    {"Game", "Training", "Frame advance (L+R+Z)", "frame_advance", E, TOGGLE},
    {"Game", "Training", "Random numbers", "practice_rng", E, CHOICE, {"The game's own", "Same each start", "Frozen"}, {"0", "1", "2"}},
    {"Game", "Training", "Practice menu look", "practicestyle", E, CHOICE, {"1", "2", "3", "4"}, {"0", "1", "2", "3"}},
    {"Game", "Training", "Practice menu hint in Training", "matchmakinghint", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    // ---- CONTROLS ----
    {"Controls", "Rumble", "Controller rumble", "rumble", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1", "Every controller. Each controller also has its own switch in game."},
    {"Controls", "Players", "Buttons, sticks, triggers and profiles", "", E, NOTE, {}, {}, 0, 1, 1, 1, "", "",
     "Set per controller in game (F1 > Controls), where the controller you are holding is live: remapping, deadzones, radius, "
     "snapback, trigger points, the keyboard layout, PhobGCC and saved profiles."},
    // ---- OVERLAYS ----
    {"Overlays", "Overlays", "Controller input display", "inputoverlay", E, TOGGLE},
    {"Overlays", "Overlays", "Ports shown", "inputoverlayports", E, CHOICE, {"P1", "P2", "P1 and P2", "All four"}, {"1", "2", "3", "15"}, 0, 1, 1, 1, "", "1"},
    {"Overlays", "Overlays", "Show values", "inputoverlayvalues", E, TOGGLE},
    {"Overlays", "Overlays", "Input display size", "inputoverlaystick", E, SLIDER, {}, {}, 1, 10, 1, 1, "%.0f", "5"},
    {"Overlays", "Overlays", "No border", "inputoverlayhideborder", E, TOGGLE},
    {"Overlays", "Overlays", "Frame counter", "showfps", E, TOGGLE},
    {"Overlays", "Overlays", "Performance graph", "performance", E, TOGGLE},
    {"Overlays", "Overlays", "Performance HUD", "hud", L, TOGGLE},
    {"Overlays", "Overlays", "Player names above fighters (online)", "playernicknames", E, TOGGLE},
    {"Overlays", "Overlays", "Clean replays", "clean_replays", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "0", "Replays play without the Arcade look's hitboxes."},
    {"Overlays", "L-cancel", "L-cancel flash", "lcancel_flash_mode", E, CHOICE, {"Off", "Missed", "Made", "Both"}, {"0", "1", "5", "6"}, 0, 1, 1, 1, "", "0",
     "20XX TE's own flash modes are offered in game when its save is in use."},
    {"Overlays", "L-cancel", "Made L-cancel colour", "lcancel_success_flash", E, CHOICE, {"None", "White", "Green"}, {"0", "1", "2"}, 0, 1, 1, 1, "", "2"},
    {"Overlays", "Advanced", "Display latency test (X + D-pad Down)", "latencytest", E, TOGGLE},
    {"Overlays", "Advanced", "“Settings: F1” reminder in menus", "settingsreminder", E, TOGGLE},
    {"Overlays", "Advanced", "GameCube adapter report rate", "adapterrate", E, TOGGLE},
    {"Overlays", "Notices", "Note when skins are off online", "skin_online_notice", L, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    // ---- CUSTOMIZE ----
    {"Customize", "Appearance", "Menu style", "overlaystyle", L, CHOICE, {"Clean side", "Icon tiles", "GD Melee", "Radial", "Wide tabs", "Simple"}, {"0", "1", "2", "3", "4", "5"}},
    {"Customize", "Appearance", "Window transparency", "settingstransparency", L, SLIDER, {}, {}, 0, 65, 5, 1, "%.0f%%", "0"},
    {"Customize", "Appearance", "Use custom accent", "menucustom", L, TOGGLE},
    {"Customize", "Legacy menu", "Legacy menu (F11 switches)", "legacymenu", L, TOGGLE},
    {"Customize", "Legacy menu", "Legacy menu style", "legacymenustyle", L, CHOICE, {"Legacy Old (v0.6.6)", "Legacy New"}, {"7", "6"}, 0, 1, 1, 1, "", "7"},
    {"Customize", "Launcher", "Launcher colour", "mu_accent", L, CHOICE, {"Purple", "Blue", "Green", "Amber", "Red", "Pink"}, {"0", "1", "2", "3", "4", "5"}, 0, 1, 1, 1, "", "0",
     "The colour wheel on the Play page picks any colour."},
    // ---- GECKO CODES ----
    {"Gecko codes", "Built-in codes", "Retail game (no built-in codes)", "vanilla", E, TOGGLE},
    {"Gecko codes", "Built-in codes", "Slippi menus (Online Play)", "slippi_menus", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Gecko codes", "Your codes", "GeckoCodes.ini", "GeckoCodes.ini", E, PATH, {}, {}, 0, 1, 1, 1, "", "",
     "Your own codes live in melee-unlocked/GeckoCodes.ini (Dolphin's format); switch each on in game under Gecko codes. Offline only."},
    // ---- MODS ----
    {"Mods", "Game mods", "20XX TE features with its save", "mod_te_enabled", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1"},
    {"Mods", "Cosmetic mods", "Skins from the Cosmetics folder", "cosmetics", E, TOGGLE, {}, {}, 0, 1, 1, 1, "", "1", "Pick each fighter's skin in game under Mods > Choose skins."},
    {"Mods", "Cosmetic mods", "Add skins", "cosmetics_import", E, BUTTONS, {"Import…", "Add stage DAT…", "Open folder"}, {"cos:import", "cos:stage", "cos:open"}, 0, 1, 1, 1, "", "",
     "Import takes costume files (PlFxGr.dat), stage files (GrNBa.dat) or a .zip of them into the Cosmetics folder. Add stage DAT replaces the stage you pick."},
  };
  return r;
}
// melee-unlocked's Look presets as the engine has them (mu-engine/app/fe/looks_live.h presets(); keep the two in step):
// Lightweight, fighters, stage, menus, sharp texels, port colours, background, visual effects.
struct LookPreset { int lw, fighter, stage, menus, unfiltered, colours, background, effects; };
const LookPreset kLooks[] = {{0, 0, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 0}, {2, 0, 0, 0, 0, 0, 0, 0}, {2, 0, 0, 0, 0, 0, 1, 1},
                             {1, 1, 1, 1, 1, 0, 0, 1}, {2, 2, 2, 4, 0, 0, 1, 2}, {2, 2, 2, 4, 0, 1, 1, 2}, {5, 5, 5, 0, 0, 0, 1, 2},
                             {4, 3, 3, 0, 0, 1, 1, 2}, {1, 1, 1, 1, 1, 0, 0, 1}, {2, 3, 3, 0, 0, 1, 2, 2}, {2, 0, 0, 0, 0, 0, 1, 1}};
const char* const kLookKeys[] = {"lightweight", "look_fighter", "look_stage", "look_menus", "look_unfiltered", "look_colours", "look_background", "effects"};
int look_part(const LookPreset& p, int i) { const int v[] = {p.lw, p.fighter, p.stage, p.menus, p.unfiltered, p.colours, p.background, p.effects}; return v[i]; }
int engine_int(const char* key) { return engine_has(key) ? std::atoi(host::engine_setting(key).c_str()) : 0; }
int current_look() {
  // The engine's look_chosen first (fe/looks_live.h): Diet 64 and 64 Style, Diet Classic and Classic Style share their parts.
  if (const int c = engine_int("look_chosen"); c >= 0 && c < int(sizeof kLooks / sizeof kLooks[0])) {
    bool same = true;
    for (int k = 0; k < 8 && same; ++k) same = engine_int(kLookKeys[k]) == look_part(kLooks[c], k);
    if (same) return c;
  }
  for (int i = 0; i < int(sizeof kLooks / sizeof kLooks[0]); ++i) {
    bool same = true;
    for (int k = 0; k < 8 && same; ++k) same = engine_int(kLookKeys[k]) == look_part(kLooks[i], k);
    if (same) return i;
  }
  return -1;
}
// Cosmetics: the folder the engine serves skins from (fe/features/cosmetics.cpp: <app folder>/Cosmetics/Costumes and Stages).
std::string cosmetics_dir() { return support() + "/melee-unlocked/Cosmetics"; }
// A costume file is named for its slot (PlFxGr.dat), a stage file for its stage (GrNBa.dat), with an optional label after it.
std::string skin_kind(NSString* name) {
  NSString* n = name.lowercaseString;
  if (![n hasSuffix:@".dat"]) return "";
  if ([n hasPrefix:@"pl"]) return "Costumes";
  if ([n hasPrefix:@"gr"]) return "Stages";
  return "";
}

const std::vector<const char*> kCats = {"Video", "Audio", "Game", "Controls", "Overlays", "Customize", "Gecko codes", "Mods"};
const std::vector<const char*> kIcons = {"display", "speaker.wave.2", "gamecontroller", "dpad", "square.on.square", "paintbrush", "chevron.left.forwardslash.chevron.right", "shippingbox"};
}  // namespace

@interface MUSettingsController : NSObject <NSTableViewDataSource, NSTableViewDelegate, NSWindowDelegate>
@property(nonatomic) host::LauncherSettings* settings;
@property(nonatomic) NSWindow* window;
@property(nonatomic) NSTableView* sidebar;
@property(nonatomic) NSScrollView* scroll;
@property(nonatomic) NSInteger category;
@property(nonatomic) NSMutableArray<NSTextField*>* valueLabels;
- (void)pressed:(NSButton*)sender;
- (void)importSkins:(NSArray<NSURL*>*)urls stage:(const std::string&)target_stage;
- (void)build;
@end

@interface MUFlipped : NSView
@end
@implementation MUFlipped
- (BOOL)isFlipped { return YES; }
@end

@implementation MUSettingsController
// ---- reading and writing one row's value ----
- (std::string)value:(const Row&)r {
  host::LauncherSettings& s = *self.settings;
  bool found = false;
  switch (r.store) {
    case S: {
      const std::string k = r.key;
      if (k == "fullscreen") return s.fullscreen ? "1" : "0";
      if (k == "vsync") return s.vsync ? "1" : "0";
      if (k == "widescreen") return s.widescreen ? "1" : "0";
      if (k == "scale") return std::to_string(std::clamp(s.scale, 0, 4));
      if (k == "volume") return std::to_string(s.volume);
      if (k == "online") return s.online ? "1" : "0";
      if (k == "online_delay") return std::to_string(std::clamp(s.online_delay, 0, 9));
      return r.def;
    }
    case L: { std::string v = ini_get(r.key, &found); return found && !v.empty() ? v : r.def; }
    case B: {
      const unsigned long bits = std::strtoul(host::engine_setting("community_codes").c_str(), nullptr, 16);
      return (bits & r.bit) ? "1" : "0";
    }
    case E:
      if (std::string(r.key) == "look") { const int i = current_look(); return i < 0 ? std::string("custom") : std::to_string(i); }
      return engine_has(r.key) ? host::engine_setting(r.key) : std::string(r.def);
  }
  return r.def;
}
- (void)set:(const Row&)r to:(const std::string&)v {
  host::LauncherSettings& s = *self.settings;
  switch (r.store) {
    case S: {
      const std::string k = r.key; const int n = std::atoi(v.c_str());
      if (k == "fullscreen") s.fullscreen = n != 0;
      else if (k == "vsync") s.vsync = n != 0;
      else if (k == "widescreen") { s.widescreen = n != 0; host::set_engine_setting("widescreen", v); }
      else if (k == "scale") s.scale = std::clamp(n, 0, 4);
      else if (k == "volume") s.volume = std::clamp(n, 0, 100);
      else if (k == "online") s.online = n != 0;
      else if (k == "online_delay") s.online_delay = std::clamp(n, 0, 9);
      break;   // saved to launcher.ini with the rest of the launcher's choices
    }
    case L: ini_put(r.key, v); break;
    case B: {
      unsigned long bits = std::strtoul(host::engine_setting("community_codes").c_str(), nullptr, 16);
      bits = v == "1" ? (bits | r.bit) : (bits & ~(unsigned long)r.bit);
      char hex[16]; std::snprintf(hex, sizeof hex, "%lX", bits);
      host::set_engine_setting("community_codes", hex);
      break;
    }
    case E:
      if (std::string(r.key) == "look") {   // the preset's parts, as the engine's Look row saves them
        const int i = std::atoi(v.c_str());
        if (i >= 0 && i < int(sizeof kLooks / sizeof kLooks[0])) {
          for (int k = 0; k < 8; ++k) host::set_engine_setting(kLookKeys[k], std::to_string(look_part(kLooks[i], k)));
          host::set_engine_setting("look_chosen", std::to_string(i));   // which of the presets that share parts (Diet 64 / 64 Style ...)
        }
        [self build];
        break;
      }
      host::set_engine_setting(r.key, v);
      if (std::string(r.key) == "look_colours") [self build];   // the Look row follows
      break;
  }
  host::log("settings: %s = %s", r.store == B ? (std::string("community_codes bit ") + std::to_string(r.bit)).c_str() : r.key, v.c_str());
}
// ---- the window ----
- (void)open:(NSInteger)category {
  self.category = std::clamp<NSInteger>(category, 0, (NSInteger)kCats.size() - 1);
  NSWindow* w = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 820, 600)
                                            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskFullSizeContentView
                                              backing:NSBackingStoreBuffered defer:NO];
  w.title = @"Settings";
  w.minSize = NSMakeSize(680, 420);
  w.releasedWhenClosed = NO;
  w.delegate = self;
  NSSplitView* split = [[NSSplitView alloc] initWithFrame:w.contentView.bounds];
  split.vertical = YES; split.dividerStyle = NSSplitViewDividerStyleThin; split.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  // The sidebar: melee-unlocked's categories, in its order.
  NSVisualEffectView* side = [[NSVisualEffectView alloc] initWithFrame:NSMakeRect(0, 0, 220, 600)];
  side.material = NSVisualEffectMaterialSidebar; side.blendingMode = NSVisualEffectBlendingModeBehindWindow;
  NSScrollView* sideScroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 220, 552)];
  sideScroll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable; sideScroll.drawsBackground = NO;
  NSTableView* table = [[NSTableView alloc] initWithFrame:sideScroll.bounds];
  table.style = NSTableViewStyleSourceList; table.headerView = nil; table.rowHeight = 32; table.backgroundColor = NSColor.clearColor;
  [table addTableColumn:[[NSTableColumn alloc] initWithIdentifier:@"c"]];
  table.dataSource = self; table.delegate = self;
  sideScroll.documentView = table;
  [side addSubview:sideScroll];
  self.sidebar = table;
  NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 600, 600)];
  scroll.hasVerticalScroller = YES; scroll.autohidesScrollers = YES; scroll.drawsBackground = YES;
  scroll.backgroundColor = NSColor.windowBackgroundColor; scroll.automaticallyAdjustsContentInsets = YES;
  self.scroll = scroll;
  [split addSubview:side]; [split addSubview:scroll];
  [split setPosition:220 ofDividerAtIndex:0];
  [split setHoldingPriority:NSLayoutPriorityDefaultHigh forSubviewAtIndex:0];
  w.contentView = split;
  self.window = w;
  [table reloadData];
  [table selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)self.category] byExtendingSelection:NO];
  [self build];
}
- (NSInteger)numberOfRowsInTableView:(NSTableView*)t { return (NSInteger)kCats.size(); }
- (NSView*)tableView:(NSTableView*)t viewForTableColumn:(NSTableColumn*)c row:(NSInteger)row {
  NSTableCellView* cell = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 200, 32)];
  NSImageView* icon = [NSImageView imageViewWithImage:[NSImage imageWithSystemSymbolName:@(kIcons[size_t(row)]) accessibilityDescription:nil] ?: [[NSImage alloc] init]];
  icon.frame = NSMakeRect(8, 8, 18, 16); icon.contentTintColor = NSColor.secondaryLabelColor;
  icon.symbolConfiguration = [NSImageSymbolConfiguration configurationWithPointSize:13 weight:NSFontWeightRegular];
  NSTextField* label = [NSTextField labelWithString:@(kCats[size_t(row)])];
  label.frame = NSMakeRect(34, 7, 160, 18); label.font = [NSFont systemFontOfSize:13];
  [cell addSubview:icon]; [cell addSubview:label];
  cell.textField = label; cell.imageView = icon;
  return cell;
}
- (void)tableViewSelectionDidChange:(NSNotification*)n {
  if (self.sidebar.selectedRow < 0 || self.sidebar.selectedRow == self.category) return;
  self.category = self.sidebar.selectedRow;
  [self build];
}
// The page: the category's sections, each a heading and its rows (label left, control right, a hint under it).
- (void)build {
  const CGFloat W = std::max<CGFloat>(440, self.scroll.contentSize.width), pad = 24, ctrlW = 220;
  MUFlipped* doc = [[MUFlipped alloc] initWithFrame:NSMakeRect(0, 0, W, 100)];
  doc.autoresizingMask = NSViewWidthSizable;
  self.valueLabels = [NSMutableArray array];
  CGFloat y = 52;   // under the title bar (full-size content view)
  NSTextField* title = [NSTextField labelWithString:@(kCats[size_t(self.category)])];
  title.font = [NSFont systemFontOfSize:22 weight:NSFontWeightSemibold]; title.frame = NSMakeRect(pad, y, W - 2 * pad, 28);
  [doc addSubview:title];
  y += 40;
  std::string section;
  NSInteger index = 0;
  for (const Row& r : rows()) {
    ++index;
    if (std::string(r.cat) != kCats[size_t(self.category)]) continue;
    if (section != r.sec) {
      section = r.sec;
      y += 12;
      NSTextField* h = [NSTextField labelWithString:@(r.sec)];
      h.font = [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold]; h.textColor = NSColor.secondaryLabelColor;
      h.frame = NSMakeRect(pad, y, W - 2 * pad, 18);
      [doc addSubview:h];
      y += 26;
      NSBox* line = [[NSBox alloc] initWithFrame:NSMakeRect(pad, y - 4, W - 2 * pad, 1)];
      line.boxType = NSBoxSeparator; line.autoresizingMask = NSViewWidthSizable;
      [doc addSubview:line];
    }
    const CGFloat rowH = 32;
    NSTextField* label = [NSTextField labelWithString:@(r.label)];
    label.font = [NSFont systemFontOfSize:13]; label.lineBreakMode = NSLineBreakByTruncatingTail;
    label.frame = NSMakeRect(pad, y + 7, W - 2 * pad - ctrlW - 12, 18);
    [doc addSubview:label];
    const NSRect cr = NSMakeRect(W - pad - ctrlW, y + 4, ctrlW, 24);
    NSControl* control = nil;
    const std::string v = r.kind == NOTE ? "" : [self value:r];
    switch (r.kind) {
      case TOGGLE: {
        NSSwitch* sw = [[NSSwitch alloc] initWithFrame:NSZeroRect];
        [sw sizeToFit];
        sw.frame = NSMakeRect(NSMaxX(cr) - sw.frame.size.width, y + 4, sw.frame.size.width, sw.frame.size.height);
        sw.state = v == "1" || (v != "0" && !v.empty() && r.store != S) ? NSControlStateValueOn : NSControlStateValueOff;
        sw.action = @selector(changed:);
        control = sw;
        break;
      }
      case CHOICE: {
        NSPopUpButton* pop = [[NSPopUpButton alloc] initWithFrame:cr pullsDown:NO];
        NSInteger pick = -1;
        for (size_t i = 0; i < r.titles.size(); ++i) {
          [pop addItemWithTitle:@(r.titles[i])];
          if (strcasecmp(r.values[i], v.c_str()) == 0) pick = NSInteger(i);
        }
        if (pick < 0 && !v.empty()) { [pop addItemWithTitle:v == "custom" ? @"Custom" : [NSString stringWithFormat:@"Other (%s)", v.c_str()]]; pick = pop.numberOfItems - 1; }
        [pop selectItemAtIndex:std::max<NSInteger>(pick, 0)];
        pop.action = @selector(changed:);
        control = pop;
        break;
      }
      case SLIDER: {
        NSSlider* sl = [NSSlider sliderWithValue:std::atof(v.c_str()) minValue:r.lo maxValue:r.hi target:self action:@selector(changed:)];
        sl.frame = NSMakeRect(cr.origin.x, y + 4, ctrlW - 76, 24);
        sl.numberOfTickMarks = 0; sl.continuous = YES;
        NSTextField* value = [NSTextField labelWithString:@""];
        value.font = [NSFont monospacedDigitSystemFontOfSize:12 weight:NSFontWeightRegular]; value.textColor = NSColor.secondaryLabelColor;
        value.alignment = NSTextAlignmentRight; value.frame = NSMakeRect(NSMaxX(cr) - 70, y + 8, 70, 16);
        char text[48]; std::snprintf(text, sizeof text, r.fmt, std::atof(v.c_str()) * r.mult);
        value.stringValue = @(text);
        value.tag = index; [doc addSubview:value]; [self.valueLabels addObject:value];
        control = sl;
        break;
      }
      case TEXT: {
        NSTextField* f = [NSTextField textFieldWithString:@(v.c_str())];
        f.frame = cr; f.placeholderString = r.def[0] ? @(r.def) : @"None"; f.target = self; f.action = @selector(changed:);
        f.font = [NSFont systemFontOfSize:13];
        control = f;
        break;
      }
      case PATH: {
        NSButton* b = [NSButton buttonWithTitle:std::string(r.key) == "GeckoCodes.ini" ? @"Show in Finder" : @"Choose…" target:self action:@selector(choosePath:)];
        [b sizeToFit];
        b.frame = NSMakeRect(NSMaxX(cr) - b.frame.size.width, y + 2, b.frame.size.width, b.frame.size.height);
        if (std::string(r.key) == "replaydir") {
          const std::string shown = v.empty() ? self.settings->replay_dir : v;
          NSTextField* p = [NSTextField labelWithString:[@(shown.c_str()) stringByAbbreviatingWithTildeInPath]];
          p.font = [NSFont systemFontOfSize:11]; p.textColor = NSColor.secondaryLabelColor; p.lineBreakMode = NSLineBreakByTruncatingMiddle;
          p.frame = NSMakeRect(pad, y + 26, W - 2 * pad - ctrlW, 14); p.tag = index;
          [doc addSubview:p]; [self.valueLabels addObject:p];
          y += 12;
        }
        control = b;
        break;
      }
      case NOTE: break;
      case BUTTONS: {
        CGFloat x = NSMaxX(cr);
        for (size_t i = r.titles.size(); i-- > 0;) {
          NSButton* b = [NSButton buttonWithTitle:@(r.titles[i]) target:self action:@selector(pressed:)];
          [b sizeToFit];
          x -= b.frame.size.width;
          b.frame = NSMakeRect(x, y + 2, b.frame.size.width, b.frame.size.height);
          b.identifier = @(r.values[i]);
          [doc addSubview:b];
          x -= 8;
        }
        break;
      }
    }
    if (control) { control.tag = index; if (!control.target) control.target = self; [doc addSubview:control]; }
    y += rowH;
    if (r.hint) {
      NSTextField* hint = [NSTextField wrappingLabelWithString:@(r.hint)];
      hint.font = [NSFont systemFontOfSize:11]; hint.textColor = NSColor.tertiaryLabelColor;
      const CGFloat hw = r.kind == NOTE ? W - 2 * pad : W - 2 * pad - ctrlW - 12;
      const CGFloat hh = [hint.cell cellSizeForBounds:NSMakeRect(0, 0, hw, 200)].height;
      hint.frame = NSMakeRect(pad, y - 6, hw, hh);
      [doc addSubview:hint];
      y += hh + 2;
    }
  }
  y += 24;
  doc.frame = NSMakeRect(0, 0, W, y);   // no taller than its rows: a short page has no scroller
  self.scroll.documentView = doc;
  [doc scrollPoint:NSZeroPoint];
}
- (const Row*)rowFor:(NSInteger)tag { return tag >= 1 && tag <= (NSInteger)rows().size() ? &rows()[size_t(tag - 1)] : nullptr; }
- (void)changed:(NSControl*)sender {
  const Row* r = [self rowFor:sender.tag];
  if (!r) return;
  std::string v;
  if ([sender isKindOfClass:NSSwitch.class]) v = ((NSSwitch*)sender).state == NSControlStateValueOn ? "1" : "0";
  else if ([sender isKindOfClass:NSPopUpButton.class]) {
    const NSInteger i = ((NSPopUpButton*)sender).indexOfSelectedItem;
    if (i < 0 || i >= (NSInteger)r->values.size()) return;   // "Other": a value set by hand stays
    v = r->values[size_t(i)];
  } else if ([sender isKindOfClass:NSSlider.class]) {
    const double snapped = std::round((((NSSlider*)sender).doubleValue - r->lo) / r->step) * r->step + r->lo;
    char text[48];
    std::snprintf(text, sizeof text, r->step < 1 ? "%.2f" : "%.0f", snapped); v = text;
    for (NSTextField* l in self.valueLabels) if (l.tag == sender.tag) { std::snprintf(text, sizeof text, r->fmt, snapped * r->mult); l.stringValue = @(text); }
    if (NSApp.currentEvent.type == NSEventTypeLeftMouseDragged) { [self set:*r to:v]; return; }
  } else if ([sender isKindOfClass:NSTextField.class]) {
    v = ((NSTextField*)sender).stringValue.UTF8String;
    v.erase(std::remove_if(v.begin(), v.end(), [](char c) { return c == ' ' || c == '\t'; }), v.end());
    if (std::string(r->key).rfind("blockcode", 0) == 0) { for (auto& c : v) c = char(std::toupper((unsigned char)c)); v = v.substr(0, 16); }
  }
  [self set:*r to:v];
}
- (void)pressed:(NSButton*)sender {
  const std::string act = sender.identifier.UTF8String;
  if (act == "arcade:on" || act == "arcade:off") {   // upstream's set_all: every Arcade look row on (the grid) or off
    const bool on = act == "arcade:on";
    for (const char* k : {"arcade_hitboxes", "arcade_projectile_hitboxes", "arcade_hitboxes_behind", "arcade_camera_2d", "arcade_combo_counter"}) host::set_engine_setting(k, on ? "1" : "0");
    host::set_engine_setting("arcade_backdrop", on ? "2" : "0");
    host::log("settings: Arcade look %s", on ? "preset (every row on)" : "reset (every row off)");
    [self build];
    return;
  }
  NSFileManager* fm = NSFileManager.defaultManager;
  NSString* root = @(cosmetics_dir().c_str());
  if (act == "cos:open") {
    for (NSString* sub in @[@"Costumes", @"Stages"]) [fm createDirectoryAtPath:[root stringByAppendingPathComponent:sub] withIntermediateDirectories:YES attributes:nil error:nil];
    [NSWorkspace.sharedWorkspace openURL:[NSURL fileURLWithPath:root]];
    return;
  }
  NSOpenPanel* panel = [NSOpenPanel openPanel];
  panel.allowsMultipleSelection = act == "cos:import";
  panel.allowedFileTypes = act == "cos:import" ? @[@"dat", @"zip"] : @[@"dat"];
  panel.message = act == "cos:import" ? @"Choose skins: costume files (PlFxGr.dat), stage files (GrNBa.dat) or a .zip of them" : @"Choose the stage file to use";
  if ([panel runModal] != NSModalResponseOK) return;
  std::string target_stage;
  if (act == "cos:stage") {   // "Stage to replace": the stage this file is served for
    static const char* const kStages[][2] = {{"GrNBa", "Battlefield"}, {"GrNLa", "Final Destination"}, {"GrSt", "Yoshi's Story"}, {"GrIz", "Fountain of Dreams"},
                                             {"GrOp", "Dream Land"}, {"GrPs", "Pokémon Stadium"}, {"GrCs", "Princess Peach's Castle"}, {"GrKg", "Kongo Jungle"},
                                             {"GrCn", "Corneria"}, {"GrRc", "Rainbow Cruise"}, {"GrMc", "Mute City"}, {"GrBb", "Big Blue"}};
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Stage to replace"; alert.informativeText = panel.URL.lastPathComponent;
    NSPopUpButton* pop = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 260, 26) pullsDown:NO];
    for (const auto& s : kStages) [pop addItemWithTitle:@(s[1])];
    alert.accessoryView = pop;
    [alert addButtonWithTitle:@"Add"]; [alert addButtonWithTitle:@"Cancel"];
    if ([alert runModal] != NSAlertFirstButtonReturn) return;
    target_stage = kStages[pop.indexOfSelectedItem][0];
  }
  [self importSkins:panel.URLs stage:target_stage];
}
// Copies skins into the Cosmetics folder: the .dat files given, or those inside a given .zip (unpacked with the system's
// ditto). `target_stage` (Add stage DAT): the file is served for that stage, named "<stage> - <its name>.dat".
- (void)importSkins:(NSArray<NSURL*>*)urls stage:(const std::string&)target_stage {
  NSFileManager* fm = NSFileManager.defaultManager;
  NSString* root = @(cosmetics_dir().c_str());
  NSMutableArray<NSString*>* files = [NSMutableArray array];
  NSString* scratch = [NSTemporaryDirectory() stringByAppendingPathComponent:[NSUUID UUID].UUIDString];
  for (NSURL* url in urls) {
    if ([url.pathExtension.lowercaseString isEqualToString:@"zip"]) {
      NSTask* t = [NSTask launchedTaskWithExecutableURL:[NSURL fileURLWithPath:@"/usr/bin/ditto"] arguments:@[@"-x", @"-k", url.path, scratch] error:nil terminationHandler:nil];
      [t waitUntilExit];
      for (NSString* rel in [fm enumeratorAtPath:scratch]) if ([rel.pathExtension.lowercaseString isEqualToString:@"dat"]) [files addObject:[scratch stringByAppendingPathComponent:rel]];
    } else [files addObject:url.path];
  }
  int added = 0, refused = 0;
  for (NSString* f in files) {
    NSString* name = f.lastPathComponent;
    std::string kind = skin_kind(name);
    if (!target_stage.empty()) { kind = "Stages"; name = [NSString stringWithFormat:@"%s - %@", target_stage.c_str(), name.stringByDeletingPathExtension]; name = [name stringByAppendingPathExtension:@"dat"]; }
    if (kind.empty()) { ++refused; host::log("settings: %s is not a costume (Pl...dat) or stage (Gr...dat) file; not added", name.UTF8String); continue; }
    NSString* dir = [root stringByAppendingPathComponent:@(kind.c_str())];
    [fm createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    NSString* to = [dir stringByAppendingPathComponent:name];
    [fm removeItemAtPath:to error:nil];
    if ([fm copyItemAtPath:f toPath:to error:nil]) { ++added; host::log("settings: skin added: %s", to.UTF8String); }
  }
  [fm removeItemAtPath:scratch error:nil];
  NSAlert* done = [[NSAlert alloc] init];
  done.messageText = added ? [NSString stringWithFormat:@"%d skin%s added", added, added == 1 ? "" : "s"] : @"No skins added";
  done.informativeText = refused ? [NSString stringWithFormat:@"%d file%s skipped: only costume (Pl…dat) and stage (Gr…dat) files are skins.", refused, refused == 1 ? "" : "s"]
                                 : @"Pick them on the character and stage select, or in game under Mods > Choose skins.";
  if (std::getenv("MELEE_NO_ALERTS")) host::log("settings: %s. %s", done.messageText.UTF8String, done.informativeText.UTF8String);
  else [done runModal];
}
- (void)choosePath:(NSButton*)sender {
  const Row* r = [self rowFor:sender.tag];
  if (!r) return;
  if (std::string(r->key) == "GeckoCodes.ini") {
    NSString* file = @((support() + "/melee-unlocked/GeckoCodes.ini").c_str());
    if (![NSFileManager.defaultManager fileExistsAtPath:file])
      [@"# Your Gecko codes, in Dolphin's format: a $Name line, then the code lines. Switch each on in game under Gecko codes.\n"
          writeToFile:file atomically:YES encoding:NSUTF8StringEncoding error:nil];
    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[[NSURL fileURLWithPath:file]]];
    return;
  }
  NSOpenPanel* panel = [NSOpenPanel openPanel];
  panel.canChooseDirectories = YES; panel.canChooseFiles = NO; panel.canCreateDirectories = YES;
  panel.message = @"Choose the folder for your replays";
  if ([panel runModal] != NSModalResponseOK) return;
  const std::string path = panel.URL.path.UTF8String;
  [self set:*r to:path];
  self.settings->replay_dir = path;
  for (NSTextField* l in self.valueLabels) if (l.tag == sender.tag) l.stringValue = [@(path.c_str()) stringByAbbreviatingWithTildeInPath];
}
- (void)windowWillClose:(NSNotification*)n { [NSApp stopModal]; }
@end

namespace host {
// Opens the Settings window on `category` (0 Video .. 7 Mods) and returns when it is closed. With `shot` set (tests and
// screenshots) the window stays off every screen, is drawn into that file after a moment and closes by itself.
void mu_settings_window_run(LauncherSettings& settings, int category, const char* shot) {
  @autoreleasepool {
    MUSettingsController* c = [[MUSettingsController alloc] init];
    c.settings = &settings;
    [c open:category];
    if (shot) {
      [c.window setFrameOrigin:NSMakePoint(-20000, -20000)];
      [c.window orderFront:nil];
      std::printf("settings: window %ld\n", (long)c.window.windowNumber); std::fflush(stdout);
      // Tests: import these skins (paths separated by ":"; MELEE_SETTINGS_IMPORT_STAGE=GrNBa for Add stage DAT) before the shot.
      if (const char* import = std::getenv("MELEE_SETTINGS_IMPORT")) {
        NSMutableArray<NSURL*>* urls = [NSMutableArray array];
        for (NSString* p in [@(import) componentsSeparatedByString:@":"]) if (p.length) [urls addObject:[NSURL fileURLWithPath:p]];
        const char* stage = std::getenv("MELEE_SETTINGS_IMPORT_STAGE");
        [c importSkins:urls stage:stage ? stage : ""];
      }
      if (const char* press = std::getenv("MELEE_SETTINGS_PRESS")) {   // tests: press a button row (arcade:on, arcade:off)
        NSButton* b = [[NSButton alloc] init]; b.identifier = @(press); [c pressed:b];
      }
      if (const char* scroll = std::getenv("MELEE_SETTINGS_SCROLL")) [c.scroll.documentView scrollPoint:NSMakePoint(0, std::atof(scroll))];
      NSString* out = @(shot);
      const double wait = std::getenv("MELEE_SETTINGS_WAIT") ? std::atof(std::getenv("MELEE_SETTINGS_WAIT")) : 1.0;
      NSTimer* t = [NSTimer timerWithTimeInterval:wait repeats:NO block:^(NSTimer*) {
        NSView* v = c.window.contentView.superview ?: c.window.contentView;
        NSBitmapImageRep* rep = [v bitmapImageRepForCachingDisplayInRect:v.bounds];
        [v cacheDisplayInRect:v.bounds toBitmapImageRep:rep];
        [[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:out atomically:YES];
        std::printf("settings: screenshot %s\n", out.UTF8String); std::fflush(stdout);
        [c.window close];
      }];
      [NSRunLoop.mainRunLoop addTimer:t forMode:NSRunLoopCommonModes];
    } else {
      [c.window center];
      [c.window makeKeyAndOrderFront:nil];
    }
    [NSApp runModalForWindow:c.window];
    [c.window orderOut:nil];
  }
}
}  // namespace host
