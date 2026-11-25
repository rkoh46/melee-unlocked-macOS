// melee-unlocked's launcher, rebuilt for the Mac. The layout, palette and words are melee-unlocked's own Win32 launcher
// (port/app/launcher.cpp and its pages in the fetched tree): a left rail carrying the wordmark, the page nav and Bailey,
// and a content column on a dark gradient, in layout units of a 720 x 480 window. Everything is painted by one view, as
// the original paints in WM_PAINT; the artwork (the MU mark and Bailey) is converted from the fetched tree's bitmaps at
// build time into the bundle and never committed.
//
// Pages: Play (disc, Game Build, status rows, PLAY), Settings, Replay Viewer, Mods, and the launcher colour picker.
// melee-unlocked's YouGame line, lobby, Build page and version picker are left out.
// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>
#include "mac_launcher.h"
#include "dashboard.h"
#include "input_config.h"
#include "mods_facade.h"

#include <algorithm>
#include <array>
#include <memory>
#include <cmath>
#include <fstream>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
// ---- melee-unlocked's palette (launcher.cpp) and launcher_theme.h --------------------------------------------------
struct RGB8 { int r, g, b; };
NSColor* col(RGB8 c, double a = 1) { return [NSColor colorWithSRGBRed:c.r / 255.0 green:c.g / 255.0 blue:c.b / 255.0 alpha:a]; }
const RGB8 C_CONTENT_TOP{0x17, 0x1F, 0x30}, C_CONTENT_BOT{0x11, 0x18, 0x25};
const RGB8 C_RAIL_TOP{0x0B, 0x11, 0x1E}, C_RAIL_BOT{0x13, 0x1B, 0x2C};
const RGB8 C_DIVIDER{0x27, 0x31, 0x4A}, C_SEP{0x22, 0x2B, 0x42};
const RGB8 C_TEXT{0xE7, 0xEC, 0xF5}, C_DIM{0x8D, 0x9B, 0xB5}, C_FAINT{0x66, 0x74, 0x8E};
const RGB8 C_FIELD{0x0E, 0x15, 0x22}, C_FIELD_BORDER{0x2B, 0x36, 0x52};
const RGB8 C_BTN{0x21, 0x2B, 0x42}, C_BTN_BORDER{0x35, 0x41, 0x5F}, C_BTN_DOWN{0x18, 0x21, 0x34};
const RGB8 C_NAV_ON{0x1F, 0x2A, 0x40}, C_NAV_HOT{0x18, 0x21, 0x34};
const RGB8 C_OFF_FILL{0x1A, 0x21, 0x32}, C_OFF_BORDER{0x28, 0x31, 0x47}, C_OFF_TEXT{0x5C, 0x68, 0x7E};
const RGB8 C_OK{0x5A, 0xC8, 0x8A}, C_WARN{0xE5, 0xA8, 0x4A}, C_BAD{0xE0, 0x6B, 0x5B};

RGB8 g_accent{142, 84, 239};
RGB8 mix(RGB8 a, RGB8 b, double t) {
  t = std::clamp(t, 0.0, 1.0);
  auto ch = [t](int x, int y) { return int(std::lround(x + (y - x) * t)); };
  return {ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b)};
}
RGB8 acc_top() { return mix(g_accent, {255, 255, 255}, .19); }
RGB8 acc_bottom() { return mix(g_accent, {0, 0, 0}, .16); }
RGB8 acc_pressed() { return mix(g_accent, {0, 0, 0}, .34); }
RGB8 on_accent() { return .2126 * g_accent.r + .7152 * g_accent.g + .0722 * g_accent.b > 169 ? RGB8{16, 23, 38} : RGB8{255, 255, 255}; }
RGB8 hsv(double h, double s, double v) {
  h = std::fmod(h + 360.0, 360.0); s = std::clamp(s, 0.0, 1.0); v = std::clamp(v, 0.0, 1.0);
  double c = v * s, x = c * (1 - std::abs(std::fmod(h / 60.0, 2.0) - 1)), m = v - c, r = 0, g = 0, b = 0;
  if (h < 60) { r = c; g = x; } else if (h < 120) { r = x; g = c; } else if (h < 180) { g = c; b = x; }
  else if (h < 240) { g = x; b = c; } else if (h < 300) { r = x; b = c; } else { r = c; b = x; }
  return {int((r + m) * 255 + .5), int((g + m) * 255 + .5), int((b + m) * 255 + .5)};
}
void to_hsv(RGB8 c, double& h, double& s, double& v) {
  double r = c.r / 255.0, g = c.g / 255.0, b = c.b / 255.0, hi = std::max({r, g, b}), lo = std::min({r, g, b}), d = hi - lo;
  v = hi; s = hi == 0 ? 0 : d / hi; h = 0;
  if (d > 0) { if (hi == r) h = 60 * std::fmod((g - b) / d + 6, 6.0); else if (hi == g) h = 60 * ((b - r) / d + 2); else h = 60 * ((r - g) / d + 4); }
}

// Layout units of the original 720 x 480 client area.
const CGFloat WIN_W = 720, WIN_H = 480, RAIL_W = 190, CX = 212, CW = 486, NAV_Y = 152, NAV_H = 34, NAV_GAP = 38;
NSRect LR(CGFloat x, CGFloat y, CGFloat w, CGFloat h) { return NSMakeRect(x, y, w, h); }

// The original's fonts: the system message font at 12, 11 (small), 14 semibold (nav), 10 bold (labels), 21 bold (PLAY).
NSFont* f_body() { return [NSFont systemFontOfSize:12]; }
NSFont* f_small() { return [NSFont systemFontOfSize:11]; }
NSFont* f_nav() { return [NSFont systemFontOfSize:14 weight:NSFontWeightSemibold]; }
NSFont* f_label() { return [NSFont systemFontOfSize:10 weight:NSFontWeightBold]; }
NSFont* f_big() { return [NSFont systemFontOfSize:21 weight:NSFontWeightBold]; }

NSString* ns(const std::string& s) {   // Slippi names can be Shift-JIS
  if (NSString* utf8 = [NSString stringWithUTF8String:s.c_str()]) return utf8;
  if (NSString* sjis = [[NSString alloc] initWithBytes:s.data() length:s.size() encoding:NSShiftJISStringEncoding]) return sjis;
  return [[NSString alloc] initWithBytes:s.data() length:s.size() encoding:NSISOLatin1StringEncoding] ?: @"";
}

void vgrad(NSRect r, RGB8 top, RGB8 bottom) {   // the view is flipped: the start colour is at the top
  [[[NSGradient alloc] initWithStartingColor:col(top) endingColor:col(bottom)] drawInRect:r angle:90];
}
void round_rect(NSRect r, CGFloat radius, RGB8 top, RGB8 bottom, const RGB8* border) {
  NSBezierPath* p = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(r, .5, .5) xRadius:radius yRadius:radius];
  [[[NSGradient alloc] initWithStartingColor:col(top) endingColor:col(bottom)] drawInBezierPath:p angle:90];
  if (border) { [col(*border) setStroke]; p.lineWidth = 1; [p stroke]; }
}
// ---- launcher.ini (the app's file in ~/Library/Application Support/Dashdance; the launcher's own keys: language, notesseen) ----
std::string support_dir() { const char* h = std::getenv("HOME"); return std::string(h ? h : ".") + "/Library/Application Support/Dashdance"; }
std::string ini_get(const std::string& key) {
  std::ifstream in(support_dir() + "/launcher.ini");
  std::string line, out;
  while (std::getline(in, line)) if (line.compare(0, key.size() + 1, key + "=") == 0) out = line.substr(key.size() + 1);
  return out;
}
void ini_set(const std::string& key, const std::string& value) {
  std::vector<std::string> lines; bool found = false;
  { std::ifstream in(support_dir() + "/launcher.ini"); std::string line; while (std::getline(in, line)) lines.push_back(line); }
  for (auto& l : lines) if (l.compare(0, key.size() + 1, key + "=") == 0) { l = key + "=" + value; found = true; }
  if (!found) lines.push_back(key + "=" + value);
  std::ofstream out(support_dir() + "/launcher.ini", std::ios::trunc);
  for (const auto& l : lines) out << l << "\n";
}

// The in-game panel's CUSTOMIZE > Launcher colour (launcher.ini mu_accent, upstream's six colours) is the launcher's colour
// too: taken when it changed since it was last taken (mu_accent_applied), so the colour wheel's own pick is kept otherwise.
void apply_mu_accent(host::LauncherSettings& s) {
  const std::string pick = ini_get("mu_accent");
  if (pick.empty() || pick == ini_get("mu_accent_applied")) return;
  static const uint32_t kColours[] = {0x8E54EF, 0x3D7EEA, 0x3FAE6A, 0xE5A84A, 0xE06B5B, 0xE0569B};   // Purple .. Pink
  const uint32_t c = kColours[std::clamp(std::atoi(pick.c_str()), 0, 5)];
  s.accent = c; g_accent = {int((c >> 16) & 255), int((c >> 8) & 255), int(c & 255)};
  ini_set("accent", std::to_string(c)); ini_set("mu_accent_applied", pick);
  host::log("launcher: launcher colour %06X from the settings' Launcher colour (%s)", c, pick.c_str());
}

// ---- melee-unlocked's launcher languages (port/app/launcher_lang.h: its list, its file format, its lookup by English wording) ----
// The tables are its lang/*.txt, copied from the fetched tree into the bundle at packaging (Resources/mu-lang). Every fixed
// English string drawn by text() is looked up ("@English = ..." lines); names, paths and lines without a translation stay as
// they are. Japanese, Chinese and Korean come from the system's fonts (AppKit's font fallback). launcher.ini `language`.
struct Language { const char* code; const char* native; };
const Language kLanguages[] = {{"en", "English"}, {"es", "Español"}, {"pt-BR", "Português (Brasil)"}, {"fr", "Français"}, {"de", "Deutsch"},
                               {"ru", "Русский"}, {"ja", "日本語"}, {"zh-Hans", "简体中文"}, {"ko", "한국어"}};
