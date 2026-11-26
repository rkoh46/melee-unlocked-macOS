// Winsock names for melee-unlocked's peer lobby on POSIX sockets (see windows.h beside this file).
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "windows.h"

typedef int SOCKET;
#define INVALID_SOCKET (-1)
inline int closesocket(SOCKET s) { return ::close(s); }
// The lobby's one use: FIONBIO on its UDP socket. fcntl works on macOS (as on Linux and libnx).
#ifndef FIONBIO
#define FIONBIO 0x5421
#endif
inline int ioctlsocket(SOCKET s, long command, u_long* value) {
  if (command != FIONBIO || !value) return -1;
  const int flags = ::fcntl(s, F_GETFL, 0);
  if (flags < 0) return -1;
  return ::fcntl(s, F_SETFL, *value ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
}
// Winsock's sendto takes an address length longer than the address (jech/dht announces with sizeof(struct sockaddr_storage) for an
// IPv4 node, through the lobby's dht_sendto); macOS refuses that with EINVAL, so without this no announce ever left a Mac and its
// lobby could find no one. The length is cut to the address family's own, as Windows and Linux read it.
inline ssize_t mu_shim_sendto(int s, const void* data, size_t length, int flags, const struct sockaddr* to, socklen_t size) {
  if (to && to->sa_family == AF_INET && size > (socklen_t)sizeof(struct sockaddr_in)) size = sizeof(struct sockaddr_in);
  if (to && to->sa_family == AF_INET6 && size > (socklen_t)sizeof(struct sockaddr_in6)) size = sizeof(struct sockaddr_in6);
  return ::sendto(s, data, length, flags, to, size);
}
#define sendto mu_shim_sendto
