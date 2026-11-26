// The launcher's Multiplayer Lobby page, included by mu_launcher.mm (one translation unit: the palette, text() and the buttons are
// its own). Drawn as melee-unlocked's Win32 lobby window draws it (port/app/launcher_lobby_ui.inl paint_lobby, draw_control and
// layout(), in its 96-dpi units; launcher.cpp opens the page at 980 x 768 with the rail on the left): the title and Go Online, the
// left card with its tabs (Lobby chat with a tab per private chat, Friends, Match history, Your profile), the Players card with
// Send Match Request, the peer-to-peer match and Add Friend, the request bar with Accept and Decline, and the status line. The
// lobby itself runs on its own thread (mu_lobby.cpp); this page reads its snapshot ten times a second.
// SPDX-License-Identifier: GPL-3.0-or-later

namespace {
const RGB8 UI_BG{19, 26, 42}, UI_PANEL{12, 18, 33}, UI_BORDER{43, 54, 82}, UI_FIELD{16, 23, 38}, UI_ROW{33, 43, 66};
RGB8 glow() { return mix(g_accent, {255, 255, 255}, .35); }
NSFont* f_title() { return [NSFont systemFontOfSize:20 weight:NSFontWeightBold]; }
NSFont* f_name() { return [NSFont systemFontOfSize:12 weight:NSFontWeightSemibold]; }
std::string jstr(const nlohmann::json& j, const char* key, const std::string& fallback = std::string()) {
  return j.is_object() && j.count(key) && j[key].is_string() ? j[key].get<std::string>() : fallback;
}
std::string str(NSString* s) { return s.length ? std::string(s.UTF8String) : std::string(); }   // nil is ""
std::string stamp(int64_t wall) {
  if (wall < 1000000000) return {};
  const std::time_t seconds = std::time_t(wall); std::tm local{}; localtime_r(&seconds, &local);
  char buffer[16] = {}; std::strftime(buffer, sizeof buffer, "%I:%M %p", &local);
  return buffer;
}
const char* kAbbrev[26] = {"CF", "DK", "Fx", "GW", "Kb", "Bw", "Lk", "Lg", "Mr", "Ms", "Mt", "Ns", "Pc", "Pk", "IC", "JP", "Sm", "Ys",
                           "Zd", "Sk", "Fc", "YL", "Dr", "Ry", "Pi", "Gn"};
}  // namespace

@implementation MULaunchView (Lobby)
// ---- the page's own text fields (upstream's EDIT controls), shown on the Lobby page only --------------------------------
- (NSTextField*)lobbyField:(NSString*)cue {
  NSTextField* f = [[NSTextField alloc] initWithFrame:NSZeroRect];
  f.bezeled = NO; f.drawsBackground = NO; f.editable = YES; f.selectable = YES; f.focusRingType = NSFocusRingTypeNone;
  f.textColor = col(C_TEXT); f.font = f_body(); f.hidden = YES;
  f.placeholderAttributedString = [[NSAttributedString alloc] initWithString:tx(cue) attributes:@{NSForegroundColorAttributeName: col(C_FAINT), NSFontAttributeName: f_body()}];
  f.target = self; f.action = @selector(lobbyFieldReturn:);
  f.cell.scrollable = YES; f.cell.lineBreakMode = NSLineBreakByClipping;
  [self addSubview:f];
  return f;
}
- (void)lobbyFieldReturn:(NSTextField*)f {
  if (f == self.chatField) [self activate:@"lb:send"];
  else if (f == self.friendField) [self activate:@"lb:friendsend"];
  else if (f == self.nameField || f == self.locationField) [self activate:@"lb:save"];
}
- (void)place:(NSTextField*)f rect:(NSRect)r shown:(BOOL)shown {
  if (!f) return;
  f.hidden = !shown;
  if (!shown) return;
  // upstream's field: a border box 5 units around the control, then the field colour
  const NSRect box = NSInsetRect(r, -5, -5);
  round_rect(box, 7, UI_BORDER, UI_BORDER, nullptr);
  round_rect(NSInsetRect(box, 1, 1), 7, UI_FIELD, UI_FIELD, nullptr);
  const CGFloat h = 17;
  const NSRect at = NSMakeRect(r.origin.x, r.origin.y + (r.size.height - h) / 2, r.size.width, h);
  if (!NSEqualRects(f.frame, at)) f.frame = at;
}
- (void)hideLobbyFields { for (NSTextField* f in @[self.chatField ?: [NSNull null], self.nameField ?: [NSNull null], self.locationField ?: [NSNull null], self.friendField ?: [NSNull null]])
    if ([f isKindOfClass:NSTextField.class]) f.hidden = YES; }
// A lobby button: upstream's draw_control (radius 9, the gradient for primary ones, the 1-unit border for the rest, the body font).
- (void)lb:(NSString*)name rect:(NSRect)r title:(NSString*)title primary:(BOOL)primary enabled:(BOOL)enabled {
  self.buttons[name] = [NSValue valueWithRect:r];
  const BOOL down = [self.down isEqualToString:name];
  RGB8 top = primary ? acc_top() : UI_ROW, bot = primary ? acc_bottom() : UI_ROW;
  if (!enabled) top = bot = RGB8{26, 33, 50};
  if (down && enabled) top = bot = primary ? acc_pressed() : RGB8{24, 33, 52};
  const RGB8 border = enabled ? RGB8{53, 65, 95} : UI_BORDER;
  round_rect(r, 9, top, bot, primary && enabled ? nullptr : &border);
  text(title, r, f_body(), enabled ? (primary ? on_accent() : C_TEXT) : C_DIM, CENTER);
  if (!enabled) [self.buttons removeObjectForKey:name];
}
// upstream's empty_state: a badge, a title and a line.
- (void)emptyState:(NSRect)area title:(NSString*)title detail:(NSString*)detail social:(BOOL)social {
  const CGFloat x = area.origin.x + 24, y = area.origin.y + 28;
  round_rect(LR(x, y, 46, 46), 16, {32, 35, 59}, {32, 35, 59}, nullptr);
  [col(glow()) setStroke];
  NSBezierPath* p = [NSBezierPath bezierPath]; p.lineWidth = 2;
  if (social) { [p appendBezierPathWithOvalInRect:LR(x + 16, y + 9, 14, 14)]; [p appendBezierPathWithRoundedRect:LR(x + 9, y + 26, 28, 13) xRadius:6 yRadius:6]; }
  else { [p appendBezierPathWithRoundedRect:LR(x + 10, y + 11, 26, 20) xRadius:3 yRadius:3]; [p moveToPoint:NSMakePoint(x + 16, y + 31)]; [p lineToPoint:NSMakePoint(x + 16, y + 37)]; [p lineToPoint:NSMakePoint(x + 23, y + 31)]; }
  [p stroke];
  text(title, LR(x, y + 64, area.size.width - 44, 60), f_title(), C_TEXT, LEFT, 0, true);
  text(detail, LR(x, y + 132, area.size.width - 48, 60), f_body(), C_DIM, LEFT, 0, true);
}
// A character's small badge (upstream draws the stock icon; without the game's icons here, its fallback: two letters in a box).
- (void)stock:(int)n rect:(NSRect)r selected:(BOOL)selected {
  round_rect(r, 7, selected ? mix(g_accent, UI_PANEL, .7) : RGB8{23, 31, 48}, selected ? mix(g_accent, UI_PANEL, .7) : RGB8{23, 31, 48}, selected ? nullptr : &UI_BORDER);
  if (n >= 0 && n < 26) text(ns(kAbbrev[n]), r, r.size.height > 24 ? f_name() : f_label(), C_TEXT, CENTER);
}