std::string g_lang = "en";
NSMutableDictionary<NSString*, NSString*>* g_words = nil;
NSString* lang_dir() {
  if (const char* e = std::getenv("MELEE_MU_LANG")) return [NSString stringWithUTF8String:e];
  return [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"mu-lang"];
}
NSString* unescape(NSString* s) {   // launcher_lang.h detail::unescape
  NSMutableString* out = [NSMutableString string];
  for (NSUInteger i = 0; i < s.length; ++i) {
    const unichar c = [s characterAtIndex:i];
    if (c == '\\' && i + 1 < s.length) {
      const unichar n = [s characterAtIndex:i + 1];
      if (n == 'n') { [out appendString:@"\n"]; ++i; continue; }
      if (n == 't') { [out appendString:@"\t"]; ++i; continue; }
      if (n == '\\') { [out appendString:@"\\"]; ++i; continue; }
    }
    [out appendFormat:@"%C", c];
  }
  return out;
}
bool lang_set(const std::string& code) {
  g_words = nil; g_lang = "en";
  if (code.empty() || code == "en") return true;
  bool known = false;
  for (const Language& l : kLanguages) known = known || code == l.code;
  if (!known) return false;
  NSString* file = [lang_dir() stringByAppendingPathComponent:[NSString stringWithFormat:@"%s.txt", code.c_str()]];
  NSString* all = [NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil];
  if (!all) { host::log("launcher: language %s: %s is missing", code.c_str(), file.UTF8String); return false; }
  NSMutableDictionary* words = [NSMutableDictionary dictionary];
  NSCharacterSet* space = NSCharacterSet.whitespaceCharacterSet;
  for (NSString* raw in [all componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) {
    NSString* t = [raw stringByTrimmingCharactersInSet:space];
    if ([t hasPrefix:@"\uFEFF"]) t = [t substringFromIndex:1];
    if (!t.length || [t hasPrefix:@"#"] || [t hasPrefix:@";"]) continue;
    const NSRange eq = [t rangeOfString:@"="];
    if (eq.location == NSNotFound) continue;
    NSString* key = unescape([[t substringToIndex:eq.location] stringByTrimmingCharactersInSet:space]);
    NSString* value = unescape([[t substringFromIndex:eq.location + 1] stringByTrimmingCharactersInSet:space]);
    if (key.length && value.length) words[key] = value;
  }
  g_words = words; g_lang = code;
  host::log("launcher: language %s (%lu lines)", code.c_str(), (unsigned long)words.count);
  return true;
}
NSString* tx(NSString* english) {
  if (!g_words || !english.length) return english;
  NSString* t = g_words[[@"@" stringByAppendingString:english]];
  return t.length ? t : english;
}

enum Align { LEFT, CENTER, RIGHT };
void text(NSString* s, NSRect r, NSFont* font, RGB8 color, Align align = LEFT, CGFloat kern = 0, bool wrap = false) {
  s = tx(s);
  NSMutableParagraphStyle* ps = [[NSMutableParagraphStyle alloc] init];
  ps.alignment = align == CENTER ? NSTextAlignmentCenter : align == RIGHT ? NSTextAlignmentRight : NSTextAlignmentLeft;
  ps.lineBreakMode = wrap ? NSLineBreakByWordWrapping : NSLineBreakByTruncatingTail;
  NSDictionary* a = @{NSFontAttributeName: font, NSForegroundColorAttributeName: col(color), NSParagraphStyleAttributeName: ps, NSKernAttributeName: @(kern)};
  NSRect box = r;
  if (!wrap) {   // DT_VCENTER
    const CGFloat h = font.ascender - font.descender + font.leading;
    box = NSMakeRect(r.origin.x, r.origin.y + (r.size.height - h) / 2, r.size.width, h);
  }
  [s drawWithRect:box options:NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingTruncatesLastVisibleLine attributes:a];
}
CGFloat text_width(NSString* s, NSFont* font) { return [tx(s) sizeWithAttributes:@{NSFontAttributeName: font}].width; }   // as text() draws it
void dot(CGFloat x, CGFloat y, RGB8 c) { round_rect(LR(x, y, 8, 8), 4, c, c, nullptr); }

std::string home() { const char* h = std::getenv("HOME"); return h ? h : "."; }

// The artwork the build converted from the fetched tree (tools/mu_launcher_art.py): the MU mark and Bailey.
NSImage* art(NSString* name) {
  NSString* dir = nil;
  if (const char* e = std::getenv("MELEE_MU_ART")) dir = ns(e);
  else dir = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"mu-art"];
  return [[NSImage alloc] initWithContentsOfFile:[dir stringByAppendingPathComponent:name]];
}
// The player's own rail picture in Bailey's place (upstream's rail-image.*, chosen by clicking Bailey): rail-image.<ext> in the
// melee-unlocked folder; none, or one that cannot be read: Bailey.
const char* const kRailExts[] = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".heic"};
std::string rail_image_path() {
  for (const char* ext : kRailExts) {
    const std::string p = support_dir() + "/melee-unlocked/rail-image" + ext;
    if (FILE* f = std::fopen(p.c_str(), "rb")) { std::fclose(f); return p; }
  }
  return {};
}
// What's new: this app's notes (whats-new.txt in the bundle, newest first), shown by themselves at the first start of a new
// version (launcher.ini notesseen, upstream's key) and from the version line's link.
NSArray<NSString*>* whats_new_lines() {
  NSString* file = std::getenv("MELEE_WHATS_NEW") ? [NSString stringWithUTF8String:std::getenv("MELEE_WHATS_NEW")]
                                                  : [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"whats-new.txt"];
  NSMutableArray* out = [NSMutableArray array];
  for (NSString* l in [[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] ?: @"" componentsSeparatedByString:@"\n"])
    if (l.length && ![l hasPrefix:@"#"]) [out addObject:l];
  return out;
}
}  // namespace

// ---- melee-unlocked's crash report (mu-engine/app/crash_report.cpp) and the mod installer (mu-engine/modinstall through
// mu-engine/app/mods_facade.cpp), both linked into the launcher ----------------------------------------------------------
namespace mu_crash {
bool save(const std::string& support, const std::string& replay_dir, std::string* where, std::string* error);
}
// ---- the Replay Viewer's stats card (mu-engine/app/replay_stats_facade.cpp, melee-unlocked's replay data, stats and session
// trace overview from the fetched tree). Declared as in replay_stats_facade.h (keep the two in step).
namespace mu_replay_stats {
struct Card {
  bool ok = false;
  std::string title, date;
  std::vector<std::pair<std::string, std::string>> rows;
  bool trace = false;
  std::vector<std::array<float, 5>> bins;
  std::string trace_summary;
};
__attribute__((weak)) Card read(const std::string&) { return {}; }
__attribute__((weak)) int stage_of(const std::string&) { return -1; }
}
// The mod profile the engine loads at its next start: settings.ini's mod_profile (fe/settings.cpp reads it; the engine's
// MODS page writes the same key).
namespace {
// The disc check: a GameCube disc image starts with its game code (GALE01 is Melee NTSC) and has the revision at byte 7
// (2 for NTSC 1.02, the one melee-unlocked and Slippi play). Compressed images (.ciso, .rvz) are named, not read.
std::string disc_check(const std::string& path) {
  static std::string for_path, answer;
  if (path == for_path) return answer;
  for_path = path;
  std::string ext = path.size() > 4 ? path.substr(path.find_last_of('.') + 1) : "";
  for (auto& c : ext) c = char(std::tolower((unsigned char)c));
  if (ext == "rvz" || ext == "ciso" || ext == "gcz" || ext == "wia") return answer = "Disc: a compressed image (." + ext + "); melee-unlocked reads a plain .iso";
  FILE* f = std::fopen(path.c_str(), "rb");
  unsigned char h[8] = {};
  const bool read = f && std::fread(h, 1, sizeof h, f) == sizeof h;
  if (f) std::fclose(f);
  if (!read) return answer = "Disc: the image cannot be read (moved or renamed?)";
  const std::string code(reinterpret_cast<const char*>(h), 6);
  if (code != "GALE01") return answer = "Disc: " + code + " is not Melee NTSC (GALE01)";
  if (h[7] != 2) return answer = "Disc: Melee NTSC 1.0" + std::to_string(int(h[7])) + "; melee-unlocked and Slippi need NTSC 1.02";
  return answer = "Disc: Melee NTSC 1.02";
}
// Each mod disc's costume skins, as the engine's last start counted them (fe/features/cosmetics.cpp writes the file).
size_t disc_skins(const std::string& profile) {
  std::ifstream in(support_dir() + "/melee-unlocked/mods-cache/disc-skins.txt");
  std::string line;
  while (std::getline(in, line)) {
    const size_t eq = line.find('=');
    if (eq != std::string::npos && line.substr(0, eq) == profile) return size_t(std::strtoul(line.c_str() + eq + 1, nullptr, 10));
  }
  return 0;
}
std::string mod_profile_next() { return host::engine_setting("mod_profile"); }
void mod_profile_set(const std::string& profile) { host::set_engine_setting("mod_profile", profile); }
}  // namespace


// ---- the colour picker (launcher_theme_picker.inl): a 268 x 324 popover with an HSV wheel, a value bar, hex, Apply ----
@class MULaunchView;
@interface MUThemeView : NSView
@property(nonatomic, weak) MULaunchView* owner;
@property(nonatomic) NSTextField* hex;
@property(nonatomic) double h, s, v;
@property(nonatomic) RGB8 before;
@property(nonatomic) BOOL applied;
- (void)apply;
@end

@interface MULaunchView : NSView
@property(nonatomic) host::LauncherSettings* settings;
@property(nonatomic) int tab;          // 0 Play, 1 Settings, 2 Replay Viewer, 3 Mods
@property(nonatomic) int hotNav;
@property(nonatomic) NSString* hot;    // the button under the pointer
@property(nonatomic) NSString* down;   // the button being pressed
@property(nonatomic) NSImage* mark;
@property(nonatomic) NSImage* bailey;
@property(nonatomic) host::Dashboard dashboard;
@property(nonatomic) std::string error;
@property(nonatomic) std::string crashNote;   // the crash report button's last result
@property(nonatomic) NSPanel* themePanel;
@property(nonatomic) NSMutableDictionary<NSString*, NSValue*>* buttons;   // name -> rect, rebuilt every paint
@property(nonatomic) NSTimer* modsTimer;   // repaints the Mods page while a job runs
@property(nonatomic) std::string modsMessage;
// Watch live: the Slippi Launcher's spectate folder, each new game file opened as it appears (melee-unlocked's Watch live).
@property(nonatomic) BOOL liveWatch;
@property(nonatomic) NSMutableSet<NSString*>* liveSeen;
@property(nonatomic) NSTimer* liveTimer;
@property(nonatomic) NSTask* watchTask;    // the engine playing a replay now (one at a time)
@property(nonatomic) NSString* liveStatus;
// The picked replay's card (upstream's Replay Viewer: its stats and the session trace graph), read off the main thread.
@property(nonatomic) NSInteger picked;
@property(nonatomic) std::shared_ptr<mu_replay_stats::Card> card;
@property(nonatomic) BOOL cardLoading;
@property(nonatomic) NSString* replayNote;   // why a replay did not start (upstream's replay_status line)
// The update check: this repository's branch on GitHub against the commit this app was built from (Info.plist MUBuildCommit).
// Never melee-unlocked's own updater: this app is built from source, so an update is the install line run again.
@property(nonatomic) NSString* updateText;
@property(nonatomic) BOOL notesOpen;   // What's new over the page
@property(nonatomic) NSImage* railPicture;
@property(nonatomic) BOOL updateAvailable;
- (void)checkForUpdate;
- (void)openTheme;
- (void)closeTheme;
@end

