// See abi_macos.h.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "abi_macos.h"
#include "fixedmap.h"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/ucontext.h>
#include <unistd.h>

namespace {
decltype(MuHostApi::slippi_command) slippi_command_original = nullptr;
decltype(MuHostApi::replay_event) replay_event_original = nullptr;
int32_t slippi_command_adapter(uint32_t command, const uint8_t* payload, uint32_t payload_size, uint8_t* response,
                               uint32_t response_capacity, uint32_t* response_size) {
  return slippi_command_original(uint8_t(command), payload, payload_size, response, response_capacity, response_size);
}
void replay_event_adapter(uint32_t command, const uint8_t* payload, uint32_t size) {
  replay_event_original(uint8_t(command), payload, size);
}

struct sigaction previous_segv, previous_bus;
bool fixup_enabled = false, fixup_frames = false;
uint64_t fixup_pcs[256];
int fixup_count = 0;
bool console_low(uint64_t v) { return (v >= 0x80000000u && v < 0x84000000u) || (v >= 0xE0000000u && v < 0xE0004000u); }
// A load or store through a console address the game did not widen: put MU_HIGH_BASE into the
// register that holds it and retry. Diagnosis (MU_FIXUP=1): each instruction is reported once.
bool fix_up(uint64_t address, _STRUCT_ARM_THREAD_STATE64& ss) {
  if (!fixup_enabled || !console_low(address)) return false;
  const uint64_t base = uint64_t(MU_HIGH_BASE);
  if (ss.__pc < base || ss.__pc >= base + 0x84000000u) return false;     // only inside the game image
  const uint32_t insn = *reinterpret_cast<const uint32_t*>(ss.__pc);
  if ((insn & 0x0A000000u) != 0x08000000u) return false;                 // not a load/store encoding
  const unsigned rn = (insn >> 5) & 31, rm = (insn >> 16) & 31;
  uint64_t* reg = nullptr;
  if (rn != 31 && console_low(ss.__x[rn]) && !(ss.__x[rn] >> 32)) reg = &ss.__x[rn];
  else if (rm != 31 && console_low(ss.__x[rm]) && !(ss.__x[rm] >> 32)) reg = &ss.__x[rm];
  if (!reg) return false;
  *reg |= base;
  bool seen = false;
  for (int i = 0; i < fixup_count; ++i) if (fixup_pcs[i] == ss.__pc) { seen = true; break; }
  if (!seen && fixup_count < 256) {
    fixup_pcs[fixup_count++] = ss.__pc;
    char line[160];
    const int n = std::snprintf(line, sizeof line, "fixup: image pc %llx widened x%u (address %llx)\n", (unsigned long long)(ss.__pc - base),
                                unsigned(reg - ss.__x), (unsigned long long)address);
    (void)!write(2, line, size_t(n));
    if (fixup_frames) {             // MU_FIXUP=2: the frame-pointer chain, for addr2line
      uint64_t fp = ss.__fp;
      int k = std::snprintf(line, sizeof line, "fixup:   lr %llx", (unsigned long long)(ss.__lr - base));
      for (int depth = 0; depth < 8 && fp && !(fp & 15) && k < 140; ++depth) {
        const uint64_t* frame = reinterpret_cast<const uint64_t*>(fp);
        k += std::snprintf(line + k, sizeof line - size_t(k), " %llx", (unsigned long long)(frame[1] - base));
        fp = frame[0];
      }
      line[k++] = '\n'; (void)!write(2, line, size_t(k));
    }
  }
  return true;
}
void on_crash(int sig, siginfo_t* info, void* context) {
  auto& ss = static_cast<ucontext_t*>(context)->uc_mcontext->__ss;
  if (fix_up(uint64_t(uintptr_t(info->si_addr)), ss)) return;
  const uint64_t base = uint64_t(MU_HIGH_BASE);
  char line[512];
  const uint64_t pc = ss.__pc, lr = ss.__lr, address = uint64_t(uintptr_t(info->si_addr));
  int n = std::snprintf(line, sizeof line, "crash: signal %d at address %llx, pc %llx (image %llx), lr %llx (image %llx)\n", sig,
                        (unsigned long long)address, (unsigned long long)pc, (unsigned long long)(pc - base),
                        (unsigned long long)lr, (unsigned long long)(lr - base));
  (void)!write(2, line, size_t(n));
  if (address >= 0x80000000u && address < 0x100000000ull)
    n = std::snprintf(line, sizeof line, "crash: a console address used without MU_HIGH_BASE (a missed widening in the game)\n"), (void)!write(2, line, size_t(n));
  for (int i = 0; i < 29; i += 4) {
    n = std::snprintf(line, sizeof line, "  x%-2d %016llx x%-2d %016llx x%-2d %016llx x%-2d %016llx\n", i, (unsigned long long)ss.__x[i],
                      i + 1, (unsigned long long)ss.__x[i + 1], i + 2, (unsigned long long)ss.__x[i + 2], i + 3, (unsigned long long)ss.__x[i + 3]);
    (void)!write(2, line, size_t(n));
  }
  sigaction(sig, sig == SIGBUS ? &previous_bus : &previous_segv, nullptr);
}
} // namespace

void mu_abi_adapt_host_table(MuHostApi* host) {
  if (host->slippi_command && host->slippi_command != reinterpret_cast<decltype(host->slippi_command)>(&slippi_command_adapter)) {
    slippi_command_original = host->slippi_command;
    host->slippi_command = reinterpret_cast<decltype(host->slippi_command)>(&slippi_command_adapter);
  }
  if (host->replay_event && host->replay_event != reinterpret_cast<decltype(host->replay_event)>(&replay_event_adapter)) {
    replay_event_original = host->replay_event;
    host->replay_event = reinterpret_cast<decltype(host->replay_event)>(&replay_event_adapter);
  }
}

void mu_abi_install_crash_report() {
  const char* fixup = getenv("MU_FIXUP");
  fixup_enabled = fixup && (*fixup == '1' || *fixup == '2');
  fixup_frames = fixup && *fixup == '2';
  struct sigaction action;
  std::memset(&action, 0, sizeof action);
  action.sa_sigaction = on_crash;
  action.sa_flags = SA_SIGINFO;
  sigemptyset(&action.sa_mask);
  sigaction(SIGSEGV, &action, &previous_segv);
  sigaction(SIGBUS, &action, &previous_bus);
}