// ---- the page ----------------------------------------------------------------------------------------------------------
- (void)paintLobby {
  using Json = nlohmann::json;
  const Json snap = self.lobbySnap;
  const Json& s = snap.is_object() ? snap : Json::object();
  const CGFloat X = RAIL_W, W = self.bounds.size.width - RAIL_W, H = self.bounds.size.height;
  auto L = [&](CGFloat x, CGFloat y, CGFloat w, CGFloat h) { return LR(X + x, y, w, h); };
  [col(UI_BG) setFill]; NSRectFill(L(0, 0, W, H));
  const std::string phase = jstr(s, "phase", "offline");
  const bool online = phase == "online", playing = s.value("playing", false) || mu_lobby::playing();
  const std::string self_id = s.count("self") ? jstr(s["self"], "id") : std::string();
  text(@"Multiplayer Lobby", L(24, 24, W - 172, 30), f_title(), C_TEXT);
  [self lb:@"lb:go" rect:L(W - 132, 24, 108, 30) title:online ? @"Leave" : phase == "starting" ? @"Going online..." : @"Go Online" primary:!online enabled:phase != "starting" && phase != "held"];
  // Both kinds of match, as Melee-NX-NetPlay's lobby offers them: the peer-to-peer match's fighter and its request, left of Go Online
  // (where upstream puts its P2P Direct controls).
  const mu_lobby::Profile prof = mu_lobby::profile();
  {
    const int ch = prof.p2p_character >= 0 ? prof.p2p_character : prof.mains.empty() ? 2 : prof.mains[0];
    const int colors = mu_lobby::color_count(ch);
    const std::string who = prof.p2p_character >= 0 ? std::string(mu_lobby::character_name(ch))
                                                    : launcher::lang::tr("lobby.p2p.fighter_first", {{"character", std::string(mu_lobby::character_name(ch))}});
    const std::string caption = launcher::lang::tr("lobby.p2p.fighter_button", {{"character", who}, {"number", std::to_string((prof.p2p_color < colors ? prof.p2p_color : 0) + 1)}});
    [self lb:@"lb:fighter" rect:L(W - 132 - 158 - 204, 24, 196, 30) title:ns(caption) primary:NO enabled:YES];
  }
  text(online ? @"You're online. Select a player to send a match request; a peer-to-peer match needs no Slippi servers." :
                @"Go Online shows your name to other players and lets them send you match requests while the launcher is open.",
       L(24, 66, W - 48, 34), f_small(), C_DIM, LEFT, 0, true);
  const CGFloat right = W - 316, left_w = right - 42, bottom = H - 94;
  const NSRect left = L(24, 110, right - 18 - 24, bottom - 110), players = L(right, 110, W - 24 - right, bottom - 110);
  for (NSRect card : {left, players}) { round_rect(card, 12, UI_BORDER, UI_BORDER, nullptr); round_rect(NSInsetRect(card, 1, 1), 12, UI_PANEL, UI_PANEL, nullptr); }

  // ---- the Players card ----
  Json rows = Json::array();
  for (const auto& p : s.value("players", Json::array())) if (p.is_object()) rows.push_back(p);
  text([NSString stringWithFormat:@"%@ (%zu)", tx(@"Players online"), rows.size()], L(right + 14, 124, 264, 24), f_name(), C_TEXT);
  const Json* chosen = nullptr;
  if (rows.empty()) { NSRect area = players; area.origin.y += 46; area.size.height -= 46; [self emptyState:area title:@"Find your next match" detail:@"Go online to discover players.\nTheir location and mains appear here." social:YES]; }
  else {
    const CGFloat list_y = 154, list_h = bottom - 204 - 48, item = 78;
    const int fit = std::max(1, int(list_h / item));
    const int first = std::clamp(int(self.lobbyPlayerScroll), 0, std::max(0, int(rows.size()) - fit));
    for (int i = first; i < int(rows.size()) && i < first + fit; ++i) {
      const Json& p = rows[size_t(i)];
      const NSRect r = L(right + 12, list_y + (i - first) * item, 268, item - 4);
      const bool selected = jstr(p, "id") == str(self.lobbyPlayer);
      if (selected) chosen = &p;
      round_rect(r, 6, selected ? mix(g_accent, UI_PANEL, .82) : UI_PANEL, selected ? mix(g_accent, UI_PANEL, .82) : UI_PANEL, nullptr);
      self.buttons[[NSString stringWithFormat:@"lb:player:%d", i]] = [NSValue valueWithRect:r];
      [self paintPerson:p rect:r selected:selected pings:s.value("pings", Json::object())];
    }
  }
  const bool can_ask = online && chosen && !playing;
  [self lb:@"lb:pm" rect:L(right + 12, bottom - 46, 268, 32) title:ns(launcher::lang::tr("lobby.pm.button")) primary:NO enabled:can_ask];
  [self lb:@"lb:request" rect:L(right, bottom + 12, 181, 34) title:@"Send Match Request" primary:YES enabled:can_ask && chosen->value("hosted_ok", true)];
  [self lb:@"lb:addfriend" rect:L(right + 189, bottom + 12, 103, 34) title:@"Add Friend" primary:NO enabled:can_ask && !chosen->value("friend", false)];
  [self lb:@"lb:p2p" rect:L(W - 132 - 158, 24, 150, 30) title:@"Peer-to-peer match" primary:NO enabled:can_ask && chosen->value("p2p", false)];

  // ---- the tabs under the left card ----
  NSArray<NSString*>* tabs = @[@"Profile", @"Friends", @"History"];
  const int tab_of[3] = {3, 1, 2};
  for (int i = 0; i < 3; ++i)
    [self lb:[NSString stringWithFormat:@"lb:tab:%d", tab_of[i]] rect:L(24 + i * 90, bottom + 12, 82, 34) title:tabs[i] primary:self.lobbyTab == tab_of[i] enabled:YES];
  if (self.lobbyTab != 0) [self lb:@"lb:tab:0" rect:L(294, bottom + 12, 132, 34) title:@"Back to Chat" primary:NO enabled:YES];

  // ---- the left card: requests first (they shorten the chat), then the tab ----
  Json pending = Json::array();
  for (const auto& r : s.value("requests", Json::array()))
    if (jstr(r, "state") == "pending" || jstr(r, "state") == "accepted") pending.push_back(r);
  for (const auto& room : s.value("private", Json::array())) if (jstr(room, "state") == "incoming") pending.push_back(Json{{"room", room}});
  const bool show_pending = !pending.empty() && self.lobbyTab == 0;
  const CGFloat content_bottom = bottom - (show_pending ? 122 : 0);
  NSArray<NSString*>* headings = @[@"Lobby chat", @"Friends", @"Match history", @"Your profile"];
  Json rooms = s.value("private", Json::array());
  Json open_rooms = Json::array();
  for (const auto& room : rooms) if (jstr(room, "state") == "open" || jstr(room, "state") == "closed" || jstr(room, "state") == "outgoing") open_rooms.push_back(room);
  if (self.lobbyTab == 0 && !open_rooms.empty()) [self paintRoomTabs:open_rooms card:left];
  else text(headings[self.lobbyTab], LR(left.origin.x + 16, left.origin.y + 12, left.size.width - 32, 30), f_name(), C_TEXT);
  [self hideLobbyFields];
  if (self.lobbyTab == 0) [self paintChat:s left:left contentBottom:content_bottom leftWidth:left_w rooms:open_rooms selfId:self_id online:online];
  else if (self.lobbyTab == 1) [self paintFriends:s left:left bottom:bottom leftWidth:left_w online:online];
  else if (self.lobbyTab == 2) [self paintHistory:left leftWidth:left_w bottom:bottom];
  else [self paintProfile:left leftWidth:left_w];
  if (show_pending) [self paintPending:pending at:content_bottom leftWidth:left_w selfId:self_id];

  // ---- the status line ----
  std::string status = jstr(s, "notice");
  if (phase == "error") status = jstr(s, "error");
  if (playing) status = "In game: the match runs in the game's window. The lobby comes back here when it is over.";
  const std::string copy = jstr(s, "copy_code");
  const CGFloat copy_x = copy.empty() ? 0 : std::min<CGFloat>(24 + text_width(ns(status), f_small()) + 12, W - 24 - 104);
  text(ns(status), L(24, bottom + 58, copy.empty() ? W - 48 : copy_x - 32, 18), f_small(), phase == "error" ? C_BAD : C_DIM);
  if (!copy.empty()) [self lb:@"lb:copycode" rect:L(copy_x, bottom + 53, 104, 26) title:ns(launcher::lang::tr("lobby.copy_code")) primary:NO enabled:YES];
}
// One player card (upstream's draw_control for the PLAYERS and FRIENDS lists).
- (void)paintPerson:(const nlohmann::json&)p rect:(NSRect)r selected:(BOOL)selected pings:(const nlohmann::json&)pings {
  using Json = nlohmann::json;
  const Json mains = p.value("mains", Json::array());
  const int main = !mains.empty() && mains[0].is_number_integer() ? mains[0].get<int>() : -1;
  [self stock:main rect:LR(r.origin.x + 10, r.origin.y + 13, 32, 32) selected:NO];
  std::string name = jstr(p, "name", "Player");
  if (p.value("is_self", false)) name += "  (You)";
  std::string version = jstr(p, "build"), kind;
  if (const size_t colon = version.find(':'); colon != std::string::npos) { kind = version.substr(colon + 1); version.resize(colon); }
  if (!version.empty() && version[0] != 'v' && version[0] != 'V') version = "v" + version;
  if (kind == "source") version += "  Source Port"; else if (kind == "recomp") version += "  Static Recomp";
  const CGFloat tl = r.origin.x + 54, tr = NSMaxX(r) - 12;
  const CGFloat bw = version.empty() ? 0 : std::min<CGFloat>(text_width(ns(version), f_small()) + 2, (tr - tl) / 2);
  text(ns(name), LR(tl, r.origin.y + 8, tr - tl - bw - 8, 18), f_name(), C_TEXT);
  if (bw > 0) text(ns(version), LR(tr - bw, r.origin.y + 8, bw, 18), f_small(), glow(), RIGHT);
  std::string detail = p.value("incoming", false) ? "Friend request" : jstr(p, "status", "Online");
  if (detail == "Online" && !p.value("ready", true)) detail = launcher::lang::tr("lobby.card.setup_unfinished");
  if (const std::string loc = jstr(p, "location"); !loc.empty()) detail += "  /  " + loc;
  if (pings.is_object() && pings.count(jstr(p, "id")) && pings[jstr(p, "id")].is_number_integer()) detail += "  /  " + std::to_string(pings[jstr(p, "id")].get<int>()) + " ms";
  text(ns(detail), LR(tl, r.origin.y + 30, tr - tl, 18), f_small(), C_DIM);
  const Json stocks = p.value("stocks", Json::array());
  if (stocks.is_array() && !stocks.empty()) {
    std::string line = "Stocks: ";
    for (const auto& n : stocks) if (n.is_number_integer()) line += std::to_string(n.get<int>()) + " ";
    text(ns(line), LR(tl, r.origin.y + 51, tr - tl, 18), f_small(), C_DIM);
  } else if (selected) {
    std::string record = "W " + std::to_string(p.value("wins", 0)) + "  /  L " + std::to_string(p.value("losses", 0));
    if (const std::string code = jstr(p, "code"); !code.empty()) record += "   " + code;
    text(ns(record), LR(tl, r.origin.y + 51, tr - tl, 18), f_small(), glow());
  } else {
    CGFloat x = tl;
    for (const auto& c : mains) if (c.is_number_integer()) { [self stock:c.get<int>() rect:LR(x, r.origin.y + 51, 18, 18) selected:NO]; x += 26; }
  }
}
// upstream's paint_room_tabs: the public chat and a tab per private chat, a close mark on each private one, a dot when unread.
- (void)paintRoomTabs:(const nlohmann::json&)rooms card:(NSRect)card {
  struct Tab { std::string id, title; bool unread; };
  std::vector<Tab> tabs{{std::string(), launcher::lang::tr("lobby.pm.tab_public"), false}};
  for (const auto& room : rooms) {
    const auto messages = room.value("messages", nlohmann::json::array());
    bool unread = false;
    if (messages.is_array() && !messages.empty() && jstr(messages.back(), "sender") == jstr(room, "peer"))
      unread = [self.lobbySeen[ns(jstr(room, "id"))] isEqualToString:ns(jstr(messages.back(), "id"))] == NO && jstr(room, "id") != str(self.lobbyRoom);
    tabs.push_back({jstr(room, "id"), jstr(room, "name", "?"), unread});
  }
  const CGFloat gap = 6, pad = 11, x0 = card.origin.x + 14, x1 = NSMaxX(card) - 14, top = card.origin.y + 10;
  const CGFloat share = std::max<CGFloat>(40, (x1 - x0 - gap * (tabs.size() - 1)) / tabs.size());
  CGFloat x = x0;
  for (size_t i = 0; i < tabs.size(); ++i) {
    const Tab& t = tabs[i];
    const bool front = t.id == str(self.lobbyRoom), closable = !t.id.empty();
    NSFont* font = t.unread || front ? f_name() : f_body();
    const CGFloat w = std::min<CGFloat>(share, text_width(ns(t.title), font) + 2 * pad + (closable ? 18 : 0) + (t.unread ? 12 : 0));
    const NSRect r = LR(x, top, w, 30);
    if (NSMaxX(r) > x1) break;
    const RGB8 fill = front ? mix(g_accent, UI_PANEL, .55) : UI_ROW;
    round_rect(r, 9, fill, fill, nullptr);
    CGFloat lx = r.origin.x + pad;
    if (t.unread) { round_rect(LR(lx, r.origin.y + 12, 7, 7), 3.5, glow(), glow(), nullptr); lx += 12; }
    text(ns(t.title), LR(lx, r.origin.y, NSMaxX(r) - pad - (closable ? 18 : 0) - lx, 30), font, front || t.unread ? C_TEXT : C_DIM);
    self.buttons[[NSString stringWithFormat:@"lb:room:%zu", i]] = [NSValue valueWithRect:r];
    if (closable) {
      const NSRect cr = LR(NSMaxX(r) - pad - 12, r.origin.y, 17, 30);
      text(@"×", cr, f_body(), front ? C_TEXT : C_DIM, CENTER);
      self.buttons[[NSString stringWithFormat:@"lb:roomclose:%zu", i]] = [NSValue valueWithRect:cr];
    }
    x = NSMaxX(r) + gap;
  }
}
- (void)paintChat:(const nlohmann::json&)s left:(NSRect)left contentBottom:(CGFloat)content_bottom leftWidth:(CGFloat)left_w rooms:(const nlohmann::json&)rooms
           selfId:(const std::string&)self_id online:(bool)online {
  using Json = nlohmann::json;
  const CGFloat X = RAIL_W;
  // The room in front: the public chat, or a private room still there.
  Json room_obj; bool room_found = false;
  for (const auto& room : rooms) if (jstr(room, "id") == str(self.lobbyRoom)) { room_obj = room; room_found = true; }
  if (!room_found && self.lobbyRoom.length) self.lobbyRoom = @"";
  const Json messages = room_found ? room_obj.value("messages", Json::array()) : s.value("messages", Json::array());
  if (room_found && messages.is_array() && !messages.empty()) self.lobbySeen[self.lobbyRoom] = ns(jstr(messages.back(), "id"));
  const NSRect log = LR(X + 38, 166, left_w - 28, content_bottom - 222);
  if (messages.empty() && room_found) {
    text(ns(launcher::lang::tr("lobby.pm.empty", {{"name", jstr(room_obj, "name")}})), LR(left.origin.x + 24, left.origin.y + 64, left.size.width - 48, 64), f_body(), C_DIM, LEFT, 0, true);
  } else if (messages.empty()) {
    NSRect area = left; area.origin.y += 46; area.size.height -= 46;
    [self emptyState:area title:@"Ready for a few games?" detail:@"Join the lobby, say hello, and send a match request when you're ready." social:NO];
  } else {
    const CGFloat item = 52;
    const int fit = std::max(1, int(log.size.height / item));
    const int n = int(messages.size());
    const int last = std::clamp(n - int(self.lobbyChatScroll), std::min(n, fit), n);
    for (int i = std::max(0, last - fit), row = 0; i < last; ++i, ++row) {
      const Json& m = messages[size_t(i)];
      const NSRect r = LR(log.origin.x, log.origin.y + row * item, log.size.width, item - 4);
      round_rect(r, 6, UI_PANEL, UI_PANEL, nullptr);
      if (m.value("system", false)) { text(ns(jstr(m, "text")), NSInsetRect(r, 12, 0), f_small(), C_DIM, CENTER); continue; }
      text(ns(jstr(m, "name", "Player")), LR(r.origin.x + 12, r.origin.y + 5, r.size.width - 90, 22), f_name(), glow());
      text(ns(stamp(m.value("wall_time", int64_t(0)))), LR(NSMaxX(r) - 88, r.origin.y + 5, 76, 22), f_small(), C_DIM, RIGHT);
      text(ns(jstr(m, "text")), LR(r.origin.x + 12, r.origin.y + 27, r.size.width - 24, 20), f_body(), C_TEXT);
    }
  }
  const bool closed = room_found && jstr(room_obj, "state") != "open";
  [self place:self.chatField rect:LR(X + 38, content_bottom - 43, left_w - 130, 26) shown:YES];
  self.chatField.enabled = online && !closed;
  [self lb:@"lb:emoji" rect:LR(X + left_w - 85, content_bottom - 46, 30, 32) title:@"\U0001F602" primary:NO enabled:online && !closed];
  [self lb:@"lb:send" rect:LR(X + left_w - 50, content_bottom - 46, 62, 32) title:@"Send" primary:NO enabled:online && !closed];
}
- (void)paintPending:(const nlohmann::json&)pending at:(CGFloat)content_bottom leftWidth:(CGFloat)left_w selfId:(const std::string&)self_id {
  const CGFloat X = RAIL_W;
  const nlohmann::json& first = pending[0];
  std::string what; bool incoming = false;
  if (first.count("room")) {
    incoming = true;
    what = launcher::lang::tr("lobby.pm.request_from", {{"name", jstr(first["room"], "name", "?")}}) + ". " + launcher::lang::tr("lobby.pm.request_detail");
  } else {
    const std::string name = first.count("peer") ? jstr(first["peer"], "name", "?") : "?";
    const bool p2p = jstr(first, "transport") == launcher::lobby::kP2pTransport;
    incoming = jstr(first, "to") == self_id;
    const std::string state = jstr(first, "state");
    if (state == "accepted") what = p2p ? launcher::lang::tr("lobby.p2p.starting", {{"name", name}}) : "Match accepted. Starting Slippi Direct against " + name + "...";
    else if (incoming) what = name + (p2p ? " wants a peer-to-peer match (no Slippi servers)" : " wants to play (Slippi Direct)");
    else what = (p2p ? "Peer-to-peer match request sent to " : "Match request sent to ") + name + ". Waiting for an answer (" + std::to_string(first.value("remaining", 0)) + " s).";
  }
  const NSRect box = LR(X + 38, content_bottom + 4, left_w - 28, 62);
  round_rect(box, 6, mix(g_accent, UI_PANEL, .82), mix(g_accent, UI_PANEL, .82), nullptr);
  text(ns(what), NSInsetRect(box, 10, 6), f_small(), C_TEXT, LEFT, 0, true);
  const bool accepted = jstr(first, "state") == "accepted";
  [self lb:@"lb:accept" rect:LR(X + 38, content_bottom + 72, 116, 30) title:first.count("room") ? @"Accept" : @"Accept Match" primary:YES enabled:incoming && !accepted];
  [self lb:@"lb:decline" rect:LR(X + 162, content_bottom + 72, 126, 30) title:@"Decline / Cancel" primary:NO enabled:!accepted];
}
- (nlohmann::json)friendRows:(const nlohmann::json&)s {
  nlohmann::json rows = nlohmann::json::array();
  for (const auto& r : s.value("friend_requests", nlohmann::json::array())) { nlohmann::json f = r; f["incoming"] = true; if (!f.count("id") && f.count("from")) f["id"] = f["from"]; rows.push_back(f); }
  for (const auto& f : s.value("friends", nlohmann::json::array())) rows.push_back(f);
  return rows;
}
- (void)paintFriends:(const nlohmann::json&)s left:(NSRect)left bottom:(CGFloat)bottom leftWidth:(CGFloat)left_w online:(bool)online {
  const CGFloat X = RAIL_W;
  const nlohmann::json rows = [self friendRows:s];
  if (rows.empty() && !self.addingFriend) {
    NSRect area = left; area.origin.y += 46; area.size.height -= 46;
    [self emptyState:area title:@"Friends, one click away." detail:@"Add a player to see when they're online and ready for another game." social:YES];
  }
  const CGFloat item = 78, list_h = bottom - (self.addingFriend ? 320 : 222);
  const int fit = std::max(1, int(list_h / item));
  for (int i = 0; i < int(rows.size()) && i < fit; ++i) {
    const NSRect r = LR(X + 38, 166 + i * item, left_w - 28, item - 4);
    const bool selected = i == self.lobbyFriend;
    round_rect(r, 6, selected ? mix(g_accent, UI_PANEL, .82) : UI_PANEL, selected ? mix(g_accent, UI_PANEL, .82) : UI_PANEL, nullptr);
    self.buttons[[NSString stringWithFormat:@"lb:friend:%d", i]] = [NSValue valueWithRect:r];
    [self paintPerson:rows[size_t(i)] rect:r selected:selected pings:nlohmann::json::object()];
  }
  const bool picked = self.lobbyFriend >= 0 && self.lobbyFriend < (NSInteger)rows.size();
  const bool incoming = picked && rows[size_t(self.lobbyFriend)].value("incoming", false);
  if (self.addingFriend) {
    text(@"Enter an online player's Slippi code.", LR(X + 38, bottom - 136, left_w - 28, 36), f_small(), C_DIM, LEFT, 0, true);
    [self place:self.friendField rect:LR(X + 44, bottom - 90, left_w - 46, 26) shown:YES];
    [self lb:@"lb:friendsend" rect:LR(X + 38, bottom - 46, 150, 30) title:@"Send Friend Request" primary:NO enabled:online];
  } else if (incoming) {
    [self lb:@"lb:friendaccept" rect:LR(X + 38, bottom - 44, 118, 30) title:@"Accept Friend" primary:YES enabled:YES];
    [self lb:@"lb:frienddecline" rect:LR(X + 164, bottom - 44, 86, 30) title:@"Decline" primary:NO enabled:YES];
  } else if (picked) {
    const bool there = rows[size_t(self.lobbyFriend)].value("online", false);
    [self lb:@"lb:invite" rect:LR(X + 38, bottom - 44, 140, 30) title:ns(launcher::lang::tr("lobby.invite")) primary:YES enabled:online && there];
    [self lb:@"lb:unfriend" rect:LR(X + 186, bottom - 44, 120, 30) title:@"Remove Friend" primary:NO enabled:YES];
    [self lb:@"lb:pmfriend" rect:LR(X + 314, bottom - 44, 140, 30) title:ns(launcher::lang::tr("lobby.pm.button")) primary:NO enabled:online && there];
  } else [self lb:@"lb:addcode" rect:LR(X + 38, bottom - 44, 150, 30) title:@"Add by code..." primary:NO enabled:YES];
}
- (void)paintHistory:(NSRect)left leftWidth:(CGFloat)left_w bottom:(CGFloat)bottom {
  const CGFloat X = RAIL_W;
  const nlohmann::json games = mu_lobby::history();
  const auto rec = mu_lobby::record();
  if (games.empty()) {
    NSRect area = left; area.origin.y += 68; area.size.height -= 68;
    [self emptyState:area title:@"Match history" detail:@"Completed lobby games appear here, with your wins and losses." social:NO];
    return;
  }
  text([NSString stringWithFormat:@"%@ %d  /  %@ %d", tx(@"Wins"), rec.first, tx(@"Losses"), rec.second], LR(X + 38, 168, left_w - 28, 24), f_name(), glow());
  const CGFloat item = 34;
  const int fit = std::max(1, int((bottom - 216) / item));
  for (int i = 0; i < int(games.size()) && i < fit; ++i) {
    const nlohmann::json& g = games[size_t(i)];
    const NSRect r = LR(X + 38, 202 + i * item, left_w - 28, item - 4);
    round_rect(r, 6, UI_PANEL, UI_PANEL, &UI_BORDER);
    const std::string result = jstr(g, "result");
    text(ns(result == "win" ? "WIN" : result == "loss" ? "LOSS" : "—"), LR(r.origin.x + 10, r.origin.y, 44, r.size.height), f_name(), result == "win" ? C_OK : result == "loss" ? C_BAD : C_DIM);
    text(ns(jstr(g, "opponent", "?") + (jstr(g, "code").empty() ? "" : "  " + jstr(g, "code"))), LR(r.origin.x + 58, r.origin.y, r.size.width - 250, r.size.height), f_body(), C_TEXT);
    text(ns((jstr(g, "transport") == launcher::lobby::kP2pTransport ? "peer to peer  " : "Slippi Direct  ") + jstr(g, "when")), LR(NSMaxX(r) - 190, r.origin.y, 180, r.size.height), f_small(), C_DIM, RIGHT);
  }
}
- (void)paintProfile:(NSRect)left leftWidth:(CGFloat)left_w {
  const CGFloat X = RAIL_W;
  if (!self.profileLoaded) {
    const mu_lobby::Profile p = mu_lobby::profile();
    self.nameField.stringValue = ns(p.name); self.locationField.stringValue = ns(p.location);
    self.editMains = p.mains; self.editVolume = p.request_volume; self.editAutoReject = p.auto_reject; self.editSound = p.request_sound;
    self.profileLoaded = YES;
  }
  NSArray<NSString*>* labels = @[@"Display name", @"Slippi connect code (linked)", @"Location"];
  for (int i = 0; i < 3; ++i) text(labels[i], LR(X + 40, 166 + i * 64, left_w - 32, 18), f_small(), C_DIM);
  [self place:self.nameField rect:LR(X + 44, 194, left_w - 40, 24) shown:YES];
  {   // the code is the Slippi account's, not editable (upstream's CODE field is read-only)
    const NSRect box = NSInsetRect(LR(X + 44, 194 + 64, left_w - 40, 24), -5, -5);
    round_rect(box, 7, UI_BORDER, UI_BORDER, nullptr); round_rect(NSInsetRect(box, 1, 1), 7, UI_FIELD, UI_FIELD, nullptr);
    const std::string code = self.settings->account_code;
    text(code.empty() ? @"Sign in through Slippi Launcher" : ns(code), LR(X + 44, 194 + 64, left_w - 40, 24), f_body(), code.empty() ? C_FAINT : C_DIM);
  }
  [self place:self.locationField rect:LR(X + 44, 194 + 128, left_w - 40, 24) shown:YES];
  text(@"Your three mains", LR(X + 40, 358, left_w - 32, 20), f_small(), C_DIM);
  const std::vector<int> mains = self.editMains;
  for (int i = 0; i < 26; ++i) {
    const NSRect r = LR(X + 40 + (i % 9) * 42, 388 + (i / 9) * 42, 36, 36);
    const bool on = std::find(mains.begin(), mains.end(), i) != mains.end();
    round_rect(r, 9, on ? acc_top() : UI_BORDER, on ? acc_top() : UI_BORDER, nullptr);
    [self stock:i rect:NSInsetRect(r, 2, 2) selected:on];
    self.buttons[[NSString stringWithFormat:@"lb:main:%d", i]] = [NSValue valueWithRect:r];
  }
  [self lb:@"lb:autoreject" rect:LR(X + 40, 524, 174, 30) title:self.editAutoReject ? @"Auto reject invites: On" : @"Auto reject invites: Off" primary:NO enabled:YES];
  [self lb:@"lb:sound" rect:LR(X + 222, 524, 172, 30) title:self.editSound ? @"Request sound: On" : @"Request sound: Off" primary:NO enabled:YES];
  // the volume slider (upstream's track, amount and knob)
  const CGFloat sl = X + 44, sr = X + left_w - 8, sy = 574;
  round_rect(LR(sl, sy, sr - sl, 5), 2.5, UI_BORDER, UI_BORDER, nullptr);
  round_rect(LR(sl, sy, (sr - sl) * self.editVolume / 100, 5), 2.5, g_accent, g_accent, nullptr);
  const CGFloat knob = sl + (sr - sl) * self.editVolume / 100;
  round_rect(LR(knob - 7, sy - 5, 14, 15), 7, glow(), glow(), nullptr);
  self.buttons[@"lb:volume"] = [NSValue valueWithRect:LR(sl - 8, sy - 10, sr - sl + 16, 25)];
  [self lb:@"lb:save" rect:LR(X + 40, 610, 114, 30) title:@"Save Profile" primary:YES enabled:YES];
  text([NSString stringWithFormat:@"%@ %d%%", tx(@"Volume"), self.editVolume], LR(X + 170, 610, left_w - 162, 30), f_body(), C_DIM);
}

