// The few Win32 names melee-unlocked's texture_pack.cpp, video_background.cpp and flicker_scan.cpp (compiled from the fetched
// tree by mu-engine/app/gfx/gen_gfx.sh) still name on the Mac, answered with POSIX and macOS calls. Their folders come from
// gfx_paths.h (patches/0130-0132), so GetModuleFileNameW is not asked for a real answer.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <string>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

typedef uint32_t DWORD;
typedef int BOOL;
typedef uint64_t DWORDLONG;
typedef void* HWND;
typedef void* HMODULE;
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

inline DWORD GetModuleFileNameW(HMODULE, wchar_t* buffer, DWORD size) { if (size) buffer[0] = 0; return 0; }
inline BOOL CreateDirectoryA(const char* path, void*) { return ::mkdir(path, 0755) == 0; }

struct MEMORYSTATUSEX { DWORD dwLength = 0; DWORD dwMemoryLoad = 0; DWORDLONG ullTotalPhys = 0; DWORDLONG ullAvailPhys = 0;
                       DWORDLONG ullTotalPageFile = 0; DWORDLONG ullAvailPageFile = 0; DWORDLONG ullTotalVirtual = 0; DWORDLONG ullAvailVirtual = 0; };
inline BOOL GlobalMemoryStatusEx(MEMORYSTATUSEX* m) {
  uint64_t bytes = 0; size_t size = sizeof bytes;
  if (sysctlbyname("hw.memsize", &bytes, &size, nullptr, 0) != 0) return 0;
  m->ullTotalPhys = bytes; m->ullAvailPhys = bytes / 2; m->ullTotalPageFile = m->ullAvailPageFile = bytes;
  m->ullTotalVirtual = m->ullAvailVirtual = bytes;
  return 1;
}

// Shows a folder in the Finder (`open PATH`), as ShellExecuteW "open" shows it in Explorer.
inline void* ShellExecuteW(HWND, const wchar_t*, const char* path_utf8, const wchar_t*, const wchar_t*, int) {
  if (!path_utf8 || std::getenv("MELEE_NO_FINDER")) return nullptr;   // tests: never open a Finder window
  pid_t pid = 0;
  const char* argv[] = {"/usr/bin/open", path_utf8, nullptr};
  if (posix_spawn(&pid, "/usr/bin/open", nullptr, nullptr, const_cast<char* const*>(argv), nullptr) == 0) waitpid(pid, nullptr, 0);
  return nullptr;
}
inline void* ShellExecuteW(HWND, const wchar_t*, const wchar_t* file, const wchar_t*, const wchar_t*, int) {
  if (!file) return nullptr;
  std::string path;
  for (const wchar_t* p = file; *p; ++p) {
    const uint32_t c = uint32_t(*p);
    if (c < 0x80) path += char(c);
    else if (c < 0x800) { path += char(0xC0 | (c >> 6)); path += char(0x80 | (c & 0x3F)); }
    else if (c < 0x10000) { path += char(0xE0 | (c >> 12)); path += char(0x80 | ((c >> 6) & 0x3F)); path += char(0x80 | (c & 0x3F)); }
    else { path += char(0xF0 | (c >> 18)); path += char(0x80 | ((c >> 12) & 0x3F)); path += char(0x80 | ((c >> 6) & 0x3F)); path += char(0x80 | (c & 0x3F)); }
  }
  return ShellExecuteW(nullptr, nullptr, path.c_str(), nullptr, nullptr, 0);
}
