// A loopback stand-in for Slippi's matchmaking service, for tests only: it speaks the protocol the app's client speaks
// (JSON messages over ENet, port/runtime/hle/slippi_net.cpp), listens on 127.0.0.1 only, and pairs the first two tickets
// of the same mode. Ranked tickets are refused. Point the app at it with MELEE_MM_LOOPBACK_PORT=<port>; nothing here or
// in a test that uses it talks to Slippi's servers.
//
// build: clang++ -std=c++17 -O2 -Iport/third_party/enet/include -Iport/third_party tools/mac/mock_matchmaking.cpp
//        port/third_party/enet/{callbacks,compress,host,list,packet,peer,protocol,unix}.c -o build/mock_matchmaking
// run:   mock_matchmaking [--port 43113] [--seconds 600] [--dead-lan]
//        --dead-lan: every player's LAN address becomes 127.0.0.1:9, where nothing answers (the same-router fallback test:
//        both players share the public address 127.0.0.1, so each dials the dead LAN address first, then the public one).
// SPDX-License-Identifier: GPL-3.0-or-later
#include <enet/enet.h>
#include <nlohmann/json.hpp>
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using json = nlohmann::json;

struct Ticket { ENetPeer* peer; json user; int mode; std::string lan; };

static void send(ENetPeer* peer, const json& msg) {
  const std::string s = msg.dump();
  enet_peer_send(peer, 0, enet_packet_create(s.data(), s.size(), ENET_PACKET_FLAG_RELIABLE));
}
static std::string address_of(const ENetAddress& a) {
  char ip[INET_ADDRSTRLEN] = "127.0.0.1";
  in_addr in; in.s_addr = a.host;
  inet_ntop(AF_INET, &in, ip, sizeof ip);
  return std::string(ip) + ":" + std::to_string(a.port);
}

int main(int argc, char** argv) {
  int port = 43113, seconds = 600;
  bool dead_lan = false;
  for (int i = 1; i < argc; ++i) if (!std::strcmp(argv[i], "--dead-lan")) dead_lan = true;
  for (int i = 1; i + 1 < argc; ++i) {
    if (!std::strcmp(argv[i], "--port")) port = std::atoi(argv[++i]);
    else if (!std::strcmp(argv[i], "--seconds")) seconds = std::atoi(argv[++i]);
  }
  if (enet_initialize() != 0) { std::fprintf(stderr, "mock mm: enet_initialize failed\n"); return 1; }
  ENetAddress address; enet_address_set_host(&address, "127.0.0.1"); address.port = (enet_uint16)port;
  ENetHost* host = enet_host_create(&address, 32, 3, 0, 0);
  if (!host) { std::fprintf(stderr, "mock mm: cannot listen on 127.0.0.1:%d\n", port); return 1; }
  std::printf("mock mm: listening on 127.0.0.1:%d for %d s\n", port, seconds); std::fflush(stdout);
  std::vector<Ticket> waiting;
  int matches = 0;
  const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
  while (std::chrono::steady_clock::now() < end) {
    ENetEvent ev;
    if (enet_host_service(host, &ev, 200) <= 0) continue;
    if (ev.type == ENET_EVENT_TYPE_CONNECT) { std::printf("mock mm: client %s connected\n", address_of(ev.peer->address).c_str()); continue; }
    if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
      for (size_t i = 0; i < waiting.size(); ++i) if (waiting[i].peer == ev.peer) { waiting.erase(waiting.begin() + long(i)); break; }
      continue;
    }
    if (ev.type != ENET_EVENT_TYPE_RECEIVE) continue;
    const std::string text(reinterpret_cast<const char*>(ev.packet->data), ev.packet->dataLength);
    enet_packet_destroy(ev.packet);
    json msg = json::parse(text, nullptr, false);
    if (msg.is_discarded() || msg.value("type", "") != "create-ticket") continue;
    const int mode = msg["search"].value("mode", -1);
    std::printf("mock mm: ticket from %s mode %d (%s)\n", address_of(ev.peer->address).c_str(), mode,
                msg["user"].value("connectCode", "?").c_str());
    if (mode == 0) { send(ev.peer, {{"type", "create-ticket-resp"}, {"error", "Ranked is not offered by this test service"}}); continue; }
    send(ev.peer, {{"type", "create-ticket-resp"}});
    waiting.push_back({ev.peer, msg["user"], mode, dead_lan ? std::string("127.0.0.1:9") : msg.value("ipAddressLan", "")});
    for (size_t a = 0; a < waiting.size(); ++a) for (size_t b = a + 1; b < waiting.size(); ++b) {
      if (waiting[a].mode != waiting[b].mode) continue;
      const Ticket pair[2] = {waiting[a], waiting[b]};
      waiting.erase(waiting.begin() + long(b)); waiting.erase(waiting.begin() + long(a));
      const std::string id = "mode.mock-" + std::to_string(++matches);
      for (int me = 0; me < 2; ++me) {
        json players = json::array();
        for (int p = 0; p < 2; ++p)
          players.push_back({{"uid", pair[p].user.value("uid", "")}, {"displayName", pair[p].user.value("displayName", "")},
                             {"connectCode", pair[p].user.value("connectCode", "")}, {"port", p + 1}, {"isLocalPlayer", p == me},
                             {"ipAddress", address_of(pair[p].peer->address)}, {"ipAddressLan", pair[p].lan}});
        send(pair[me].peer, {{"type", "get-ticket-resp"}, {"matchId", id}, {"isHost", me == 0}, {"players", players},
                             {"stages", {2, 3, 8, 28, 31, 32}}});
      }
      std::printf("mock mm: matched %s and %s as %s\n", address_of(pair[0].peer->address).c_str(), address_of(pair[1].peer->address).c_str(), id.c_str());
      std::fflush(stdout);
      a = waiting.size(); break;
    }
    std::fflush(stdout);
  }
  enet_host_destroy(host);
  enet_deinitialize();
  return 0;
}