// ---- actions ---------------------------------------------------------------------------------------------------------------
- (BOOL)lobbyActivate:(NSString*)key index:(int)index {
  using Json = nlohmann::json;
  const Json snap = self.lobbySnap;
  const Json& s = snap.is_object() ? snap : Json::object();
  Json rows = s.value("players", Json::array());
  auto chosen_id = [&]() -> std::string { return str(self.lobbyPlayer); };
  auto room_ids = [&]() {
    std::vector<std::string> ids{std::string()};
    for (const auto& room : s.value("private", Json::array()))
      if (jstr(room, "state") == "open" || jstr(room, "state") == "closed" || jstr(room, "state") == "outgoing") ids.push_back(jstr(room, "id"));
    return ids;
  };
  if ([key isEqualToString:@"lb:go"]) {
    if (jstr(s, "phase") == "online") mu_lobby::leave(); else mu_lobby::go_online();
  } else if ([key isEqualToString:@"lb:tab"]) { self.lobbyTab = index; self.addingFriend = NO; self.profileLoaded = NO; }
  else if ([key isEqualToString:@"lb:player"]) { if (index >= 0 && index < (int)rows.size()) self.lobbyPlayer = ns(jstr(rows[size_t(index)], "id")); }
  else if ([key isEqualToString:@"lb:request"] || [key isEqualToString:@"lb:p2p"]) {
    Json data = {{"target", chosen_id()}, {"mode", "vanilla"}};
    if ([key isEqualToString:@"lb:p2p"]) data["transport"] = launcher::lobby::kP2pTransport;
    mu_lobby::command("request", data);
  } else if ([key isEqualToString:@"lb:addfriend"]) mu_lobby::command("friend", {{"target", chosen_id()}});
  else if ([key isEqualToString:@"lb:pm"]) mu_lobby::command("pm_request", {{"target", chosen_id()}});
  else if ([key isEqualToString:@"lb:send"]) {
    const std::string line = self.chatField.stringValue.UTF8String ?: "";
    if (!line.empty()) {
      if (self.lobbyRoom.length) mu_lobby::command("pm", {{"room", str(self.lobbyRoom)}, {"text", line}});
      else mu_lobby::command("chat", {{"text", line}});
      self.chatField.stringValue = @"";
    }
  } else if ([key isEqualToString:@"lb:emoji"]) {
    NSMenu* menu = [[NSMenu alloc] init];
    for (NSString* e in @[@"\U0001F44B", @"\U0001F44D", @"\U0001F602", @"\U0001F525", @"❤️", @"\U0001F3AE", @"\U0001F91D", @"\U0001F389"]) {
      NSMenuItem* item = [menu addItemWithTitle:e action:@selector(lobbyEmoji:) keyEquivalent:@""]; item.target = self;
    }
    const NSRect r = self.buttons[@"lb:emoji"].rectValue;
    [menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(r.origin.x, NSMaxY(r)) inView:self];
  } else if ([key isEqualToString:@"lb:accept"] || [key isEqualToString:@"lb:decline"]) {
    const bool accept = [key isEqualToString:@"lb:accept"];
    for (const auto& r : s.value("requests", Json::array()))
      if (jstr(r, "state") == "pending") { mu_lobby::command(accept ? "accept" : "cancel", {{"request", jstr(r, "id")}}); return YES; }
    for (const auto& room : s.value("private", Json::array()))
      if (jstr(room, "state") == "incoming") {
        mu_lobby::command(accept ? "pm_accept" : "pm_decline", {{"room", jstr(room, "id")}});
        if (accept) self.lobbyRoom = ns(jstr(room, "id"));
        return YES;
      }
  } else if ([key isEqualToString:@"lb:room"]) { const auto ids = room_ids(); if (index < (int)ids.size()) { self.lobbyRoom = ns(ids[size_t(index)]); self.chatField.stringValue = @""; } }
  else if ([key isEqualToString:@"lb:roomclose"]) {
    const auto ids = room_ids();
    if (index > 0 && index < (int)ids.size()) { mu_lobby::command("pm_close", {{"room", ids[size_t(index)]}}); if (ids[size_t(index)] == str(self.lobbyRoom)) self.lobbyRoom = @""; }
  } else if ([key isEqualToString:@"lb:friend"]) self.lobbyFriend = index;
  else if ([key isEqualToString:@"lb:friendaccept"] || [key isEqualToString:@"lb:frienddecline"] || [key isEqualToString:@"lb:unfriend"] ||
           [key isEqualToString:@"lb:invite"] || [key isEqualToString:@"lb:pmfriend"]) {
    const Json frows = [self friendRows:s];
    if (self.lobbyFriend < 0 || self.lobbyFriend >= (NSInteger)frows.size()) return YES;
    const std::string target = jstr(frows[size_t(self.lobbyFriend)], "id");
    if ([key isEqualToString:@"lb:invite"]) mu_lobby::command("request", {{"target", target}, {"mode", "vanilla"}});
    else if ([key isEqualToString:@"lb:pmfriend"]) mu_lobby::command("pm_request", {{"target", target}});
    else mu_lobby::command([key isEqualToString:@"lb:friendaccept"] ? "friend_accept" : [key isEqualToString:@"lb:frienddecline"] ? "friend_decline" : "unfriend", {{"target", target}});
    if (![key isEqualToString:@"lb:invite"] && ![key isEqualToString:@"lb:pmfriend"]) self.lobbyFriend = -1;
  } else if ([key isEqualToString:@"lb:addcode"]) self.addingFriend = YES;
  else if ([key isEqualToString:@"lb:friendsend"]) {
    const std::string code = launcher::lobby::normalize_code(self.friendField.stringValue.UTF8String ?: "");
    if (launcher::lobby::valid_code(code)) { mu_lobby::command("friend_code", {{"code", code}}); self.addingFriend = NO; self.friendField.stringValue = @""; }
  } else if ([key isEqualToString:@"lb:main"]) {
    std::vector<int> m = self.editMains;
    auto it = std::find(m.begin(), m.end(), index);
    if (it != m.end()) { if (m.size() > 1) m.erase(it); } else if (m.size() < 3) m.push_back(index);
    self.editMains = m;
  } else if ([key isEqualToString:@"lb:autoreject"]) self.editAutoReject = !self.editAutoReject;
  else if ([key isEqualToString:@"lb:sound"]) self.editSound = !self.editSound;
  else if ([key isEqualToString:@"lb:save"]) {
    mu_lobby::Profile p = mu_lobby::profile();
    p.name = self.nameField.stringValue.UTF8String ?: ""; p.location = self.locationField.stringValue.UTF8String ?: "";
    p.mains = self.editMains; p.auto_reject = self.editAutoReject; p.request_sound = self.editSound; p.request_volume = self.editVolume;
    mu_lobby::set_profile(p);
    host::log("launcher: lobby profile saved (auto reject %s, request sound %s at %d%%)", p.auto_reject ? "on" : "off", p.request_sound ? "on" : "off", p.request_volume);
  } else if ([key isEqualToString:@"lb:fighter"]) [self chooseLobbyFighter];
  else if ([key isEqualToString:@"lb:copycode"]) {
    [NSPasteboard.generalPasteboard clearContents];
    [NSPasteboard.generalPasteboard setString:ns(jstr(s, "copy_code")) forType:NSPasteboardTypeString];
  } else return NO;
  return YES;
}
- (void)lobbyEmoji:(NSMenuItem*)item { self.chatField.stringValue = [self.chatField.stringValue stringByAppendingString:item.title]; }
- (void)lobbyVolumeAt:(NSPoint)p {
  const CGFloat sl = RAIL_W + 44, sr = RAIL_W + (self.bounds.size.width - RAIL_W - 316 - 42) - 8;
  self.editVolume = int(std::lround(std::clamp((p.x - sl) / (sr - sl), 0.0, 1.0) * 100));
  [self setNeedsDisplay:YES];
}
// upstream's choose_fighter: the peer-to-peer match's colour, "First main" and the twenty-six characters in two columns.
- (void)chooseLobbyFighter {
  const mu_lobby::Profile p = mu_lobby::profile();
  const int ch = p.p2p_character >= 0 ? p.p2p_character : p.mains.empty() ? 2 : p.mains[0];
  NSMenu* menu = [[NSMenu alloc] init];
  NSMenu* shades = [[NSMenu alloc] init];
  for (int k = 0; k < mu_lobby::color_count(ch); ++k) {
    NSMenuItem* i = [shades addItemWithTitle:ns(launcher::lang::tr("lobby.p2p.color", {{"number", std::to_string(k + 1)}})) action:@selector(lobbyFighterPick:) keyEquivalent:@""];
    i.target = self; i.tag = 200 + k; i.state = k == p.p2p_color ? NSControlStateValueOn : NSControlStateValueOff;
  }
  NSMenuItem* sub = [menu addItemWithTitle:ns(launcher::lang::tr("lobby.p2p.color_menu")) action:nil keyEquivalent:@""]; sub.submenu = shades;
  [menu addItem:NSMenuItem.separatorItem];
  NSMenuItem* first = [menu addItemWithTitle:ns(launcher::lang::tr("lobby.p2p.fighter_first", {{"character", std::string(mu_lobby::character_name(p.mains.empty() ? 2 : p.mains[0]))}}))
                                      action:@selector(lobbyFighterPick:) keyEquivalent:@""];
  first.target = self; first.tag = 1; first.state = p.p2p_character < 0 ? NSControlStateValueOn : NSControlStateValueOff;
  for (int i = 0; i < 26; ++i) {
    NSMenuItem* item = [menu addItemWithTitle:ns(mu_lobby::character_name(i)) action:@selector(lobbyFighterPick:) keyEquivalent:@""];
    item.target = self; item.tag = 100 + i; item.state = p.p2p_character == i ? NSControlStateValueOn : NSControlStateValueOff;
  }
  const NSRect r = self.buttons[@"lb:fighter"].rectValue;
  [menu popUpMenuPositioningItem:nil atLocation:NSMakePoint(r.origin.x, NSMaxY(r) + 4) inView:self];
}
- (void)lobbyFighterPick:(NSMenuItem*)item {
  mu_lobby::Profile p = mu_lobby::profile();
  if (item.tag == 1) p.p2p_character = -1;
  else if (item.tag >= 100 && item.tag < 126) p.p2p_character = int(item.tag - 100);
  else if (item.tag >= 200) p.p2p_color = int(item.tag - 200);
  mu_lobby::set_profile(p);
  [self setNeedsDisplay:YES];
}

