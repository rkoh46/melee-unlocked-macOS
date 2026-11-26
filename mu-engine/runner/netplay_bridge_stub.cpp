// The netplay bridge for builds without online play (the headless parity runner): every call says
// "offline", so net_host never starts a session. The real bridge is netplay_bridge.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "netplay_bridge.h"

namespace netplay_bridge {
void set_online_prefs(const OnlinePrefs&) {}
uint32_t port_colour_bits() { return 0; }
uint32_t shield_color(int) { return 0; }
bool start(const Setup&, std::string* error) { if (error) *error = "this build has no online play"; return false; }
void shutdown() {}
bool handle(uint8_t, const uint8_t*, uint32_t, std::vector<uint8_t>*) { return false; }
bool request_match(int, const std::string&, uint8_t, uint8_t) { return false; }
const char* ranked_refusal() { return "this build has no online play"; }
bool has_services() { return false; }
uint64_t rollbacks_seen_by_device() { return 0; }
int ping_ms() { return -1; }
double emulation_speed() { return 1.0; }
SearchInfo search_info() { return {}; }
void keep_search(bool) {}
bool search_kept() { return false; }
void set_chat(int, bool) {}
int local_slot() { return 0; }
void end_kept_search(const char*) {}
FrameTrace frame_trace() { return {}; }
void set_delay(int, bool) {}
void replay_event(uint8_t, const uint8_t*, uint32_t) {}
void set_suggestion_hooks(int (*)(int, uint8_t*), int (*)()) {}
void suggestion_note_poll() {}
bool opponent_auto_lcancel() { return false; }
void set_gecko_hooks(const GeckoHooks&) {}
bool p2p_option(const std::string&, const char*) { return false; }
bool p2p_requested() { return false; }
int p2p_local_slot() { return 0; }
int p2p_character() { return 2; }
bool p2p_start(const P2pHooks&, std::string* error) { if (error) *error = "this build has no online play"; return false; }
bool p2p_handle(uint8_t, const uint8_t*, uint32_t, std::vector<uint8_t>*) { return false; }
int p2p_exit_request() { return -1; }
void p2p_report() {}
void p2p_shutdown() {}
void p2p_reset() {}
}
