// macOS back end of fixedmap.h. Apple silicon keeps the low 4 GB of every arm64 process unmapped (the
// kernel refuses to start a binary with a smaller __PAGEZERO), so the console ranges are placed at
// MU_HIGH_BASE plus their console address; the game image is built to match (mu_disc.h). Pages are
// 16 KiB here. Like the other back ends, nothing is accessible at the fixed address until
// fixedmap_protect. Code pages go read/write -> read/execute with mprotect, which macOS allows for a
// process without the hardened runtime.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixedmap.h"
#include <errno.h>
#include <libkern/OSCacheControl.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define MAX_RANGES 8
static struct Range { uintptr_t addr; size_t size; } g_ranges[MAX_RANGES];
static char g_error[192];
const char* fixedmap_error(void) { return g_error; }

void* fixedmap_stage(size_t size) {
  void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (p == MAP_FAILED) { snprintf(g_error, sizeof g_error, "staging mmap: %s", strerror(errno)); return NULL; }
  return p;
}

int fixedmap_commit(uintptr_t addr, void* staged, size_t size, const char* name) {
  struct Range* slot = NULL;
  for (int i = 0; i < MAX_RANGES; ++i) if (!g_ranges[i].size) { slot = &g_ranges[i]; break; }
  mach_vm_address_t at = addr;
  // VM_FLAGS_FIXED without VM_FLAGS_OVERWRITE: fails when anything already lies in the range.
  kern_return_t kr = slot ? mach_vm_allocate(mach_task_self(), &at, size, VM_FLAGS_FIXED) : KERN_NO_SPACE;
  if (kr != KERN_SUCCESS || at != addr) {
    snprintf(g_error, sizeof g_error, "%s: cannot map %zu KiB at %llx: %s", name, size >> 10, (unsigned long long)addr,
             slot ? mach_error_string(kr) : "too many ranges");
    munmap(staged, size);
    return -1;
  }
  memcpy((void*)addr, staged, size);
  munmap(staged, size);
  mprotect((void*)addr, size, PROT_NONE);
  slot->addr = addr; slot->size = size;
  return 0;
}

int fixedmap_protect(uintptr_t addr, size_t size, int prot) {
  int p = prot == FIXEDMAP_RX ? PROT_READ | PROT_EXEC : prot == FIXEDMAP_RW ? PROT_READ | PROT_WRITE : PROT_READ;
  if (mprotect((void*)addr, size, p)) { snprintf(g_error, sizeof g_error, "mprotect(%llx): %s", (unsigned long long)addr, strerror(errno)); return -1; }
  if (prot == FIXEDMAP_RX) sys_icache_invalidate((void*)addr, size);
  return 0;
}

void* fixedmap_backing(uintptr_t addr) { (void)addr; return NULL; }

void fixedmap_release(uintptr_t addr) {
  for (int i = 0; i < MAX_RANGES; ++i) if (g_ranges[i].size && g_ranges[i].addr == addr) {
    mach_vm_deallocate(mach_task_self(), g_ranges[i].addr, g_ranges[i].size);
    g_ranges[i].size = 0;
  }
}
void fixedmap_release_all(void) {
  for (int i = 0; i < MAX_RANGES; ++i) if (g_ranges[i].size) fixedmap_release(g_ranges[i].addr);
}

int fixedmap_startup(int* argc, char*** argv, const uintptr_t* ranges, int count) { (void)argc; (void)argv; (void)ranges; (void)count; return 0; }
const char* fixedmap_layout(void) { return "layout: macOS arm64, the console ranges sit at 0x100000000000 + their console address"; }
int fixedmap_relaunch_count(void) { return 0; }
int fixedmap_relaunch(void) { snprintf(g_error, sizeof g_error, "no relaunch on macOS"); return -1; }