// ---- the page's clock: the snapshot ten times a second, an accepted match handed to the game, the test script ----------------
- (void)lobbyTick {
  nlohmann::json snap = mu_lobby::snapshot();
  const std::string now = snap.dump();
  if (now != self.lobbyShown) { self.lobbyShown = now; self.lobbySnap = snap; if (self.tab == 4) [self setNeedsDisplay:YES]; }
  mu_lobby::Match m;
  if (!self.gameTask && mu_lobby::take_match(&m)) [self startLobbyMatch:m];
  [self lobbyScriptStep];
}
- (void)startLobbyMatch:(const mu_lobby::Match&)m {
  if (self.settings->iso.empty()) { mu_lobby::command("available"); host::log("launcher: a lobby match was accepted with no disc chosen"); return; }
  mu_lobby::game_starting(m);
  std::vector<std::string> args = host::engine_arguments(*self.settings);
  if (m.p2p) {   // the peer-to-peer match: melee-unlocked's own, no Slippi (the engine's --p2p-* mode)
    for (const auto& a : m.args) args.push_back(a);
  } else {
    args.insert(args.end(), {"--lobby-direct", m.code, "--lobby-character", std::to_string(m.character)});
  }
  args.insert(args.end(), {"--lobby-status-file", mu_lobby::status_file()});
  NSMutableArray* list = [NSMutableArray array];
  for (const auto& a : args) [list addObject:ns(a)];
  NSTask* task = [[NSTask alloc] init];
  task.executableURL = [NSURL fileURLWithPath:ns(host::engine_path())];
  task.arguments = list;
  __weak MULaunchView* weak = self;
  task.terminationHandler = ^(NSTask* t) {
    const int code = t.terminationStatus;
    dispatch_async(dispatch_get_main_queue(), ^{
      MULaunchView* v = weak;
      host::log("launcher: the lobby match's game ended (%d): back to the Multiplayer Lobby", code);
      mu_lobby::game_ended();
      if (!v) return;
      v.gameTask = nil;
      [v selectTab:4];
      if (!std::getenv("MELEE_LAUNCHER_SHOT") && !std::getenv("MELEE_LAUNCHER_OFFSCREEN")) { [v.window makeKeyAndOrderFront:nil]; [NSApp activateIgnoringOtherApps:YES]; }
      [v setNeedsDisplay:YES];
    });
  };
  NSError* error = nil;
  if (![task launchAndReturnError:&error]) {
    host::log("launcher: the lobby match's game did not start: %s", error.localizedDescription.UTF8String);
    mu_lobby::game_ended();
    return;
  }
  self.gameTask = task;
  host::log("launcher: the lobby match's game started (%s against %s)", m.p2p ? "peer to peer" : "Slippi Direct", m.name.c_str());
}