@implementation MUThemeView
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirty {
  [col(C_CONTENT_BOT) setFill]; NSRectFill(self.bounds);
  round_rect(LR(1, 1, 266, 322), 12, C_FIELD, C_FIELD, &C_FIELD_BORDER);
  text(@"Launcher color", LR(20, 15, 180, 24), f_nav(), C_TEXT);
  text(@"Pick a color or enter a hex code", LR(20, 42, 232, 20), f_small(), C_DIM);
  const CGFloat cx = 134, cy = 156, radius = 78;
  for (int y = -78; y <= 78; ++y) for (int x = -78; x <= 78; ++x) {   // the wheel, a pixel at a time as the original
    const double d = std::sqrt(double(x * x + y * y));
    if (d > radius + .5) continue;
    const double hue = std::fmod(std::atan2(double(-y), double(x)) * 180 / M_PI + 360, 360.0);
    [col(hsv(hue, d / radius, 1), std::clamp(radius + .5 - d, 0.0, 1.0)) setFill];
    NSRectFillUsingOperation(LR(cx + x, cy + y, 1, 1), NSCompositingOperationSourceOver);
  }
  const double rad = self.h * M_PI / 180.0;
  const CGFloat dx = cx + std::cos(rad) * self.s * radius, dy = cy - std::sin(rad) * self.s * radius;
  const RGB8 ring{22, 27, 42};
  round_rect(LR(dx - 6, dy - 6, 12, 12), 6, {255, 255, 255}, {255, 255, 255}, &ring);
  for (int x = 44; x < 224; ++x) { [col(hsv(self.h, self.s, (x - 44) / 180.0)) setFill]; NSRectFill(LR(x, 246, 1, 15)); }
  [col(C_TEXT) setFill]; NSRectFill(LR(44 + self.v * 180 - 2, 243, 4, 21));
  round_rect(LR(20, 272, 105, 30), 6, C_FIELD, C_FIELD, &C_FIELD_BORDER);
  round_rect(LR(137, 268, 109, 35), 7, acc_top(), acc_bottom(), nullptr);
  text(@"Apply", LR(137, 268, 109, 35), f_body(), on_accent(), CENTER);
}
- (void)pickAt:(NSPoint)p {
  const double dx = p.x - 134, dy = 156 - p.y, d = std::sqrt(dx * dx + dy * dy);
  if (d <= 80 && d >= 3) { self.h = std::fmod(std::atan2(dy, dx) * 180 / M_PI + 360, 360.0); self.s = std::clamp(d / 78, 0.0, 1.0); }
  else if (p.y >= 246 && p.y <= 261 && p.x >= 44 && p.x <= 224) self.v = std::clamp((p.x - 44) / 180.0, 0.0, 1.0);
  else return;
  [self preview:hsv(self.h, self.s, self.v)];
}
- (void)preview:(RGB8)c {
  g_accent = c;
  self.hex.stringValue = [NSString stringWithFormat:@"#%02X%02X%02X", c.r, c.g, c.b];
  [self setNeedsDisplay:YES]; [(NSView*)self.owner setNeedsDisplay:YES];
}
- (void)mouseDown:(NSEvent*)e {
  const NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
  if (p.x >= 137 && p.x <= 246 && p.y >= 268 && p.y <= 303) { [self apply]; return; }
  [self pickAt:p];
}
- (void)mouseDragged:(NSEvent*)e { [self pickAt:[self convertPoint:e.locationInWindow fromView:nil]]; }
- (void)apply {
  NSString* t = [self.hex.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
  if ([t hasPrefix:@"#"]) t = [t substringFromIndex:1];
  unsigned rgb = 0;
  if (t.length != 6 || ![[NSScanner scannerWithString:t] scanHexInt:&rgb]) { NSBeep(); return; }
  g_accent = {int((rgb >> 16) & 255), int((rgb >> 8) & 255), int(rgb & 255)};
  self.owner.settings->accent = (uint32_t(g_accent.r) << 16) | (uint32_t(g_accent.g) << 8) | uint32_t(g_accent.b);
  self.applied = YES;
  [self.owner closeTheme];
}
- (void)cancelOperation:(id)sender { g_accent = self.before; [self.owner closeTheme]; }
@end

@implementation MULaunchView
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)e { return YES; }
- (void)updateTrackingAreas {
  for (NSTrackingArea* t in self.trackingAreas) [self removeTrackingArea:t];
  [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:self.bounds options:NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect owner:self userInfo:nil]];
}
// A button: registered for clicks and drawn in the original's owner-drawn style.
- (void)button:(NSString*)name rect:(NSRect)r title:(NSString*)title primary:(BOOL)primary enabled:(BOOL)enabled {
  self.buttons[name] = [NSValue valueWithRect:r];
  const BOOL down = [self.down isEqualToString:name];
  RGB8 top, bot, txt; const RGB8* border = nullptr;
  if (primary) {
    top = acc_top(); bot = acc_bottom(); txt = on_accent();
    if (down) top = bot = acc_pressed();
    if (!enabled) { top = {0x36, 0x33, 0x40}; bot = {0x30, 0x2D, 0x3A}; txt = {0x78, 0x74, 0x84}; }
  } else {
    top = bot = down ? C_BTN_DOWN : C_BTN; border = &C_BTN_BORDER; txt = C_TEXT;
    if (!enabled) { top = bot = C_OFF_FILL; border = &C_OFF_BORDER; txt = C_OFF_TEXT; }
  }
  round_rect(r, primary ? 10 : 7, top, bot, border);
  text(title, r, primary ? f_big() : f_body(), txt, CENTER, primary ? 2 : 0);
  if (!enabled) [self.buttons removeObjectForKey:name];
}
// A row of segments (the GAME BUILD row's look): the chosen one filled in the accent.
- (void)segments:(NSString*)name x:(CGFloat)x y:(CGFloat)y w:(CGFloat)w titles:(NSArray<NSString*>*)titles selected:(NSInteger)sel {
  const CGFloat gap = 6, each = (w - gap * (titles.count - 1)) / titles.count;
  for (NSUInteger i = 0; i < titles.count; ++i) {
    const NSRect r = LR(x + i * (each + gap), y, each, 28);
    self.buttons[[NSString stringWithFormat:@"%@:%lu", name, (unsigned long)i]] = [NSValue valueWithRect:r];
    if ((NSInteger)i == sel) round_rect(r, 7, acc_top(), acc_bottom(), nullptr);
    else round_rect(r, 7, C_BTN, C_BTN, &C_BTN_BORDER);
    text(titles[i], r, f_small(), (NSInteger)i == sel ? on_accent() : C_DIM, CENTER);
  }
}
- (void)drawRect:(NSRect)dirty {
  self.buttons = [NSMutableDictionary dictionary];
  vgrad(LR(RAIL_W, 0, WIN_W - RAIL_W, WIN_H), C_CONTENT_TOP, C_CONTENT_BOT);
  [self paintRail];
  switch (self.tab) {
    case 0: [self paintPlay]; break;
    case 1: [self paintSettings]; break;
    case 2: [self paintReplays]; break;
    default: [self paintMods]; break;
  }
  if (self.notesOpen) [self paintNotes];
}
// What's new, a card over the launcher (upstream opens its changelog window).
- (void)paintNotes {
  [self.buttons removeAllObjects];
  [col({0, 0, 0}, .55) setFill]; NSRectFillUsingOperation(self.bounds, NSCompositingOperationSourceOver);
  NSArray<NSString*>* lines = whats_new_lines();
  const CGFloat w = 520, x = (WIN_W - w) / 2, h = std::min<CGFloat>(WIN_H - 60, 96 + 40 * std::max<NSUInteger>(lines.count, 1)), y = (WIN_H - h) / 2;
  round_rect(LR(x, y, w, h), 12, C_CONTENT_TOP, C_CONTENT_BOT, &C_FIELD_BORDER);
  round_rect(LR(x, y, w, 4), 2, acc_top(), acc_bottom(), nullptr);
  text(@"What's new", LR(x + 20, y + 16, w - 40, 24), f_nav(), C_TEXT);
  text(ns(std::string("Version ") + MELEE_PORT_VERSION), LR(x + 20, y + 38, w - 40, 18), f_small(), C_FAINT);
  CGFloat ly = y + 64;
  for (NSString* l in lines) {
    if (ly > y + h - 76) break;
    dot(x + 22, ly + 5, C_FAINT);   // quiet bullets: the accent stays on the strip and the selection
    text(l, LR(x + 38, ly, w - 58, 36), f_small(), C_DIM, LEFT, 0, true);
    ly += 40;
  }
  [self button:@"notesok" rect:LR(x + w - 120, y + h - 50, 100, 32) title:@"OK" primary:NO enabled:YES];
}
// ---- the rail (paint_rail): gradient, the mark on top, the page nav, Bailey under it -------------------------------
- (void)paintRail {
  vgrad(LR(0, 0, RAIL_W, WIN_H), C_RAIL_TOP, C_RAIL_BOT);
  NSArray<NSString*>* names = @[@"Play", @"Settings", @"Replay Viewer", @"Mods"];
  if (NSImage* dog = self.railPicture ?: self.bailey) {
    const CGFloat top = NAV_Y + 4 * NAV_GAP + 10, room_h = WIN_H - top - 12, room_w = RAIL_W - 36;
    const NSSize s = dog.size;
    const CGFloat k = std::min(room_w / s.width, room_h / s.height), w = s.width * k, h = s.height * k;
    const NSRect at = LR((RAIL_W - w) / 2, top + (WIN_H - top - h) / 2, w, h);
    [dog drawInRect:at fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES
              hints:@{NSImageHintInterpolation: @(NSImageInterpolationHigh)}];
    self.buttons[@"dog"] = [NSValue valueWithRect:at];   // upstream: a click on the dog chooses the rail picture
  }
  [col(C_DIVIDER) setFill]; NSRectFill(LR(RAIL_W - 1, 0, 1, WIN_H));
  if (self.mark) {
    const NSSize s = self.mark.size;
    const CGFloat k = std::min((RAIL_W - 44) / s.width, (NAV_Y - 22) / s.height), w = s.width * k, h = s.height * k;
    [self.mark drawInRect:LR((RAIL_W - w) / 2, 14, w, h) fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES
                    hints:@{NSImageHintInterpolation: @(NSImageInterpolationHigh)}];
  } else {
    text(@"MELEE", LR(0, 50, RAIL_W, 30), [NSFont systemFontOfSize:24 weight:NSFontWeightHeavy], C_TEXT, CENTER, 2);
    text(@"UNLOCKED", LR(0, 80, RAIL_W, 24), [NSFont systemFontOfSize:14 weight:NSFontWeightBold], C_DIM, CENTER, 3);
  }
  for (int i = 0; i < (int)names.count; ++i) {
    const NSRect nr = LR(12, NAV_Y + i * NAV_GAP, RAIL_W - 24, NAV_H);
    self.buttons[[NSString stringWithFormat:@"nav:%d", i]] = [NSValue valueWithRect:nr];
    if (self.tab == i) {
      round_rect(nr, 8, C_NAV_ON, C_NAV_ON, nullptr);
      round_rect(LR(12, NAV_Y + i * NAV_GAP + 8, 4, 18), 2, acc_top(), acc_bottom(), nullptr);
    } else if (self.hotNav == i) round_rect(nr, 8, C_NAV_HOT, C_NAV_HOT, nullptr);
    text(names[i], LR(nr.origin.x + 18, nr.origin.y, nr.size.width - 18, nr.size.height), f_nav(), self.tab == i ? C_TEXT : C_DIM);
  }
}
// One status row (the original's ui_fix_status rows): a dot, dim words, a faint bar, then the row's link.
- (void)row:(CGFloat)y dot:(RGB8)c words:(NSString*)words link:(NSString*)link name:(NSString*)name {
  dot(CX, y + 3, c);
  const CGFloat w = std::min(text_width(words, f_body()) + 1, CW - 15 - (link ? text_width(@"   |   ", f_body()) + text_width(link, f_body()) : 0));
  text(words, LR(CX + 15, y - 3, w, 17), f_body(), C_DIM);
  if (!link) return;
  const CGFloat bw = text_width(@"   |   ", f_body()), lw = text_width(link, f_body()) + 2;
  text(@"   |   ", LR(CX + 15 + w, y - 3, bw, 17), f_body(), C_FAINT);
  const NSRect lr = LR(CX + 15 + w + bw, y - 3, lw, 17);
  text(link, lr, f_body(), C_TEXT);
  self.buttons[name] = [NSValue valueWithRect:lr];
}
// ---- Play (paint_play) ---------------------------------------------------------------------------------------------
- (void)paintPlay {
  host::LauncherSettings& s = *self.settings;
  text(@"MELEE NTSC 1.02 DISC IMAGE", LR(CX, 32, CW, 18), f_label(), C_FAINT, LEFT, 1);
  round_rect(LR(CX, 58, 380, 34), 7, C_FIELD, C_FIELD, &C_FIELD_BORDER);
  NSString* iso = s.iso.empty() ? @"" : ns(s.iso);
  text(iso.length ? iso.lastPathComponent : @"No disc image chosen", LR(CX + 10, 66, 360, 18), f_body(), iso.length ? C_TEXT : C_FAINT);
  if (iso.length) self.toolTip = iso;
  [self button:@"browse" rect:LR(602, 58, 96, 34) title:@"Browse..." primary:NO enabled:YES];

  text(@"GAME BUILD", LR(CX, 102, 130, 19), f_label(), C_FAINT, LEFT, 1);
  const NSInteger on = s.vanilla ? 1 : 0;
  const CGFloat label_w = std::max<CGFloat>(84, text_width(@"GAME BUILD", f_label()) + 1.0 * 9 + 12);   // a longer translation pushes the row right
  const CGFloat left = CX + label_w, total = CW - label_w, gap = 6, w = (total - gap) / 2;
  NSArray<NSString*>* names = @[@"Source Port", @"Vanilla"];
  for (int i = 0; i < 2; ++i) {
    const NSRect r = LR(left + i * (w + gap), 98, w, 28);
    RGB8 top = C_BTN, bot = C_BTN, txt = C_DIM; const RGB8* border = &C_BTN_BORDER;
    if (i == on) { top = acc_top(); bot = acc_bottom(); border = nullptr; txt = on_accent(); }
    round_rect(r, 7, top, bot, border);
    text(names[i], r, f_small(), txt, CENTER);
    self.buttons[[NSString stringWithFormat:@"build:%d", i]] = [NSValue valueWithRect:r];
  }
  [self button:@"play" rect:LR(CX, 134, CW, 62) title:@"PLAY" primary:YES enabled:!s.iso.empty()];

  const CGFloat pitch = 24; CGFloat y = 210;
  NSString* version = [NSString stringWithFormat:@"Version %s (melee-unlocked 1.0.5 engine)%@", MELEE_PORT_VERSION,
                                                 self.updateText.length ? [@" · " stringByAppendingString:self.updateText] : @""];
  [self row:y dot:self.updateAvailable ? C_WARN : C_OK words:version link:self.updateAvailable ? @"How to update" : @"What's new"
       name:self.updateAvailable ? @"howtoupdate" : @"whatsnew"]; y += pitch;
  if (s.vanilla) {
    text(@"Vanilla: the game as the disc plays it, with melee-unlocked's engine (no codes, nothing unlocked)", LR(CX, y - 3, CW, 17), f_small(), C_DIM);
  } else if (!s.account_name.empty()) {
    [self row:y dot:C_OK words:ns("Slippi: signed in as " + s.account_name + (s.account_code.empty() ? "" : " (" + s.account_code + ")")) link:nil name:@""];
  } else {
    [self row:y dot:C_WARN words:@"Slippi: not signed in. Online play needs a Slippi account." link:nil name:@""];
  }
  y += pitch;
  [self row:y dot:C_FAINT words:@"Mods: none active" link:@"Open Mods folder" name:@"modsfolder"]; y += pitch;
  if (!s.iso.empty()) {   // the disc check (melee-unlocked's launcher checks the disc before a start): its header, read once per path
    const std::string check = disc_check(s.iso);
    const bool ok = check.rfind("Disc: Melee NTSC 1.02", 0) == 0;
    [self row:y dot:ok ? C_OK : C_WARN words:ns(check) link:nil name:@""]; y += pitch;
  }
  [col(C_SEP) setFill]; NSRectFill(LR(CX, y + 1, CW, 1));
  const CGFloat note_y = y + 10;
  if (s.iso.empty())
    text(@"No Melee disc chosen yet: press Browse and pick your Melee NTSC 1.02 ISO.", LR(CX, note_y, CW, 32), f_small(), C_WARN, LEFT, 0, true);
  else {
    const host::KeyboardMap& k = host::keyboard_map();
    NSString* hint = [NSString stringWithFormat:@"F1 or Esc opens the menu in game. GameCube adapters work automatically.\nKeyboard: stick %s %s %s %s, A %s, B %s, Start %s.",
      host::key_name(k.key[host::KB_STICK_UP]).c_str(), host::key_name(k.key[host::KB_STICK_LEFT]).c_str(), host::key_name(k.key[host::KB_STICK_DOWN]).c_str(),
      host::key_name(k.key[host::KB_STICK_RIGHT]).c_str(), host::key_name(k.key[host::GC_CTL_A]).c_str(), host::key_name(k.key[host::GC_CTL_B]).c_str(),
      host::key_name(k.key[host::GC_CTL_START]).c_str()];
    text(hint, LR(CX, note_y, CW, 32), f_small(), C_FAINT, LEFT, 0, true);
  }
  const CGFloat by = note_y + 40;
  [self button:@"logs" rect:LR(CX, by, 150, 32) title:@"Show game logs" primary:NO enabled:YES];
  [self button:@"crashreport" rect:LR(CX + 158, by, 160, 32) title:@"Save crash report" primary:NO enabled:YES];
  // The Language button (upstream's Play page): each language named in itself.
  NSString* native = @"English";
  for (const Language& l : kLanguages) if (g_lang == l.code) native = [NSString stringWithUTF8String:l.native];
  [self button:@"language" rect:LR(CX + 326, by, 120, 32) title:native primary:NO enabled:YES];
  if (!self.error.empty()) text(ns(self.error), LR(CX, by + 38, CW, 30), f_small(), C_BAD, LEFT, 0, true);
  else if (!self.crashNote.empty()) text(ns(self.crashNote), LR(CX, by + 38, CW, 30), f_small(), C_DIM, LEFT, 0, true);
  // The launcher colour button: the compact wheel (theme_wheel, compact).
  const NSRect wr = LR(666, by, 32, 32);
  self.buttons[@"theme"] = [NSValue valueWithRect:wr];
  for (int yy = -13; yy <= 13; ++yy) for (int xx = -13; xx <= 13; ++xx) {
    const double d = std::sqrt(double(xx * xx + yy * yy));
    if (d > 13.5) continue;
    const double hue = std::fmod(std::atan2(double(-yy), double(xx)) * 180 / M_PI + 360, 360.0);
    [col(hsv(hue, std::max(.42, d / 13), 1), std::clamp(13.5 - d, 0.0, 1.0)) setFill];
    NSRectFillUsingOperation(LR(wr.origin.x + 16 + xx, wr.origin.y + 16 + yy, 1, 1), NSCompositingOperationSourceOver);
  }
}
// ---- Settings: the dashboard's choices, in the Game Build row's style -----------------------------------------------
- (void)paintSettings {
  host::LauncherSettings& s = *self.settings;
  CGFloat y = 32;
  auto heading = [&](NSString* t) { text(t, LR(CX, y, CW, 18), f_label(), C_FAINT, LEFT, 1); y += 24; };
  auto line = [&](NSString* label, NSString* name, NSArray<NSString*>* titles, NSInteger sel) {
    text(label, LR(CX, y, 170, 28), f_body(), C_TEXT);
    [self segments:name x:CX + 176 y:y w:CW - 176 titles:titles selected:sel];
    y += 36;
  };
  heading(@"DISPLAY");
  const int scales[] = {0, 1, 2, 3, 4}; NSInteger scale = 0;
  for (int i = 0; i < 5; ++i) if (scales[i] == s.scale) scale = i;
  line(@"Internal resolution", @"scale", @[@"Auto", @"1×", @"2×", @"3×", @"4×"], scale);
  line(@"Start full screen", @"fullscreen", @[@"Off", @"On"], s.fullscreen ? 1 : 0);
  line(@"Display sync", @"vsync", @[@"Off", @"On"], s.vsync ? 1 : 0);
  line(@"Widescreen (16:9)", @"widescreen", @[@"Off", @"On"], s.widescreen ? 1 : 0);
  y += 6; heading(@"SOUND");
  const int vols[] = {0, 25, 50, 70, 100}; NSInteger vol = 3;
  for (int i = 0; i < 5; ++i) if (vols[i] == s.volume) vol = i;
  line(@"Volume", @"volume", @[@"0", @"25", @"50", @"70", @"100"], vol);
  y += 6; heading(@"ONLINE");
  line(@"Slippi Online", @"online", @[@"Off", @"On"], s.online ? 1 : 0);
  line(@"Input delay (frames)", @"delay", @[@"Auto", @"1", @"2", @"3", @"4"], s.online_delay <= 0 ? 0 : std::clamp(s.online_delay, 1, 4));
  [self button:@"allsettings" rect:LR(CX, y + 4, 150, 32) title:@"All settings…" primary:NO enabled:YES];
  text(@"Every setting, grouped as in game. F1 or Esc opens them in game too, with the controller setup.", LR(CX + 162, y + 4, CW - 162, 32), f_small(), C_FAINT, LEFT, 0, true);
}
// ---- Replay Viewer: the newest games as cards ----------------------------------------------------------------------
// The picked replay's stats (upstream launcher_replay_stats.inl's rows, two columns) and its session trace graph
// (launcher_trace_view.cpp's overview: frame time, work, waits, rollbacks, marks across the session), then Move to Trash.
- (void)paintCard:(const host::GameRow&)row y:(CGFloat)y {
  const CGFloat h = WIN_H - 58 - y;
  round_rect(LR(CX, y, CW, h), 10, C_FIELD, C_FIELD, &C_FIELD_BORDER);
  [self button:@"trash" rect:LR(CX + CW - 120, y + 8, 110, 24) title:@"Move to Trash" primary:NO enabled:!row.path.empty()];
  [self button:@"unpick" rect:LR(CX + CW - 190, y + 8, 64, 24) title:@"Close" primary:NO enabled:YES];
  mu_replay_stats::Card* c = self.card.get();
  if (self.cardLoading || !c) { text(@"Reading the replay...", LR(CX + 14, y + 10, 300, 20), f_small(), C_DIM); return; }
  if (!c->ok) { text(@"This replay's stats could not be read.", LR(CX + 14, y + 10, 300, 20), f_small(), C_DIM); return; }
  text(ns(c->date), LR(CX + 14, y + 8, CW - 190, 22), f_small(), C_DIM);
  static const int kShown[] = {1, 2, 3, 4, 6, 7, 8, 11};   // Kills .. Inputs per minute, Ledge grabs (Players is the list's own line)
  const CGFloat col_w = (CW - 28) / 2;
  for (int k = 0; k < 8; ++k) {
    const int r = kShown[k];
    if (r >= (int)c->rows.size()) continue;
    const CGFloat x = CX + 14 + (k % 2) * col_w, ry = y + 36 + (k / 2) * 19;
    text(ns(c->rows[size_t(r)].first), LR(x, ry, 118, 17), f_label(), C_FAINT);
    text(ns(c->rows[size_t(r)].second), LR(x + 120, ry, col_w - 124, 17), f_small(), C_TEXT);
  }
  const CGFloat gy = y + 36 + 4 * 19 + 6, gh = std::max<CGFloat>(20, h - (gy - y) - 24);
  if (!c->trace || c->bins.empty()) {
    text(@"No session trace beside this replay (online matches with a mark or a long wait keep one).", LR(CX + 14, gy, CW - 28, 18), f_small(), C_FAINT);
    return;
  }
  round_rect(LR(CX + 14, gy, CW - 28, gh), 5, C_CONTENT_BOT, C_CONTENT_BOT, &C_FIELD_BORDER);
  float top = 33.4f;
  for (const auto& b : c->bins) top = std::max(top, std::max(b[0], b[1]));
  const CGFloat gx0 = CX + 16, gw = CW - 32, bw = gw / CGFloat(c->bins.size());
  for (size_t i = 0; i < c->bins.size(); ++i) {
    const auto& b = c->bins[i];
    const CGFloat x = gx0 + bw * CGFloat(i), w = std::max<CGFloat>(bw, 0.6);
    const CGFloat hi = std::min<CGFloat>(gh - 4, (gh - 4) * b[0] / top), wk = std::min<CGFloat>(gh - 4, (gh - 4) * b[1] / top);
    [col(b[2] > 0 ? C_WARN : mix(g_accent, C_FIELD, .35), .9) setFill]; NSRectFillUsingOperation(LR(x, gy + gh - 2 - hi, w, hi), NSCompositingOperationSourceOver);
    [col(C_OK, .8) setFill]; NSRectFillUsingOperation(LR(x, gy + gh - 2 - wk, w, wk), NSCompositingOperationSourceOver);
    if (b[3] > 0) { [col(C_BAD) setFill]; NSRectFill(LR(x, gy + 2, w, 3)); }
    if (int(b[4]) & 0x78) { [col(C_TEXT) setFill]; NSRectFill(LR(x, gy, std::max<CGFloat>(w, 0.8), 2)); }
  }
  const CGFloat line = gy + gh - 2 - (gh - 4) * 16.7f / top;
  [col(C_TEXT, .25) setFill]; NSRectFill(LR(gx0, line, gw, 1));
  text(ns("Session trace: " + c->trace_summary + "   (green work, accent frame time, amber waits, red rollbacks, white marks)"),
       LR(CX + 14, gy + gh + 2, CW - 28, 16), f_small(), C_FAINT);
}
- (void)pickReplay:(NSInteger)index {
  const std::vector<host::GameRow> rows = self.dashboard.rows();
  if (index < 0 || index >= (NSInteger)rows.size() || rows[size_t(index)].path.empty()) return;
  self.picked = index; self.card = nullptr; self.cardLoading = YES;
  const std::string path = rows[size_t(index)].path;
  __weak MULaunchView* weak = self;
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    auto card = std::make_shared<mu_replay_stats::Card>(mu_replay_stats::read(path));
    host::log("launcher: replay stats of %s: %s, %zu rows, trace %s", path.c_str(), card->ok ? "read" : "not read", card->rows.size(), card->trace ? card->trace_summary.c_str() : "none");
    dispatch_async(dispatch_get_main_queue(), ^{ MULaunchView* v = weak; if (v && v.picked == index) { v.card = card; v.cardLoading = NO; [v setNeedsDisplay:YES]; } });
  });
}
- (void)reloadReplays {
  host::LauncherSettings& settings = *self.settings;
  if (settings.replay_dir.empty()) return;
  const std::string dir = settings.replay_dir, code = settings.account_code, name = settings.account_name;
  __weak MULaunchView* weak_view = self;
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    host::Dashboard d;
    d.code = code; d.name = name;   // whose games: WIN and LOSS from this player's side
    host::dashboard_load_games(dir, d, 8);
    dispatch_async(dispatch_get_main_queue(), ^{
      MULaunchView* v = weak_view;
      if (!v) return;
      v.dashboard = d; [v setNeedsDisplay:YES];
      static bool first = true;
      if (!first) return;
      first = false;
      if (const char* n = std::getenv("MELEE_PLAY_REPLAY")) [v activate:[NSString stringWithFormat:@"watch:%d", std::atoi(n)]];   // tests: Watch on recent game n
      if (const char* n = std::getenv("MELEE_LAUNCHER_PICK")) [v pickReplay:std::atoi(n)];   // screenshots: recent game n's stats card
    });
  });
}
- (void)paintReplays {
  text(@"REPLAYS", LR(CX, 32, CW, 18), f_label(), C_FAINT, LEFT, 1);
  const std::vector<host::GameRow> rows = self.dashboard.rows();
  CGFloat y = 58;
  const BOOL can_watch = !self.settings->iso.empty();
  if (rows.empty()) text(@"No replays yet. Every game you play is saved here as a .slp file.", LR(CX, y, CW, 20), f_body(), C_DIM);
  const BOOL picking = self.picked >= 0 && self.picked < (NSInteger)rows.size();
  const size_t shown = picking ? 3 : self.liveWatch ? 7 : 8;
  const size_t first = picking ? size_t(std::max<NSInteger>(0, std::min<NSInteger>(self.picked - 1, (NSInteger)rows.size() - 3))) : 0;
  for (size_t i = first; i < rows.size() && i < first + shown; ++i) {
    const host::GameRow& r = rows[i];
    self.buttons[[NSString stringWithFormat:@"pick:%zu", i]] = [NSValue valueWithRect:LR(CX, y, CW - 126, 38)];   // the card opens its stats
    if (picking && (NSInteger)i == self.picked) { round_rect(LR(CX, y, CW, 38), 8, C_NAV_ON, C_NAV_ON, nullptr); round_rect(LR(CX, y + 9, 3, 20), 1.5, acc_top(), acc_bottom(), nullptr); }
    else
    round_rect(LR(CX, y, CW, 38), 8, C_FIELD, C_FIELD, &C_FIELD_BORDER);
    text(ns(r.title), LR(CX + 12, y + 3, CW - 190, 18), f_body(), C_TEXT);
    text(ns(r.subtitle), LR(CX + 12, y + 19, CW - 190, 16), f_small(), C_FAINT);
    text(ns(r.result), LR(CX + CW - 178, y, 50, 38), f_small(), r.win ? C_OK : r.loss ? C_BAD : C_DIM, RIGHT);
    if (!r.path.empty()) {
      [self button:[NSString stringWithFormat:@"watch:%zu", i] rect:LR(CX + CW - 120, y + 6, 54, 26) title:@"Watch" primary:NO enabled:can_watch];
      [self button:[NSString stringWithFormat:@"show:%zu", i] rect:LR(CX + CW - 60, y + 6, 54, 26) title:@"Show" primary:NO enabled:YES];
    }
    y += 44;
  }
  if (self.replayNote.length) text(self.replayNote, LR(CX, WIN_H - 84, CW, 30), f_small(), C_WARN, LEFT, 0, true);
  if (picking) [self paintCard:rows[size_t(self.picked)] y:58 + std::min<size_t>(3, rows.size()) * 44];
  else if (self.liveWatch) text(self.liveStatus ?: @"", LR(CX, 58 + 7 * 44 + 6, CW, 34), f_small(), C_DIM, LEFT, 0, true);
  [self button:@"replaysfolder" rect:LR(CX, WIN_H - 50, 166, 32) title:@"Open Replays folder" primary:NO enabled:YES];
  [self button:@"openreplay" rect:LR(CX + 174, WIN_H - 50, 140, 32) title:@"Open a replay..." primary:NO enabled:can_watch];
  [self button:@"watchlive" rect:LR(CX + 322, WIN_H - 50, CW - 322, 32) title:self.liveWatch ? @"Stop watching live" : @"Watch live"
       primary:NO enabled:can_watch];
  if (!can_watch) text(@"Watching a replay needs your disc: choose it on the Play page first.", LR(CX, WIN_H - 82, CW, 28), f_small(), C_FAINT, LEFT, 0, true);
}
// The Slippi Launcher's spectate folder: its own setting (spectateSlpPath), else <its replay folder>/Spectate, else its default
// ~/Documents/Slippi/Spectate (as melee-unlocked's live_spectate_folder reads it on Windows).
- (NSString*)spectateFolder {
  if (const char* dir = std::getenv("MELEE_SPECTATE_DIR")) return ns(dir);   // tests: a folder of their own
  NSString* settings = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/Slippi Launcher/Settings"];
  if (NSData* data = [NSData dataWithContentsOfFile:settings]) {
    id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    NSDictionary* d = [json isKindOfClass:NSDictionary.class] ? json : nil;
    if ([d[@"settings"] isKindOfClass:NSDictionary.class]) d = d[@"settings"];
    if ([d[@"spectateSlpPath"] isKindOfClass:NSString.class] && [d[@"spectateSlpPath"] length]) return d[@"spectateSlpPath"];
    if ([d[@"rootSlpPath"] isKindOfClass:NSString.class] && [d[@"rootSlpPath"] length]) return [d[@"rootSlpPath"] stringByAppendingPathComponent:@"Spectate"];
  }
  return [NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Slippi/Spectate"];
}
- (NSArray<NSString*>*)liveFiles:(NSString*)folder {
  NSMutableArray<NSString*>* out = [NSMutableArray array];
  for (NSString* name in [NSFileManager.defaultManager contentsOfDirectoryAtPath:folder error:nil] ?: @[])
    if ([name.pathExtension.lowercaseString isEqualToString:@"slp"]) [out addObject:[folder stringByAppendingPathComponent:name]];
  return out;
}
- (void)toggleLive {
  if (self.liveWatch) {
    self.liveWatch = NO; [self.liveTimer invalidate]; self.liveTimer = nil;
    host::log("launcher: watch live off");
    return;
  }
  NSString* folder = [self spectateFolder];
  self.liveWatch = YES;
  self.liveSeen = [NSMutableSet setWithArray:[self liveFiles:folder]];   // games already there are not live
  self.liveStatus = [NSString stringWithFormat:@"Watching %@ for live games: start spectating in the Slippi Launcher and each new game opens here.",
                                               [folder stringByAbbreviatingWithTildeInPath]];
  host::log("launcher: watch live on (%s, %lu files already there)", folder.UTF8String, (unsigned long)self.liveSeen.count);
  __weak MULaunchView* weak = self;
  self.liveTimer = [NSTimer timerWithTimeInterval:1.0 repeats:YES block:^(NSTimer* t) {
    MULaunchView* v = weak;
    if (!v || !v.liveWatch) { [t invalidate]; return; }
    if (v.watchTask && v.watchTask.isRunning) return;   // the game being watched first; the next one opens after it
    NSString* newest = nil; NSDate* newest_at = nil;
    for (NSString* path in [v liveFiles:folder]) {
      if ([v.liveSeen containsObject:path]) continue;
      NSDate* at = [NSFileManager.defaultManager attributesOfItemAtPath:path error:nil].fileModificationDate;
      if (!newest || [at compare:newest_at] == NSOrderedDescending) { newest = path; newest_at = at; }
    }
    if (!newest) return;
    [v.liveSeen addObject:newest];
    host::log("launcher: watch live opens %s", newest.UTF8String);
    v.liveStatus = [NSString stringWithFormat:@"Watching live: %@", newest.lastPathComponent];
    [v setNeedsDisplay:YES];
    [v watch:newest];
  }];
  [NSRunLoop.mainRunLoop addTimer:self.liveTimer forMode:NSRunLoopCommonModes];   // the launcher runs modally
}
- (void)checkForUpdate {
  if (const char* off = std::getenv("MELEE_UPDATE_CHECK"); off && *off == '0') return;
  NSString* commit = [NSBundle.mainBundle objectForInfoDictionaryKey:@"MUBuildCommit"];
  if (![commit isKindOfClass:NSString.class] || commit.length < 7) { host::log("launcher: update check skipped (no build commit in Info.plist)"); return; }
  // GitHub's compare of the built commit with the branch: "ahead_by" newer commits on the branch. MELEE_UPDATE_URL: tests.
  NSString* base = std::getenv("MELEE_UPDATE_URL") ? ns(std::getenv("MELEE_UPDATE_URL")) : @"https://api.github.com/repos/rkoh46/melee-unlocked-macOS/compare/";
  NSURL* url = [NSURL URLWithString:[NSString stringWithFormat:@"%@%@...melee-unlocked-macos", base, commit]];
  NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url cachePolicy:NSURLRequestReloadIgnoringLocalCacheData timeoutInterval:10];
  [request setValue:@"melee-unlocked-macOS" forHTTPHeaderField:@"User-Agent"];
  [request setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
  __weak MULaunchView* weak = self;
  [[NSURLSession.sharedSession dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
    const NSInteger status = [response isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse*)response).statusCode : 0;
    NSDictionary* json = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    if (error || status != 200 || ![json isKindOfClass:NSDictionary.class]) {
      host::log("launcher: update check failed (%s)", error ? error.localizedDescription.UTF8String : [NSString stringWithFormat:@"HTTP %ld", (long)status].UTF8String);
      return;
    }
    const long ahead = [json[@"ahead_by"] respondsToSelector:@selector(longValue)] ? [json[@"ahead_by"] longValue] : 0;
    NSString* state = [json[@"status"] isKindOfClass:NSString.class] ? json[@"status"] : @"";
    host::log("launcher: update check: built from %s, the branch is %s (%ld newer)", commit.UTF8String, state.UTF8String, ahead);
    dispatch_async(dispatch_get_main_queue(), ^{
      MULaunchView* v = weak;
      if (!v) return;
      v.updateAvailable = ahead > 0;
      v.updateText = ahead > 0 ? [NSString stringWithFormat:@"%ld newer change%@ on GitHub", ahead, ahead == 1 ? @"" : @"s"] : @"up to date";
      [v setNeedsDisplay:YES];
    });
  }] resume];
}
- (void)openReplay {
  NSOpenPanel* panel = [NSOpenPanel openPanel];
  panel.message = @"Choose a Slippi replay to watch";
  panel.allowedFileTypes = @[@"slp", @"slpz"];
  panel.directoryURL = [NSURL fileURLWithPath:ns(self.settings->replay_dir)];
  if ([panel runModal] == NSModalResponseOK) [self watch:panel.URL.path];
}
// ---- Mods: melee-unlocked's catalog, installed by this app's own installer (download, SHA-256, archive, xdelta3) ----
- (void)paintMods {
  host::LauncherSettings& st = *self.settings;
  const std::string mods_dir = home() + "/Library/Application Support/Dashdance/melee-unlocked/Mods";
  text(@"MODS", LR(CX, 32, CW, 18), f_label(), C_FAINT, LEFT, 1);
  mu_mods::configure(mods_dir, st.iso);
  const std::vector<mu_mods::Row> list = mu_mods::rows();
  if (list.empty()) text(ns("The catalog could not be read: " + mu_mods::catalog_error()), LR(CX, 58, CW, 40), f_small(), C_BAD, LEFT, 0, true);
  CGFloat y = 56;
  for (size_t i = 0; i < list.size() && i < 10; ++i) {
    const mu_mods::Row& r = list[i];
    round_rect(LR(CX, y, CW, 34), 7, C_FIELD, C_FIELD, &C_FIELD_BORDER);
    text(ns(r.name), LR(CX + 10, y + 2, 230, 17), f_body(), C_TEXT);
    // A mod melee-unlocked offers from its page only (no licence or permission in its catalog to fetch it for the player):
    // Download page opens that page, Choose File takes the archive from anywhere, and a download landing in ~/Downloads turns
    // the row into Install by itself. Every file is checked against the catalog's SHA-256 before anything is built.
    const bool by_hand = !r.automatic && (r.state == 0 || r.state == 2);
    NSString* found = r.found.empty() ? nil : ns(r.found);
    NSString* where = !found ? nil : [found.stringByDeletingLastPathComponent.lastPathComponent isEqualToString:@"Downloads"] &&
                                         ![found containsString:@"/melee-unlocked/Mods/"] ? @"Downloads" : @"the Mods folder";
    NSString* sub = r.state == 3 || (!r.line.empty() && r.failed) ? ns(r.line) : r.state == 1 ? ns("Installed · " + r.version)
                  : r.state == 2 && !by_hand ? ns("Update available · " + r.version) : r.automatic ? ns("Version " + r.version)
                  : found ? [NSString stringWithFormat:@"Ready to install from %@", where]
                  : @"Not installed · download it from its page";
    if (r.state == 3 && r.percent >= 0) sub = [NSString stringWithFormat:@"%@ (%d%%)", sub, r.percent];
    if (r.state == 1) {   // its disc's costume skins, as the last game start counted them (mods-cache/disc-skins.txt)
      const size_t skins = disc_skins(r.id);
      if (skins) sub = [NSString stringWithFormat:@"%@ · %zu skin%s (Settings > Mods > Choose skins in game)", tx(sub), skins, skins == 1 ? "" : "s"];
    }
    const BOOL two = by_hand && r.state != 3;
    text(sub, LR(CX + 10, y + 17, CW - (two ? 222 : 120), 15), f_small(), r.failed ? C_BAD : r.state == 1 ? C_OK : found && two ? C_TEXT : C_FAINT);
    NSString* name = [NSString stringWithFormat:@"mod:%zu", i];
    if (r.state == 3) [self button:@"modcancel" rect:LR(CX + CW - 96, y + 4, 88, 26) title:@"Cancel" primary:NO enabled:YES];
    else if (r.state == 1) {
      const bool chosen = mod_profile_next() == r.id;
      [self button:[@"use-" stringByAppendingString:name] rect:LR(CX + CW - 190, y + 4, 88, 26) title:chosen ? @"In use" : @"Use" primary:chosen enabled:YES];
      [self button:[@"remove-" stringByAppendingString:name] rect:LR(CX + CW - 96, y + 4, 88, 26) title:@"Remove" primary:NO enabled:!mu_mods::busy()];
    }
    else if (two) {
      [self button:[@"choose-" stringByAppendingString:name] rect:LR(CX + CW - 208, y + 4, 100, 26) title:@"Choose File…" primary:NO enabled:!mu_mods::busy()];
      if (found) [self button:[@"install-" stringByAppendingString:name] rect:LR(CX + CW - 104, y + 4, 96, 26) title:r.state == 2 ? @"Update" : @"Install" primary:NO
                      enabled:!mu_mods::busy()];
      else [self button:[@"page-" stringByAppendingString:name] rect:LR(CX + CW - 104, y + 4, 96, 26) title:@"Download page" primary:NO enabled:!r.page.empty()];
    }
    else [self button:[@"install-" stringByAppendingString:name] rect:LR(CX + CW - 96, y + 4, 88, 26) title:r.state == 2 ? @"Update" : @"Install" primary:NO
              enabled:!mu_mods::busy()];
    y += 37;
  }
  [self button:@"modsfolder" rect:LR(CX, WIN_H - 46, 150, 30) title:@"Open Mods folder" primary:NO enabled:YES];
  bool any_by_hand = false;
  for (const auto& r : list) any_by_hand = any_by_hand || (!r.automatic && r.state == 0);
  NSString* note = !self.modsMessage.empty() ? ns(self.modsMessage)
                 : any_by_hand ? @"A mod with a download page: download it there and it shows up here from Downloads, checked against its official hash."
                 : @"Mods install as discs built from your own disc. Use one and it loads at the next offline start (Online off).";
  text(note, LR(CX + 160, WIN_H - 48, CW - 160, 34), f_small(), self.modsMessage.empty() ? C_FAINT : C_WARN, LEFT, 0, true);
  // While the page is up it repaints itself twice a second: a job's progress, and a download that has just landed in
  // ~/Downloads (the row turns into Install without a restart, as melee-unlocked's page checks every two seconds).
  if (!self.modsTimer) {
    __weak MULaunchView* weak = self;
    self.modsTimer = [NSTimer timerWithTimeInterval:0.5 repeats:YES block:^(NSTimer* t) {
      MULaunchView* v = weak;
      if (!v || v.tab != 3) { [t invalidate]; if (v) v.modsTimer = nil; return; }
      [v setNeedsDisplay:YES];
    }];
    [NSRunLoop.mainRunLoop addTimer:self.modsTimer forMode:NSRunLoopCommonModes];   // the launcher runs modally
  }
}
// ---- input ------------------------------------------------------------------------------------------------------------
- (NSString*)buttonAt:(NSPoint)p {
  if (self.notesOpen) { NSValue* ok = self.buttons[@"notesok"]; return ok && NSPointInRect(p, ok.rectValue) ? @"notesok" : nil; }
  for (NSString* k in self.buttons) if (![k hasPrefix:@"pick:"] && NSPointInRect(p, self.buttons[k].rectValue)) return k;
  for (NSString* k in self.buttons) if (NSPointInRect(p, self.buttons[k].rectValue)) return k;
  return nil;
}
- (void)mouseMoved:(NSEvent*)e {
  const NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
  int nav = -1;
  for (int i = 0; i < 4 && !self.notesOpen; ++i) if (NSPointInRect(p, LR(12, NAV_Y + i * NAV_GAP, RAIL_W - 24, NAV_H))) nav = i;
  if (nav != self.hotNav) { self.hotNav = nav; [self setNeedsDisplay:YES]; }
  [([self buttonAt:p] ? NSCursor.pointingHandCursor : NSCursor.arrowCursor) set];
}
- (void)mouseDown:(NSEvent*)e { self.down = [self buttonAt:[self convertPoint:e.locationInWindow fromView:nil]]; [self setNeedsDisplay:YES]; }
- (void)mouseUp:(NSEvent*)e {
  NSString* hit = [self buttonAt:[self convertPoint:e.locationInWindow fromView:nil]];
  NSString* pressed = self.down; self.down = nil; [self setNeedsDisplay:YES];
  if (hit && [hit isEqualToString:pressed]) [self activate:hit];
}
- (void)keyDown:(NSEvent*)e {
  if (self.notesOpen && (e.keyCode == 36 || e.keyCode == 53)) [self activate:@"notesok"];   // Return or Esc closes What's new
  else if (e.keyCode == 36 && !self.settings->iso.empty() && self.tab == 0) [self activate:@"play"];   // Return plays
  else [super keyDown:e];
}
- (BOOL)acceptsFirstResponder { return YES; }
- (void)reveal:(const std::string&)path create:(BOOL)create {
  if (create) [NSFileManager.defaultManager createDirectoryAtPath:ns(path) withIntermediateDirectories:YES attributes:nil error:nil];
  [NSWorkspace.sharedWorkspace openURL:[NSURL fileURLWithPath:ns(path)]];
}
- (void)activate:(NSString*)name {
  host::LauncherSettings& s = *self.settings;
  NSArray<NSString*>* parts = [name componentsSeparatedByString:@":"];
  NSString* key = parts[0]; const int index = parts.count > 1 ? parts[1].intValue : 0;
  const std::string support = home() + "/Library/Application Support/Dashdance";
  if ([key isEqualToString:@"nav"]) { self.tab = index; self.toolTip = nil; }
  else if ([key isEqualToString:@"build"]) s.vanilla = index == 1;
  else if ([key isEqualToString:@"play"]) { [NSApp stopModalWithCode:NSModalResponseOK]; return; }
  else if ([key isEqualToString:@"browse"]) {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.message = @"Choose your Super Smash Bros. Melee NTSC 1.02 disc image";
    panel.allowedFileTypes = @[@"iso", @"gcm", @"ciso", @"rvz"];
    if ([panel runModal] == NSModalResponseOK) { s.iso = panel.URL.path.UTF8String; self.error.clear(); }
  }
  else if ([key isEqualToString:@"modsfolder"]) [self reveal:support + "/melee-unlocked/Mods" create:YES];
  else if ([key isEqualToString:@"logs"]) [self reveal:support + "/melee-unlocked" create:YES];
  else if ([key isEqualToString:@"crashreport"]) {   // scrubbed by melee-unlocked's rules, saved here, never sent
    std::string where, why;
    if (mu_crash::save(support, s.replay_dir, &where, &why)) {
      self.crashNote = "Saved (stays on this Mac)";
      [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[[NSURL fileURLWithPath:ns(where)]]];
    } else self.crashNote = why;
  }
  else if ([key isEqualToString:@"replaysfolder"]) [self reveal:s.replay_dir create:YES];
  else if ([key isEqualToString:@"openreplay"]) [self openReplay];
  else if ([key isEqualToString:@"howtoupdate"]) [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://github.com/rkoh46/melee-unlocked-macOS#install"]];
  else if ([key isEqualToString:@"watchlive"]) [self toggleLive];
  else if ([key isEqualToString:@"whatsnew"]) self.notesOpen = YES;
  else if ([key isEqualToString:@"notesok"]) self.notesOpen = NO;
  else if ([key isEqualToString:@"pick"]) [self pickReplay:index];
  else if ([key isEqualToString:@"unpick"]) { self.picked = -1; self.card = nullptr; }
  else if ([key isEqualToString:@"trash"]) {   // upstream: delete to the Recycle Bin; here the Finder's Trash (Put Back works)
    const std::vector<host::GameRow> rows = self.dashboard.rows();
    if (self.picked >= 0 && self.picked < (NSInteger)rows.size()) {
      NSURL* url = [NSURL fileURLWithPath:ns(rows[size_t(self.picked)].path)];
      [NSWorkspace.sharedWorkspace recycleURLs:@[url] completionHandler:^(NSDictionary*, NSError* error) {
        host::log("launcher: %s moved to the Trash%s", url.path.UTF8String, error ? " (failed)" : "");
        if (!error) dispatch_async(dispatch_get_main_queue(), ^{ [self reloadReplays]; });
      }];
      self.picked = -1; self.card = nullptr;
    }
  }
  else if ([key isEqualToString:@"allsettings"]) {   // upstream's rail Settings opens its settings panel as a window
    host::mu_settings_window_run(s, 0, nullptr);
    apply_mu_accent(s);
  }
  else if ([key isEqualToString:@"language"]) [self chooseLanguage];
  else if ([key isEqualToString:@"dog"]) [self chooseRailPicture];
  else if ([key isEqualToString:@"show"] || [key isEqualToString:@"watch"]) {
    const std::vector<host::GameRow> rows = self.dashboard.rows();
    if (index < 0 || index >= (int)rows.size()) return;
    NSString* path = ns(rows[index].path);
    if ([key isEqualToString:@"show"]) [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[[NSURL fileURLWithPath:path]]];
    else [self watch:path];
  }
  else if ([key isEqualToString:@"theme"]) [self openTheme];
  else if ([key hasPrefix:@"install-mod"] || [key hasPrefix:@"remove-mod"]) {
    const std::vector<mu_mods::Row> list = mu_mods::rows();
    if (index >= 0 && index < (int)list.size()) {
      std::string why;
      const bool ok = [key hasPrefix:@"install"] ? mu_mods::install(list[index].id, &why) : mu_mods::remove(list[index].id, &why);
      self.modsMessage = ok ? std::string() : why;
    }
  }
  else if ([key hasPrefix:@"page-mod"]) {   // the mod's official download page, in the browser
    const std::vector<mu_mods::Row> list = mu_mods::rows();
    if (index >= 0 && index < (int)list.size() && !list[index].page.empty()) {
      host::log("launcher: %s: opening its download page %s", list[index].id.c_str(), list[index].page.c_str());
      [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:ns(list[index].page)]];
    }
  }
  else if ([key hasPrefix:@"choose-mod"]) {   // upstream's "Add file...": the downloaded archive or .xdelta, wherever it is
    const std::vector<mu_mods::Row> list = mu_mods::rows();
    if (index >= 0 && index < (int)list.size()) {
      NSOpenPanel* panel = [NSOpenPanel openPanel];
      panel.message = [NSString stringWithFormat:@"Choose the download of %@", ns(list[index].name)];
      panel.allowedFileTypes = @[@"zip", @"7z", @"xdelta", @"gci"];
      panel.directoryURL = [NSURL fileURLWithPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Downloads"]];
      if ([panel runModal] == NSModalResponseOK) {
        std::string why;
        self.modsMessage = mu_mods::install_file(list[index].id, panel.URL.path.UTF8String, &why) ? std::string() : why;
      }
    }
  }
  else if ([key isEqualToString:@"modcancel"]) mu_mods::cancel();
  else if ([key hasPrefix:@"use-mod"]) {   // Use chooses the mod for the next start; In use goes back to the retail game
    const std::vector<mu_mods::Row> list = mu_mods::rows();
    if (index >= 0 && index < (int)list.size()) mod_profile_set(mod_profile_next() == list[index].id ? std::string() : list[index].id);
  }
  else if ([key isEqualToString:@"scale"]) { const int v[] = {0, 1, 2, 3, 4}; s.scale = v[std::clamp(index, 0, 4)]; }
  else if ([key isEqualToString:@"fullscreen"]) s.fullscreen = index == 1;
  else if ([key isEqualToString:@"vsync"]) s.vsync = index == 1;
  else if ([key isEqualToString:@"widescreen"]) { s.widescreen = index == 1; host::set_engine_setting("widescreen", s.widescreen ? "1" : "0"); }
  else if ([key isEqualToString:@"volume"]) { const int v[] = {0, 25, 50, 70, 100}; s.volume = v[std::clamp(index, 0, 4)]; }
  else if ([key isEqualToString:@"online"]) s.online = index == 1;
  else if ([key isEqualToString:@"delay"]) s.online_delay = index;   // 0 automatic
  [self setNeedsDisplay:YES];
}
// Replays play in the melee-unlocked engine (MeleeUnlockedEngine --replay), in its own window beside the launcher.
- (void)watch:(NSString*)replay {
  // upstream's rule (launcher_replays.inl): a match played on a mod's stage would sit on a blank screen; say so instead.
  if (const int stage = mu_replay_stats::stage_of(replay.UTF8String); stage > 85) {
    host::log("launcher: %s was recorded on stage %d, not in the standard game: not started", replay.UTF8String, stage);
    self.replayNote = [NSString stringWithFormat:@"This replay was recorded with a mod (stage %d is not in the standard game). It needs that mod to play.", stage];
    [self setNeedsDisplay:YES];
    return;
  }
  if (self.settings->iso.empty()) { host::mac_show_error("Choose your disc first", "Watching a replay needs the same disc image as playing."); return; }
  NSTask* task = [[NSTask alloc] init];
  task.executableURL = [NSURL fileURLWithPath:ns(host::engine_path())];
  NSString* support = ns(support_dir());   // not the replay folder's parent: the player can choose that folder (replaydir)
  task.arguments = @[@"--iso", ns(self.settings->iso), @"--replay", replay, @"--profile-dir", support,
                     @"--volume", [NSString stringWithFormat:@"%d", self.settings->volume]];
  NSError* error = nil;
  if (![task launchAndReturnError:&error]) host::mac_show_error("The replay could not start", error.localizedDescription.UTF8String);
  else { self.watchTask = task; host::log("launcher: watching %s", replay.UTF8String); }
}
// upstream's Language button: a menu of its languages, each in its own words; the choice is kept in launcher.ini.
- (void)chooseLanguage {
  NSMenu* menu = [[NSMenu alloc] init];
  for (const Language& l : kLanguages) {
    NSMenuItem* item = [menu addItemWithTitle:[NSString stringWithUTF8String:l.native] action:@selector(pickLanguage:) keyEquivalent:@""];
    item.target = self; item.representedObject = [NSString stringWithUTF8String:l.code];
    item.state = g_lang == l.code ? NSControlStateValueOn : NSControlStateValueOff;
  }
  const NSRect r = self.buttons[@"language"].rectValue;
  [menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(r.origin.x, NSMaxY(r) + 4) inView:self];
}
- (void)pickLanguage:(NSMenuItem*)item {
  const std::string code = [item.representedObject UTF8String];
  if (lang_set(code)) ini_set("language", code);
  [self setNeedsDisplay:YES];
}
// upstream's choose_rail_image: Choose a picture... (copied in as rail-image.<ext>), Bring back Bailey.
- (void)chooseRailPicture {
  NSMenu* menu = [[NSMenu alloc] init];
  [menu addItemWithTitle:tx(@"Choose a picture...") action:@selector(pickRailPicture) keyEquivalent:@""].target = self;
  if (!rail_image_path().empty()) [menu addItemWithTitle:tx(@"Bring back Bailey") action:@selector(bringBackBailey) keyEquivalent:@""].target = self;
  const NSRect r = self.buttons[@"dog"].rectValue;
  [menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(NSMidX(r), NSMidY(r)) inView:self];
}
- (void)bringBackBailey {
  for (const char* ext : kRailExts) std::remove((support_dir() + "/melee-unlocked/rail-image" + ext).c_str());
  self.railPicture = nil;
  [self setNeedsDisplay:YES];
}
- (void)pickRailPicture {
  NSOpenPanel* panel = [NSOpenPanel openPanel];
  panel.message = tx(@"Choose a picture for the launcher");
  panel.allowedFileTypes = @[@"png", @"jpg", @"jpeg", @"bmp", @"gif", @"heic"];
  if ([panel runModal] != NSModalResponseOK) return;
  NSImage* picture = [[NSImage alloc] initWithContentsOfURL:panel.URL];
  if (!picture || picture.size.width < 1) { host::mac_show_error("That picture could not be opened.", panel.URL.path.UTF8String); return; }
  [self bringBackBailey];
  NSString* dir = ns(support_dir() + "/melee-unlocked");
  [NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
  NSString* to = [dir stringByAppendingPathComponent:[@"rail-image." stringByAppendingString:panel.URL.pathExtension.lowercaseString]];
  [NSFileManager.defaultManager copyItemAtPath:panel.URL.path toPath:to error:nil];
  self.railPicture = picture;
  host::log("launcher: rail picture %s", to.UTF8String);
  [self setNeedsDisplay:YES];
}
- (void)openTheme {
  if (self.themePanel) { [self.themePanel makeKeyAndOrderFront:nil]; return; }
  NSPanel* panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 268, 324) styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                                                backing:NSBackingStoreBuffered defer:NO];
  panel.opaque = NO; panel.backgroundColor = NSColor.clearColor; panel.hasShadow = YES; panel.level = NSFloatingWindowLevel;
  MUThemeView* view = [[MUThemeView alloc] initWithFrame:NSMakeRect(0, 0, 268, 324)];
  view.owner = self; view.before = g_accent;
  double h, s, v; to_hsv(g_accent, h, s, v); view.h = h; view.s = s; view.v = v;
  view.hex = [[NSTextField alloc] initWithFrame:NSMakeRect(30, 277, 85, 20)];
  view.hex.bezeled = NO; view.hex.drawsBackground = NO; view.hex.textColor = col(C_TEXT); view.hex.font = f_body();
  view.hex.stringValue = [NSString stringWithFormat:@"#%02X%02X%02X", g_accent.r, g_accent.g, g_accent.b];
  view.hex.target = view; view.hex.action = @selector(apply);
  [view addSubview:view.hex];
  panel.contentView = view;
  // Above the colour button, as the original anchors it.
  const NSRect button = [self convertRect:self.buttons[@"theme"].rectValue toView:nil];
  const NSRect on_screen = [self.window convertRectToScreen:button];
  [panel setFrameOrigin:NSMakePoint(NSMaxX(on_screen) - 268, NSMaxY(on_screen) + 8)];
  [panel makeKeyAndOrderFront:nil];
  self.themePanel = panel;
}
- (void)closeTheme { [self.themePanel orderOut:nil]; self.themePanel = nil; [self setNeedsDisplay:YES]; }
@end