// MELEE_LAUNCHER_LOBBY_SCRIPT=<file> (tests): one step per line, each waiting for what it needs before the next:
//   online | leave | wait-player NAME | wait-players N | select NAME | request NAME | p2p NAME | accept | decline | chat TEXT
//   pm NAME | pm-accept | pm-text TEXT | wait-chat TEXT | wait-pm TEXT | friend NAME | wait-friend NAME | auto-reject on|off
//   wait-history N | wait-game-end | wait-request | wait-notice TEXT | wait-file PATH | tab N | sleep MS | shot FILE | log TEXT | quit
- (void)lobbyScriptStep {
  using Json = nlohmann::json;
  static std::vector<std::string> lines;
  static size_t at = 0;
  static bool loaded = false;
  static std::chrono::steady_clock::time_point since = std::chrono::steady_clock::now();
  if (!loaded) {
    loaded = true;
    if (const char* file = std::getenv("MELEE_LAUNCHER_LOBBY_SCRIPT")) { std::ifstream in(file); std::string l; while (std::getline(in, l)) if (!l.empty() && l[0] != '#') lines.push_back(l); }
  }
  if (at >= lines.size()) return;
  const std::string line = lines[at];
  const size_t sp = line.find(' ');
  const std::string verb = line.substr(0, sp), arg = sp == std::string::npos ? std::string() : line.substr(sp + 1);
  const Json snap = self.lobbySnap;
  const Json& s = snap.is_object() ? snap : Json::object();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - since).count();
  auto player = [&](const std::string& name) -> std::string {
    for (const auto& p : s.value("players", Json::array())) if (jstr(p, "name") == name) return jstr(p, "id");
    return {};
  };
  auto has_text = [](const Json& messages, const std::string& text) {
    for (const auto& m : messages) if (jstr(m, "text") == text) return true;
    return false;
  };
  bool done = true;
  if (verb == "online") mu_lobby::go_online();
  else if (verb == "leave") mu_lobby::leave();
  else if (verb == "wait-player") done = !player(arg).empty();
  else if (verb == "wait-players") done = int(s.value("players", Json::array()).size()) >= std::atoi(arg.c_str());
  else if (verb == "select") { const std::string id = player(arg); done = !id.empty(); if (done) self.lobbyPlayer = ns(id); }
  else if (verb == "request" || verb == "p2p") {
    const std::string id = player(arg); done = !id.empty();
    if (done) { self.lobbyPlayer = ns(id); [self activate:verb == "p2p" ? @"lb:p2p" : @"lb:request"]; }
  } else if (verb == "accept" || verb == "decline") {
    done = false;
    for (const auto& r : s.value("requests", Json::array())) if (jstr(r, "state") == "pending" && jstr(r, "to") == jstr(s.value("self", Json::object()), "id")) done = true;
    if (done) [self activate:verb == "accept" ? @"lb:accept" : @"lb:decline"];
  } else if (verb == "chat") { self.lobbyRoom = @""; self.chatField.stringValue = ns(arg); [self activate:@"lb:send"]; }
  else if (verb == "pm") { const std::string id = player(arg); done = !id.empty(); if (done) { self.lobbyPlayer = ns(id); [self activate:@"lb:pm"]; } }
  else if (verb == "pm-accept") {
    done = false;
    for (const auto& room : s.value("private", Json::array())) if (jstr(room, "state") == "incoming") done = true;
    if (done) [self activate:@"lb:accept"];
  } else if (verb == "pm-text") {
    done = false;
    for (const auto& room : s.value("private", Json::array())) if (jstr(room, "state") == "open") { self.lobbyRoom = ns(jstr(room, "id")); done = true; }
    if (done) { self.lobbyTab = 0; self.chatField.stringValue = ns(arg); [self activate:@"lb:send"]; }
  } else if (verb == "wait-chat") done = has_text(s.value("messages", Json::array()), arg);
  else if (verb == "wait-pm") {
    done = false;
    for (const auto& room : s.value("private", Json::array())) if (has_text(room.value("messages", Json::array()), arg)) { done = true; self.lobbyRoom = ns(jstr(room, "id")); }
  } else if (verb == "friend") { const std::string id = player(arg); done = !id.empty(); if (done) { self.lobbyPlayer = ns(id); [self activate:@"lb:addfriend"]; } }
  else if (verb == "wait-friend") { done = false; for (const auto& f : s.value("friends", Json::array())) if (jstr(f, "name") == arg) done = true; }
  else if (verb == "auto-reject") { mu_lobby::Profile p = mu_lobby::profile(); p.auto_reject = arg == "on"; mu_lobby::set_profile(p); self.profileLoaded = NO; }
  else if (verb == "wait-history") done = int(mu_lobby::history().size()) >= std::atoi(arg.c_str()) && !self.gameTask;
  else if (verb == "wait-game-end") done = !self.gameTask && !mu_lobby::playing();
  else if (verb == "wait-request") {
    done = false;
    for (const auto& r : s.value("requests", Json::array())) if (jstr(r, "state") == "pending" && jstr(r, "to") == jstr(s.value("self", Json::object()), "id")) done = true;
  } else if (verb == "wait-notice") done = jstr(s, "notice").find(arg) != std::string::npos;
  else if (verb == "wait-file") { std::error_code ec; done = std::filesystem::exists(arg, ec); }
  else if (verb == "tab") self.lobbyTab = std::clamp(std::atoi(arg.c_str()), 0, 3);
  else if (verb == "sleep") done = elapsed >= std::atoi(arg.c_str());
  else if (verb == "shot") {
    [self display];
    NSBitmapImageRep* rep = [self bitmapImageRepForCachingDisplayInRect:self.bounds];
    [self cacheDisplayInRect:self.bounds toBitmapImageRep:rep];
    [[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:ns(arg) atomically:YES];
    host::log("launcher: lobby script: screenshot %s", arg.c_str());
  } else if (verb == "log") host::log("launcher: lobby script: %s", arg.c_str());
  else if (verb == "quit") { host::log("launcher: lobby script: done"); mu_lobby::stop(); [NSApp abortModal]; }
  if (!done && elapsed > 240000) { host::log("launcher: lobby script: step \"%s\" timed out", line.c_str()); done = true; }
  if (done) {
    if (verb != "shot" && verb != "sleep" && verb != "log") host::log("launcher: lobby script: %s", line.c_str());
    ++at; since = std::chrono::steady_clock::now();
    [self setNeedsDisplay:YES];
  }
}
@end