@interface MULaunchWindowDelegate : NSObject <NSWindowDelegate>
@end
@implementation MULaunchWindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)w { [NSApp stopModalWithCode:NSModalResponseCancel]; return YES; }
@end

namespace host {
void mac_show_error(const std::string& title, const std::string& detail) {
  // Screenshot and test runs never put an alert on the player's screen: the message goes to the log instead.
  if (std::getenv("MELEE_LAUNCHER_SHOT") || std::getenv("MELEE_NO_ALERTS")) { host::log("launcher: (alert not shown) %s: %s", title.c_str(), detail.c_str()); return; }
  @autoreleasepool {
    [NSApplication sharedApplication];
    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = ns(title);
    alert.informativeText = ns(detail);
    [alert runModal];
  }
}
bool mu_launcher_run(LauncherSettings& settings, const std::string& error) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    if (settings.accent) g_accent = {int((settings.accent >> 16) & 255), int((settings.accent >> 8) & 255), int(settings.accent & 255)};
    apply_mu_accent(settings);
    // Screenshots of the Settings window (MELEE_SETTINGS_SHOT=<file.png>, MELEE_LAUNCHER_SETTINGS=<category 0..7>): off-screen, no launcher.
    if (const char* shot = std::getenv("MELEE_SETTINGS_SHOT")) {
      const char* cat = std::getenv("MELEE_LAUNCHER_SETTINGS");
      host::mu_settings_window_run(settings, cat ? std::atoi(cat) : 0, shot);
      return false;
    }
    NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, WIN_W, WIN_H)
                                                   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
                                                     backing:NSBackingStoreBuffered defer:NO];
    window.title = @"melee-unlocked";
    window.releasedWhenClosed = NO;
    MULaunchWindowDelegate* delegate = [[MULaunchWindowDelegate alloc] init];
    window.delegate = delegate;
    MULaunchView* view = [[MULaunchView alloc] initWithFrame:NSMakeRect(0, 0, WIN_W, WIN_H)];
    view.settings = &settings; view.hotNav = -1; view.error = error;
    view.mark = art(@"melee_mark.png"); view.bailey = art(@"bailey.png");
    lang_set(ini_get("language"));
    if (const std::string rail = rail_image_path(); !rail.empty()) view.railPicture = [[NSImage alloc] initWithContentsOfFile:ns(rail)];
    {   // the first start of a new version shows What's new by itself (screenshots: MELEE_LAUNCHER_NOTES=1 shows it, 0 never)
      const char* notes = std::getenv("MELEE_LAUNCHER_NOTES");
      const bool shot_run = std::getenv("MELEE_LAUNCHER_SHOT") != nullptr;
      if (notes ? *notes == '1' : (!shot_run && ini_get("notesseen") != MELEE_PORT_VERSION && whats_new_lines().count)) {
        view.notesOpen = YES;
        if (!notes) ini_set("notesseen", MELEE_PORT_VERSION);
      }
    }
    if (const char* page = std::getenv("MELEE_LAUNCHER_PAGE")) view.tab = std::clamp(std::atoi(page), 0, 3);   // screenshots
    // The replay list is read off the main thread (parsing .slp files takes a moment), as the original scans.
    view.picked = -1;
    [view reloadReplays];
    window.contentView = view;
    // MELEE_LAUNCHER_SHOT=<file.png> (screenshots, tests): the window opens off every screen, is drawn into that file after
    // two seconds (the update check and the replay list have answered by then) and closes as if dismissed.
    const char* shot = std::getenv("MELEE_LAUNCHER_SHOT");
    if (shot) [window setFrameOrigin:NSMakePoint(-20000, -20000)];
    else [window center];
    if (shot) [window orderFront:nil]; else [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:view];
    if (!shot) [NSApp activateIgnoringOtherApps:YES];
    if (shot) {
      // Off every screen the window counts as hidden, and App Nap would hold the timer back: keep the app awake.
      static id activity = [NSProcessInfo.processInfo beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep reason:@"launcher screenshot"];
      (void)activity;
      NSString* out = ns(shot);
      NSTimer* timer = [NSTimer timerWithTimeInterval:2.0 repeats:NO block:^(NSTimer*) {
        NSBitmapImageRep* rep = [view bitmapImageRepForCachingDisplayInRect:view.bounds];
        [view cacheDisplayInRect:view.bounds toBitmapImageRep:rep];
        [[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:out atomically:YES];
        std::printf("launcher: screenshot %s\n", out.UTF8String); std::fflush(stdout);
        [NSApp abortModal];
      }];
      [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];   // the launcher runs modally
    }
    if (std::getenv("MELEE_LAUNCHER_PLAY") && !settings.iso.empty())   // test runs: press PLAY a moment after the window shows
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{ [NSApp stopModalWithCode:NSModalResponseOK]; });
    if (std::getenv("MELEE_LAUNCHER_THEME")) dispatch_async(dispatch_get_main_queue(), ^{ [view openTheme]; });   // screenshots
    [view checkForUpdate];
    if (std::getenv("MELEE_LAUNCHER_LIVE")) dispatch_async(dispatch_get_main_queue(), ^{ [view toggleLive]; [view setNeedsDisplay:YES]; });   // tests
    const NSModalResponse r = [NSApp runModalForWindow:window];
    [view closeTheme];
    [window orderOut:nil];
    return r == NSModalResponseOK;
  }
}
}  // namespace host
